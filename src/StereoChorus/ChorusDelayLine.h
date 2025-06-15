#pragma once

#include "plugin.hpp"

#define N_TAPS					31
#define N_SUBSAMPLE				1000

class ChorusDelayLine
{
public:
public:
	ChorusDelayLine(float fSamplerate, int nChannels, float dMaxDelayS);
	ChorusDelayLine(const ChorusDelayLine&) = delete;
	~ChorusDelayLine();

public:
	static int MaxTaps() { return 1023; }

	void Feed(float fL, float fR);
	float Read(int nChannel, float fDelayS);
	void Advance();
	void UpdateSamplerate(float fSamplerate);

	bool BuildIRs(float fCutoffFrequency = 0.45f);
	void DeleteOldIRs();

	int MaxDelaySamples() const { return m_nMaxDelaySamples; }

	ChorusDelayLine & operator=(const ChorusDelayLine&) = delete;

private:
	void DeleteTempIRs();
	void DeleteIRs();
	void UpdateIRs();

private:
	const int m_nChannels;
	const float m_fMaxDelayS;
	float m_fSamplerate;
	int m_nMaxDelaySamples = 0;		// enough for MaxDelayS

	float** m_ppDelayLines = nullptr;	// array of float pointers to the delays lines for each channel

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
