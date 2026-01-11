#pragma once

#include "AB/ABDisplay.h"

struct AB8Module : Module
{
public:
	enum ParamId
	{
		ParamAB,
		NumParams
	};
	enum InputId
	{
		InputA1,
		InputA2,
		InputA3,
		InputA4,
		InputA5,
		InputA6,
		InputA7,
		InputA8,
		InputB1,
		InputB2,
		InputB3,
		InputB4,
		InputB5,
		InputB6,
		InputB7,
		InputB8,
		InputAB,
		NumInputs
	};
	enum OutputId
	{
		Output1,
		Output2,
		Output3,
		Output4,
		Output5,
		Output6,
		Output7,
		Output8,
		OutputAB,
		NumOutputs
	};
	enum LightId
	{
		LightB,
		NumLights
	};

public:
	AB8Module();

public:
	void process(const ProcessArgs& args) override;
	bool GetB() const { return m_bParamB; }
	void SetWidget(struct AB8Widget* pWidget) { m_pWidget = pWidget; }

private:
	struct AB8Widget* m_pWidget = nullptr;
	dsp::SchmittTrigger m_trigAB;
	bool m_bParamB = false;
	bool m_bInputB = false;
	bool m_bInitialized = false;
};

struct AB8Widget : ModuleWidget
{
public:
	AB8Widget(AB8Module* pModule);

public:
	void SetState(bool bB);

private:
	shared_ptr<rack::window::Svg> m_pSvgA;
	shared_ptr<rack::window::Svg> m_pSvgB;
	ABDisplay* m_pSvgWidget;
};

extern Model* the_pAB8Model;
