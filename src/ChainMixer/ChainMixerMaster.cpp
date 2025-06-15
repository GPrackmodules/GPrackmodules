#include "plugin.hpp"
#include "Faders.h"
#include "Knobs.h"
#include "ChainMixerChannel.h"
#include "ChainMixerAux.h"

#include "ChainMixerMaster.h"

#define RIGHT_3U_MM		((13.5f))

// ========================================================================================================================================
// MODULE
// ========================================================================================================================================

ChainMixerMasterModule::ChainMixerMasterModule() :
	ChainMixerModule(ChainMixerModule::ModuleType::Master),
	m_fadeMain(m_fFactorFader, FADE_MS, 0.0f),
	m_fadeAux1(m_fFactorAux1, FADE_MS, 0.0f),
	m_fadeAux2(m_fFactorAux2, FADE_MS, 0.0f)
{
	config(NumParams, 0, NumOutputs, NumLights);
	configParam<SendQuantity>(ParamAux1, 0.f, (float)SEND_STEPS, (float)SEND_STEPS, "Aux 1 Send Level");
	configParam<SendQuantity>(ParamAux2, 0.f, (float)SEND_STEPS, (float)SEND_STEPS, "Aux 2 Send Level");
	configParam<FaderGainQuantity>(ParamGain, 0.f, FADER_STEPS_F, FADER_ZERO_DB, "Gain");
	paramQuantities[ParamGain]->snapEnabled = true;
	configParam(ParamMute, 0.f, 1.f, 0.f, "Mute");
	configOutput(OutputL, "Input Left");
	configOutput(OutputR, "Input Rught");
}

void ChainMixerMasterModule::onSampleRateChange(const SampleRateChangeEvent &e) /*override*/
{
	m_fadeMain.SetSamplerate(e.sampleRate);
	m_fadeAux1.SetSamplerate(e.sampleRate);
	m_fadeAux1.SetSamplerate(e.sampleRate);
}

void ChainMixerMasterModule::process(const ProcessArgs& args) /*override*/
{
	bool bWasDisabled = Disabled();
	DetermineTypeInstance(the_pChainMixerMasterModel);
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
	}

	if (HandleMute(ParamMute))
		lights[LightMute].setBrightness(Mute() ? MUTE_BRIGHTNESS : DIM_BRIGHTNESS);

	class ChainMixerAuxModule* pAuxModule = nullptr;

	// check solo buttons, look for AUX module
	DetermineSolo(pAuxModule);
	// Initialize bus values and pointer to them
	SetupBusses();
	// collect audio from channel modules
	ProcessChannelModules(args); //, pAuxModule);
	// Apply Aux send level knobs
	ProcessAuxGain();
	// collect audio from AUX module, write to AUX send outputs
	if (pAuxModule != nullptr)
		pAuxModule->ProcessAudioBusses(args, m_pMainL, m_pMainR, m_pAux1L, m_pAux1R, m_pAux2L, m_pAux2R, false, m_AuxInfo);
	// Process this module, write main output(s)
	ProcessAudioBusses(args, m_pMainL, m_pMainR, m_pAux1L, m_pAux1R, m_pAux2L, m_pAux2R, false, m_AuxInfo);
}

void ChainMixerMasterModule::DetermineSolo(ChainMixerAuxModule*& rpAuxModule)
{
	// iterate over modules on the left
	ChainMixerModule *pModule;
	rpAuxModule = nullptr;
	m_bAnyChannelSolo = false;
	for (pModule = dynamic_cast<ChainMixerModule*>(leftExpander.module); pModule != nullptr; pModule = dynamic_cast<ChainMixerModule*>(pModule->leftExpander.module))
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
	if (rpAuxModule == nullptr)
		memset(m_AuxInfo, 0, sizeof(m_AuxInfo));
}

void ChainMixerMasterModule::SetupBusses()
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
			m_pMainL = &m_fMainR;
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

void ChainMixerMasterModule::ProcessChannelModules(const ProcessArgs& args)
{
	// Collect audio from channel modules ...
	// ... left ...
	ChainMixerModule *pModule = dynamic_cast<ChainMixerModule*>(leftExpander.module);
	while (pModule != nullptr)
	{
		if (pModule->Type() == ChainMixerModule::ModuleType::Channel && !pModule->Disabled())
		{
			pModule->ProcessAudioBusses(args,
				m_pMainL, m_pMainR,
				m_pAux1L, m_pAux1R,
				m_pAux2L, m_pAux2R,
				m_bAnyChannelSolo, m_AuxInfo);
		}
		pModule = dynamic_cast<ChainMixerModule*>(pModule->leftExpander.module);
	}
	// ... and right
	pModule = dynamic_cast<ChainMixerModule*>(rightExpander.module);
	while (pModule != nullptr)
	{
		if (pModule->Type() == ChainMixerModule::ModuleType::Channel && !pModule->Disabled())
		{
			pModule->ProcessAudioBusses(args,
				m_pMainL, m_pMainR,
				m_pAux1L, m_pAux1R,
				m_pAux2L, m_pAux2R,
				m_bAnyChannelSolo, m_AuxInfo);
		}
		pModule = dynamic_cast<ChainMixerChannelModule*>(pModule->rightExpander.module);
	}
}

void ChainMixerMasterModule::ProcessAuxGain()
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

void ChainMixerMasterModule::ProcessAudioBusses(
	const ProcessArgs& args,
	float* pMainL, float* pMainR,
	float* pAux1L, float* pAux1R,
	float* pAux2L, float* pAux2R,
	bool bAnyChannelSolo,
	struct AuxInfo rInfo[2])
{
	float fFaderFactor = 0.0f;
	if (!Mute())
		fFaderFactor = GPaudioFader::GainFactor(paramQuantities[ParamGain]->getValue());
	m_fadeMain.Start(fFaderFactor);
	m_fMainL *= m_fFactorFader;
	m_fMainR *= m_fFactorFader;
	if (outputs[OutputL].isConnected())
	{
		if (outputs[OutputR].isConnected())
		{
			outputs[OutputL].setVoltage(m_fMainL);
			outputs[OutputR].setVoltage(m_fMainR);
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
		}
		else
		{
			outputs[OutputL].setVoltage(0.0f);
			outputs[OutputR].setVoltage(0.0f);
		}
	}

	m_fadeMain.Advance();
}

// ========================================================================================================================================
// WIDGET
// ========================================================================================================================================

ChainMixerMasterWidget::ChainMixerMasterWidget(ChainMixerMasterModule* pModule)
{
	if (pModule != nullptr)
		pModule->SetWidget(this);

	setModule(pModule);
	setPanel(createPanel(asset::plugin(the_pPluginInstance, "res/ChainMixerMaster.svg"), asset::plugin(the_pPluginInstance, "res/ChainMixerMaster-dark.svg")));

	addChild(createWidget<ThemedScrew>(Vec(0, 0)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	addParam(createParamCentered<PointyKnob10mm>(mm2px(Vec(RIGHT_3U_MM, KNOB_AUX1_Y_MM)), pModule, ChainMixerMasterModule::ParamAux1));
	addParam(createParamCentered<PointyKnob10mm>(mm2px(Vec(RIGHT_3U_MM, KNOB_AUX2_Y_MM)), pModule, ChainMixerMasterModule::ParamAux2));

	m_pFader = createParamCentered<GPaudioSlider44mm>(mm2px(Vec(RIGHT_3U_MM, SLIDER_Y_MM)), pModule, ChainMixerMasterModule::ParamGain);
	addParam(m_pFader);

	addParam(createParamCentered<VCVLatch>(mm2px(Vec(RIGHT_3U_MM, MUTE_Y_MM)), pModule, ChainMixerMasterModule::ParamMute));
	addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(RIGHT_3U_MM, MUTE_Y_MM)), pModule, ChainMixerMasterModule::LightMute));

	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(RIGHT_3U_MM, SOCKET_L_Y_MM)), pModule, ChainMixerMasterModule::OutputL));
	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(RIGHT_3U_MM, SOCKET_R_Y_MM)), pModule, ChainMixerMasterModule::OutputR));
}

void ChainMixerMasterWidget::step() /*override*/
{
	ModuleWidget::step();
	if (m_pFader != nullptr)
		m_pFader->UpdateDarkMode();
}


Model* the_pChainMixerMasterModel = createModel<ChainMixerMasterModule, ChainMixerMasterWidget>("ChainMixerMaster");
