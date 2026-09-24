#include "plugin.h"
#include "Faders.h"

#include "ChainMixerChannel.h"
#include "ChainMixerCommon.h"
#include "Knobs.h"
#include "MenuSlider.h"

static shared_ptr<Svg> NumberSvg(int nNumber)
{
	static shared_ptr<Svg> s_SvgsLight[MAX_CHAINMIXER_CHANNELS + 1];
	static shared_ptr<Svg> s_SvgsDark[MAX_CHAINMIXER_CHANNELS + 1];
	mutex s_mtxNumberSvgs;

	lock_guard<mutex> lock(s_mtxNumberSvgs);
	if (s_SvgsLight[0] == nullptr)
	{
		for (int i = 0; i < MAX_CHAINMIXER_CHANNELS; i++)
		{
			std::stringstream ssLight;
			ssLight << "res/Number" << i + 1 << ".svg";
			s_SvgsLight[i] = Svg::load(asset::plugin(the_pPluginInstance, ssLight.str()));
			std::stringstream ssDark;
			ssDark << "res/Number" << i + 1 << "-dark.svg";
			s_SvgsDark[i] = Svg::load(asset::plugin(the_pPluginInstance, ssDark.str()));
		}
		s_SvgsLight[MAX_CHAINMIXER_CHANNELS] = Svg::load(asset::plugin(the_pPluginInstance, "res/NoNumber.svg"));
		s_SvgsLight[MAX_CHAINMIXER_CHANNELS] = Svg::load(asset::plugin(the_pPluginInstance, "res/NoNumber-dark.svg"));
	}
	nNumber--;
	if (nNumber < 0 || nNumber >= MAX_CHAINMIXER_CHANNELS || s_SvgsLight[nNumber] == nullptr)
		return settings::preferDarkPanels ? s_SvgsDark[MAX_CHAINMIXER_CHANNELS] : s_SvgsLight[MAX_CHAINMIXER_CHANNELS];
	return settings::preferDarkPanels ? s_SvgsDark[nNumber] : s_SvgsLight[nNumber];
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MODULE
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChainMixerChannelModule::ChainMixerChannelModule() :
	ChainMixerChannelBase(ChainMixerModule::ModuleType::Channel),
	m_fadeMainMute(m_fMainBusFactor, FADE_MS, 1.0f)
{
	config(NumParams, NumInputs, 0, NumLights);
	configParam<SendQuantity>(ParamAux1, 0.f, SEND_STEPS_F, 0.f, "Aux 1 Send Level");
	configParam<SendQuantity>(ParamAux2, 0.f, SEND_STEPS_F, 0.f, "Aux 2 Send Level");
	configParam<PanBalQuantity>(ParamPanBal, MIN_PANBAL_F, MAX_PANBAL_F, 0.f, "Pan/Balance");
	configParam<FaderGainQuantity>(ParamGain, 0.f, FADER_STEPS_F, FADER_ZERO_DB, "Gain");
#ifdef CHAIN_MIXER_SNAPGAIN
	paramQuantities[ParamGain]->snapEnabled = true;
#endif
	configParam(ParamSolo, 0.f, 1.f, 0.f, "Solo");
	configParam(ParamMute, 0.f, 1.f, 0.f, "Mute");
	configParam<TrimQuantity>(ParamTrim, -TRIM_STEPS_F, TRIM_STEPS_F, 0.f, "Input Gain Trim");

	configInput(InputL, "Left");
	configInput(InputR, "Right");

	shared_ptr<Svg> pForceInitializationOfSingleton = NumberSvg(1);
}

void ChainMixerChannelModule::process(const ProcessArgs& args) /*override*/
{
	bool bWasDisabled = Disabled();
	DetermineTypeInstance(the_pChainMixerChannelModel);
	if (!bWasDisabled && Disabled())
	{
		lights[LightSolo].setBrightness(0.0f);
		lights[LightMute].setBrightness(0.0f);
	}
	if (Disabled())
		return;

	if (!m_bInitialized || bWasDisabled)
	{
		m_bInitialized = true;
		HandleMute(ParamMute, true);
		lights[LightMute].setBrightness(Mute() ? MUTE_BRIGHTNESS : DIM_BRIGHTNESS);
		HandleSolo(ParamSolo, true);
		lights[LightSolo].setBrightness(Solo() ? SOLO_BRIGHTNESS : DIM_BRIGHTNESS);
	}

	HandleMuteSoloQueue();
	if (HandleMute(ParamMute))
		lights[LightMute].setBrightness(Mute() ? MUTE_BRIGHTNESS : DIM_BRIGHTNESS);
	if (HandleSolo(ParamSolo))
		lights[LightSolo].setBrightness(Solo() ? SOLO_BRIGHTNESS : DIM_BRIGHTNESS);
}

// Audio processing for all buses
// Called from the main module's process() function

void ChainMixerChannelModule::ProcessAudioBuses(
	const ProcessArgs& args,
	float* pMainL, float* pMainR,
	float* pAux1L, float* pAux1R,
	float* pAux2L, float* pAux2R,
	float fMainFactor,
	bool bMainMute,
	bool bAnyChannelSolo,
	struct AuxInfo rAuxInfo[2])
{
	base::ProcessAudioBuses(args, pMainL, pMainR, pAux1L, pAux1R, pAux2L, pAux2R, fMainFactor, bMainMute, bAnyChannelSolo, rAuxInfo);
	AdvanceFades();

	// if (m_bFadesInitialized)
	// {
	// 	m_bFadesInitialized = true;
	// 	m_fadeMainMono.SetSamplerate(args.sampleRate);
	// 	m_fadeMainStereo.SetSamplerate(args.sampleRate);
	// 	m_fadeAux1.SetSamplerate(args.sampleRate);
	// 	m_fadeAux2.SetSamplerate(args.sampleRate);
	// }
	// if ((bAnyChannelSolo && !Solo()) || Mute())
	// {
	// 	FadeToZero(true);
	// 	return;
	// }
	// bool bHaveMono = false;
	// bool bNeedMono = false;
	// bool bNeedStereo = false;
	//
	// //
	// // Input
	// //
	// float fTrimFactor = TrimQuantity::GainFactor(params[ParamTrim].getValue());
	// float fInL = 0.0f;	// fInL also used for mono, regardless of connected mono channel
	// float fInR = 0.0f;
	// if (inputs[InputL].isConnected())
	// {
	// 	fInL = inputs[InputL].getVoltageSum() * fTrimFactor;
	// 	if (inputs[InputR].isConnected())
	// 		fInR = inputs[InputR].getVoltageSum() * fTrimFactor;
	// 	else
	// 		bHaveMono = true;
	// }
	// else
	// {
	// 	if (inputs[InputR].isConnected())
	// 	{
	// 		bHaveMono = true;
	// 		fInL = inputs[InputR].getVoltageSum() * fTrimFactor;
	// 	}
	// 	else
	// 	{
	// 		FadeToZero(true);
	// 		return;
	// 	}
	// }
	//
	// //
	// // determine required output signals
	// //
	// if (pMainL != nullptr)
	// {
	// 	if (pMainR != nullptr)
	// 		bNeedStereo = true;
	// 	else
	// 		bNeedMono = true;
	// }
	// if (pAux1L != nullptr)
	// {
	// 	if (pAux1R != nullptr)
	// 		bNeedStereo = true;
	// 	else
	// 		bNeedMono = true;
	// }
	// if (pAux2L != nullptr)
	// {
	// 	if (pAux2R != nullptr)
	// 		bNeedStereo = true;
	// 	else
	// 		bNeedMono = true;
	// }
	// if (!bNeedMono && !bNeedStereo)
	// {
	// 	FadeToZero(true);
	// 	return;
	// }
	// float fMainL = 0.0f;
	// float fMainR = 0.0f;
	// float fMainMono = 0.0f;
	// float fFaderFactor = GPaudioFader::GainFactor(params[ParamGain].getValue());
	// if (bHaveMono)
	// {
	// 	// only one input, value in fInL
	// 	if (bNeedMono)
	// 	{
	// 		m_fadeMainMono.Start(fFaderFactor);
	// 		if (!Mute())
	// 			fMainMono = fInL * m_fFaderFactorMono;
	// 	}
	// 	else
	// 		m_fadeMainMono.Start(0.0f);
	// 	if (bNeedStereo)
	// 	{
	// 		float fPanL = PanBalQuantity::GainFactorL(params[ParamPanBal].getValue(), false);
	// 		float fPanR = PanBalQuantity::GainFactorR(params[ParamPanBal].getValue(), false);
	// 		m_fadeMainStereo.Start(fFaderFactor * fPanL, fFaderFactor * fPanR);
	// 		if (!Mute())
	// 		{
	// 			fMainL = fInL * m_fMainFactorsStereo[0];
	// 			fMainR = fInL * m_fMainFactorsStereo[1];
	// 		}
	// 	}
	// 	else
	// 		m_fadeMainStereo.Start(0.0f);
	// }
	// else
	// {
	// 	// stereo input
	// 	m_fadeMainMono.Start(0.0);	// not using this, since mono output is left + right (including balance)
	// 	float fParamPanBal = params[ParamPanBal].getValue();
	// 	float fBalL = PanBalQuantity::GainFactorL(fParamPanBal, true);
	// 	float fBalR = PanBalQuantity::GainFactorR(fParamPanBal, true);
	// 	m_fadeMainStereo.Start(fFaderFactor * fBalL, fFaderFactor * fBalR);
	// 	fMainL = fInL * m_fMainFactorsStereo[0];
	// 	fMainR = fInR * m_fMainFactorsStereo[1];
	// 	fMainMono = (fMainL + fMainR) * g_fMinus3dB;
	// }
	// if (!rAuxInfo[0].bSolo && !rAuxInfo[1].bSolo)
	// {
	// 	if (pMainL != nullptr)
	// 	{
	// 		if (pMainR != nullptr)
	// 		{
	// 			*pMainL += fMainL;
	// 			*pMainR += fMainR;
	// 		}
	// 		else
	// 			*pMainL += fMainMono;
	// 	}
	// }
	//
	// //
	// // Aux sends
	// //
	// if (pAux1L != nullptr)
	// {
	// 	float fParam = params[ParamAux1].getValue();
	// 	float fFactor = SendQuantity::GainFactor(fParam);
	// 	m_fadeAux1.Start(fFactor);
	// 	if (pAux1R != nullptr)
	// 	{
	// 		*pAux1L += fMainL * fFactor;
	// 		*pAux1R += fMainR * fFactor;
	// 	}
	// 	else
	// 		*pAux1L += fMainMono * fFactor;
	// }
	// else
	// 	m_fadeAux1.Start(0.0f);
	//
	// if (pAux2L != nullptr)
	// {
	// 	float fFactor = SendQuantity::GainFactor(params[ParamAux2].getValue());
	// 	m_fadeAux2.Start(fFactor);
	// 	if (pAux2R != nullptr)
	// 	{
	// 		*pAux2L += fMainL * fFactor;
	// 		*pAux2R += fMainR * fFactor;
	// 	}
	// 	else
	// 		*pAux2L += fMainMono * fFactor;
	// }
	// else
	// 	m_fadeAux2.Start(0.0f);
	//
	// m_fadeMainMono.Advance();
	// m_fadeMainStereo.Advance();
	// m_fadeAux1.Advance();
	// m_fadeAux2.Advance();
	//
	// // Not in case class !!!!!
	// m_fadeMainMute.Advance();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// WIDGET
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChainMixerChannelWidget::ChainMixerChannelWidget(ChainMixerChannelModule* pModule) :
	m_bDarkMode(settings::preferDarkPanels)
{
	setModule(pModule);
	setPanel(createPanel(asset::plugin(the_pPluginInstance, "res/ChainMixerChannel.svg"), asset::plugin(the_pPluginInstance, "res/ChainMixerChannel-dark.svg")));

	//addChild(createWidget<ScrewSilver>(Vec(0, 0)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	//addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	math::Vec vecSvg(CENTER_2U_MM - 5.0f, 5.0f);
	m_pNumberWidget = createWidget<SvgWidget>(mm2px(vecSvg));
	m_pNumberWidget->setSvg(NumberSvg(m_nTypeInstance));
	addChild(m_pNumberWidget);

	addParam(createParamCentered<PointyKnob10mm>(mm2px(Vec(CENTER_2U_MM, KNOB_AUX1_Y_MM)), pModule, ChainMixerChannelModule::ParamAux1));
	addParam(createParamCentered<PointyKnob10mm>(mm2px(Vec(CENTER_2U_MM, KNOB_AUX2_Y_MM)), pModule, ChainMixerChannelModule::ParamAux2));
	addParam(createParamCentered<PointyKnob10mm>(mm2px(Vec(CENTER_2U_MM, KNOB_PANBAL_Y_MM)), pModule, ChainMixerChannelModule::ParamPanBal));

	m_pFader = createParamCentered<GPaudioSlider44mm>(mm2px(Vec(CENTER_2U_MM, SLIDER_Y_MM)), pModule, ChainMixerChannelModule::ParamGain);
	addParam(m_pFader);

	addParam(createParamCentered<MuteSoloButton>(mm2px(Vec(CENTER_2U_MM, SOLO_Y_MM)), pModule, ChainMixerChannelModule::ParamSolo));
	addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(CENTER_2U_MM, SOLO_Y_MM)), pModule, ChainMixerChannelModule::LightSolo));
	addParam(createParamCentered<MuteSoloButton>(mm2px(Vec(CENTER_2U_MM, MUTE_Y_MM)), pModule, ChainMixerChannelModule::ParamMute));
	addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(CENTER_2U_MM, MUTE_Y_MM)), pModule, ChainMixerChannelModule::LightMute));

	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_2U_MM, SOCKET_L_Y_MM)), pModule, ChainMixerChannelModule::InputL));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_2U_MM, SOCKET_R_Y_MM)), pModule, ChainMixerChannelModule::InputR));
}

void ChainMixerChannelWidget::appendContextMenu(Menu* pMainMenu) /*override*/
{
	if (module == nullptr)
		return;
	auto pModule = dynamic_cast<ChainMixerModule*>(getModule());
	if (pModule == nullptr)
		return;
	pMainMenu->addChild(new MenuSeparator);
	auto pSlider = new MenuSlider(pModule, ChainMixerChannelModule::ParamTrim);
	pSlider->box.size.x = 200;
	pMainMenu->addChild(pSlider);
}

void ChainMixerChannelWidget::step() /*override*/
{
	ModuleWidget::step();
	if (m_pFader != nullptr)
		m_pFader->UpdateDarkMode();

	bool bDarkModeChanged = false;
	if (settings::preferDarkPanels != m_bDarkMode)
	{
		m_bDarkMode = settings::preferDarkPanels;
		bDarkModeChanged = true;
	}

	bool bTypeInstanceChanged = false;
	auto pMixerModule = dynamic_cast<ChainMixerChannelModule*>(module);
	if (pMixerModule != nullptr)
	{
		int nTypeInstance = pMixerModule->TypeInstance();
		if (nTypeInstance != m_nTypeInstance)
		{
			bTypeInstanceChanged = true;
			m_nTypeInstance = nTypeInstance;
		}
	}
	if (bDarkModeChanged || bTypeInstanceChanged)
		m_pNumberWidget->setSvg(NumberSvg(m_nTypeInstance));
}

Model* the_pChainMixerChannelModel = createModel<ChainMixerChannelModule, ChainMixerChannelWidget>("ChainMixerChannel");
