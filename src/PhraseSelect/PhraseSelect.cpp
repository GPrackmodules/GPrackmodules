#include "plugin.h"
#include "common/Knobs.h"
#include "PhraseSelect.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MODULE
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define THRESHOLD_TOLERANCE		(0.000001f)

// C3-C5 = 25 phrases
#define MIN_PHRASE		(1.0f)
#define MAX_PHRASE		(25.0f)

#define MIN_PHRASE_V	(-1.0001f) // allow some minor imprecision
#define MAX_PHRASE_V	(1.0001f)  // allow some minor imprecision

// offset +/- 4 octaves
#define MIN_OFFSET		(-48.0f)
#define MAX_OFFSET		(48.0f)

constexpr unsigned int ERROR_HOLD_MS = 1000;

PhraseSelectModule::PhraseSelectModule()
	: m_trigGate()
{
	config(NumParams, NumInputs, NumOutputs, NumLights);
	configParam(ParamDefault, MIN_PHRASE, MAX_PHRASE, 1.0f, "Default Phrase #");
	paramQuantities[ParamDefault]->snapEnabled = true;
	configParam(ParamApply, 0, 1.0f, 0.0f, "Apply Default Phrase");
	configParam(ParamOffset, MIN_OFFSET, MAX_OFFSET, 0.0f, "Offset", " halftones");
	paramQuantities[ParamOffset]->snapEnabled = true;
	configInput(InputVOct, "V/Oct");
	configInput(InputGate, "Gate");
	configOutput(OutputPhrase, "Phrase (C3-C5)");
}

void PhraseSelectModule::onSampleRateChange(const SampleRateChangeEvent &e) /*override*/
{
	m_nErrorHoldSamples = static_cast<uint64_t>(e.sampleRate) * ERROR_HOLD_MS / 1000;
}

void PhraseSelectModule::process(const ProcessArgs& args) /*override*/
{
	if (!m_bInitialized)
	{
		m_bInitialized = true;
		HandleDefault(true);
		HandleOffset(true);
		outputs[OutputPhrase].setVoltage(m_fVoltDefault);
	}
	HandleDefault();
	HandleApply();
	HandleOffset();

	int n = inputs[InputGate].getChannels();
	bool bError = false;
	bool bGood = false;
	int c;
	for (c = 0; c < n; c++)
	{
		if (c < inputs[InputVOct].getChannels())
		{
			if (m_trigGate[c].process(inputs[InputGate].getVoltage(c)))
			{
				float fPhrase = inputs[InputVOct].getVoltage(c) + m_fVoltOffset;
				if (fPhrase >= MIN_PHRASE_V && fPhrase <= MAX_PHRASE_V)
				{
					outputs[OutputPhrase].setVoltage(fPhrase);
					bGood = true;
					break; // gate with lowest number and valid voltage wins}
				}
				bError = true;
			}
		}
		else
			m_trigGate[c].process(0.0f);
	}
	for ( ; c < n; c++)
		m_trigGate[c].process(inputs[InputGate].getVoltage(c));
	for ( ; c < PORT_MAX_CHANNELS; c++)
		m_trigGate[c].reset();

	if (bError)
	{
		if (!bGood)
		{
			if (m_nErrorStartSample < 0)
				lights[LightError].setBrightness(1.0f);
			m_nErrorStartSample = args.frame;
		}
	}
	else if (m_nErrorStartSample >= 0)
	{
		if (args.frame > m_nErrorStartSample + m_nErrorHoldSamples || bGood)
		{
			lights[LightError].setBrightness(0.0f);
			m_nErrorStartSample = -1;
		}
	}
}

void PhraseSelectModule::HandleDefault(bool bForce /*= false*/)
{
	float fParam = params[ParamDefault].getValue();
	if (fParam != m_fParamDefault || bForce)
	{
		m_fParamDefault = fParam;
		m_fVoltDefault = -1.0f + (fParam - 1.0f) / 12.0f;
	}
}

void PhraseSelectModule::HandleApply()
{
	bool bApply = params[ParamApply].getValue() > 0.0f;
	if (bApply != m_bParamApply)
	{
		m_bParamApply = bApply;
		if (bApply)
			outputs[OutputPhrase].setVoltage(m_fVoltDefault);
	}
}

void PhraseSelectModule::HandleOffset(bool bForce /*= false*/)
{
	float fParam = params[ParamOffset].getValue();
	if (fParam != m_fParamOffset || bForce)
	{
		m_fParamOffset = fParam;
		m_fVoltOffset = fParam / 12.0f;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// WIDGET
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// All coordinates are mm

// X coordinates
#define TOTAL_HPS					(3.0f)		// total horizontal grid units
#define TOTAL_WIDTH					(TOTAL_HPS * RACK_GRID_WIDTH_MM)
#define HALF_WIDTH					(TOTAL_WIDTH / 2.0f)

// Y coordinates
#define DEFAULT_Y					(24.0f)
#define APPLY_Y						(39.0f)
#define OFFSET_Y					(59.5f)
#define SOCKET_VOCT_Y				(SOCKET_GATE_Y - 13.0f)
#define SOCKET_GATE_Y				(SOCKET_PHRASE_Y - 15.0f)
#define SOCKET_PHRASE_Y				(RACK_GRID_HEIGHT_MM - 14.0f)


PhraseSelectWidget::PhraseSelectWidget(PhraseSelectModule* pModule)
{
	setModule(pModule);
	setPanel(createPanel(asset::plugin(the_pPluginInstance, "res/PhraseSelect.svg"), asset::plugin(the_pPluginInstance, "res/PhraseSelect-dark.svg")));

	// addChild(createWidget<ThemedScrew>(Vec(0, 0)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	//addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	addParam(createParamCentered<PointyKnob12mm>(mm2px(Vec(HALF_WIDTH, DEFAULT_Y)), pModule, PhraseSelectModule::ParamDefault));
	addParam(createParamCentered<VCVButton>(mm2px(Vec(HALF_WIDTH, APPLY_Y)), pModule, PhraseSelectModule::ParamApply));
	addParam(createParamCentered<PointyKnob12mm>(mm2px(Vec(HALF_WIDTH, OFFSET_Y)), pModule, PhraseSelectModule::ParamOffset));
	addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(HALF_WIDTH, OFFSET_Y)), pModule, PhraseSelectModule::LightError));

	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(HALF_WIDTH, SOCKET_VOCT_Y)), pModule, PhraseSelectModule::InputVOct));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(HALF_WIDTH, SOCKET_GATE_Y)), pModule, PhraseSelectModule::InputGate));
	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(HALF_WIDTH, SOCKET_PHRASE_Y)), pModule, PhraseSelectModule::OutputPhrase));

}

Model* the_pPhraseSelectModel = createModel<PhraseSelectModule, PhraseSelectWidget>("Phrase");
