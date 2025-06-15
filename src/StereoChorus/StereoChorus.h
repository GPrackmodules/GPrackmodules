#pragma once

#include "Knobs.h"
#include "Fade.h"
#include "Fade2.h"
#include "ChorusDelayLine.h"

#define	STEREO_CHORUS_VOICES		4

#define	MIN_DELAY					(0.0002f)		// 0.2ms @1 Hz
#define	MAX_DELAY					(0.012f)		// 12ms @1 Hz, Center delay, max is double that

/////////////////////////////////////////////////////
/// Module
/////////////////////////////////////////////////////

class StereoChorusModule : public Module
{
public:
	enum ParamId
	{
		ParamRate,
		ParamDepth,
		ParamTone,
		ParamVoices,
		ParamWet,
		ParamCVRate,
		ParamCVDepth,
		ParamCVTone,
		ParamCVWet,
		NumParams,
	};
	enum InputId
	{
		InputL,
		InputR,
		InputMod1,
		InputMod2,
		InputMod3,
		InputMod4,
		InputCVRate,
		InputCVDepth,
		InputCVTone,
		InputCVWet,
		NumInputs
	};
	enum OutputId
	{
		OutputL,
		OutputR,
		NumOutputs
	};
	enum LightId
	{
		Light1Mono,
		Light2Mono,
		Light3Mono,
		Light4Mono,
		Light1Left,
		Light2Left,
		Light3Left,
		Light4Left,
		Light1Right,
		Light2Right,
		Light3Right,
		Light4Right,
		NumLights
	};

public:
	StereoChorusModule();
	~StereoChorusModule();

public:
	void setSampleRate(float fSamplerate);
	bool Stereo() const { return m_bStereo; }
	void SetDarkMode(bool bDarkMode);

	void process(const ProcessArgs& args) override;
	void onSampleRateChange(const SampleRateChangeEvent &e) override;
	void SetWidget(struct StereoChorusWidget* pWidget) { m_pWidget = pWidget; }

private:
	typedef struct _LFO
	{
		float fPhase = 0.0f;		// runs from 0..1.0f
		float fFrequency = 1.0f;	// 1.0f / freuqency
		float fSine;
		float fCosine;				// only valid if m_bStereo
	}
	LFO;
private:
	void AdvanceLFO(LFO& rLFO);
	void HandleVoices(bool bForce = false); // also handles mono/stereo
	void HandleRate(bool bForce = false);
	void HandleDepth(bool bForce = false);
	void HandleTone(bool bForce = false);
	void HandleWet(bool bForce = false);
	void CalcGainFactor();
	void LPCutoff(float fHz);
	void HPCutoff(float fHz);
	void UpdateSamplerate(float fSamplerate);

private:
	struct StereoChorusWidget* m_pWidget = nullptr;
	bool m_bInitialized = false;
	bool m_bHasInput = false;
	int64_t m_nLastInputFrame;

	int m_nVoices = 1;
	bool m_bStereo = false;
	float m_fInvertedSamplerate = 1.0f;
	float m_fSamplerate = 1.0f;

	float m_fValueRate = 0.5f;
	float m_fAvgFrequency = 1.0f;
	LFO m_LFOs[STEREO_CHORUS_VOICES];

	float m_fValueDepth = 0.5f;
	float m_fDepthDelay = 0.0f;
	float m_fDelayRanges[STEREO_CHORUS_VOICES] = { 0.0f, 0.0f, 0.0f, 0.0f };
	Fade m_fadeDelayRanges[STEREO_CHORUS_VOICES];

	float m_fValueTone;
	dsp::TRCFilter<simd::float_4> m_fltTone;// channel 0/1 = Lowpass left/right, 2/3 = Highpass Left/right
	Fade2 m_fadeLowpass;					// smooth cutoff frequency fades
	Fade2 m_fadeHighpass;					// smooth cutoff frequency fades

	Fade m_fadeVoices[STEREO_CHORUS_VOICES - 1];	// nor for voice 1, always on
	float m_fVoiceFactors[STEREO_CHORUS_VOICES] = { 1.0f, 0.0f, 0.0f, 0.0f };
	Fade m_fadeGainCorrection;
	float m_fGainCorrection = 0.25f; // depends on depth and number of voices

		float m_fValueWet = 0.5f;
	Fade2 m_fadeWet;
	float m_fDryFactor = 0.0f;
	float m_fWetFactor = 0.0f;

	// factors and offsets for lights
	float m_fLightOff	= -1.0f;
	// Brightness = offset + sin/cosine * FactorModulation + DepthPercent * FactorDepth
	bool m_bDarkMode = false;
	float m_fLightFactorModulation = 0.3f;
	float m_fLightFactorDepth =	0.003f;
	float m_fLightFactorOffset = 0.4f;

	ChorusDelayLine* m_pDelayLine = nullptr;
};

/////////////////////////////////////////////////////
/// Widget
/////////////////////////////////////////////////////

struct StereoChorusWidget : ModuleWidget
{
public:
	StereoChorusWidget(StereoChorusModule* pModule);

protected:
	void step() override;

private:
	StereoChorusModule* m_pModule;
	bool m_bDarkMode;
	bool m_bStereo = false;

	MediumLight<BlueLight>* m_aLightsMono[STEREO_CHORUS_VOICES];
	MediumLight<BlueLight>* m_aLightsStereo[STEREO_CHORUS_VOICES][2];
};

extern Model* the_pStereoChorusModel;
