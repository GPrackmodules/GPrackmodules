#include "plugin.h"
#include "common/Knobs.h"
#include "Clamp.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MODULE
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define THRESHOLD_TOLERANCE		(0.000001f)

#define MIN_NOTE	(-60.0f)	// 10 octaves
#define MAX_NOTE	(60.0f)	// 10 octaves

static char s_szNotes[12][4] =
{
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

namespace
{
class ModeQuantity : public ParamQuantity
{
public:
	std::string getDisplayValueString() override
	{
		float fValue = getValue();
		if (fValue < 0.5f)
			return "Drop";
		if (fValue < 1.5f)
			return "Shift Octave";
		return "Limit";
	}
};

class NoteQuantity : public ParamQuantity
{
public:
	NoteQuantity()
	{
		snapEnabled = true;
	}

public:
	std::string getDisplayValueString() override
	{
		int nValue = iround(getValue());

		int nOctave;
		int nNote;
		if (nValue >= 0)
		{
			nOctave = nValue / 12;
			nNote = nValue - nOctave * 12;
			nOctave += 4;
		}
		else
		{
			nValue += 96;
			nOctave = nValue / 12;
			nNote = nValue - nOctave * 12;
			nOctave -= 4;
		}
		// printf("Value %d, Oct %d Note %d\n", nValue, nOctave, nNote);
		if (nNote < 0)
			return "Too low";
		if (nNote > 11)
			return "Too High";
		char szValue[32];
		snprintf(szValue, sizeof(szValue), "%s%d", s_szNotes[nNote], nOctave);
		return szValue;
	}
	void setDisplayValueString(std::string s) override
	{
		float fHz;
		if (StrToFloat(s.c_str(), fHz ))
			setValue(12.0f * log2(fHz / dsp::FREQ_C4));
		else
		{
			char szNote[4];
			int nOctave;
			if (sscanf(s.c_str(), "%1s%d", szNote, &nOctave) != 2) // NOLINT the world will keep turning if the octave is out of range
				if (sscanf(s.c_str(), "%2s%d", szNote, &nOctave) != 2)  // NOLINT the world will keep turning if the octave is out of range
					return;
			clamp(nOctave, -1, 9);
			int nNote = -1;
			for (int i = 0; i < 12; i++)
				if (strcasecmp(s_szNotes[i], szNote) == 0)
				{
					nNote = i;
					break;
				}
			if (nNote < 0)
				return;
			setValue(static_cast<float>(((nOctave - 4) * 12 + nNote)));
		}
	}

	static float VOct(float fNote)
	{
		return fNote / 12.0f;
	}
};

}

ClampModule::ClampModule()
{
	config(NumParams, NumInputs, NumOutputs, NumLights);
	configParam<ModeQuantity>(ParamMode, 0.f, 2.f, 1.f, "Out Of Ramge Action");
	paramQuantities[ClampModule::ParamMode]->snapEnabled = true;
	configParam<NoteQuantity>(ParamUpperLimit, MIN_NOTE, MAX_NOTE, 12.0f, "Upper Note Limit");
	configParam<NoteQuantity>(ParamLowerLimit, MIN_NOTE, MAX_NOTE, -12.0f, "Lower Note Limit");
	configInput(InputVOct, "V/Oct");
	configInput(InputGate, "Gate");
	configOutput(OutputVOct, "V/Oct");
	configOutput(OutputGate, "Gate");
}

void ClampModule::process(const ProcessArgs& args) /*override*/
{
	if (!m_bInitialized)
	{
		m_bInitialized = true;
		HandleMode(true);
		HandleUpperLimit(true);
		HandleLowerLimit(true);
	}
	HandleMode();
	HandleUpperLimit();
	HandleLowerLimit();
	auto n = static_cast<uint8_t>(inputs[InputVOct].getChannels());
	outputs[OutputVOct].setChannels(n);
	outputs[OutputGate].setChannels(n);
	switch (m_eMode)
	{
		case Mode::Drop:
			for (uint8_t c = 0; c < n; c++)
			{
				float fIn = inputs[InputVOct].getPolyVoltage(c);
				if (fIn > m_fUpperVOctThreshold || fIn < m_fLowerVOctThreshold)
					outputs[OutputGate].setVoltage(0.0f, c);
				else
				{
					outputs[OutputVOct].setVoltage(fIn, c);
					outputs[OutputGate].setVoltage(inputs[InputGate].getPolyVoltage(c), c);
				}
			}
			break;

		case Mode::ShiftOctave:
			for (uint8_t c = 0; c < n; c++)
			{
				float fIn = inputs[InputVOct].getPolyVoltage(c);
				while (fIn > m_fUpperVOctThreshold)
					fIn -= 1.0f;
				while (fIn < m_fLowerVOctThreshold)
					fIn += 1.0f;
				if (fIn > m_fUpperVOctThreshold)	// range is smaller than an octave, drop
					outputs[OutputGate].setVoltage(0.0f, c);
				else
				{
					outputs[OutputVOct].setVoltage(fIn, c);
					outputs[OutputGate].setVoltage(inputs[InputGate].getPolyVoltage(c), c);
				}
			}
			break;

		case Mode::Limit:
			if (m_bError)
			{
				for (uint8_t c = 0; c < n; c++)
					outputs[OutputGate].setVoltage(0.0f, c);
				break;
			}
			for (uint8_t c = 0; c < n; c += 4)
			{
				simd::float_4 f4In = inputs[InputVOct].getPolyVoltageSimd<simd::float_4>(c);
				outputs[OutputVOct].setVoltageSimd(simd::clamp(f4In, m_fLowerVOct, m_fUpperVOct), c);
				f4In = inputs[InputGate].getPolyVoltageSimd<simd::float_4>(c);
				outputs[OutputGate].setVoltageSimd(f4In, c);
			}
			break;
	}
}

void ClampModule::CheckParamError()
{
	if (m_fUpperVOct < m_fLowerVOct || (m_eMode == Mode::ShiftOctave && m_fUpperVOctThreshold < m_fLowerVOctThreshold + 11.0f / 12.0f))
	{
		if (!m_bError)
		{
			m_bError = true;
			lights[LightError].setBrightness(1.0f);
		}
	}
	else
	{
		if (m_bError)
		{
			m_bError = false;
			lights[LightError].setBrightness(0.0f);
		}
	}
}

void ClampModule::HandleMode(bool bForce /*= false*/)
{
	float fParam = params[ParamMode].getValue();
	if (fParam != m_fParamMode || bForce)
	{
		m_fParamMode = fParam;
		if (fParam < 0.5f)
			m_eMode = Mode::Drop;
		else if (fParam < 1.5f)
			m_eMode = Mode::ShiftOctave;
		else
			m_eMode = Mode::Limit;
		CheckParamError();
	}
}

void ClampModule::HandleUpperLimit(bool bForce /*= false*/)
{
	float fParam = params[ParamUpperLimit].getValue();
	if (fParam != m_fParamUpperLimit || bForce)
	{
		m_fParamUpperLimit = fParam;
		m_fUpperVOct = NoteQuantity::VOct(fParam);
		m_fUpperVOctThreshold = m_fUpperVOct + THRESHOLD_TOLERANCE;
		CheckParamError();
	}
}

void ClampModule::HandleLowerLimit(bool bForce /*= false*/)
{
	float fParam = params[ParamLowerLimit].getValue();
	if (fParam != m_fParamLowerLimit || bForce)
	{
		m_fParamLowerLimit = fParam;
		m_fLowerVOct = NoteQuantity::VOct(fParam);
		m_fLowerVOctThreshold = m_fLowerVOct - THRESHOLD_TOLERANCE;
		CheckParamError();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// WIDGET
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// All coordinates are mm

// X coordinates
#define TOTAL_HPS					(5.0f)		// total horizontal grid units
#define TOTAL_WIDTH					(TOTAL_HPS * RACK_GRID_WIDTH_MM)
#define HALF_WIDTH					(TOTAL_WIDTH / 2.0f)
#define SOCKET_X1					(6.08f)
#define SOCKET_X2					(TOTAL_WIDTH - SOCKET_X1)

// Y coordinates
#define SOCKET_LOWER_Y				(RACK_GRID_HEIGHT_MM - 14.0f)
#define SOCKET_UPPER_Y				(SOCKET_LOWER_Y - 13.0f)

#define MODE_Y						(33.0f)
#define UPPER_Y						(53.0f)
#define LOWER_Y						(75.0f)
#define LIGHT_Y						(81.5f)

ClampWidget::ClampWidget(ClampModule* pModule)
{
	setModule(pModule);
	setPanel(createPanel(asset::plugin(the_pPluginInstance, "res/Clamp.svg"), asset::plugin(the_pPluginInstance, "res/Clamp-dark.svg")));

	addChild(createWidget<ThemedScrew>(Vec(0, 0)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	auto pModeKnob = createParamCentered<PointyKnob12mm>(mm2px(Vec(HALF_WIDTH, MODE_Y)), pModule, ClampModule::ParamMode);
	pModeKnob->minAngle = -M_PI_2 / 2.0f;
	pModeKnob->maxAngle = M_PI_2 / 2.0f;
	addParam(pModeKnob);
	addParam(createParamCentered<FilledKnob14mm>(mm2px(Vec(HALF_WIDTH, UPPER_Y)), pModule, ClampModule::ParamUpperLimit));
	addParam(createParamCentered<FilledKnob14mm>(mm2px(Vec(HALF_WIDTH, LOWER_Y)), pModule, ClampModule::ParamLowerLimit));

	addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(HALF_WIDTH, MODE_Y)), pModule, ClampModule::LightError));

	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_X1, SOCKET_UPPER_Y)), pModule, ClampModule::InputVOct));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_X1, SOCKET_LOWER_Y)), pModule, ClampModule::InputGate));
	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_X2, SOCKET_UPPER_Y)), pModule, ClampModule::OutputVOct));
	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(SOCKET_X2, SOCKET_LOWER_Y)), pModule, ClampModule::OutputGate));

}

Model* the_pClampModel = createModel<ClampModule, ClampWidget>("Clamp");
