#include "plugin.hpp"
#include "AB8.h"

AB8Module::AB8Module()
{
	config(NumParams, NumInputs, NumOutputs, NumLights);
	configParam(ParamAB, 0.f, 1.f, 0.f, "");
	configInput(InputA1, "A1");
	configInput(InputA2, "A2");
	configInput(InputA3, "A3");
	configInput(InputA4, "A4");
	configInput(InputA5, "A5");
	configInput(InputA6, "A6");
	configInput(InputA7, "A7");
	configInput(InputA8, "A8");
	configInput(InputB1, "B1");
	configInput(InputB2, "B2");
	configInput(InputB3, "B3");
	configInput(InputB4, "B4");
	configInput(InputB5, "B5");
	configInput(InputB6, "B6");
	configInput(InputB7, "B7");
	configInput(InputB8, "B8");
	configInput(InputAB, "CV A/B");
	configOutput(Output1, "1");
	configOutput(Output2, "2");
	configOutput(Output3, "3");
	configOutput(Output4, "4");
	configOutput(Output5, "5");
	configOutput(Output6, "6");
	configOutput(Output7, "7");
	configOutput(Output8, "8");
	configOutput(OutputAB, "CV A/B");
}

void AB8Module::process(const ProcessArgs& args) /*override*/
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

		n = inputs[InputB5].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output5].setVoltage(inputs[InputB5].getVoltage(i), i);
		outputs[Output5].setChannels(n);

		n = inputs[InputB6].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output6].setVoltage(inputs[InputB6].getVoltage(i), i);
		outputs[Output6].setChannels(n);

		n = inputs[InputB7].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output7].setVoltage(inputs[InputB7].getVoltage(i), i);
		outputs[Output7].setChannels(n);

		n = inputs[InputB8].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output8].setVoltage(inputs[InputB8].getVoltage(i), i);
		outputs[Output8].setChannels(n);

		outputs[OutputAB].setVoltage(10.0f);
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

		n = inputs[InputA5].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output5].setVoltage(inputs[InputA5].getVoltage(i), i);
		outputs[Output5].setChannels(n);

		n = inputs[InputA6].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output6].setVoltage(inputs[InputA6].getVoltage(i), i);
		outputs[Output6].setChannels(n);

		n = inputs[InputA7].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output7].setVoltage(inputs[InputA7].getVoltage(i), i);
		outputs[Output7].setChannels(n);

		n = inputs[InputA8].getChannels();
		for (int i = 0; i < n; i++)
			outputs[Output8].setVoltage(inputs[InputA8].getVoltage(i), i);
		outputs[Output8].setChannels(n);

		outputs[OutputAB].setVoltage(0.0f);
	}
}

AB8Widget::AB8Widget(AB8Module* pModule)
{
	if (pModule != nullptr)
		pModule->SetWidget(this);

	m_pSvgA = APP->window->loadSvg(asset::plugin(the_pPluginInstance, "res/A.svg"));
	m_pSvgB = APP->window->loadSvg(asset::plugin(the_pPluginInstance, "res/B.svg"));

	setModule(pModule);
	setPanel(createPanel(asset::plugin(the_pPluginInstance, "res/AB8.svg"), asset::plugin(the_pPluginInstance, "res/AB8-dark.svg")));

	addChild(createWidget<ThemedScrew>(Vec(0, 0)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	float fX = 5.5f;
	float fY = 16.5f;
	addParam(createParamCentered<VCVLatch>(mm2px(Vec(fX, fY + 10.16f)), pModule, AB8Module::ParamAB));
	addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(fX, fY + 10.16f)), pModule, AB8Module::LightB));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(fX, fY)), pModule, AB8Module::InputAB));
	fX = 3 * RACK_GRID_WIDTH_MM;
	fY += 5.08f;
	Vec vecSvg = mm2px(Vec(fX, fY)).minus(Vec(m_pSvgA->getSize().x / 2.0f, m_pSvgA->getSize().y / 2.0f));
	m_pSvgWidget = createWidget<ABDisplay>(vecSvg);
	if (pModule != nullptr)
		SetState(pModule->GetB());
	addChild(m_pSvgWidget);
	fX = 5.5f;
	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(6 * RACK_GRID_WIDTH_MM - fX, fY)), pModule, AB8Module::OutputAB));

	Vec vecBottom(0, RACK_GRID_HEIGHT);
	fY = -14.0f - 7 * 10.16f;
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY))), pModule, AB8Module::InputA1));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 10.16f))), pModule, AB8Module::InputA2));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 10.16f))), pModule, AB8Module::InputA3));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 10.16f))), pModule, AB8Module::InputA4));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 10.16f))), pModule, AB8Module::InputA5));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 10.16f))), pModule, AB8Module::InputA6));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 10.16f))), pModule, AB8Module::InputA7));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 10.16f))), pModule, AB8Module::InputA8));

	fY = -14.0f - 7 * 10.16f;
	fX = 3 * RACK_GRID_WIDTH_MM;
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY))), pModule, AB8Module::InputB1));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 10.16f))), pModule, AB8Module::InputB2));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 10.16f))), pModule, AB8Module::InputB3));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 10.16f))), pModule, AB8Module::InputB4));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 10.16f))), pModule, AB8Module::InputB5));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 10.16f))), pModule, AB8Module::InputB6));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 10.16f))), pModule, AB8Module::InputB7));
	addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY += 10.16f))), pModule, AB8Module::InputB8));

	Vec vecBottomRight(box.size.x, RACK_GRID_HEIGHT);
	fX = 5.5f;
	fY = -14.0f - 7 * 10.16f;
	addOutput(createOutputCentered<ThemedPJ301MPort>(vecBottomRight.plus(mm2px(Vec(-fX, fY))), pModule, AB8Module::Output1));
	addOutput(createOutputCentered<ThemedPJ301MPort>(vecBottomRight.plus(mm2px(Vec(-fX, fY += 10.16f))), pModule, AB8Module::Output2));
	addOutput(createOutputCentered<ThemedPJ301MPort>(vecBottomRight.plus(mm2px(Vec(-fX, fY += 10.16f))), pModule, AB8Module::Output3));
	addOutput(createOutputCentered<ThemedPJ301MPort>(vecBottomRight.plus(mm2px(Vec(-fX, fY += 10.16f))), pModule, AB8Module::Output4));
	addOutput(createOutputCentered<ThemedPJ301MPort>(vecBottomRight.plus(mm2px(Vec(-fX, fY += 10.16f))), pModule, AB8Module::Output5));
	addOutput(createOutputCentered<ThemedPJ301MPort>(vecBottomRight.plus(mm2px(Vec(-fX, fY += 10.16f))), pModule, AB8Module::Output6));
	addOutput(createOutputCentered<ThemedPJ301MPort>(vecBottomRight.plus(mm2px(Vec(-fX, fY += 10.16f))), pModule, AB8Module::Output7));
	addOutput(createOutputCentered<ThemedPJ301MPort>(vecBottomRight.plus(mm2px(Vec(-fX, fY += 10.16f))), pModule, AB8Module::Output8));
}

void AB8Widget::SetState(bool bB)
{
	if (bB)
		m_pSvgWidget->setSvg(m_pSvgB);
	else
		m_pSvgWidget->setSvg(m_pSvgA);
}

Model* the_pAB8Model = createModel<AB8Module, AB8Widget>("AB8");