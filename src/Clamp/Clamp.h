#pragma once

struct ClampModule : Module
{
///////////////////////////////////////////////
// Public enums
///////////////////////////////////////////////

public:
	enum ParamId
	{
		ParamMode,
		ParamUpperLimit,
		ParamLowerLimit,
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
		OutputVOct,
		OutputGate,
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
	ClampModule();

///////////////////////////////////////////////
// Public API
///////////////////////////////////////////////

public:
	void process(const ProcessArgs& args) override;

///////////////////////////////////////////////
// Internal methods
///////////////////////////////////////////////

private:
	void CheckParamError();
	void HandleMode(bool bForce = false);
	void HandleUpperLimit(bool bForce = false);
	void HandleLowerLimit(bool bForce = false);

///////////////////////////////////////////////
// Data
///////////////////////////////////////////////

private:
	enum class Mode : int
	{
		Drop = 0,
		ShiftOctave,
		Limit
	};

	struct ClampWidget* m_pWidget = nullptr;
	bool m_bInitialized = false;

	bool m_bError = false;
	float m_fParamMode = 0.0f;
	float m_fParamUpperLimit = 0.0f;
	float m_fParamLowerLimit = 0.0f;

	Mode m_eMode = Mode::ShiftOctave;
	float m_fUpperVOct = 0.0f;
	float m_fLowerVOct = 0.0f;
	float m_fUpperVOctThreshold = 0.0f;	// VOct plus 1e-6 tolerance
	float m_fLowerVOctThreshold = 0.0f;	// VOct plus 1e-6 tolerance
};

struct ClampWidget : ModuleWidget
{
public:
	ClampWidget(ClampModule* pModule);

private:
};

extern Model* the_pClampModel;
