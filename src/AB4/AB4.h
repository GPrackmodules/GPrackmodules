#pragma once

struct AB4Module : Module
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
		InputB1,
		InputB2,
		InputB3,
		InputB4,
		NumInputs
	};
	enum OutputId
	{
		Output1,
		Output2,
		Output3,
		Output4,
		NumOutputs
	};
	enum LightId
	{
		LightB,
		NumLights
	};

public:
	AB4Module();

public:
	void process(const ProcessArgs& args) override;
	bool GetB() const { return m_bB; }
	void SetWidget(struct AB4Widget* pWidget) { m_pWidget = pWidget; }

private:
	struct AB4Widget* m_pWidget = nullptr;
	bool m_bB = false;
	bool m_bInitialized = false;
};

struct ABDisplay : public rack::widget::SvgWidget
{
public:
	ABDisplay();

private:
	void drawLayer(const rack::widget::Widget::DrawArgs& args, int nLayer) override;
};

struct AB4Widget : ModuleWidget
{
public:
	AB4Widget(AB4Module* pModule);

public:
	void SetState(bool bB);

private:
	shared_ptr<rack::window::Svg> m_pSvgA;
	shared_ptr<rack::window::Svg> m_pSvgB;
	ABDisplay* m_pSvgWidget;
};

extern Model* the_pAB4Model;
