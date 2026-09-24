#include "plugin.h"
#include "Faders.h"
#include "MuteSoloButton.h"

#include "ChainMixerAux.h"

#define SEND_L_Y_MM		SOCKET_L_Y_MM
#define SEND_R_Y_MM		SOCKET_R_Y_MM

#define RETURN_R_Y_MM	(SEND_L_Y_MM - 15.0f)
#define RETURN_L_Y_MM	(RETURN_R_Y_MM - SOCKET_STEP_MM)

#define AUXMUTE_Y_MM	(RETURN_L_Y_MM - 14.0f)
#define AUXSOLO_Y_MM	(AUXMUTE_Y_MM - SWITCH_STEP_MM)

#define AUXSLIDER_Y_MM	((16.0f + AUXSOLO_Y_MM - SWITCH_STEP_MM / 2.0f) / 2.0f)

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MODULE
////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChainMixerAuxModule::ChainMixerAuxModule() :
	ChainMixerModule(ChainMixerModule::ModuleType::Aux),
	m_fadeReturn1(m_fFactorReturn1, FADE_MS, 0.0f),
	m_fadeReturn2(m_fFactorReturn2, FADE_MS, 0.0f)
{
	config(NumParams, NumInputs, NumOutputs, NumLights);
	configParam<FaderGainQuantity>(ParamGain1, 0.f, FADER_STEPS_F, FADER_ZERO_DB, "Aux Return 1 Gain");
	configParam<FaderGainQuantity>(ParamGain2, 0.f, FADER_STEPS_F, FADER_ZERO_DB, "Aux Return 2 Gain");
#ifdef CHAIN_MIXER_SNAPGAIN
	paramQuantities[ParamGain1]->snapEnabled = true;
	paramQuantities[ParamGain2]->snapEnabled = true;
#endif
	configParam(ParamSolo1, 0.f, 1.f, 0.f, "Solo AUX 1");
	configParam(ParamSolo2, 0.f, 1.f, 0.f, "Solo AUX 2");
	configParam(ParamMute1, 0.f, 1.f, 0.f, "Mute AUX 1");
	configParam(ParamMute2, 0.f, 1.f, 0.f, "Mute AUX 2");
	configParam(ParamMuteSendsOnMainMute, 0.f, 1.f, 0.f, "Mute Aux sends on Main Mute");
	configInput(Return1L, "AUX Return 1 Left");
	configInput(Return1R, "AUX Return 1 Right");
	configInput(Return2L, "AUX Return 2 Left");
	configInput(Return2R, "AUX Return 2 Right");
	configOutput(Send1L, "AUX Send 1 Left");
	configOutput(Send1R, "AUX Send 1 Right");
	configOutput(Send2L, "AUX Send 2 Left");
	configOutput(Send2R, "AUX Send 2 Right");
}

void ChainMixerAuxModule::onSampleRateChange(const SampleRateChangeEvent &e) /*override*/
{
	m_fadeReturn1.SetSamplerate(e.sampleRate);
	m_fadeReturn2.SetSamplerate(e.sampleRate);
	m_bFadesInitialized = true;
}

void ChainMixerAuxModule::process(const ProcessArgs& args) /*override*/
{
	bool bWasDisabled = Disabled();
	DetermineTypeInstance(the_pChainMixerAuxModel);
	if (!bWasDisabled && Disabled())
	{
		lights[LightSolo1].setBrightness(0.0f);
		lights[LightMute1].setBrightness(0.0f);
		lights[LightSolo2].setBrightness(0.0f);
		lights[LightMute2].setBrightness(0.0f);
	}
	if (Disabled())
		return;

	if (!m_bInitialized || bWasDisabled)
	{
		m_bInitialized = true;
		HandleBoolParam(m_AuxInfo[0].bSolo, ParamSolo1, true);
		lights[LightSolo1].setBrightness(m_AuxInfo[0].bSolo ? SOLO_BRIGHTNESS : DIM_BRIGHTNESS);
		HandleBoolParam(m_AuxInfo[1].bSolo, ParamSolo2, true);
		lights[LightSolo2].setBrightness(m_AuxInfo[1].bSolo ? SOLO_BRIGHTNESS : DIM_BRIGHTNESS);
		HandleBoolParam(m_AuxInfo[0].bMute, ParamMute1, true);
		lights[LightMute1].setBrightness(m_AuxInfo[0].bMute ? MUTE_BRIGHTNESS : DIM_BRIGHTNESS);
		HandleBoolParam(m_AuxInfo[1].bMute, ParamMute2, true);
		lights[LightMute2].setBrightness(m_AuxInfo[1].bMute ? MUTE_BRIGHTNESS : DIM_BRIGHTNESS);
		m_bMuteSendsOnMainMute = params[ParamMuteSendsOnMainMute].getValue() > 0.5f;
	}

	HandleMuteSoloQueue();
	if (HandleBoolParam(m_AuxInfo[0].bSolo, ParamSolo1))
		lights[LightSolo1].setBrightness(m_AuxInfo[0].bSolo ? SOLO_BRIGHTNESS : DIM_BRIGHTNESS);
	if (HandleBoolParam(m_AuxInfo[1].bSolo, ParamSolo2))
		lights[LightSolo2].setBrightness(m_AuxInfo[1].bSolo ? SOLO_BRIGHTNESS : DIM_BRIGHTNESS);
	if (HandleBoolParam(m_AuxInfo[0].bMute, ParamMute1))
		lights[LightMute1].setBrightness(m_AuxInfo[0].bMute ? MUTE_BRIGHTNESS : DIM_BRIGHTNESS);
	if (HandleBoolParam(m_AuxInfo[1].bMute, ParamMute2))
		lights[LightMute2].setBrightness(m_AuxInfo[1].bMute ? MUTE_BRIGHTNESS : DIM_BRIGHTNESS);
}

void ChainMixerAuxModule::MuteSendsOnMainMute(bool bMuteSendsOnMainMute)
{
	m_bMuteSendsOnMainMute = bMuteSendsOnMainMute;
	paramQuantities[ParamMuteSendsOnMainMute]->setValue(bMuteSendsOnMainMute ? 1.0f : 0.0f);
}

void ChainMixerAuxModule::GetAuxInfo(struct AuxInfo rInfo[2])
{
	m_AuxInfo[0].bConnected = outputs[Send1L].isConnected() || outputs[Send1R].isConnected();
	m_AuxInfo[1].bConnected = outputs[Send2L].isConnected() || outputs[Send2R].isConnected();
	m_AuxInfo[0].bMono = outputs[Send1L].isConnected() != outputs[Send1R].isConnected();
	m_AuxInfo[1].bMono = outputs[Send2L].isConnected() != outputs[Send2R].isConnected();
	memcpy(rInfo, m_AuxInfo, sizeof(m_AuxInfo));
}

void ChainMixerAuxModule::ProcessAudioBuses(
	const ProcessArgs& args,
	float* pMainL, float* pMainR,
	float* pAux1L, float* pAux1R,
	float* pAux2L, float* pAux2R,
	float fMainFactor,
	bool bMainMute,
	bool bAnyChannelSolo,
	struct AuxInfo rInfo[2]) /*override*/
{
	if (!m_bFadesInitialized)
	{
		m_bFadesInitialized = true;
		m_fadeReturn1.SetSamplerate(args.sampleRate);
		m_fadeReturn2.SetSamplerate(args.sampleRate);
	}

	//
	// Sends
	//
	float fSend1L;
	float fSend1R;
	float fSend2L;
	float fSend2R;
	if (m_bMuteSendsOnMainMute && bMainMute)
	{
		fSend1L = 0.0f;
		fSend1R = 0.0f;
		fSend2L = 0.0f;
		fSend2R = 0.0f;
	}
	else
	{
		fSend1L = pAux1L != nullptr ? *pAux1L : 0.0f;
		fSend1R = pAux1R != nullptr ? *pAux1R : 0.0f;
		fSend2L = pAux2L != nullptr ? *pAux2L : 0.0f;
		fSend2R = pAux2R != nullptr ? *pAux2R : 0.0f;
	}
	if (outputs[Send1L].isConnected() && pAux1L != nullptr)
	{
		outputs[Send1L].setVoltage(fSend1L);
		if (outputs[Send1R].isConnected() && pAux1R != nullptr)
			outputs[Send1R].setVoltage(fSend1R);
		else
			outputs[Send1R].setVoltage(0.0f);
	}
	else
	{
		outputs[Send1L].setVoltage(0.0f);
		if (outputs[Send1R].isConnected() && pAux1L != nullptr)
			outputs[Send1R].setVoltage(fSend1L);	// yes, L
		else
			outputs[Send1R].setVoltage(0.0f);
	}

	if (outputs[Send2L].isConnected() && pAux2L != nullptr)
	{
		outputs[Send2L].setVoltage(fSend2L);
		if (outputs[Send2R].isConnected() && pAux2R != nullptr)
			outputs[Send2R].setVoltage(fSend2R);
		else
			outputs[Send2R].setVoltage(0.0f);
	}
	else
	{
		outputs[Send2L].setVoltage(0.0f);
		if (outputs[Send2R].isConnected() && pAux2L != nullptr)
			outputs[Send2R].setVoltage(fSend2L);	// yes, L
		else
			outputs[Send2R].setVoltage(0.0f);
	}

	//
	// Returns
	//

	// Return 1

	if (pMainL == nullptr)
	{
		m_fadeReturn1.Start(0.0f);
		m_fadeReturn2.Start(0.0f);
	}
	else
	{
		if (m_AuxInfo[0].bMute ||
			(m_AuxInfo[1].bSolo && !m_AuxInfo[0].bSolo))
		{
			m_fadeReturn1.Start(0.0f);
		}
		else
		{
			float fParam = params[ParamGain1].getValue();
			m_fadeReturn1.Start(GPaudioFader::GainFactor(fParam));
		}
		if (m_fFactorReturn1 > 0.0f)
		{
			if (inputs[Return1L].isConnected())
			{
				if (inputs[Return1R].isConnected())
				{ // return is stereo
					if (pMainR != nullptr)
					{
						*pMainL += inputs[Return1L].getVoltageSum() * m_fFactorReturn1;
						*pMainR += inputs[Return1R].getVoltageSum() * m_fFactorReturn1;
					}
					else
						*pMainL += (inputs[Return1L].getVoltageSum() + inputs[Return1R].getVoltageSum()) * m_fFactorReturn1 * g_fMinus3dB;
				}
				else
				{ // return is left only
					float fReturn = inputs[Return1L].getVoltageSum();
					// *pMainL +=  fReturn * m_fFactorReturn1;
					if (pMainR != nullptr)
					{
						fReturn *= g_fMinus3dB;
						*pMainL += fReturn * m_fFactorReturn1;
						*pMainR += fReturn * m_fFactorReturn1;
					}
					else
						*pMainL += fReturn * m_fFactorReturn1;

				}
			} // if (inputs[Return1L].isConnected())
			else
			{ // return is right only
				float fReturn = inputs[Return1R].getVoltageSum();
				if (pMainR != nullptr)
				{
					fReturn *= g_fMinus3dB;
					*pMainL += fReturn * m_fFactorReturn1;
					*pMainR += fReturn * m_fFactorReturn1;
				}
				else
					*pMainL += fReturn * m_fFactorReturn1;
			}
		} // if (m_fFactorReturn1 > 0.0f)

		// Return 2

		if (m_AuxInfo[1].bMute ||
			(m_AuxInfo[0].bSolo && !m_AuxInfo[1].bSolo))
		{
			m_fadeReturn2.Start(0.0f);
		}
		else
		{
			float fParam = params[ParamGain2].getValue();
			// float fFactor = GPaudioFader::GainFactor(fParam);
			m_fadeReturn2.Start(GPaudioFader::GainFactor(fParam));
		}
		if (m_fFactorReturn2 > 0.0f)
		{
			if (inputs[Return2L].isConnected())
			{
				if (inputs[Return2R].isConnected())
				{ // return is stereo
					if (pMainR != nullptr)
					{
						*pMainL += inputs[Return2L].getVoltageSum() * m_fFactorReturn2;
						*pMainR += inputs[Return2R].getVoltageSum() * m_fFactorReturn2;
					}
					else
						*pMainL += (inputs[Return2L].getVoltageSum() + inputs[Return2R].getVoltageSum()) * m_fFactorReturn2 * g_fMinus3dB;
				}
				else
				{ // return is left only
					float fReturn = inputs[Return2L].getVoltageSum();
					if (pMainR != nullptr)
					{
						fReturn *= g_fMinus3dB;
						*pMainL += fReturn * m_fFactorReturn2;
						*pMainR += fReturn * m_fFactorReturn2;
					}
					else
						*pMainL += fReturn * m_fFactorReturn2;
				}
			} // if (inputs[Return2L].isConnected())
			else
			{
				float fReturn = inputs[Return2R].getVoltageSum();
				*pMainL +=  fReturn * m_fFactorReturn2;
				if (pMainR != nullptr)
				{
					fReturn *= g_fMinus3dB;
					*pMainL += fReturn * m_fFactorReturn2;
					*pMainR += fReturn * m_fFactorReturn2;
				}
				else
					*pMainL +=  fReturn * m_fFactorReturn2;
			}
		} // if (m_fFactorReturn1 > 0.0f)
	} // fMainL != nullptr
	m_fadeReturn1.Advance();
	m_fadeReturn2.Advance();
}

void ChainMixerAuxModule::HandleMuteSoloQueue()
{
	MuteSoloEvent ev;
	lock_guard<mutex> lg(MuteSoloMutex());
	while (MuteSoloEventFromQueue(ev))
	{
		if (!ev.bPressed)
			continue;

		bool bIsMute;
		int nIndex;
		int nOtherParamId;

		switch (ev.nParamId)
		{
			case ParamSolo1: bIsMute = false; nIndex = 0; nOtherParamId = ParamSolo2; break;
			case ParamSolo2: bIsMute = false; nIndex = 1; nOtherParamId = ParamSolo1; break;
			case ParamMute1: bIsMute = true; nIndex = 0; nOtherParamId = ParamMute2; break;
			case ParamMute2: bIsMute = true; nIndex = 1; nOtherParamId = ParamMute1; break;
			default: continue;
		}
		int nOtherIndex = nIndex ^ 1;
		const bool bCurrentValue = params[ev.nParamId].getValue() > 0.5f;
		float fNewValue = bCurrentValue ? 0.0f : 1.0f;

		auto pChanges = new MultiParamChange();
		history::ParamChange chg;
		chg.moduleId = id;
		chg.paramId = ev.nParamId;
		chg.oldValue = params[ev.nParamId].getValue();
		chg.newValue = fNewValue;
		pChanges->vChanges.emplace_back(chg);
		params[ev.nParamId].setValue(fNewValue);
		if (ev.bCtrlKeyDown)
		{
			chg.paramId = nOtherParamId;
			chg.oldValue = params[nOtherParamId].getValue();
			if (bIsMute)
				chg.newValue = 0.0f;
			else
			{
				// Solo
				m_bOldSoloValid[nIndex] = false;
				if (ev.bShiftKeyDown)
				{
					chg.newValue = 0.0f;
					m_bOldSoloValid[nOtherIndex] = false;
				}
				else
				{
					if (bCurrentValue)
					{
						if (m_bOldSoloValid[nOtherIndex])
						{
							chg.newValue = m_bOldSolo[nOtherIndex] ? 1.0f : 0.0f;
							m_bOldSoloValid[nOtherIndex] = false;
						}
						else
							continue; // do not affect ofther cahnnel
					}
					else
					{
						m_bOldSolo[nOtherIndex] = m_AuxInfo[nOtherIndex].bSolo;
						m_bOldSoloValid[nOtherIndex] = true;
						chg.newValue = 0.0f;
					}
				}
			}
			pChanges->vChanges.emplace_back(chg);
			params[nOtherParamId].setValue(chg.newValue);
		}
		APP->history->push(pChanges);
	} // while (MuteSoloEventFromQueue(ev))
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// WIDGET
////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChainMixerAuxWidget::ChainMixerAuxWidget(ChainMixerAuxModule* pModule)
{
	if (pModule != nullptr)
		pModule->SetWidget(this);

	setModule(pModule);
	setPanel(createPanel(asset::plugin(the_pPluginInstance, "res/ChainMixerAux.svg"), asset::plugin(the_pPluginInstance, "res/ChainMixerAux-dark.svg")));

	addChild(createWidget<ThemedScrew>(Vec(0, 0)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	m_pFader1 = createParamCentered<GPaudioSlider38mm>(mm2px(Vec(CENTER_LEFT_MM, AUXSLIDER_Y_MM)), pModule, ChainMixerAuxModule::ParamGain1);
	addParam(m_pFader1);
	m_pFader2 = createParamCentered<GPaudioSlider38mm>(mm2px(Vec(CENTER_RIGHT_MM, AUXSLIDER_Y_MM)), pModule, ChainMixerAuxModule::ParamGain2);
	addParam(m_pFader2);

	addParam(createParamCentered<MuteSoloButton>(mm2px(Vec(CENTER_LEFT_MM, AUXSOLO_Y_MM)), pModule, ChainMixerAuxModule::ParamSolo1));
	addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(CENTER_LEFT_MM, AUXSOLO_Y_MM)), pModule, ChainMixerAuxModule::LightSolo1));
	addParam(createParamCentered<MuteSoloButton>(mm2px(Vec(CENTER_LEFT_MM, AUXMUTE_Y_MM)), pModule, ChainMixerAuxModule::ParamMute1));
	addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(CENTER_LEFT_MM, AUXMUTE_Y_MM)), pModule, ChainMixerAuxModule::LightMute1));

	addParam(createParamCentered<MuteSoloButton>(mm2px(Vec(CENTER_RIGHT_MM, AUXSOLO_Y_MM)), pModule, ChainMixerAuxModule::ParamSolo2));
	addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(CENTER_RIGHT_MM, AUXSOLO_Y_MM)), pModule, ChainMixerAuxModule::LightSolo2));
	addParam(createParamCentered<MuteSoloButton>(mm2px(Vec(CENTER_RIGHT_MM, AUXMUTE_Y_MM)), pModule, ChainMixerAuxModule::ParamMute2));
	addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(CENTER_RIGHT_MM, AUXMUTE_Y_MM)), pModule, ChainMixerAuxModule::LightMute2));

	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_LEFT_MM, RETURN_L_Y_MM)), pModule, ChainMixerAuxModule::Return1L));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_LEFT_MM, RETURN_R_Y_MM)), pModule, ChainMixerAuxModule::Return1R));
	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_LEFT_MM, SEND_L_Y_MM)), pModule, ChainMixerAuxModule::Send1L));
	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_LEFT_MM, SEND_R_Y_MM)), pModule, ChainMixerAuxModule::Send1R));

	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_RIGHT_MM, RETURN_L_Y_MM)), pModule, ChainMixerAuxModule::Return2L));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_RIGHT_MM, RETURN_R_Y_MM)), pModule, ChainMixerAuxModule::Return2R));
	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_RIGHT_MM, SEND_L_Y_MM)), pModule, ChainMixerAuxModule::Send2L));
	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_RIGHT_MM, SEND_R_Y_MM)), pModule, ChainMixerAuxModule::Send2R));
}

void ChainMixerAuxWidget::appendContextMenu(Menu* pMainMenu) /*override*/
{
	auto pModule = dynamic_cast<ChainMixerAuxModule*>(getModule());
	if (pModule == nullptr)
		return;
	pMainMenu->addChild(new MenuSeparator);
	ui::MenuItem* pNumberItem = createCheckMenuItem(
		"Mute Aux Sends on Main Mute",
		"",
		[=]() { return pModule->MuteSendsOnMainMute(); },
		[=]() { pModule->MuteSendsOnMainMute(!pModule->MuteSendsOnMainMute()); } );
	pMainMenu->addChild(pNumberItem);
}

void ChainMixerAuxWidget::step() /*override*/
{
	ModuleWidget::step();
	if (m_pFader1 != nullptr)
		m_pFader1->UpdateDarkMode();
	if (m_pFader2 != nullptr)
		m_pFader2->UpdateDarkMode();
}

Model* the_pChainMixerAuxModel = createModel<ChainMixerAuxModule, ChainMixerAuxWidget>("ChainMixerAux");
