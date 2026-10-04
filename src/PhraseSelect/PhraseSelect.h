#pragma once

struct PhraseSelectModule : Module
{
///////////////////////////////////////////////
// Public enums
///////////////////////////////////////////////

public:
	enum ParamId
	{
		ParamDefault,
		ParamApply,
		ParamOffset,
		NumParams
	};
	enum InputId
	{
		InputVOct,
		InputGate,
		NumInputs
	};
	enum OutputId
	{
		OutputPhrase,
		NumOutputs
	};
	enum LightId
	{
		LightError,
		NumLights
	};

///////////////////////////////////////////////
// Construction
///////////////////////////////////////////////

public:
	PhraseSelectModule();

///////////////////////////////////////////////
// Public API
///////////////////////////////////////////////

public:
	void onSampleRateChange(const SampleRateChangeEvent &e) override;
	void process(const ProcessArgs& args) override;

///////////////////////////////////////////////
// Internal methods
///////////////////////////////////////////////

private:
	// void CheckParamError();
	void HandleDefault(bool bForce = false);
	void HandleApply();
	void HandleOffset(bool bForce = false);

///////////////////////////////////////////////
// Data
///////////////////////////////////////////////

private:
	bool m_bInitialized = false;

	float m_fParamDefault = 0.0f;
	float m_fVoltDefault = 0.0f;
	bool m_bParamApply = false;
	float m_fParamOffset = 0.0f;
	float m_fVoltOffset = 0.0f;

	int64_t m_nErrorHoldSamples = 48000;
	int64_t m_nErrorStartSample = -1;

	rack::dsp::SchmittTrigger m_trigGate[PORT_MAX_CHANNELS];
};

struct PhraseSelectWidget : ModuleWidget
{
public:
	explicit PhraseSelectWidget(PhraseSelectModule* pModule);
};

extern Model* the_pPhraseSelectModel;
