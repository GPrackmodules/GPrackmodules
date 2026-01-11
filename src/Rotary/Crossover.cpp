//
// class Crossover
// ===============
// IIR filter for the crossover
// Summing LP/HP-LP/HP bands with a -1 factor for one of the bands
// results in a flat frequncy response without dips.
// They are only 12dB/oct, but that's ok since it creates more band overlap with two
// speakers rotating at slightly different speeds.
//

#include "plugin.hpp"
#include "Crossover.h"
#include <cmath>

Crossover::Crossover(bool b24dBPerOct) :
	m_b24dBPerOct(b24dBPerOct)
{
	// frequenceys don't matter, just making sure everything is initialized in the filters
	Frequency(BassLoPass, 1000.0f);
	Frequency(MidHiPass, 1000.0f);
	Frequency(MidLoPass, 1000.0f);
	Frequency(TrebleHiPass, 1000.0f);
}

void Crossover::TwoWay(float fSplitHz)
{
	Frequency(BassLoPass, fSplitHz);
	Frequency(TrebleHiPass, fSplitHz);
	m_bThreeWay = false;
}

void Crossover::ThreeWay(float fSplitHzLo, float fSplitHzHi)
{
	Frequency(BassLoPass, fSplitHzLo);
	Frequency(MidHiPass, fSplitHzLo);
	Frequency(MidLoPass, fSplitHzHi);
	Frequency(TrebleHiPass, fSplitHzHi);
	m_bThreeWay = true;
}

void Crossover::Samplerate(float fSamplerate)
{
	if (m_fSamplerate != fSamplerate)
	{
		m_fSamplerate = fSamplerate;
		for (int i = BassLoPass; i <= TrebleHiPass; i++)
			if (m_fFrequency[i] > 0.0f)
				CalcCoeffs(i);
	}
}

void Crossover::Reset()
{
	m_f4X1 = m_f4X2 = m_f4Y1 = m_f4Y2 = 0.0f;
}

void Crossover::Process(float fInput)
{
	simd::float_4 f4Input(fInput, fInput, m_f4Output.s[MidHiPass], fInput);
	m_f4Output = m_f4B0 * f4Input + m_f4B1 * m_f4X1 + m_f4B2 * m_f4X2 - m_f4A1 * m_f4Y1 - m_f4A2 * m_f4Y2;

	// Update state variables
	m_f4X2 = m_f4X1;
	m_f4X1 = f4Input;
	m_f4Y2 = m_f4Y1;
	m_f4Y1 = m_f4Output;
}

float Crossover::Bass() const
{
	return m_f4Output.s[BassLoPass];
}

float Crossover::Mid() const
{
	if (m_bThreeWay)
		return -m_f4Output.s[MidLoPass];
	return 0.0f;
}

float Crossover::Treble() const
{
	if (m_bThreeWay)
		return m_f4Output.s[TrebleHiPass];
	return -m_f4Output.s[TrebleHiPass];
}

void Crossover::Frequency(int nChannel, float fFrequency)
{
	if (fFrequency != m_fFrequency[nChannel])
	{
		m_fFrequency[nChannel] = fFrequency;
		if (m_fSamplerate > 0.0f)
			CalcCoeffs(nChannel);
	}
}

void Crossover::CalcCoeffs(int nChannel)
{
	const float omega = 2.0f * M_PI * m_fFrequency[nChannel] / m_fSamplerate;

	// Compute coefficients for the normalized frequency
	const float k = tanf(omega * 0.5f);
	const float k2 = k * k;
	const float denominator = k2 + 2.0f * k + 1.0f;
	const float denominatorRecip = 1.0f / denominator;

	if (nChannel == BassLoPass || nChannel == MidLoPass)
	{
		// Lowpass coefficients
		m_f4B0.s[nChannel] = k2 * denominatorRecip;
		m_f4B1.s[nChannel] = 2.0f * m_f4B0.s[nChannel];
		m_f4B2.s[nChannel] = m_f4B0.s[nChannel];
		m_f4A1.s[nChannel] = 2.0f * (k2 - 1.0f) * denominatorRecip;
		m_f4A2.s[nChannel] = (k2 - 2.0f * k + 1.0f) * denominatorRecip;
	}
	else
	{ // HIGHPASS
		// Highpass coefficients
		m_f4B0.s[nChannel] = denominatorRecip;
		m_f4B1.s[nChannel] = -2.0f * m_f4B0.s[nChannel];
		m_f4B2.s[nChannel] = m_f4B0.s[nChannel];
		m_f4A1.s[nChannel] = 2.0f * (k2 - 1.0f) * denominatorRecip;
		m_f4A2.s[nChannel] = (k2 - 2.0f * k + 1.0f) * denominatorRecip;
	}
}
