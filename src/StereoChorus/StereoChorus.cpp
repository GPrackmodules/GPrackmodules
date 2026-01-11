#include <iomanip>
#include "plugin.hpp"
#include "StereoChorus.h"

#define RACK_GRID_WIDTH_MM	(5.08f)
#define RACK_GRID_HEIGHT_MM	(128.5f)

#define SOCKET_COLUMN1_MM	(6.08f)
#define SOCKET_STEP			((9.0f * RACK_GRID_WIDTH_MM - 2.0f * SOCKET_COLUMN1_MM) / 3.0f)
#define SOCKET_COLUMN2_MM		(SOCKET_COLUMN1_MM + SOCKET_STEP)
#define SOCKET_COLUMN3_MM		(SOCKET_COLUMN1_MM + 2.0f * SOCKET_STEP)
#define SOCKET_COLUMN4_MM		(SOCKET_COLUMN1_MM + 3.0f * SOCKET_STEP)
#define SOCKET_COLUMN5		(SOCKET_COLUMN1_MM + 4.0f * SOCKET_STEP)

#define SOCKET_BOTTOM_MM	(RACK_GRID_HEIGHT_MM - 14.0f)
#define SOCKET_STEP_Y_MM	(9.0f)

#define SOCKET_L_Y_MM		(SOCKET_BOTTOM_MM - SOCKET_STEP_Y_MM)
#define SOCKET_R_Y_MM		SOCKET_BOTTOM_MM					// numbered from bottom upwards
#define SOCKET_CV_Y_MM		(SOCKET_L_Y_MM - 17.0f)

#define	CV_PARAM_Y_MM		(SOCKET_CV_Y_MM - 18.0f)

#define PARAM_COLUMN1_2_MM	9.0f
#define PARAM_COLUMN2_2_MM	(4.5f * RACK_GRID_WIDTH_MM)
#define PARAM_COLUMN3_MM	(9.0f * RACK_GRID_WIDTH_MM - PARAM_COLUMN1_2_MM)
#define	PARAM_ROW2_2_Y_MM	(CV_PARAM_Y_MM - 16.0f)

#define PARAM_COLUMN1_1_MM	12.5f
#define PARAM_COLUMN2_1_MM	(9.0f * RACK_GRID_WIDTH_MM - PARAM_COLUMN1_1_MM)
#define	PARAM_ROW1_Y_MM		(19.0f)

#define LIGHT_STEP_MM		(8.0f)
#define	LIGHT_COLUMN1_MM	(4.5f * RACK_GRID_WIDTH_MM - 1.5f * LIGHT_STEP_MM)
#define LIGHT_MONO_Y_MM		(38.6f)
#define LIGHT_LEFT_Y_MM		(35.6f)
#define LIGHT_RIGHT_Y_MM	(41.6f)

static const float s_fMinus4Pt5dB = pow(10.0, -4.50f / 20.0f);
static const float s_fExternalMod = -0.0001f;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PARAMETER QUANTITIES
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct RateQuantity : public ParamQuantity
{
	std::string getDisplayValueString() override
	{
		float fparamValue = getValue();
		if (fparamValue < 0.0f)
			return "External Modulation Only";
		float fHertz = Hertz(fparamValue);
		std::stringstream sstrHoldTime;
		sstrHoldTime << std::fixed << std::setprecision(2) << fHertz << " Hz";
		return sstrHoldTime.str();
	}

	void setDisplayValueString(std::string s) override
	{
		float fHertz;
		if (sscanf(s.c_str(), "%f", &fHertz) != 1)
			fHertz = 1.0f;
		setDisplayValue(Value(fHertz));
	}

	static float Hertz(float fValue)
	{
		if (fValue < 0.0f)
			return 1.0f;
		return 1.0f * pow(10.0f, fValue);
	}

	static float Value(float fHertz)
	{
		if (fHertz < 0.0f)
			return s_fExternalMod;
		return log10(fHertz);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MODULE
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

StereoChorusModule::StereoChorusModule() :
	m_fadeDelayRanges
	{
		{ m_fDelayRanges[0], 30.0f, 0.0f },
		{ m_fDelayRanges[1], 30.0f, 0.0f },
		{ m_fDelayRanges[2], 30.0f, 0.0f },
		{ m_fDelayRanges[3], 30.0f, 0.0f }
	},
	m_fadeLowpass(m_fltTone.c[0], m_fltTone.c[1], 30.0f, 0.0f),
	m_fadeHighpass(m_fltTone.c[2], m_fltTone.c[3], 30.0f, 0.0f),
	m_fadeVoices
	{
		{ m_fVoiceFactors[1], 30.0f, 0.0f },
		{ m_fVoiceFactors[2], 30.0f, 0.0f },
		{ m_fVoiceFactors[3], 30.0f, 0.0f }
	},
	m_fadeGainCorrection(m_fGainCorrection, 30.0f, 0.0f),
	m_fadeWet(m_fDryFactor, m_fWetFactor, 30.0f, 0.0f)
{
	config(NumParams, NumInputs, NumOutputs, NumLights);

	// Parameters
	configParam<RateQuantity>(ParamRate, s_fExternalMod, 1.0f, 0.5f, "Rate");
	configParam(ParamDepth, 1.0f, 100.0f, 33.0f, "Depth", "%");
	paramQuantities[ParamDepth]->snapEnabled = true;
	configParam(ParamTone, -100.0f, 100.0f, 0.f, "Tone", "%");
	paramQuantities[ParamTone]->snapEnabled = true;
	configParam(ParamVoices, 1.0f, 4.0f, 3.f, "Number of Voices");
	paramQuantities[ParamVoices]->snapEnabled = true;
	configParam(ParamWet, 0.0f, 100.0f, 50.0f, "Wet", "%");
	paramQuantities[ParamWet]->snapEnabled = true;
	// Attenuverters
	configParam(ParamCVRate, -100.0f, 100.0f, 0.0f, "Rate CV Attenuverter", "%");
	paramQuantities[ParamCVRate]->snapEnabled = true;
	configParam(ParamCVDepth, -100.0f, 100.0f, 0.0f, "Depth CV Attenuverter", "%");
	paramQuantities[ParamCVDepth]->snapEnabled = true;
	configParam(ParamCVTone, -100.0f, 100.0f, 0.0f, "Tone CV Attenuverter", "%");
	paramQuantities[ParamCVTone]->snapEnabled = true;
	configParam(ParamCVWet, -100.0f, 100.0f, 0.0f, "Wet CV Attenuverter", "%");
	paramQuantities[ParamCVWet]->snapEnabled = true;

	configInput(InputL, "Left");
	configInput(InputR, "Right");
	configInput(InputMod1, "Moduulation 1 / Polyphonic (x8)");
	configInput(InputMod2, "Moduulation 2");
	configInput(InputMod3, "Moduulation 3");
	configInput(InputMod4, "Moduulation 4");
	configInput(InputCVRate, "CV Rate");
	configInput(InputCVDepth, "CV Depth");
	configInput(InputCVTone, "CV Tone");
	configInput(InputCVWet, "CV Wet");

	configOutput(OutputL, "Left");
	configOutput(OutputR, "Right");
}

StereoChorusModule::~StereoChorusModule()
{
	delete m_pDelayLine;
}

void StereoChorusModule::SetDarkMode(bool bDarkMode)
{
	// Brightness = offset + sin/cosine * FactorModulation + DepthPercent * FactorDepth
	if (bDarkMode)
	{
		m_fLightFactorModulation = 0.3;
		m_fLightFactorDepth =	0.002f;
		m_fLightFactorOffset = 0.5f;
		m_fLightOff = 0.0f;
	}
	else
	{
		m_fLightFactorModulation = 0.3f;
		m_fLightFactorDepth =	0.002f;
		m_fLightFactorOffset = 0.3f;
		m_fLightOff = settings::rackBrightness;
	}
	float fOff = m_fLightOff * m_fLightOff;
	for (int v = m_nVoices; v < STEREO_CHORUS_VOICES; v++)
	{
		lights[Light1Left + v].setBrightness(fOff);
		lights[Light1Right + v].setBrightness(fOff);
		lights[Light1Mono + v].setBrightness(fOff);
	}
	m_bDarkMode = bDarkMode;
}

void StereoChorusModule::process(const ProcessArgs& args) /*override*/
{
	if (!m_bInitialized)
	{
		delete m_pDelayLine;
		m_pDelayLine = new ChorusDelayLine(args.sampleRate, 2, 2.5f * MAX_DELAY);
		UpdateSamplerate(args.sampleRate);

		HandleVoices(true);
		HandleDepth(true);
		HandleTone(true);
		HandleWet(true);
		m_bInitialized = true;
	}
	// Update parameters
	HandleVoices();
	HandleRate();
	HandleDepth();
	HandleWet();
	HandleTone();

	bool bHasInput = true;
	m_pDelayLine->Advance();
	float fInL, fInR;
	if (inputs[InputL].isConnected())
	{
		fInL = inputs[InputL].getVoltageSum();
		if (inputs[InputR].isConnected())
			fInR = inputs[InputR].getVoltageSum();
		else
		{
			fInL *= s_fMinus4Pt5dB;
			fInR = fInL;
		}
	}
	else
	{
		if (inputs[InputR].isConnected())
		{
			fInR = inputs[InputR].getVoltageSum() * s_fMinus4Pt5dB;
			fInL = fInR;
		}
		else
		{
			fInL = 0.0f;
			fInR = 0.0f;
			bHasInput = false;
		}
	}

	simd::float_4 fInput = { fInL, fInR, m_fltTone.ystate[0][0], m_fltTone.ystate[0][1] }; // lowpasses feeding the highpasses
	m_fltTone.process(fInput);

	if (bHasInput)
	{
		m_nLastInputFrame = args.frame;
		//m_pDelayLine->Feed(fInL, fInR);
		m_pDelayLine->Feed(m_fltTone.highpass()[2], m_fltTone.highpass()[3]);
	}
	else
	{
		m_pDelayLine->Feed(0.0f, 0.0f);
		if (m_bHasInput)
		{
			// clear lights
			for (int v = 0; v < m_nVoices; v++)
			{
				lights[Light1Left + v].setBrightness(m_fLightOff);
				lights[Light1Right + v].setBrightness(m_fLightOff);
				lights[Light1Mono + v].setBrightness(m_fLightOff);
			}
		}
	}
	m_bHasInput = bHasInput;

	//
	// LFOs
	// Lights
	//

	float fLightBase = m_fLightFactorOffset + m_fValueDepth * m_fLightFactorDepth;
	float fModulation[STEREO_CHORUS_VOICES][2];

	AdvanceLFO4();

	bool bPoly = inputs[InputMod1].getChannels() >= 2 * STEREO_CHORUS_VOICES; // use 8 channels from modulation input 1
	for (int v = 0; v < STEREO_CHORUS_VOICES; v++)
	{
//		AdvanceLFO(lfo);

		// determine delay modulation (+/- 1.0)
		if (params[ParamRate].getValue() < 0.0f)
		{
			// External modulation only
			if (bPoly)
			{
				fModulation[v][0] = simd::clamp(inputs[InputMod1].getVoltage(v) / 10.0f, -1.0f, 1.0f);
				fModulation[v][1] = simd::clamp(inputs[InputMod1].getVoltage(v + STEREO_CHORUS_VOICES) / 10.0f, -1.0f, 1.0f);
			}
			else
			{
				int nModChannel = 0;
				fModulation[v][0] = simd::clamp(inputs[InputMod1 + v].getVoltage() / 10.0f, -1.0f, 1.0f);
				if (inputs[InputMod1 + v].getChannels() > 1)		// second channel on dedicated modulation input for voice
					nModChannel = 1;
				fModulation[v][1] = simd::clamp(inputs[InputMod1 + v].getVoltage(nModChannel) / 10.0f, -1.0f, 1.0f);
			}
		}
		else
		{
			// LFO + modulation input, halved if input is connected
			fModulation[v][0] = m_lfo4.fSine[v];
			fModulation[v][1] = m_lfo4.fCosine[v];
			if (bPoly)
			{
				fModulation[v][0] = fModulation[v][0] * 0.5 + simd::clamp(inputs[InputMod1].getVoltage(v) / 20.0f, -0.5f, 0.5f);
				fModulation[v][1] = fModulation[v][1] * 0.5 + simd::clamp(inputs[InputMod1].getVoltage(v + STEREO_CHORUS_VOICES) / 20.0f, -0.5f, 0.5f);
			}
			else
			{
				if (inputs[InputMod1 + v].isConnected())
				{
					// Additional external modulation input
					float fCV = simd::clamp(inputs[InputMod1 + v].getVoltage(0) / 20.0f, -0.5f, 0.5f);
					fModulation[v][0] = fModulation[v][0] * 0.5 + fCV;
					if (inputs[InputMod1 + v].getChannels() > 1)	// has 2nd channel, use that for right modulation
						fCV = simd::clamp(inputs[InputMod1 + v].getVoltage(1) / 20.0f, -0.5f, 0.5f);
					fModulation[v][1] = fModulation[v][1] * 0.5 + fCV;
				}
			}
		}

		// update light of active voices
		if (m_bHasInput && v < m_nVoices)
		{
			if (m_bStereo)
			{
				lights[Light1Left + v].setBrightness(fLightBase + fModulation[v][0] * m_fLightFactorModulation);
				lights[Light1Right + v].setBrightness(fLightBase + fModulation[v][1] * m_fLightFactorModulation);
			}
			else
				lights[Light1Mono + v].setBrightness(fLightBase + fModulation[v][0] * m_fLightFactorModulation);
		}
	} // for (int v = 0; v < STEREO_CHORUS_VOICES; v++)
	// Dim lights of unsued voices to match rackBrightness (light panels only)
	if (!m_bDarkMode && settings::rackBrightness != m_fLightOff)
	{
		m_fLightOff = settings::rackBrightness;
		float fOff = m_fLightOff * m_fLightOff;
		for (int v = m_bHasInput ? m_nVoices : 0; v < STEREO_CHORUS_VOICES; v++)
		{
			lights[Light1Left + v].setBrightness(fOff);
			lights[Light1Right + v].setBrightness(fOff);
			lights[Light1Mono + v].setBrightness(fOff);
		}
	}

	//
	// Output signal
	//
	if (isBypassed())
	{
		outputs[OutputL].setVoltage(inputs[InputL].getVoltageSum());
		outputs[OutputR].setVoltage(inputs[InputR].getVoltageSum());
	}
	else if (args.frame > m_nLastInputFrame + m_pDelayLine->MaxDelaySamples())
	{
		// stop processing once all samples in delay have been played
		outputs[OutputL].setVoltage(0.0f);
		outputs[OutputR].setVoltage(0.0f);
	}
	else
	{
		if (outputs[OutputL].isConnected())
		{
			float fOutL = 0.0f;
			for (int v = 0; v < STEREO_CHORUS_VOICES; v++)
				if (v < m_nVoices || m_fadeVoices[v].Fading())
					fOutL += m_pDelayLine->Read(0, (1.0f + fModulation[v][0]) * m_fDelayRanges[v]) * m_fVoiceFactors[v];
			fOutL *= m_fGainCorrection * m_fWetFactor;
			outputs[OutputL].setVoltage(fOutL + fInL * m_fDryFactor);
		}
		if (outputs[OutputR].isConnected())
		{
			float fOutR = 0.0f;
			for (int v = 0; v < STEREO_CHORUS_VOICES; v++)
				if (v < m_nVoices || m_fadeVoices[v].Fading())
					fOutR += m_pDelayLine->Read(1, (1.0f + fModulation[v][1]) * m_fDelayRanges[v]) * m_fVoiceFactors[v];
			fOutR *= m_fGainCorrection * m_fWetFactor;
			outputs[OutputR].setVoltage(fOutR + fInR * m_fDryFactor);
		}
	}

	// Advance fading coefficients
	m_fadeGainCorrection.Advance();
	//m_fGainCorrection = 1.0f / m_fGainDivider;
	for (int v = 0; v < STEREO_CHORUS_VOICES - 1; v++)
		m_fadeVoices[v].Advance();
	for (int v = 0; v < STEREO_CHORUS_VOICES; v++)
		m_fadeDelayRanges[v].Advance();
	m_fadeLowpass.Advance();
	m_fadeHighpass.Advance();
	m_fadeWet.Advance();
}

void StereoChorusModule::onSampleRateChange(const SampleRateChangeEvent &e) /*override*/
{
	UpdateSamplerate(e.sampleRate);
	if (m_pDelayLine != nullptr)
		m_pDelayLine->UpdateSamplerate(e.sampleRate);
}

void StereoChorusModule::AdvanceLFO4()
{
	m_lfo4.fPhase += m_lfo4.fFrequency * m_fInvertedSamplerate;
	m_lfo4.fPhase = simd::ifelse(m_lfo4.fPhase >= 1.0f, m_lfo4.fPhase - 1.0f, m_lfo4.fPhase);
	m_lfo4.fSine = simd::sin(2.0f * M_PI * m_lfo4.fPhase);
	if (m_bStereo)
		m_lfo4.fCosine = simd::cos(2.0f * M_PI * m_lfo4.fPhase);
}

void StereoChorusModule::HandleVoices(bool bForce /*= false*/)
{
	bool bStereo = outputs[OutputL].isConnected() == outputs[OutputR].isConnected(); // both or none connected
	int nVoices = (int)params[ParamVoices].getValue();
	if (nVoices != m_nVoices || bStereo != m_bStereo || bForce)
	{
		if (nVoices > m_nVoices)
		{
			for (int v = m_nVoices; v < nVoices; v++)
				m_fadeVoices[v - 1].Start(1.0f);
		}
		else if (nVoices < m_nVoices)
		{
			float fOff = m_fLightOff * m_fLightOff;
			for (int v = nVoices; v < m_nVoices; v++)
			{
				if (bStereo)
				{
					lights[Light1Left + v].setBrightness(fOff);
					lights[Light1Right + v].setBrightness(fOff);
				}
				else
					lights[Light1Mono + v].setBrightness(fOff);
				m_fadeVoices[v - 1].Start(0.0f);
			}
		}
		m_bStereo = bStereo;
		m_nVoices = nVoices;
		CalcGainFactor();
		HandleRate(true);
	}
}

void StereoChorusModule::HandleRate(bool bForce /*= false*/)
{
	float fValueRate = params[ParamRate].getValue();
	if (inputs[InputCVRate].isConnected())
	{
		fValueRate += inputs[InputCVRate].getVoltage(0) * params[ParamCVRate].getValue() * 0.001f; // CV = 0..10V, ParamCVRate is +/- 100, need 0..1
		fValueRate = simd::clamp(fValueRate, 0.0f, 1.0f);
	}
	if (fValueRate != m_fValueRate || bForce)
	{
		m_fValueRate = fValueRate;
		m_fAvgFrequency = RateQuantity::Hertz(fValueRate);
		switch (m_nVoices)
		{
			case 1:
				m_lfo4.fFrequency[0] = m_fAvgFrequency;
				break;
			case 2:
				m_lfo4.fFrequency[0] = m_fAvgFrequency * 0.951253297f;
				m_lfo4.fFrequency[1] = m_fAvgFrequency * 1.047689563214f;
				break;
			case 3:
				m_lfo4.fFrequency[0] = m_fAvgFrequency * 0.9531235276f;
				m_lfo4.fFrequency[1] = m_fAvgFrequency;
				m_lfo4.fFrequency[2] = m_fAvgFrequency * 1.0487388954624f;
				break;
			case 4:
				m_lfo4.fFrequency[0] = m_fAvgFrequency * 0.9134651241f;
				m_lfo4.fFrequency[1] = m_fAvgFrequency * 0.9712332123;
				m_lfo4.fFrequency[2] = m_fAvgFrequency * 1.0230897645364f;
				m_lfo4.fFrequency[3] = m_fAvgFrequency * 1.0865392524524f;
				break;
		}
		HandleDepth(true);
	}
}

void StereoChorusModule::HandleDepth(bool bForce /*= false*/)
{
	float fValueDepth = params[ParamDepth].getValue();
	if (inputs[InputCVDepth].isConnected())
	{
		fValueDepth += inputs[InputCVDepth].getVoltage(0) * params[ParamCVDepth].getValue() / 10.0f; // CV = 0..10V, ParamCVRate is +/- 100, need 0.100
		fValueDepth = simd::clamp(fValueDepth, 0.0f, 100.0f);
	}
	if (fValueDepth != m_fValueDepth || bForce)
	{
		m_fValueDepth = fValueDepth;
		m_fDepthDelay = MIN_DELAY + (m_fValueDepth * m_fValueDepth) * (MAX_DELAY - MIN_DELAY) / 10000.0f;
		for (int v = 0; v < STEREO_CHORUS_VOICES; v++)
			m_fadeDelayRanges[v].Start(m_fDepthDelay / m_lfo4.fFrequency[v]);
		CalcGainFactor();
	}
}

#define HP_LOWER	(30.0f)
#define HP_UPPER	(1000.0f)

#define LP_LOWER	(500.0f)
#define LP_UPPER	(22000.0f)

void StereoChorusModule::HandleTone(bool bForce /* =true*/)
{
	float fValueTone = params[ParamTone].getValue();
	if (inputs[InputCVTone].isConnected())
	{
		fValueTone += ((inputs[InputCVTone].getVoltage(0) - 5.0f) / 5.0f) * params[ParamCVTone].getValue(); // CV = 0..10V, ParamCVRate is +/- 100, need -100..100
	 	fValueTone = simd::clamp(fValueTone, -100.0f, 100.0f);
	}
	if (fValueTone != m_fValueTone || bForce)
	{
		m_fValueTone = fValueTone;
		if (fValueTone < 0.0f)
		{
			// darker tones, Lowpass goes from UPPER (at -1%) to LOWER (at -100%)
			float fRelative = (fValueTone + 100.0f) / 100.0f;
			LPCutoff(LP_LOWER * pow (LP_UPPER / LP_LOWER, fRelative));
			HPCutoff(HP_LOWER);;
		}
		else if (fValueTone > 0.0f)
		{
			// brighter tones, Highpass goes from LOWER (at 1%) to UPPER (at 100%)
			float fRelative = fValueTone / 100.0f;
			HPCutoff(HP_LOWER * pow (HP_UPPER / HP_LOWER, fRelative));
			LPCutoff(LP_UPPER);;
		}
		else
		{
			HPCutoff(HP_LOWER);;
			LPCutoff(LP_UPPER);;
		}
	}
}

void StereoChorusModule::HandleWet(bool bForce /* =true*/)
{
	float fValueWet = params[ParamWet].getValue();
	if (inputs[InputCVWet].isConnected())
	{
		fValueWet += ((inputs[InputCVWet].getVoltage(0)/* -5.0f*/) /*/ 5.0f*/) * params[ParamCVWet].getValue() / 10.0f; // CV = 0..10V, ParamCVRate is +/- 100, need 0.100
		fValueWet = simd::clamp(fValueWet, 0.0f, 100.0f);
	}
	if (fValueWet != m_fValueWet || bForce)
	{
		m_fValueWet = fValueWet;
		float fFactorDry = simd::cos(fValueWet * M_PI_2 / 100.0f);
		float fFactorWet = simd::sin(fValueWet * M_PI_2 / 100.0f);
		m_fadeWet.Start(fFactorDry, fFactorWet);
	}
}

void StereoChorusModule::CalcGainFactor()
{
	float fCorrelated = m_nVoices;
	float fUncorrelated = sqrt((float)m_nVoices);
	float fGainCorrection = 1.0f / (fCorrelated + (fUncorrelated - fCorrelated) * m_fValueDepth / 100.0f);  // depth crossfades between the two factors
	m_fadeGainCorrection.Start(fGainCorrection);
}

void StereoChorusModule:: LPCutoff(float fHz)
{
	m_fadeLowpass.Start(m_fSamplerate / (M_PI * fHz));
}

void StereoChorusModule::HPCutoff(float fHz)
{
	m_fadeHighpass.Start(m_fSamplerate / (M_PI * fHz));
}

void StereoChorusModule::UpdateSamplerate(float fSamplerate)
{
	// printf("Module: New samplerate %f, was %f\n", fSamplerate, m_fSamplerate);
	m_fSamplerate = fSamplerate;
	m_fInvertedSamplerate = 1.0f / fSamplerate;

	m_fadeGainCorrection.SetSamplerate(fSamplerate);
	for (int v = 0; v < STEREO_CHORUS_VOICES - 1; v++)
		m_fadeVoices[v].SetSamplerate(fSamplerate);
	for (int v = 0; v < STEREO_CHORUS_VOICES; v++)
		m_fadeDelayRanges[v].SetSamplerate(fSamplerate);
	m_fadeLowpass.SetSamplerate(fSamplerate);
	m_fadeHighpass.SetSamplerate(fSamplerate);
	m_fadeWet.SetSamplerate(fSamplerate);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// WIDGET
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


StereoChorusWidget::StereoChorusWidget(StereoChorusModule* pModule) :
	m_pModule(pModule),
	m_bDarkMode(settings::preferDarkPanels)
{
	if (pModule != nullptr)
	{
		pModule->SetWidget(this);
		m_pModule->SetDarkMode(m_bDarkMode);
	}

	setModule(pModule);
	setPanel(createPanel(asset::plugin(the_pPluginInstance, "res/StereoChorus.svg"), asset::plugin(the_pPluginInstance, "res/StereoChorus-dark.svg")));

	addChild(createWidget<ThemedScrew>(Vec(0, 0)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	addParam(createParamCentered<FilledKnob16mm>(mm2px(Vec(PARAM_COLUMN1_1_MM, PARAM_ROW1_Y_MM)), pModule, StereoChorusModule::ParamRate));
	addParam(createParamCentered<FilledKnob16mm>(mm2px(Vec(PARAM_COLUMN2_1_MM, PARAM_ROW1_Y_MM)), pModule, StereoChorusModule::ParamDepth));

	for (int i = 0; i < STEREO_CHORUS_VOICES; i++)
	{
		m_aLightsMono[i] = createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(LIGHT_COLUMN1_MM + i * LIGHT_STEP_MM, LIGHT_MONO_Y_MM)), pModule, StereoChorusModule::Light1Mono + i);
		addChild(m_aLightsMono[i]);
		m_aLightsStereo[i][0] = createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(LIGHT_COLUMN1_MM + i * LIGHT_STEP_MM, LIGHT_LEFT_Y_MM)), pModule, StereoChorusModule::Light1Left + i);
		addChild(m_aLightsStereo[i][0]);
		m_aLightsStereo[i][1] = createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(LIGHT_COLUMN1_MM + i * LIGHT_STEP_MM, LIGHT_RIGHT_Y_MM)), pModule, StereoChorusModule::Light1Right + i);
		addChild(m_aLightsStereo[i][1]);
		m_aLightsStereo[i][0]->hide();
		m_aLightsStereo[i][1]->hide();
	}

	addParam(createParamCentered<PointyKnob12mm>(mm2px(Vec(PARAM_COLUMN1_2_MM, PARAM_ROW2_2_Y_MM)), pModule, StereoChorusModule::ParamTone));
	PointyKnob12mm* pVoicesKnob = createParamCentered<PointyKnob12mm>(mm2px(Vec(PARAM_COLUMN2_2_MM, PARAM_ROW2_2_Y_MM)), pModule, StereoChorusModule::ParamVoices);
	pVoicesKnob->minAngle = -M_PI_2 / 2.0f;
	pVoicesKnob->maxAngle = M_PI_2 / 2.0f;
	addParam(pVoicesKnob);
	addParam(createParamCentered<PointyKnob12mm>(mm2px(Vec(PARAM_COLUMN3_MM, PARAM_ROW2_2_Y_MM)), pModule, StereoChorusModule::ParamWet));

	addParam(createParamCentered<PointyKnob8mm>(mm2px(Vec(SOCKET_COLUMN1_MM, CV_PARAM_Y_MM)), pModule, StereoChorusModule::ParamCVRate));
	addParam(createParamCentered<PointyKnob8mm>(mm2px(Vec(SOCKET_COLUMN2_MM, CV_PARAM_Y_MM)), pModule, StereoChorusModule::ParamCVDepth));
	addParam(createParamCentered<PointyKnob8mm>(mm2px(Vec(SOCKET_COLUMN3_MM, CV_PARAM_Y_MM)), pModule, StereoChorusModule::ParamCVTone));
	addParam(createParamCentered<PointyKnob8mm>(mm2px(Vec(SOCKET_COLUMN4_MM, CV_PARAM_Y_MM)), pModule, StereoChorusModule::ParamCVWet));

	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_COLUMN1_MM, SOCKET_CV_Y_MM)), pModule, StereoChorusModule::InputCVRate));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_COLUMN2_MM, SOCKET_CV_Y_MM)), pModule, StereoChorusModule::InputCVDepth));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_COLUMN3_MM, SOCKET_CV_Y_MM)), pModule, StereoChorusModule::InputCVTone));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_COLUMN4_MM, SOCKET_CV_Y_MM)), pModule, StereoChorusModule::InputCVWet));

	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_COLUMN1_MM, SOCKET_L_Y_MM)), pModule, StereoChorusModule::InputL));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_COLUMN1_MM, SOCKET_R_Y_MM)), pModule, StereoChorusModule::InputR));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_COLUMN2_MM, SOCKET_L_Y_MM)), pModule, StereoChorusModule::InputMod1));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_COLUMN3_MM, SOCKET_L_Y_MM)), pModule, StereoChorusModule::InputMod2));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_COLUMN2_MM, SOCKET_R_Y_MM)), pModule, StereoChorusModule::InputMod3));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_COLUMN3_MM, SOCKET_R_Y_MM)), pModule, StereoChorusModule::InputMod4));

	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_COLUMN4_MM, SOCKET_L_Y_MM)), pModule, StereoChorusModule::OutputL));
	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_COLUMN4_MM, SOCKET_R_Y_MM)), pModule, StereoChorusModule::OutputR));
}

void StereoChorusWidget::step() /*override*/
{
	if (m_pModule != nullptr)
	{
		if (m_pModule->Stereo() != m_bStereo)
		{
			m_bStereo = m_pModule->Stereo();
			for (int i = 0; i < STEREO_CHORUS_VOICES; i++)
			{
				if (m_bStereo)
				{
					m_aLightsMono[i]->hide();
					m_aLightsStereo[i][0]->show();
					m_aLightsStereo[i][1]->show();
				}
				else
				{
					m_aLightsMono[i]->show();
					m_aLightsStereo[i][0]->hide();
					m_aLightsStereo[i][1]->hide();
				}
			}
		}
		bool bDarkMode = settings::preferDarkPanels;
		if (bDarkMode != m_bDarkMode)
		{
			m_bDarkMode = bDarkMode;
			m_pModule->SetDarkMode(bDarkMode);
		}
	}
	ModuleWidget::step();
}

Model* the_pStereoChorusModel = createModel<StereoChorusModule, StereoChorusWidget>("StereoChorus");
