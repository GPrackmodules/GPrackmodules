///
/// RotaryModule class
/// RotaryWidget class
/// ======================
/// Main classes for the Rotary effect
///

#include "plugin.hpp"
#include "Rotary.h"
#include "common/Knobs.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MODULE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Speeds, Rampup

#define FAST_MIN			(30.0f)
#define FAST_MAX			(600.0f)
#define FAST_HI_DEF			(400.0f)
#define FAST_MID_DEF		(387.0f)
#define FAST_LO_DEF			(375.0f)

#define SLOW_MIN			(10.0f)
#define SLOW_MAX			(260.0f)
#define SLOW_HI_DEF			(48.0f)
#define SLOW_MID_DEF		(53.0f)
#define SLOW_LO_DEF			(69.0f)

#define RAMPUP_MIN			(0.0f)
#define RAMPUP_MAX			(12.0f)
#define RAMPUP_HI_DEF		(1.5f)
#define RAMPUP_MID_DEF		(3.5f)
#define RAMPUP_LO_DEF		(5.0f)

#define RAMP_DOWN_FACTOR	(1.25f)				// Rampdown takes 20% longer than SpiUp (except for Fast->Slow transition)
#define RPM_TO_HZ			( 1.0f / 60.0f )
// Hi 2 Ramps up faster and rotates a little faster
#define RAMPUP_FACTOR_HI2	(0.92346213f)
#define RPM_HI2_FACTOR		(1.0278678564f)

#define DIST_MIN			(0.1f)
#define DIST_MAX			(1.0f)
#define DIST_DEF			(0.5f)

// Attenuverters
#define ATV_MIN				(-100.0f)
#define ATV_MAX				(100.0f)
#define ATV_DEF				(0.0f)

// Lights
#define GREEN_OFF			(0.1f)
#define GREEN_ON			(0.8f)
#define BLUE_OFF			(0.2f)
#define BLUE_ON				(1.0f)
#define YELLOW_OFF			(0.1f)
#define YELLOW_ON			(0.6f)
#define RED_OFF				(0.2f)
#define RED_ON				(1.0f)

// Crossover
#define TWOWAY_XOVER_FREQUENCY		(800.0f)
#define THREEWAY_XOVER_LO			(250.0f)
#define THREEWAY_XOVER_HI			(2000.0f)


// Delays
#define SPEED_OF_SOUND				(340.0f)	// m/s
#define DIAMETER					(7.0f * 0.0254f)
#define DIAMETER_DELAY				(DIAMETER / SPEED_OF_SOUND)
#define MIN_DELAY					(0.001f)									// some room for float precision errors
#define MAX_DELAY					(MIN_DELAY + 2 * DIAMETER_DELAY + 0.001)	// some room for float precision errors

// Gain factors
#define GAINFACTOR_NEAR				(DIST_MIN / (DIST_MIN + 2 * DIAMETER))
#define GAINFACTOR_FAR				(DIST_MAX / (DIST_MAX + 2 * DIAMETER))

#define MIN_DISTGAINFACTOR			((1.0f - GAINFACTOR_FAR) / (1.0f - GAINFACTOR_NEAR))
// factors for gain modulation in the bands, these apply to a 0..2.0 rotar value
#define TREBLE_GAINDEPTH			((1.0f - GAINFACTOR_NEAR) / 2.0f)	// rotor value runs from 0 to 2
#define MID_GAINDEPTH				(TREBLE_GAINDEPTH / 2.0f)	// speaker is less directional that horn
#define BASS_GAINDEPTH				(TREBLE_GAINDEPTH / 4.0f)	// speaker is less directional that horn


static float s_fMinus4Pt5dB = powf(10.0f, -4.5f / 20.0f);

RotaryModule::RotaryModule() :
	m_Rotor4(this),
	m_Crossover(true),
	m_fMinDelay(MIN_DELAY)
{
	config(NumParams, NumInputs, NumOutputs, NumLights);

	configParam(Param_DoubleHi, 0.0f, 1.0f, 1.0f, "Double Horns");
	paramQuantities[Param_DoubleHi]->snapEnabled = true;
	configParam(Param_RampupHi, RAMPUP_MIN, RAMPUP_MAX, RAMPUP_HI_DEF, "Ramp Up Treble", "s");
	paramQuantities[Param_RampupHi]->displayPrecision = 2;
	configParam(Param_RPMFastHi, FAST_MIN, FAST_MAX, FAST_HI_DEF, "RPM Fast Treble", "rpm");
	paramQuantities[Param_RPMFastHi]->snapEnabled = true;
	configParam(Param_RPMSlowHi, SLOW_MIN, SLOW_MAX, SLOW_HI_DEF, "RPM Slow Treble", "rpm");
	paramQuantities[Param_RPMSlowHi]->snapEnabled = true;

	configParam(Param_EnableMid, 0.0f, 1.0f, 1.0f, "Enable Mid (3-way)");
	paramQuantities[Param_EnableMid]->snapEnabled = true;
	configParam(Param_RampupMid, RAMPUP_MIN, RAMPUP_MAX, RAMPUP_MID_DEF, "Ramp Up Mid", "s");
	paramQuantities[Param_RampupMid]->displayPrecision = 2;
	configParam(Param_RPMFastMid, FAST_MIN, FAST_MAX, FAST_MID_DEF, "RPM Fast Mid", "rpm");
	paramQuantities[Param_RPMFastMid]->snapEnabled = true;
	configParam(Param_RPMSlowMid, SLOW_MIN, SLOW_MAX, SLOW_MID_DEF, "RPM Slow Mid", "rpm");
	paramQuantities[Param_RPMSlowMid]->snapEnabled = true;

	configParam(Param_RampupLo, RAMPUP_MIN, RAMPUP_MAX, RAMPUP_LO_DEF, "Ramp Up Bass", "s");
	paramQuantities[Param_RampupLo]->displayPrecision = 2;
	configParam(Param_RPMFastLo, FAST_MIN, FAST_MAX, FAST_LO_DEF, "RPM Fast Bass", "rpm");
	paramQuantities[Param_RPMFastLo]->snapEnabled = true;
	configParam(Param_RPMSlowLo, SLOW_MIN, SLOW_MAX, SLOW_LO_DEF, "RPM Slow Bass", "rpm");
	paramQuantities[Param_RPMSlowLo]->snapEnabled = true;

	configParam(Param_Slow, 0.0f, 1.0f, 0.0f, "Slow Roration");
	paramQuantities[Param_Slow]->snapEnabled = true;
	configParam(Param_Fast, 0.0f, 1.0f, 0.0f, "Fast Roration");
	paramQuantities[Param_Fast]->snapEnabled = true;

	configParam(Param_MicDistance, DIST_MIN, DIST_MAX, DIST_DEF,"Mic Distance", "m");
	paramQuantities[Param_MicDistance]->displayPrecision = 2;
	configParam(Param_MicAngle, 90.0f, 180.0f, 120.0f, "Angle between microphones", "deg");
	paramQuantities[Param_MicAngle]->snapEnabled = true;

	configInput(InputCV_RampupHi, "Rampup Treble CV");
	configInput(InputCV_RPMFastHi, "Fast RPM Treble CV");
	configInput(InputCV_RPMSlowHi,"Slow RPM Treble CV");

	configInput(InputCV_RampupMid, "Rampup Mid CV");
	configInput(InputCV_RPMFastMid, "Fast RPM Mid CV");
	configInput(InputCV_RPMSlowMid,"Slow RPM Mid CV");

	configInput(InputCV_RampupLo, "Rampup Bass CV");
	configInput(InputCV_RPMFastLo, "Fast RPM Bass CV");
	configInput(InputCV_RPMSlowLo,"Slow RPM Bass CV");

	configInput(Input_Mono, "Audio");
	configInput(Input_Slow, "Slow Gate");
	configInput(Input_Fast, "Fast Gate");
	configInput(Input_Varispeed, "Varispeed CV");
	configInput(Input_WheelSF, "Wheel Stop/Slow/Fast");

	configOutput(OutputL, "Left");
	configOutput(OutputR, "Right");
}

RotaryModule::~RotaryModule()
{
	delete m_pDelayHi;
	delete m_pDelayMid;
	delete m_pDelayLo;
}

// bool RotaryModule::VariSpeed() const
// {
// 	return m_bVariSpeed;
// }

void RotaryModule::onSampleRateChange(const SampleRateChangeEvent &e) /*override*/
{
	m_fInvertedSamplerate = 1.0f / e.sampleRate;
	if (m_pDelayHi != nullptr)
		m_pDelayHi->UpdateSamplerate(e.sampleRate);
	if (m_pDelayMid != nullptr)
		m_pDelayMid->UpdateSamplerate(e.sampleRate);
	if (m_pDelayLo != nullptr)
		m_pDelayLo->UpdateSamplerate(e.sampleRate);
}

void RotaryModule::processBypass(const ProcessArgs& args) /*override*/
{
	m_nCurrentFrame = args.frame;
	if (!m_bBypassed)
	{
		LightsOff();
		m_bBypassed = true;
	}

	float fIn = inputs[Input_Mono].getVoltageSum();
	outputs[OutputL].setVoltage(fIn);
	outputs[OutputR].setVoltage(fIn);
}

void RotaryModule::process(const ProcessArgs& args) /*override*/
{
	m_nCurrentFrame = args.frame;
	m_bBypassed = false;
	bool bForce = false;
	if (!m_bInitialized)
	{
		m_bInitialized = true;
		bForce = true;
		m_fInvertedSamplerate = 1.0f / args.sampleRate;
		m_pDelayHi = new MilliSampleDelayLine(args.sampleRate, MAX_DELAY + 0.01);	// some extra delay to allow for impreecision in math
		m_pDelayMid = new MilliSampleDelayLine(args.sampleRate, MAX_DELAY + 0.01);	// some extra delay to allow for impreecision in math
		m_pDelayLo = new MilliSampleDelayLine(args.sampleRate, MAX_DELAY + 0.01);	// some extra delay to allow for impreecision in math
		m_Crossover.Samplerate(args.sampleRate);
		m_Rotor4.Samplerate(args.sampleRate);
		if (m_bEnableMid)
			m_Crossover.ThreeWay(THREEWAY_XOVER_LO, THREEWAY_XOVER_HI);
		else
			m_Crossover.TwoWay(TWOWAY_XOVER_FREQUENCY);
	}
	HandleDoubleHi(bForce);
	HandleRampupHi(bForce);
	HandleRPMFastHi(bForce);
	HandleRPMSlowHi(bForce);
	HandleEnableMid(bForce);
	HandleRampupMid(bForce);
	HandleRPMFastMid(bForce);
	HandleRPMSlowMid(bForce);
	HandleRampupLo(bForce);
	HandleRPMFastLo(bForce);
	HandleRPMSlowLo(bForce);

	HandleMicDistance(bForce);
	HandleMicAngle(bForce);
	HandleTargetSpeed(bForce);

	bool bStereo = outputs[OutputL].isConnected() && outputs[OutputR].isConnected();
	if (bStereo != m_bStereo)
		m_bStereo = bStereo;
	m_Rotor4.Advance();

	// Crossover feeds delay lines
	float fIn = inputs[Input_Mono].getVoltageSum();
	m_Crossover.Process(fIn);

	float fHi = m_Crossover.Treble();
	m_pDelayHi->Feed(fHi);
	if (m_bEnableMid)
	{
		float fMid = m_Crossover.Mid();
		m_pDelayMid->Feed(fMid);
	}
	float fLo = m_Crossover.Bass();
	m_pDelayLo->Feed(fLo);

	// reading output from delay lines
	float fRotorL = 1.0f + m_Rotor4.OutputL(BandHi1);
	float fGainFactor = 1.0f - TREBLE_GAINDEPTH * m_fDistanceGainFactor * fRotorL;
	lights[Light_Hi1L].setBrightness(GREEN_OFF + (fRotorL * (GREEN_ON - GREEN_OFF) / 2.0f));
	float fOutL = m_pDelayHi->Read(m_fMinDelay + DIAMETER_DELAY * fRotorL) * fGainFactor;
	float fOutR = 0.0f;
	float fRotorR = 0.0f;
	if (m_bStereo)
	{
		float fRotorR = 1.0f + m_Rotor4.OutputR(BandHi1);
		fGainFactor = 1.0f - TREBLE_GAINDEPTH * m_fDistanceGainFactor * fRotorR;
		lights[Light_Hi1R].setBrightness(GREEN_OFF + (fRotorR * (GREEN_ON - GREEN_OFF) / 2.0f));
		fOutR = m_pDelayHi->Read(m_fMinDelay + DIAMETER_DELAY * fRotorR) * fGainFactor;
	}
	if (m_bDoubleHi)
	{
		fRotorL = 1.0f + m_Rotor4.OutputL(BandHi2);
		fGainFactor = 1.0f - TREBLE_GAINDEPTH * m_fDistanceGainFactor * fRotorL;
		lights[Light_Hi2L].setBrightness(GREEN_OFF + (fRotorL * (GREEN_ON - GREEN_OFF) / 2.0f));
		fOutL += m_pDelayHi->Read(m_fMinDelay + DIAMETER_DELAY * fRotorL) * fGainFactor;
		fOutL *= s_fMinus4Pt5dB;
		if (m_bStereo)
		{
			fRotorR = 1.0f + m_Rotor4.OutputR(BandHi2);
			fGainFactor = 1.0f - TREBLE_GAINDEPTH * m_fDistanceGainFactor * fRotorR;
			lights[Light_Hi2R].setBrightness(GREEN_OFF + (fRotorR * (GREEN_ON - GREEN_OFF) / 2.0f));
			fOutR += m_pDelayHi->Read(m_fMinDelay + DIAMETER_DELAY * fRotorR) * fGainFactor;
			fOutR *= s_fMinus4Pt5dB;
		}
	}

	// Bass and Mid rotate in the opposite direction, swap L and R
	fRotorL = 1.0f + m_Rotor4.OutputR(BandLo);
	fGainFactor = 1.0f - BASS_GAINDEPTH * m_fDistanceGainFactor * fRotorL;
	lights[Light_LoL].setBrightness(GREEN_OFF + (fRotorL * (GREEN_ON - GREEN_OFF) / 2.0f));
	fOutL += m_pDelayLo->Read(m_fMinDelay + DIAMETER_DELAY * fRotorL) * fGainFactor;
	if (m_bStereo)
	{
		fRotorR = 1.0f + m_Rotor4.OutputL(BandLo);
		fGainFactor = 1.0f - BASS_GAINDEPTH * m_fDistanceGainFactor * fRotorR;
		lights[Light_LoR].setBrightness(GREEN_OFF + (fRotorR * (GREEN_ON - GREEN_OFF) / 2.0f));
		fOutR += m_pDelayLo->Read(m_fMinDelay + DIAMETER_DELAY * fRotorL) * fGainFactor;
	}

	if (m_bEnableMid)
	{
		fRotorL = 1.0f + m_Rotor4.OutputR(BandMid);
		fGainFactor = 1.0f - MID_GAINDEPTH * m_fDistanceGainFactor * fRotorL;
		lights[Light_MidL].setBrightness(GREEN_OFF + (fRotorL * (GREEN_ON - GREEN_OFF) / 2.0f));
		fOutL += m_pDelayMid->Read(m_fMinDelay + DIAMETER_DELAY * fRotorL) * fGainFactor;
		if (m_bStereo)
		{
			fRotorR = 1.0f + m_Rotor4.OutputL(BandMid);
			fGainFactor = 1.0f - MID_GAINDEPTH * m_fDistanceGainFactor * fRotorR;
			lights[Light_MidR].setBrightness(GREEN_OFF + (fRotorR * (GREEN_ON - GREEN_OFF) / 2.0f));
			fOutR += m_pDelayMid->Read(m_fMinDelay + DIAMETER_DELAY * fRotorL) * fGainFactor;
		}
	}
	if (m_bStereo)
	{
		outputs[OutputL].setVoltage(fOutL);
		outputs[OutputR].setVoltage(fOutR);
	}
	else
	{
		if (outputs[OutputL].isConnected())
			outputs[OutputL].setVoltage(fOutL);
		else if (outputs[OutputR].isConnected())
			outputs[OutputR].setVoltage(fOutL);  // mono signal is always in variables for left channel
	}

	m_pDelayHi->Advance();
	m_pDelayMid->Advance();
	m_pDelayLo->Advance();
}

void RotaryModule::HandleDoubleHi(bool bForce)
{
	bool bDoubleHi = params[Param_DoubleHi].getValue() > 0.5;
	if (bDoubleHi != m_bDoubleHi || bForce)
	{
		m_bDoubleHi = bDoubleHi;
		lights[Light_DoubleHi].setBrightness(m_bDoubleHi ? BLUE_ON : BLUE_OFF);
		if (!bDoubleHi)
		{
			lights[Light_Hi2L].setBrightness(0.0f);
			lights[Light_Hi2R].setBrightness(0.0f);
		}
	}
}

void RotaryModule::HandleRampupHi(bool bForce)
{
	float fParam = params[Param_RampupHi].getValue();
	if (fParam != m_fParamRampupHi || bForce)
	{
		m_fParamRampupHi = fParam;
		float fRampupTime = fParam;
		m_Rotor4.RampupTime(BandHi1, fRampupTime);
		fRampupTime *= RAMPUP_FACTOR_HI2;
		m_Rotor4.RampupTime(BandHi2, fRampupTime);
	}
}

void RotaryModule::HandleRPMFastHi(bool bForce)
{
	float fParam = params[Param_RPMFastHi].getValue();
	if (fParam != m_fParamRPMFastHi || bForce)
	{
		m_fParamRPMFastHi = fParam;
		float fRPM = fParam;
		m_Rotor4.FastRPM(BandHi1, fRPM);
		m_Rotor4.FastRPM(BandHi2, fRPM * RPM_HI2_FACTOR);
	}
}

void RotaryModule::HandleRPMSlowHi(bool bForce)
{
	float fParam = params[Param_RPMSlowHi].getValue();
	if (fParam != m_fParamRPMSlowHi || bForce)
	{
		m_fParamRPMSlowHi = fParam;
		float fRPM = fParam;
		m_Rotor4.SlowRPM(BandHi1, fRPM);
		m_Rotor4.SlowRPM(BandHi2, fRPM * RPM_HI2_FACTOR);
	}
}

void RotaryModule::HandleEnableMid(bool bForce)
{
	bool bEnableMid = params[Param_EnableMid].getValue() > 0.5f;
	if (bEnableMid != m_bEnableMid || bForce)
	{
		m_bEnableMid = bEnableMid;
		lights[Light_EnableMid].setBrightness(m_bEnableMid ? BLUE_ON : BLUE_OFF);
		if (m_bEnableMid)
		{
			m_Crossover.ThreeWay(THREEWAY_XOVER_LO, THREEWAY_XOVER_HI);
			m_Crossover.Reset();
		}
		else
		{
			m_Crossover.TwoWay(TWOWAY_XOVER_FREQUENCY);
			m_Crossover.Reset();
			lights[Light_MidL].setBrightness(0.0f);
			lights[Light_MidR].setBrightness(0.0f);
		}
	}
}

void RotaryModule::HandleRampupMid(bool bForce)
{
	float fParam = params[Param_RampupMid].getValue();
	if (fParam != m_fParamRampupMid || bForce)
	{
		m_fParamRampupMid = fParam;
		float fRampupTime = fParam;
		m_Rotor4.RampupTime(BandMid, fRampupTime);
	}
}

void RotaryModule::HandleRPMFastMid(bool bForce)
{
	float fParam = params[Param_RPMFastMid].getValue();
	if (fParam != m_fParamRPMFastMid || bForce)
	{
		m_fParamRPMFastMid = fParam;
		float fRPM = fParam;
		m_Rotor4.FastRPM(BandMid, fRPM);
	}
}

void RotaryModule::HandleRPMSlowMid(bool bForce)
{
	float fParam = params[Param_RPMSlowMid].getValue();
	if (fParam != m_fParamRPMSlowMid || bForce)
	{
		m_fParamRPMSlowMid = fParam;
		float fRPM = fParam;
		m_Rotor4.SlowRPM(BandMid, fRPM);
	}
}

void RotaryModule::HandleRampupLo(bool bForce)
{
	float fParam = params[Param_RampupLo].getValue();
	if (fParam != m_fParamRampupLo || bForce)
	{
		m_fParamRampupLo = fParam;
		float fRampupTime = fParam;
		m_Rotor4.RampupTime(BandLo, fRampupTime);
	}
}

void RotaryModule::HandleRPMFastLo(bool bForce)
{
	float fParam = params[Param_RPMFastLo].getValue();
	if (fParam != m_fParamRPMFastLo || bForce)
	{
		m_fParamRPMFastLo = fParam;
		float fRPM = fParam;
		m_Rotor4.FastRPM(BandLo, fRPM);
	}
}

void RotaryModule::HandleRPMSlowLo(bool bForce)
{
	float fParam = params[Param_RPMSlowLo].getValue();
	if (fParam != m_fParamRPMSlowLo || bForce)
	{
		m_fParamRPMSlowLo = fParam;
		float fRPM = fParam;
		m_Rotor4.SlowRPM(BandLo, fRPM);
	}
}

void RotaryModule::HandleTargetSpeed(bool bForce /*= false*/)
{
	bool bParamFast = params[Param_Fast].getValue() > 0.5f;
	if (bParamFast != m_bParamFast)
	{
		m_bParamFast = bParamFast;
		lights[Light_Fast].value = m_bParamFast ? RED_ON : RED_OFF;
	}
	bool bParamSlow = params[Param_Slow].getValue() > 0.5f;
	if (bParamSlow != m_bParamSlow)
	{
		m_bParamSlow = bParamSlow;
		lights[Light_Slow].value = m_bParamSlow ? YELLOW_ON : YELLOW_OFF;
	}

	if (inputs[Input_Varispeed].isConnected())
	{
		float fRelativeSpeed = inputs[Input_Varispeed].getVoltage() / 10.0f;
		m_bVariSpeed = true;
		m_Rotor4.TargetSpeed(Rotor4::SpeedTarget::VariSpeed, fRelativeSpeed);
	}
	else
	{
		float fInput = inputs[Input_WheelSF].getVoltage();
		if (inputs[Input_Fast].getVoltage() > 1.0f || fInput > 6.6666666f || bParamFast)
		{
			m_Rotor4.TargetSpeed(Rotor4::SpeedTarget::Fast);
			LightsFast();
		}
		else if (inputs[Input_Slow].getVoltage() > 1.0f || fInput > 3.3333333f || bParamSlow)
		{
			m_Rotor4.TargetSpeed(Rotor4::SpeedTarget::Slow);
			LightsSlow();
		}
		else
		{
			m_Rotor4.TargetSpeed(Rotor4::SpeedTarget::Stop);
			LightsStopped();
		}
		m_bVariSpeed = false;
	}
}

void RotaryModule::HandleMicDistance(bool bForce)
{
	float fParam = params[Param_MicDistance].getValue();
	if (fParam != m_fParam_MicDistance || bForce)
	{
		m_fParam_MicDistance = fParam;
		m_fDistanceGainFactor = MIN_DISTGAINFACTOR + (1.0f - (fParam - DIST_MIN) / (DIST_MAX - DIST_MIN)) * (1.0f - MIN_DISTGAINFACTOR);
	}
}

void RotaryModule::HandleMicAngle(bool bForce)
{
	float fParam = params[Param_MicAngle].getValue();
	if (fParam != m_fParam_MicAngle || bForce)
	{
		m_fParam_MicAngle = fParam;
		m_Rotor4.PhaseOffsetLR(fParam);
	}
}

void RotaryModule::LightsSlow()
{
	if (!m_bSlow)
	{
		lights[Light_FBSlow].setBrightness(YELLOW_ON);
		m_bSlow = true;
	}
	if (m_bFast)
	{
		lights[Light_FBFast].setBrightness(RED_OFF);
		m_bFast = false;
	}
}

void RotaryModule::LightsFast()
{
	if (m_bSlow)
	{
		lights[Light_FBSlow].setBrightness(YELLOW_OFF);
		m_bSlow = false;
	}
	if (!m_bFast)
	{
		lights[Light_FBFast].setBrightness(RED_ON);
		m_bFast = true;
	}
}

void RotaryModule::LightsStopped()
{
	if (m_bSlow)
	{
		lights[Light_FBSlow].setBrightness(YELLOW_OFF);
		m_bSlow = false;
	}
	if (m_bFast)
	{
		lights[Light_FBFast].setBrightness(RED_OFF);
		m_bFast = false;
	}
}

void RotaryModule::LightsOff()
{
	lights[Light_FBSlow].setBrightness(0.0f);
	lights[Light_FBFast].setBrightness(0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// WIDGET
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// Coordinates (all in mm)
//

#define TOTAL_HPS			(16.0f)		// total horizontal grid units
#define TOTAL_WIDTH			(TOTAL_HPS * RACK_GRID_WIDTH_MM)

#define SOCKET_X1			(7.08f)		// leftmost/rightmost socket

// These X coordinates apply to left hand and right hand controls/inputs
#define PARAM_BAND_FAST_X	(50.8f - 19.0f)
#define PARAM_BAND_RAMPUP_X	(PARAM_BAND_FAST_X - 20.22f)
#define PARAM_BAND_SLOW_X	(PARAM_BAND_FAST_X + 22.72f) // (73.52f)
#define PARAM_BAND_SWITCH_X	(PARAM_BAND_SLOW_X + 8.5f)	// Double/Enable switches in treble/Mid

#define SOCKET_STEP			(11.0f)
#define SOCKET_X_SLOW		(SOCKET_X1 + 1.0f + SOCKET_STEP)
#define SOCKET_X_FAST		(SOCKET_X1 + 1.0f + 2.0f * SOCKET_STEP)
#define SOCKET_X_WHEEL		(SOCKET_X1 + 1.0f + 3.0f * SOCKET_STEP)
#define SOCKET_X_VARISPEED	(SOCKET_X1 + 1.0f + 4.0f * SOCKET_STEP)
// #define PARAM_DISTANCE_X	(SOCKET_X_VARISPEED + (TOTAL_WIDTH - SOCKET_X1 - SOCKET_X_VARISPEED) / 3.0f)
// #define PARAM_ANGLE_X		(SOCKET_X_VARISPEED + 2.0f * (TOTAL_WIDTH - SOCKET_X1 - SOCKET_X_VARISPEED) / 3.0f)
#define PARAM_MIC_X			(73.0f)
#define PARAM_DISTANCE_Y	(SOCKET_BOTTOM - 45.0f)
#define PARAM_ANGLE_Y		(SOCKET_BOTTOM - 31.0f)

#define HI_BAND_Y			(19.0f)
#define MID_BAND_Y			(46.0f)
#define LO_BAND_Y			(73.0f)
#define BAND_ROW2_Y			(10.0f)

#define SOCKET_BOTTOM		(RACK_GRID_HEIGHT_MM - 14.0f)
#define SOCKET_STEP_Y		(9.0f)

#define SOCKET_Y_L			(SOCKET_BOTTOM - SOCKET_STEP_Y)
#define SOCKET_Y_R			SOCKET_BOTTOM					// numbered from bottom upwards
#define BOTTOM_Y_SWITCH		(SOCKET_Y_L - 11.3f)

#define FBLIGHTS_Y			(SOCKET_Y_R - 8.5f)

RotaryWidget::RotaryWidget(RotaryModule* pModule) :
	m_pModule(pModule)
{
	// Set up the widget
	setModule(pModule);
	setPanel(createPanel(asset::plugin(the_pPluginInstance, "res/Rotary.svg"), asset::plugin(the_pPluginInstance, "res/Rotary-dark.svg")));

	addChild(createWidget<ThemedScrew>(Vec(0, 0)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	CreateBandControls(HI_BAND_Y, RotaryModule::Param_DoubleHi, RotaryModule::Light_DoubleHi, 5);
	CreateBandControls(MID_BAND_Y, RotaryModule::Param_EnableMid, RotaryModule::Light_EnableMid, 3);
	CreateBandControls(LO_BAND_Y, RotaryModule::Param_RampupLo, RotaryModule::Light_LoL, 2);

	// Input Mono
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_X1, SOCKET_BOTTOM)), m_pModule, RotaryModule::Input_Mono));

	// Slow button.light and CV
	addParam(createParamCentered<VCVLatch>(mm2px(Vec(SOCKET_X_SLOW, BOTTOM_Y_SWITCH)), m_pModule, RotaryModule::Param_Slow));
	addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(SOCKET_X_SLOW, BOTTOM_Y_SWITCH)), m_pModule, RotaryModule::Light_Slow));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_X_SLOW, SOCKET_BOTTOM)), m_pModule, RotaryModule::Input_Slow));

	// Fast button.light and CV
	addParam(createParamCentered<VCVLatch>(mm2px(Vec(SOCKET_X_FAST, BOTTOM_Y_SWITCH)), m_pModule, RotaryModule::Param_Fast));
	addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(SOCKET_X_FAST, BOTTOM_Y_SWITCH)), m_pModule, RotaryModule::Light_Fast));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_X_FAST, SOCKET_BOTTOM)), m_pModule, RotaryModule::Input_Fast));

	// Varispeed CV and Wheel S/F
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_X_VARISPEED, SOCKET_BOTTOM)), m_pModule, RotaryModule::Input_Varispeed));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_X_WHEEL, SOCKET_BOTTOM)), m_pModule, RotaryModule::Input_WheelSF));

	// Mic Distance and Angle
	addParam(createParamCentered<PointyKnob12mm>(mm2px(Vec(PARAM_MIC_X, PARAM_DISTANCE_Y)), m_pModule, RotaryModule::Param_MicDistance));
	addParam(createParamCentered<PointyKnob12mm>(mm2px(Vec(PARAM_MIC_X, PARAM_ANGLE_Y)), m_pModule, RotaryModule::Param_MicAngle));

	// Output stereo
	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(TOTAL_WIDTH - SOCKET_X1, SOCKET_Y_L)), m_pModule, RotaryModule::OutputL));
	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(TOTAL_WIDTH - SOCKET_X1, SOCKET_Y_R)), m_pModule, RotaryModule::OutputR));

	m_pLightFBSlow = createLightCentered<LargeLight<YellowLight>>(mm2px(Vec(SOCKET_X_SLOW, FBLIGHTS_Y)), m_pModule, RotaryModule::Light_FBSlow);
	m_pLightFBFast = createLightCentered<LargeLight<RedLight>>(mm2px(Vec(SOCKET_X_FAST, FBLIGHTS_Y)), m_pModule, RotaryModule::Light_FBFast);
	addChild(m_pLightFBSlow);
	addChild(m_pLightFBFast);

}

RotaryWidget::~RotaryWidget()
{
}

void RotaryWidget::ShowFBLights(bool bShow)
{
	if (bShow == m_bFBLightsVisible)
		return;
	m_bFBLightsVisible = bShow;
	if (bShow)
	{
		if (m_pLightFBFast != nullptr)
			m_pLightFBFast->show();
		if (m_pLightFBSlow != nullptr)
			m_pLightFBSlow->show();
	}
	else
	{
		if (m_pLightFBFast != nullptr)
			m_pLightFBFast->hide();
		if (m_pLightFBSlow != nullptr)
			m_pLightFBSlow->hide();

	}
}

void RotaryWidget::step()
{
	if (m_pModule != nullptr)
		ShowFBLights(!m_pModule->VariSpeed());
	ModuleWidget::step();
}

void RotaryWidget::CreateBandControls(int nStartY, int nParamStart, int nFirstLight, int nLights)
{
	int nY1 = nStartY;
	int nY2 = nY1 + BAND_ROW2_Y;
	int nParam = nParamStart;
	int nLight = nFirstLight;
	if (nLights & 1) // odd number of lights -> extra light in parameter button
	{
		addParam(createParamCentered<VCVLatch>(mm2px(Vec(PARAM_BAND_SWITCH_X, nY1)), m_pModule, nParam++));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(PARAM_BAND_SWITCH_X, nY1)), m_pModule, nLight++));
	}
	// Knobs
	addParam(createParamCentered<PointyKnob12mm>(mm2px(Vec(PARAM_BAND_RAMPUP_X, nY2)), m_pModule, nParam++));
	addParam(createParamCentered<FilledKnob14mm>(mm2px(Vec(PARAM_BAND_FAST_X, nY1)), m_pModule, nParam++));
	addParam(createParamCentered<FilledKnob14mm>(mm2px(Vec(PARAM_BAND_SLOW_X, nY2)), m_pModule, nParam++));

	// Modulation lights
	addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(PARAM_BAND_FAST_X - 2.5f, nY2)), m_pModule, nLight++));
	addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(PARAM_BAND_FAST_X + 2.5f, nY2)), m_pModule, nLight++));
	if (nLights > 3)
	{
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(PARAM_BAND_FAST_X - 2.5f, nY2 + 4.0f)), m_pModule, nLight++));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(PARAM_BAND_FAST_X + 2.5f, nY2 + 4.0f)), m_pModule, nLight++));
	}
}


Model* the_pRotaryModel = createModel<RotaryModule, RotaryWidget>("Rotary");