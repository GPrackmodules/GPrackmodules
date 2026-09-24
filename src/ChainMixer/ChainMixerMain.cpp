#include <cmath>
#include "plugin.h"
#include "Faders.h"
#include "Knobs.h"
#include "ChainMixerChannel.h"
#include "ChainMixerAux.h"
#include "ChainMixerCommon.h"

#include "ChainMixerMain.h"

constexpr float RIGHT_3U_MM = 13.5f;

constexpr float OVER_HOLD_SECONDS = 0.5f;

// ========================================================================================================================================
// MODULE
// ========================================================================================================================================

ChainMixerMainModule::ChainMixerMainModule() :
	ChainMixerModule(ChainMixerModule::ModuleType::Main),
	m_fadeMain(m_fFactorFader, FADE_MS, 0.0f),
	m_fadeAux1(m_fFactorAux1, FADE_MS, 0.0f),
	m_fadeAux2(m_fFactorAux2, FADE_MS, 0.0f),
	m_nOverFrame(-OVER_HOLD_SECONDS * 768000)
{
	config(NumParams, 0, NumOutputs, NumLights);
	configParam<SendQuantity>(ParamAux1, 0.f, SEND_STEPS_F, SEND_STEPS_F, "Aux 1 Send Level");
	configParam<SendQuantity>(ParamAux2, 0.f, SEND_STEPS_F, SEND_STEPS_F, "Aux 2 Send Level");
	configParam<FaderGainQuantity>(ParamGain, 0.f, FADER_STEPS_F, FADER_ZERO_DB, "Gain");
#ifdef CHAIN_MIXER_SNAPGAIN
	paramQuantities[ParamGain]->snapEnabled = true;
#endif
	configParam(ParamMute, 0.f, 1.f, 0.f, "Mute");
	configParam(ParamOverdB, -6.0f, 6.0f, 0.f, "OVR Threshold", "dB");
	configOutput(OutputL, "Left");
	configOutput(OutputR, "Right");
}

void ChainMixerMainModule::onSampleRateChange(const SampleRateChangeEvent &e) /*override*/
{
	m_fadeMain.SetSamplerate(e.sampleRate);
	m_fadeAux1.SetSamplerate(e.sampleRate);
	m_fadeAux1.SetSamplerate(e.sampleRate);
	m_nOverHoldFrames = lround(OVER_HOLD_SECONDS * e.sampleRate);
}

void ChainMixerMainModule::OverThreshold(float fdB, bool bUpdateParamter /*= true*/)
{
	m_fOverdB = fdB;
	float fVolt = 5.0f * pow(10.0f, fdB / 20.0f);
	if (fVolt > 10.0f)
		m_fOverVolt = 10.0f;
	else
		m_fOverVolt = fVolt;
	if (bUpdateParamter)
		paramQuantities[ParamOverdB]->setValue(fdB);
}

void ChainMixerMainModule::process(const ProcessArgs& args) /*override*/
{
	bool bWasDisabled = Disabled();
	DetermineTypeInstance(the_pChainMixerMainModel);
	if (!bWasDisabled && Disabled())
		lights[LightMute].setBrightness(0.0f);

	if (Disabled())
		return;

	if (!m_bInitialized || bWasDisabled)
	{
		m_bInitialized = true;
		HandleMute(ParamMute, true);
		lights[LightMute].setBrightness(Mute() ? MUTE_BRIGHTNESS : DIM_BRIGHTNESS);
		m_fadeMain.SetSamplerate(args.sampleRate);
		m_fadeAux1.SetSamplerate(args.sampleRate);
		m_fadeAux1.SetSamplerate(args.sampleRate);
		OverThreshold(params[ParamOverdB].getValue());
	}

	if (HandleMute(ParamMute))
		lights[LightMute].setBrightness(Mute() ? MUTE_BRIGHTNESS : DIM_BRIGHTNESS);

	class ChainMixerAuxModule* pAuxModule = nullptr;

	// Fader and mute button
	float fFaderFactor = 0.0f;
	if (!Mute())
		fFaderFactor = GPaudioFader::GainFactor(paramQuantities[ParamGain]->getValue());
	m_fadeMain.Start(fFaderFactor);
	// check solo buttons, look for AUX module
	DetermineSolo(pAuxModule);
	// Initialize bus values and pointer to them
	SetupBuses();
	// collect audio from channel modules
	ProcessChannelModules(args);
	// Apply Aux send level knobs
	ProcessAuxGain();
	// collect audio from AUX module, write to AUX send outputs
	if (pAuxModule != nullptr)
		pAuxModule->ProcessAudioBuses(args, m_pMainL, m_pMainR, m_pAux1L, m_pAux1R, m_pAux2L, m_pAux2R, m_fFactorFader, Mute(), false, m_AuxInfo);
	// Process this module, write main output(s)
	ProcessAudioBuses(args, m_pMainL, m_pMainR, m_pAux1L, m_pAux1R, m_pAux2L, m_pAux2R, m_fFactorFader, Mute(), false, m_AuxInfo);
}

void ChainMixerMainModule::DetermineSolo(ChainMixerAuxModule*& rpAuxModule)
{
	// iterate over modules on the left
	ChainMixerModule *pModule;
	rpAuxModule = nullptr;
	m_bAnyChannelSolo = false;
	for (pModule = dynamic_cast<ChainMixerModule*>(leftExpander.module); pModule != nullptr; pModule = dynamic_cast<ChainMixerModule*>(pModule->leftExpander.module))
	{
		if (pModule->Disabled())
			continue;
		if (((pModule->Type() == ChainMixerModule::ModuleType::Channel) ||
			(pModule->Type() == ChainMixerModule::ModuleType::ExtChannel))
			&& !pModule->Disabled())
		{
			if (pModule->Solo())
				m_bAnyChannelSolo = true;
		}
		else if (pModule->Type() == ChainMixerModule::ModuleType::Aux && !pModule->Disabled())
		{
			rpAuxModule = dynamic_cast<ChainMixerAuxModule*>(pModule);
			rpAuxModule->GetAuxInfo(m_AuxInfo);
		}
		if (m_bAnyChannelSolo && rpAuxModule != nullptr) // all we wanted to know
			break;
	}
	if (m_bAnyChannelSolo && rpAuxModule != nullptr) // all we wanted to know
		return;
	// iterate over modules on the right
	for (pModule = dynamic_cast<ChainMixerModule*>(rightExpander.module); pModule != nullptr; pModule = dynamic_cast<ChainMixerModule*>(pModule->rightExpander.module))
	{
		if (pModule->Disabled())
			continue;
		if (pModule->Type() == ChainMixerModule::ModuleType::Channel && !pModule->Disabled())
		{
			if (pModule->Solo())
				m_bAnyChannelSolo = true;
		}
		else if (pModule->Type() == ChainMixerModule::ModuleType::Aux && !pModule->Disabled())
		{
			rpAuxModule = dynamic_cast<ChainMixerAuxModule*>(pModule);
			rpAuxModule->GetAuxInfo(m_AuxInfo);
		}
		if (m_bAnyChannelSolo && rpAuxModule != nullptr) // all we wanted to know
			break;
	}
}

void ChainMixerMainModule::SetupBuses()
{
	// The left channels are also the ones to use for mono, even if only the right channel is connected
	if (outputs[OutputL].isConnected())
	{
		m_pMainL = &m_fMainL;
		m_fMainL = 0.0f;
		if (outputs[OutputR].isConnected())
		{
			m_pMainR = &m_fMainR;
			m_fMainR = 0.0f;
		}
		else
			m_pMainR = nullptr;
	}
	else
	{
		if (outputs[OutputR].isConnected())
		{
			m_pMainL = &m_fMainL; // mono signal is always on L
			m_fMainL = 0.0f;
		}
		else
			m_pMainL = nullptr;;
		m_pMainR = nullptr;
	}
	if (m_AuxInfo[0].bConnected)
	{
		m_pAux1L = &m_fAux1L;
		m_fAux1L = 0.0f;
		if (!m_AuxInfo[0].bMono)
		{
			m_pAux1R = &m_fAux1R;
			m_fAux1R = 0.0f;
		}
		else
			m_pAux1R = nullptr;
	}
	else
	{
		m_pAux1L = nullptr;
		m_pAux1R = nullptr;
	}
	if (m_AuxInfo[1].bConnected)
	{
		m_pAux2L = &m_fAux2L;
		m_fAux2L = 0.0f;
		if (!m_AuxInfo[1].bMono)
		{
			m_pAux2R = &m_fAux2R;
			m_fAux2R = 0.0f;
		}
		else
			m_pAux2R = nullptr;
	}
	else
	{
		m_pAux2L = nullptr;
		m_pAux2R = nullptr;
	}
}

void ChainMixerMainModule::ProcessChannelModules(const ProcessArgs& args)
{
	// Collect audio from channel modules ...
	// ... left ...
	auto pModule = dynamic_cast<ChainMixerModule*>(leftExpander.module);
	while (pModule != nullptr)
	{
		if (((pModule->Type() == ChainMixerModule::ModuleType::Channel) ||
			(pModule->Type() == ChainMixerModule::ModuleType::ExtChannel)) &&
			!pModule->Disabled())
		{
			pModule->ProcessAudioBuses(args,
				m_pMainL, m_pMainR,
				m_pAux1L, m_pAux1R,
				m_pAux2L, m_pAux2R,
				m_fFactorFader,
				Mute(),
				m_bAnyChannelSolo, m_AuxInfo);
		}
		pModule = dynamic_cast<ChainMixerModule*>(pModule->leftExpander.module);
	}
	// ... and right
	pModule = dynamic_cast<ChainMixerModule*>(rightExpander.module);
	while (pModule != nullptr)
	{
		if (((pModule->Type() == ChainMixerModule::ModuleType::Channel) ||
			(pModule->Type() == ChainMixerModule::ModuleType::ExtChannel))
			&& !pModule->Disabled())
		{Mute(),
			pModule->ProcessAudioBuses(args,
				m_pMainL, m_pMainR,
				m_pAux1L, m_pAux1R,
				m_pAux2L, m_pAux2R,
				m_fFactorFader,
				Mute(),
				m_bAnyChannelSolo, m_AuxInfo);
		}
		pModule = dynamic_cast<ChainMixerChannelModule*>(pModule->rightExpander.module);
	}
}

void ChainMixerMainModule::ProcessAuxGain()
{
	float fParam = params[ParamAux1].getValue();
	float fFactor = SendQuantity::GainFactor(fParam);
	if (m_pAux1L != nullptr)
	{
		m_fadeAux1.Start(fFactor);
		*m_pAux1L *= fFactor;
		if (m_pAux1R != nullptr)
			*m_pAux1R *= fFactor;
	}
	else
		m_fadeAux1.Start(0.0f);

	fParam = params[ParamAux2].getValue();
	fFactor = SendQuantity::GainFactor(fParam);
	if (m_pAux2L != nullptr)
	{
		m_fadeAux2.Start(fFactor);
		*m_pAux2L *= fFactor;
		if (m_pAux2R != nullptr)
			*m_pAux2R *= fFactor;
	}
	else
		m_fadeAux2.Start(0.0f);
	
}

void ChainMixerMainModule::ProcessAudioBuses(
	const ProcessArgs& args,
	float* pMainL, float* pMainR,
	float* pAux1L, float* pAux1R,
	float* pAux2L, float* pAux2R,
	float fMainFactor,
	bool bMainMute,
	bool bAnyChannelSolo,
	struct AuxInfo rInfo[2])
{
	m_fMainL *= m_fFactorFader;
	m_fMainR *= m_fFactorFader;
	bool bOver = false;
	if (outputs[OutputL].isConnected())
	{
		bOver = m_fMainL < -m_fOverVolt || m_fMainL >= m_fOverVolt;
		if (outputs[OutputR].isConnected())
		{
			outputs[OutputL].setVoltage(m_fMainL);
			outputs[OutputR].setVoltage(m_fMainR);
			bOver = bOver || (m_fMainR < -m_fOverVolt || m_fMainR >= m_fOverVolt);
		}
		else
		{
			outputs[OutputL].setVoltage(m_fMainL);
			outputs[OutputR].setVoltage(0.0f);
		}
	}
	else
	{
		if (outputs[OutputR].isConnected())
		{
			outputs[OutputL].setVoltage(0.0f);
			outputs[OutputR].setVoltage(m_fMainL); // fMainL is intended
			bOver = m_fMainR < -m_fOverVolt || m_fMainR >= m_fOverVolt;
		}
		else
		{
			outputs[OutputL].setVoltage(0.0f);
			outputs[OutputR].setVoltage(0.0f);
		}
	}
	if (bOver)
		m_nOverFrame = args.frame;
	else
	{
		if (args.frame < m_nOverFrame + m_nOverHoldFrames)
			bOver = true;
	}
	if (bOver != m_bOver)
	{
		lights[LightOver].setBrightness(bOver ? 1.0f : 0.0f);
		m_bOver = bOver;
	}

	m_fadeMain.Advance();
}

// ========================================================================================================================================
// WIDGET
// ========================================================================================================================================

ChainMixerMainWidget::ChainMixerMainWidget(ChainMixerMainModule* pModule)
{
	if (pModule != nullptr)
		pModule->SetWidget(this);

	setModule(pModule);
	setPanel(createPanel(asset::plugin(the_pPluginInstance, "res/ChainMixerMain.svg"), asset::plugin(the_pPluginInstance, "res/ChainMixerMain-dark.svg")));

	addChild(createWidget<ThemedScrew>(Vec(0, 0)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	addParam(createParamCentered<PointyKnob10mm>(mm2px(Vec(RIGHT_3U_MM, KNOB_AUX1_Y_MM)), pModule, ChainMixerMainModule::ParamAux1));
	addParam(createParamCentered<PointyKnob10mm>(mm2px(Vec(RIGHT_3U_MM, KNOB_AUX2_Y_MM)), pModule, ChainMixerMainModule::ParamAux2));

	addChild(createLightCentered<LargeLight<RedLight>>(mm2px(Vec(RIGHT_3U_MM, OVER_Y_MM)), pModule, ChainMixerMainModule::LightOver));

	m_pFader = createParamCentered<GPaudioSlider44mm>(mm2px(Vec(RIGHT_3U_MM, SLIDER_Y_MM)), pModule, ChainMixerMainModule::ParamGain);
	addParam(m_pFader);

	addParam(createParamCentered<VCVLatch>(mm2px(Vec(RIGHT_3U_MM, MUTE_Y_MM)), pModule, ChainMixerMainModule::ParamMute));
	addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(RIGHT_3U_MM, MUTE_Y_MM)), pModule, ChainMixerMainModule::LightMute));

	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(RIGHT_3U_MM, SOCKET_L_Y_MM)), pModule, ChainMixerMainModule::OutputL));
	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(RIGHT_3U_MM, SOCKET_R_Y_MM)), pModule, ChainMixerMainModule::OutputR));
}

void ChainMixerMainWidget::appendContextMenu(Menu* pMainMenu) /*override*/
{
	auto pModule = dynamic_cast<ChainMixerMainModule*>(getModule());
	if (pModule == nullptr)
		return;

	WeakPtr<ModuleWidget> weakThis = this;
	if (!weakThis)
		return;

	pMainMenu->addChild(new MenuSeparator);
	MenuItem* pThresholdSubMenu = createSubmenuItem("OVR Threshold", "", [=](ui::Menu* pSubMenu)
		{
			MenuItem* pItem = createCheckMenuItem("10V", "",
				[=]() { return pModule->OverThreshold() == 6.0f; },
				[=]() {	pModule->OverThreshold(6.0f); } );
			pSubMenu->addChild(pItem);

			pItem = createCheckMenuItem("+3dBfs", "",
				[=]() { return pModule->OverThreshold() == 3.0f; },
				[=]() {	pModule->OverThreshold(3.0f); } );
			pSubMenu->addChild(pItem);

			pItem = createCheckMenuItem("0dBfs (5V)", "",
				[=]() { return pModule->OverThreshold() == 0.0f; },
				[=]() {	pModule->OverThreshold(0.0f); } );
			pSubMenu->addChild(pItem);

			pItem = createCheckMenuItem("-3dBfs", "",
				[=]() { return pModule->OverThreshold() == -3.0f; },
				[=]() {	pModule->OverThreshold(-3.0f); } );
			pSubMenu->addChild(pItem);

			pItem = createCheckMenuItem("-6dBfs", "",
				[=]() { return pModule->OverThreshold() == -6.0f; },
				[=]() {	pModule->OverThreshold(-6.0f); } );
			pSubMenu->addChild(pItem);
		}
	);
	pMainMenu->addChild(pThresholdSubMenu);
}

void ChainMixerMainWidget::step() /*override*/
{
	ModuleWidget::step();
	if (m_pFader != nullptr)
		m_pFader->UpdateDarkMode();
}


Model* the_pChainMixerMainModel = createModel<ChainMixerMainModule, ChainMixerMainWidget>("ChainMixerMaster");
