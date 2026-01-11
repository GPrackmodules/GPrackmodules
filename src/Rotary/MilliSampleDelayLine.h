#pragma once

#include "plugin.hpp"

#define N_TAPS					31
#define N_SUBSAMPLE				1000

class MilliSampleDelayLine
{
public:
public:
	MilliSampleDelayLine(float fSamplerate, float dMaxDelayS);
	MilliSampleDelayLine(const MilliSampleDelayLine&) = delete;
	~MilliSampleDelayLine();

public:
	static int MaxTaps() { return 1023; }

	void Feed(float fInput);
	float Read(float fDelayS);
	void Advance();
	void UpdateSamplerate(float fSamplerate);

	bool BuildIRs(float fCutoffFrequency = 0.45f);
	void DeleteOldIRs();

	int MaxDelaySamples() const { return m_nMaxDelaySamples; }

	MilliSampleDelayLine & operator=(const MilliSampleDelayLine&) = delete;

private:
	void DeleteTempIRs();
	void DeleteIRs();
	void UpdateIRs();

private:
	const float m_fMaxDelayS;
	float m_fSamplerate;
	int m_nMaxDelaySamples = 0;		// enough for MaxDelayS

	float* m_pDelayLine = nullptr;	// array of float pointers to the delays lines for each channel

	// Impulse responses and parameters currently in use
	mutex m_mtxIRs;			// only locked briefly for IR switching
	float** m_ppIRs = nullptr;

	mutex m_mtxOldIRs;			// only locked briefly for IR switching
	list<pair<int, float**>> m_lstOldIRs;	// int is number of subsamples IRs

	// New Impulse responses, still need to be activated
	mutex m_mtxTempIRs;		// locked a little longer while calculating subsampled IRs
	float** m_ppTempIRs = nullptr;

	int m_nWriteIndex = 0;
};
