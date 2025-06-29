#include "plugin.hpp"
#include "AB4.h"

AB4Module::AB4Module()
{
	config(NumParams, NumInputs, NumOutputs, NumLights);
	configParam(ParamAB, 0.f, 1.f, 0.f, "");
	configInput(InputA1, "Input A1");
	configInput(InputA2, "Input A2");
	configInput(InputA3, "A Input A3");
	configInput(InputA4, "A Input A4");
	configInput(InputB1, "B Input B1");
	configInput(InputB2, "B Input B2");
	configInput(InputB3, "B Input B3");
	configInput(InputB4, "B Input B4");
	configOutput(Output1, "Output 1");
	configOutput(Output2, "Output 2");
	configOutput(Output3, "Output 3");
	configOutput(Output4, "Output 4");

}

void AB4Module::process(const ProcessArgs& args) /*override*/
{
	if (!m_bInitialized)
	{
		m_bInitialized = true;
		if (params[ParamAB].getValue() < 0.5f)
		{
			m_bParamB = false;
			lights[LightB].setBrightness(0.2f);
		}
		else
		{
			m_bParamB = true;
			lights[LightB].setBrightness(1.0f);
		}
		if (m_pWidget != nullptr)
			m_pWidget->SetState(m_bParamB);
	}

	bool bParamB = params[ParamAB].getValue() > 0.5f;
	bool bInputB = (inputs[InputAB].getVoltage() > 1.0f);
	if (bParamB != m_bParamB || bInputB != m_bInputB)
	{
		m_bParamB = bParamB;
		m_bInputB = bInputB;
		if (!bParamB)
			lights[LightB].setBrightness(0.2f);
		else
			lights[LightB].setBrightness(1.0f);
		if (m_pWidget != nullptr)
			m_pWidget->SetState(m_bParamB || m_bInputB);
	}

	if (m_bParamB || m_bInputB)
	{
		int n = inputs[InputB1].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output1].setVoltage(inputs[InputB1].getVoltage(i), i);
		outputs[Output1].setChannels(n);

		n = inputs[InputB2].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output2].setVoltage(inputs[InputB2].getVoltage(i), i);
		outputs[Output2].setChannels(n);

		n = inputs[InputB3].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output3].setVoltage(inputs[InputB3].getVoltage(i), i);
		outputs[Output3].setChannels(n);

		n = inputs[InputB4].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output4].setVoltage(inputs[InputB4].getVoltage(i), i);
		outputs[Output4].setChannels(n);
	}
	else
	{
		int n = inputs[InputA1].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output1].setVoltage(inputs[InputA1].getVoltage(i), i);
		outputs[Output1].setChannels(n);

		n = inputs[InputA2].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output2].setVoltage(inputs[InputA2].getVoltage(i), i);
		outputs[Output2].setChannels(n);

		n = inputs[InputA3].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output3].setVoltage(inputs[InputA3].getVoltage(i), i);
		outputs[Output3].setChannels(n);

		n = inputs[InputA4].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output4].setVoltage(inputs[InputA4].getVoltage(i), i);
		outputs[Output4].setChannels(n);
	}
}

AB4Widget::AB4Widget(AB4Module* pModule)
{
	if (pModule != nullptr)
		pModule->SetWidget(this);

	m_pSvgA = APP->window->loadSvg(asset::plugin(the_pPluginInstance, "res/A.svg"));
	m_pSvgB = APP->window->loadSvg(asset::plugin(the_pPluginInstance, "res/B.svg"));

	setModule(pModule);
	setPanel(createPanel(asset::plugin(the_pPluginInstance, "res/AB4.svg"), asset::plugin(the_pPluginInstance, "res/AB4-dark.svg")));

	addChild(createWidget<ThemedScrew>(Vec(0, 0)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	float fX = 5.5f;
	float fY = 27.0f;
	Vec vecRight(box.size.x, 0);

	Vec vecSvg = vecRight.plus(mm2px(Vec(-fX, fY))).minus(Vec(m_pSvgA->getSize().x / 2.0f, m_pSvgA->getSize().y / 2.0f));
	m_pSvgWidget = createWidget<ABDisplay>(vecSvg);
	if (pModule != nullptr)
		SetState(pModule->GetB());
	addChild(m_pSvgWidget);

	fY += 11;
	addParam(createParamCentered<VCVLatch>(vecRight.plus(mm2px(Vec(-fX, fY))), pModule, AB4Module::ParamAB));
	addChild(createLightCentered<MediumLight<GreenLight>>(vecRight.plus(mm2px(Vec(-fX, fY))), pModule, AB4Module::LightB));
	addInput(createInputCentered<ThemedPJ301MPort>(vecRight.plus(mm2px(Vec(-fX, fY + 11.0f))), pModule, AB4Module::InputAB));


	fY = 27.0f;
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(fX, fY)), pModule, AB4Module::InputA1));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(fX, fY += 11.0f)), pModule, AB4Module::InputA2));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(fX, fY += 11.0f)), pModule, AB4Module::InputA3));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(fX, fY += 11.0f)), pModule, AB4Module::InputA4));
	Vec vecBottom(0, RACK_GRID_HEIGHT);
	fY = -16.0f - 3 * 11.0f;
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY))), pModule, AB4Module::InputB1));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 11.0f))), pModule, AB4Module::InputB2));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 11.0f))), pModule, AB4Module::InputB3));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 11.0f))), pModule, AB4Module::InputB4));

	Vec vecBottomRight(box.size.x, RACK_GRID_HEIGHT);
	fY = -16.0f - 3 * 11.0f;
	addOutput(createOutputCentered<ThemedPJ301MPort>(vecBottomRight.plus(mm2px(Vec(-fX, fY))), pModule, AB4Module::Output1));
	addOutput(createOutputCentered<ThemedPJ301MPort>(vecBottomRight.plus(mm2px(Vec(-fX, fY += 11.0f))), pModule, AB4Module::Output2));
	addOutput(createOutputCentered<ThemedPJ301MPort>(vecBottomRight.plus(mm2px(Vec(-fX, fY += 11.0f))), pModule, AB4Module::Output3));
	addOutput(createOutputCentered<ThemedPJ301MPort>(vecBottomRight.plus(mm2px(Vec(-fX, fY += 11.0f))), pModule, AB4Module::Output4));

}

ABDisplay::ABDisplay()
{

}

void ABDisplay::drawLayer(const rack::widget::Widget::DrawArgs& args, int nLayer) /*override*/
{
	if (nLayer== 1)
		draw(args);
	// SvgWidget::drawLayer(args, nLayer);
}

void AB4Widget::SetState(bool bB)
{
	if (bB)
		m_pSvgWidget->setSvg(m_pSvgB);
	else
		m_pSvgWidget->setSvg(m_pSvgA);
}

Model* the_pAB4Model = createModel<AB4Module, AB4Widget>("AB4");