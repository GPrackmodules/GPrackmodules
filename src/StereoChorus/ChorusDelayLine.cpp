#include "ChorusDelayLine.h"

ChorusDelayLine::ChorusDelayLine(float fSamplerate, int nChannels, float dMaxDelayS) :
	m_nChannels(nChannels),
	m_fMaxDelayS(dMaxDelayS),
	m_fSamplerate(fSamplerate)
{
	UpdateSamplerate(fSamplerate);
}

ChorusDelayLine::~ChorusDelayLine()
{
	DeleteTempIRs();
	DeleteIRs();
	if (m_ppDelayLines != nullptr)
	{
		for (int i = 0; i < m_nChannels; i++)
			delete m_ppDelayLines[i];
		delete[] m_ppDelayLines;
	}
}

void ChorusDelayLine::Feed(float fL, float fR)
{
	m_ppDelayLines[0][m_nWriteIndex] = fL;
	m_ppDelayLines[1][m_nWriteIndex] = fR;
	if (m_nWriteIndex >= 0 && m_nWriteIndex < N_TAPS)
	{
		m_ppDelayLines[0][m_nWriteIndex + m_nMaxDelaySamples] = fL;
		m_ppDelayLines[1][m_nWriteIndex + m_nMaxDelaySamples] = fR;
	}
}

float ChorusDelayLine::Read(int nChannel, float fDelayS)
{
	assert(fDelayS >= 0.0f);
	int64_t nDelaySamplesSubsample = static_cast<int64_t>(fDelayS * m_fSamplerate * N_SUBSAMPLE + 0.5);
	int nDelaySamples = static_cast<int>(nDelaySamplesSubsample / N_SUBSAMPLE);
	int nDelayFraction = static_cast<int>(nDelaySamplesSubsample % N_SUBSAMPLE);

	if (nDelaySamples >= m_nMaxDelaySamples)
		nDelaySamples = m_nMaxDelaySamples - 1;

	int nReadIndex = m_nWriteIndex - nDelaySamples - N_TAPS;
	if (nReadIndex < 0)
		nReadIndex += m_nMaxDelaySamples;

	float* pDelayLine = m_ppDelayLines[nChannel] + nReadIndex;
	float* pIR = m_ppIRs[nDelayFraction];

	float fReturn = 0.0;
	for (int i =0; i < N_TAPS; i++)
		fReturn += (*(pDelayLine++)) * (*(pIR++));
	return fReturn;
}

void ChorusDelayLine::Advance()
{
	if (++m_nWriteIndex >= m_nMaxDelaySamples)
		m_nWriteIndex = 0;
}

void ChorusDelayLine::UpdateSamplerate(float fSamplerate)
{
	m_fSamplerate = fSamplerate;
	m_nMaxDelaySamples = static_cast<int>(m_fMaxDelayS * m_fSamplerate) + 1;

	if (m_ppDelayLines != nullptr)
	{
		for (int nChannel = 0; nChannel < m_nChannels; ++nChannel)
			delete m_ppDelayLines[nChannel];
		delete m_ppDelayLines;
	}
	// create the delay lines
	m_ppDelayLines = new float*[m_nChannels];
	for (int nChannel = 0; nChannel < m_nChannels; ++nChannel)
	{
		m_ppDelayLines[nChannel] = new float[m_nMaxDelaySamples + MaxTaps() + 1]; // delay line allows placing FIR anywhere up MaxDelaySamples - 1
		memset(m_ppDelayLines[nChannel], 0, (m_nMaxDelaySamples + MaxTaps() + 1) * sizeof(float));
	}
	BuildIRs();
	UpdateIRs();
	DeleteOldIRs();
}

bool ChorusDelayLine::BuildIRs(float fCutoffFrequency /*= 0.45f*/)
{
	size_t nFullLength = N_SUBSAMPLE * N_TAPS;
	vector<int> vFactor(nFullLength, 0);
	vector<float> vFullFilter(nFullLength, 0.0f);

	fCutoffFrequency /= static_cast<float>(N_SUBSAMPLE);

	for(size_t i = 0; i < nFullLength; i++)
		vFactor[i] = static_cast<int>(i - nFullLength / 2);

	for(size_t i = 0; i < nFullLength / 2; i++)
	{
		float fSinC; // sin(x) / X
		if (vFactor[i] == 0)
			fSinC = 1.0f;
		else
		{
			float fRadian = M_PI * 2.0f * fCutoffFrequency * vFactor[i];
			fSinC = sin(fRadian) / fRadian;
		}
		float fCoeff = 2.0f * fCutoffFrequency * fSinC * static_cast<float>(N_SUBSAMPLE);
		vFullFilter[i] = fCoeff;
		vFullFilter[nFullLength - 1 - i] = fCoeff;
	}
	dsp::blackmanHarrisWindow(vFullFilter.data(), nFullLength);

	float ** ppNewIRs = new float*[N_SUBSAMPLE];
	for (size_t nSub = 0; nSub < N_SUBSAMPLE; nSub++)
	{
		float* pTaps = new float[N_TAPS];
		for (size_t nTap = 0; nTap < N_TAPS; nTap++)
			pTaps[N_TAPS - 1 - nTap] = vFullFilter[nTap * N_SUBSAMPLE + nSub];
		ppNewIRs[N_SUBSAMPLE - 1 - nSub] = pTaps;
	}

	lock_guard<mutex> lg(m_mtxTempIRs);
	DeleteTempIRs();
	m_ppTempIRs = ppNewIRs;
	return true;
}

void ChorusDelayLine::DeleteTempIRs()
{
	if (m_ppTempIRs == nullptr)
		return;
	for (int nSub = 0; nSub < N_SUBSAMPLE; nSub++)
		delete [] m_ppTempIRs[nSub];
	delete m_ppTempIRs;
	m_ppTempIRs = nullptr;
}

void ChorusDelayLine::DeleteOldIRs()
{
	m_mtxOldIRs.lock();
	while (!m_lstOldIRs.empty())
	{
		pair<int, float**> pairDelete = m_lstOldIRs.front();
		m_lstOldIRs.pop_front();
		m_mtxOldIRs.unlock(); // release mutex again to alloe ProcessBlock to continue while we delete memory

		for (int nSub = 0; nSub < pairDelete.first; nSub++)
			delete pairDelete.second[nSub];
		delete pairDelete.second;

		m_mtxOldIRs.lock(); // lock again to get next item from list
	}
	m_mtxOldIRs.unlock();
}

void ChorusDelayLine::DeleteIRs()
{
	if (m_ppIRs == nullptr)
		return;
	for (int nSub = 0; nSub < N_SUBSAMPLE; nSub++)
		delete [] m_ppIRs[nSub];
	delete [] m_ppIRs;
	m_ppIRs = nullptr;
}

void ChorusDelayLine::UpdateIRs()
{
	float** ppIRs;
	float** ppOldIRs;

	{ // scope for lock_guard
		if (m_ppTempIRs == nullptr)
			return;
		lock_guard<mutex> lgTemp(m_mtxTempIRs);
		ppIRs = m_ppTempIRs;
		m_ppTempIRs = nullptr;
	}
	{ // scope for lock_guard
		lock_guard<mutex> lg(m_mtxIRs);
		ppOldIRs = m_ppIRs;
		m_ppIRs = ppIRs;
	}
	{ // scope for lock_guard
		lock_guard<mutex> lg(m_mtxOldIRs);
		if (ppOldIRs != nullptr)
			m_lstOldIRs.push_back(std::pair<int, float**>(N_SUBSAMPLE, ppOldIRs));
	}
}
