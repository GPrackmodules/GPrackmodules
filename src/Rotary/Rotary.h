///
/// RotaryModule class
/// RotaryWidget class
/// ==================
/// Main classes for the Rotary effect
///

#pragma once

#include "Rotor4.h"
#include "MilliSampleDelayLine.h"
#include "Crossover.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
/// MODULE
//////////////////////////////////////////////////////////////////////////////////////////////////

struct RotaryModule : Module
{
///////////////////////////////////////////////////////////
/// Public enums
////////////////////////////////////////////////////////////

public:
	enum ParamId
	{
		Param_DoubleHi,
		// ParamCV_RampupHi,
		Param_RampupHi,
		// ParamCV_RPMFastHi,
		Param_RPMFastHi,
		// ParamCV_RPMSlowHi,
		Param_RPMSlowHi,

		Param_EnableMid,
		// ParamCV_RampupMid,
		Param_RampupMid,
		// ParamCV_RPMFastMid,
		Param_RPMFastMid,
		// ParamCV_RPMSlowMid,
		Param_RPMSlowMid,

		// ParamCV_RampupLo,
		Param_RampupLo,
		// ParamCV_RPMFastLo,
		Param_RPMFastLo,
		// ParamCV_RPMSlowLo,
		Param_RPMSlowLo,

		Param_Slow,
		Param_Fast,

		Param_MicDistance,
		Param_MicAngle,
		NumParams
	};
	enum InputId
	{
		InputCV_RampupHi,
		InputCV_RPMFastHi,
		InputCV_RPMSlowHi,

		InputCV_RampupMid,
		InputCV_RPMFastMid,
		InputCV_RPMSlowMid,

		InputCV_RampupLo,
		InputCV_RPMFastLo,
		InputCV_RPMSlowLo,

		Input_Mono,
		Input_Slow,
		Input_Fast,
		Input_Varispeed,
		Input_WheelSF,		// Mod wheel 0..33% = Stop, 33%..67% = Slow, > 67% = Fast
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
		// Light in frequency bands
		Light_DoubleHi,
		Light_Hi1L,
		Light_Hi1R,
		Light_Hi2L,
		Light_Hi2R,
		Light_EnableMid,
		Light_MidL,
		Light_MidR,
		Light_LoL,
		Light_LoR,

		// lights for slow/fast buttons and slow/fast feedback
		Light_Slow,			// light on button
		Light_Fast,			// light on button
		Light_FBSlow,		// Feedback light, showing actual state
		Light_FBFast,		// Feedback light, showing actual state

		NumLights
	};

////////////////////////////////////////////////////////////
/// Construction / destruction
////////////////////////////////////////////////////////////

public:
	RotaryModule();
	~RotaryModule();

///////////////////////////////////////////////////////////
/// Public API
////////////////////////////////////////////////////////////

public:
	void onSampleRateChange(const SampleRateChangeEvent &e) override;
	void processBypass(const ProcessArgs& args) override;
	void process(const ProcessArgs& args) override;

	bool VariSpeed() const { return m_bVariSpeed; }
	int64_t CurrentFrame() const { return m_nCurrentFrame; }

///////////////////////////////////////////////////////////
/// Private methods
////////////////////////////////////////////////////////////

private:
	void HandleDoubleHi(bool bForce = false);
	void HandleRampupHi(bool bForce = false);
	void HandleRPMFastHi(bool bForce = false);
	void HandleRPMSlowHi(bool bForce = false);

	void HandleEnableMid(bool bForce = false);
	void HandleRampupMid(bool bForce = false);
	void HandleRPMFastMid(bool bForce = false);
	void HandleRPMSlowMid(bool bForce = false);

	void HandleRampupLo(bool bForce = false);
	void HandleRPMFastLo(bool bForce = false);
	void HandleRPMSlowLo(bool bForce = false);

	void HandleTargetSpeed(bool bForce = false);
	void HandleMicDistance(bool bForce = false);
	void HandleMicAngle(bool bForce = false);

	void LightsSlow();
	void LightsFast();
	void LightsStopped();
	void LightsOff();

///////////////////////////////////////////////////////////
/// Data
////////////////////////////////////////////////////////////

private:
	enum BandIndex : int
	{
		BandLo = 0,
		BandMid,
		BandHi1,
		BandHi2,
		NumBands
	};

	bool m_bInitialized = false;
	float m_fInvertedSamplerate = 1.0f;
	int64_t m_nCurrentFrame = 0;

	// Parameter values, initial values here don't matter, they are set in the first call to process() from the actual parameters
	// The ParamCV values are the attenuverters for the CV inputs
	bool m_bDoubleHi = true;
	float m_fParamRampupHi = 0.0f;
	float m_fParamCVRampupHi = 0.0f;
	float m_fCVRampupHi = 0.0f;
	float m_fParamRPMFastHi = 0.0f;
	float m_fParamCVRPMFastHi = 0.0f;
	float m_fCVRPMFastHi = 0.0f;
	float m_fParamRPMSlowHi = 0.0f;
	float m_fParamCVRPMSlowHi = 0.0f;
	float m_fCVRPMSlowHi = 0.0f;

	bool m_bEnableMid = false;
	float m_fParamRampupMid = 0.0f;
	float m_fParamCVRampupMid = 0.0f;
	float m_fCVRampupMid = 0.0f;
	float m_fParamRPMFastMid = 0.0f;
	float m_fParamCVRPMFastMid = 0.0f;
	float m_fCVRPMFastMid = 0.0f;
	float m_fParamRPMSlowMid = 0.0f;
	float m_fParamCVRPMSlowMid = 0.0f;
	float m_fCVRPMSlowMid = 0.0f;

	float m_fParamRampupLo = 0.0f;
	float m_fParamCVRampupLo = 0.0f;
	float m_fCVRampupLo = 0.0f;
	float m_fParamRPMFastLo = 0.0f;
	float m_fParamCVRPMFastLo = 0.0;
	float m_fCVRPMFastLo = 0.0;
	float m_fParamRPMSlowLo = 0.0f;
	float m_fParamCVRPMSlowLo = 0.0f;
	float m_fCVRPMSlowLo = 0.0f;

	bool m_bParamSlow = false;
	bool m_bParamFast = false;

	float m_fParam_MicDistance = 0.0f;
	float m_fParam_MicAngle = 0.0f;

	MilliSampleDelayLine* m_pDelayHi = nullptr;
	MilliSampleDelayLine* m_pDelayMid = nullptr;
	MilliSampleDelayLine* m_pDelayLo = nullptr;

	bool m_bStereo = false;
	Rotor4 m_Rotor4;
	Crossover m_Crossover;

	float m_fMinDelay;

	float m_fDistanceGainFactor = 1.0f;		// between MIN_DISTGAINFACTOR (at max distance) and 1.0 (closest)

	bool m_bSlow = false;
	bool m_bFast = false;
	bool m_bVariSpeed = false;
	bool m_bBypassed = false;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
/// WIDGET
//////////////////////////////////////////////////////////////////////////////////////////////////

struct RotaryWidget : ModuleWidget
{
////////////////////////////////////////////////////////////
/// Construction / destruction
////////////////////////////////////////////////////////////

public:
	RotaryWidget(RotaryModule* pModule);
	~RotaryWidget();
////////////////////////////////////////////////////////////
/// Public API
////////////////////////////////////////////////////////////

	void ShowFBLights(bool bShow);

////////////////////////////////////////////////////////////
/// Private methods
////////////////////////////////////////////////////////////

private:
	void step() override;

	void CreateBandControls(int nStartY, int nParamStart, int nFirstLight, int nLights); // nLight only used if bWithSwitch

////////////////////////////////////////////////////////////
/// Data
////////////////////////////////////////////////////////////

private:
	RotaryModule * const m_pModule = nullptr;
	LargeLight<YellowLight>* m_pLightFBSlow = nullptr;
	LargeLight<RedLight>* m_pLightFBFast = nullptr;
	bool m_bFBLightsVisible = true;
};

extern Model* the_pRotaryModel;
