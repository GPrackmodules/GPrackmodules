#include "plugin.h"
#include "Faders.h"

#include "ChainMixerExtChannel.h"
#include "Knobs.h"
#include "MenuSlider.h"


static shared_ptr<Svg> NumberSvg(int nNumber)
{
	static shared_ptr<Svg> s_SvgsLight[MAX_CHAINMIXER_ExtChannelS + 1];
	static shared_ptr<Svg> s_SvgsDark[MAX_CHAINMIXER_ExtChannelS + 1];
	mutex s_mtxNumberSvgs;

	lock_guard<mutex> lock(s_mtxNumberSvgs);
	if (s_SvgsLight[0] == nullptr)
	{
		for (int i = 0; i < MAX_CHAINMIXER_ExtChannelS; i++)
		{
			std::stringstream ssLight;
			ssLight << "res/Number" << i + 1 << ".svg";
			s_SvgsLight[i] = Svg::load(asset::plugin(the_pPluginInstance, ssLight.str()));
			std::stringstream ssDark;
			ssDark << "res/Number" << i + 1 << "-dark.svg";
			s_SvgsDark[i] = Svg::load(asset::plugin(the_pPluginInstance, ssDark.str()));
		}
		s_SvgsLight[MAX_CHAINMIXER_ExtChannelS] = Svg::load(asset::plugin(the_pPluginInstance, "res/NoNumber.svg"));
		s_SvgsLight[MAX_CHAINMIXER_ExtChannelS] = Svg::load(asset::plugin(the_pPluginInstance, "res/NoNumber-dark.svg"));
	}
	nNumber--;
	if (nNumber < 0 || nNumber >= MAX_CHAINMIXER_ExtChannelS || s_SvgsLight[nNumber] == nullptr)
		return settings::preferDarkPanels ? s_SvgsDark[MAX_CHAINMIXER_ExtChannelS] : s_SvgsLight[MAX_CHAINMIXER_ExtChannelS];
	return settings::preferDarkPanels ? s_SvgsDark[nNumber] : s_SvgsLight[nNumber];
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MODULE
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChainMixerExtChannelModule::ChainMixerExtChannelModule() :
	ChainMixerChannelBase(ChainMixerModule::ModuleType::ExtChannel)
{
	config(NumParams, NumInputs, NumOutputs, NumLights);

	//
	// "Regular" channel
	//
	configParam<SendQuantity>(ParamAux1, 0.f, SEND_STEPS_F, 0.f, "Aux 1 Send Level");
	configParam<SendQuantity>(ParamAux2, 0.f, SEND_STEPS_F, 0.f, "Aux 2 Send Level");
	configParam<PanBalQuantity>(ParamPanBal, MIN_PANBAL_F, MAX_PANBAL_F, 0.f, "Pan/Balance");
	configParam<FaderGainQuantity>(ParamGain, 0.f, FADER_STEPS_F, FADER_ZERO_DB, "Gain");
#ifdef CHAIN_MIXER_SNAPGAIN
	paramQuantities[ParamGain]->snapEnabled = true;
#endif
	configParam(ParamSolo, 0.f, 1.f, 0.f, "Solo");
	configParam(ParamMute, 0.f, 1.f, 0.f, "Mute");
	configInput(InputL, "Left");
	configInput(InputR, "Right");

	//
	// Extended
	//
	configParam(ParamAux1Pre, 0.f, 1.f, 0.f, "AUx 1 Send Prefader");
	configParam(ParamAux2Pre, 0.f, 1.f, 0.f, "AUx 2 Send Prefader");
	configParam<TrimQuantity>(ParamTrim, -TRIM_STEPS_F, TRIM_STEPS_F, 0.f, "Input Gain Trim");
	configParam(ParamMuteMain, 0.f, 1.f, 0.f, "Mute Channel on Main Output");
	configParam(ParamApplyMainFader, 0.f, 1.f, 0.f, "Apply Main Fader and Mute to Direct Out");

	configInput(InputCVGain, "Gain Factor CV");
	configInput(InputCVSolo, "Solo CV");
	configInput(InputCVMute, "Mute CV");
	configInput(InputCVPanBal, "Pan/Balance CV");

	configOutput(OutputDirectL, "Left direct");
	configOutput(OutputDirectR, "Right direct");

	shared_ptr<Svg> pForceInitializationOfSingleton = NumberSvg(1);
}

void ChainMixerExtChannelModule::process(const ProcessArgs& args) /*override*/
{
	bool bWasDisabled = Disabled();
	DetermineTypeInstance(the_pChainMixerExtChannelModel); // can change Disabled()
	if (!bWasDisabled && Disabled())
	{
		lights[LightAux1Pre].setBrightness(0.0f);
		lights[LightAux2Pre].setBrightness(0.0f);
		lights[LightSolo].setBrightness(0.0f);
		lights[LightMute].setBrightness(0.0f);
		lights[LightAux1Pre].setBrightness(0.0f);
		lights[LightAux2Pre].setBrightness(0.0f);
		lights[LightMuteMain].setBrightness(0.0f);
		lights[LightApplyMainFader].setBrightness(0.0f);
	}
	if (Disabled())
		return;

	if (!m_bInitialized || bWasDisabled)
	{
		m_bInitialized = true;
		HandleMute(ParamMute, true);
		lights[LightMute].setBrightness(base::Mute() ? MUTE_BRIGHTNESS : DIM_BRIGHTNESS);
		HandleSolo(ParamSolo, true);
		lights[LightSolo].setBrightness(base::Solo() ? SOLO_BRIGHTNESS : DIM_BRIGHTNESS);
		HandleBoolParam(m_bAux1Pre, ParamAux1Pre, true);
		lights[LightAux1Pre].setBrightness(m_bAux1Pre ? AUXPRE_BRIGHTNESS : DIM_BRIGHTNESS);
		HandleBoolParam(m_bAux2Pre, ParamAux2Pre, true);
		lights[LightAux2Pre].setBrightness(m_bAux2Pre ? AUXPRE_BRIGHTNESS : DIM_BRIGHTNESS);
		HandleMuteMain(true);
		lights[LightMuteMain].setBrightness(m_bMuteMain ? MUTE_BRIGHTNESS : DIM_BRIGHTNESS);
		HandleBoolParam(m_bApplyMainFader, ParamApplyMainFader, true);
		lights[LightApplyMainFader].setBrightness(m_bApplyMainFader ? MAINFADER_BRIGHTNESS : DIM_BRIGHTNESS);
	}

	HandleMuteSoloQueue();
	if (HandleMute(ParamMute))
		lights[LightMute].setBrightness(base::Mute() ? MUTE_BRIGHTNESS : DIM_BRIGHTNESS);
	if (HandleSolo(ParamSolo))
		lights[LightSolo].setBrightness(base::Solo() ? SOLO_BRIGHTNESS : DIM_BRIGHTNESS);
	if (HandleBoolParam(m_bAux1Pre, ParamAux1Pre, false))
		lights[LightAux1Pre].setBrightness(m_bAux1Pre ? AUXPRE_BRIGHTNESS : DIM_BRIGHTNESS);
	if (HandleBoolParam(m_bAux2Pre, ParamAux2Pre, false))
		lights[LightAux2Pre].setBrightness(m_bAux2Pre ? AUXPRE_BRIGHTNESS : DIM_BRIGHTNESS);
	if (HandleMuteMain(false))
		lights[LightMuteMain].setBrightness(m_bMuteMain ? MUTE_BRIGHTNESS : DIM_BRIGHTNESS);
	if (HandleBoolParam(m_bApplyMainFader, ParamApplyMainFader, false))
		lights[LightApplyMainFader].setBrightness(m_bApplyMainFader ? MAINFADER_BRIGHTNESS : DIM_BRIGHTNESS);
}

// Audio processing for all busses
// Called from the main module's process() function

void ChainMixerExtChannelModule::ProcessAudioBuses(
	const ProcessArgs& args,
	float* pMainL, float* pMainR,
	float* pAux1L, float* pAux1R,
	float* pAux2L, float* pAux2R,
	float fMainFactor,
	bool bMainMute,
	bool bAnyChannelSolo,
	struct AuxInfo rAuxInfo[2])
{
	// do main and aux bus mixing in base class
	base::ProcessAudioBuses(args, pMainL, pMainR, pAux1L, pAux1R, pAux2L, pAux2R, fMainFactor, bMainMute, bAnyChannelSolo, rAuxInfo);

	//
	// Direct outs
	//
	if (outputs[OutputDirectL].isConnected())
	{
		if (outputs[OutputDirectR].isConnected())
		{
			if (m_bApplyMainFader)
			{
				outputs[OutputDirectL].setVoltage(m_fPostL * fMainFactor);
				outputs[OutputDirectR].setVoltage(m_fPostR * fMainFactor);
			}
			else
			{
				outputs[OutputDirectL].setVoltage(m_fPostL);
				outputs[OutputDirectR].setVoltage(m_fPostR);
			}
		}
		else
		{
			if (m_bApplyMainFader)
				outputs[OutputDirectL].setVoltage(m_fPostMono * fMainFactor);
			else
				outputs[OutputDirectL].setVoltage(m_fPostMono);
		}
	}
	else
	{
		if (outputs[OutputDirectR].isConnected())
		{
			if (m_bApplyMainFader)
				outputs[OutputDirectR].setVoltage(m_fPostMono * fMainFactor);
			else
				outputs[OutputDirectR].setVoltage(m_fPostMono);
		}
	}

	AdvanceFades();
}

bool ChainMixerExtChannelModule::HandleMuteMain(bool bForce /*= false*/)
{
	bool bMuteMain = params[ParamMuteMain].getValue() > 0.5f;
	bool bChanged = bMuteMain != m_bMuteMain;
	if (bChanged || bForce)
	{
		m_bMuteMain = bMuteMain;
		if (bMuteMain)
			m_fadeMainBus.Start(0.0f);
		else
			m_fadeMainBus.Start(1.0f);
	}
	return bChanged;
}

float ChainMixerExtChannelModule::CVGainFactor()
{
	if (inputs[InputCVGain].isConnected())
		return clamp(inputs[InputCVGain].getVoltage() / 10.0f, 0.0f, 1.0f);
	return 1.0f;
}

void ChainMixerExtChannelModule::ApplyPanBalCV(float &rParamValue) // *override
{
	// CV: +/- 5V = left/right, but allow larger values, when knob is off center
	rParamValue += clamp(inputs[InputCVPanBal].getVoltage(), -10.0f, 10.0f) * MAX_PANBAL_F / 5.0f;
	rParamValue = clamp(rParamValue, MIN_PANBAL_F, MAX_PANBAL_F);
}

void ChainMixerExtChannelModule::AddMonoStereoRequirements(bool& rNeedMono, bool& rNeedStereo)  // override
{
	if (outputs[OutputDirectL].isConnected())
	{
		if (outputs[OutputDirectR].isConnected())
			rNeedStereo = true;
		else
			rNeedMono = true;
	}
	else
		if (outputs[OutputDirectR].isConnected())
			rNeedMono = true;
}

bool ChainMixerExtChannelModule::Solo() /*override*/
{
	return ChainMixerModule::Solo() || (inputs[InputCVSolo].getVoltage() > 1.0f);
}

bool ChainMixerExtChannelModule::Mute() /*override*/
{
	return ChainMixerModule::Mute() || (inputs[InputCVMute].getVoltage() > 1.0f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// WIDGET
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GAINCV_Y_MM					(KNOB_TOP_MM + 3 * KNOB_STEP_MM + 0.5f)
#define MUTEMAIN_Y_MM				(SOLO_Y_MM - SWITCH_STEP_MM)
#define APPLYMAINFADER_Y_MM			(MUTEMAIN_Y_MM - 9.5f)

ChainMixerExtChannelWidget::ChainMixerExtChannelWidget(ChainMixerExtChannelModule* pModule) :
	m_bDarkMode(settings::preferDarkPanels)
{
	if (pModule != nullptr)
		pModule->SetWidget(this);

	setModule(pModule);
	setPanel(createPanel(asset::plugin(the_pPluginInstance, "res/ChainMixerExtChannel.svg"), asset::plugin(the_pPluginInstance, "res/ChainMixerExtChannel-dark.svg")));

	//addChild(createWidget<ScrewSilver>(Vec(0, 0)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	//addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	math::Vec vecSvg(CENTER_4U_MM - 5.0f, 5.0f);
	m_pNumberWidget = createWidget<SvgWidget>(mm2px(vecSvg));
	m_pNumberWidget->setSvg(NumberSvg(m_nTypeInstance));
	addChild(m_pNumberWidget);

	//
	// "Regular" channel
	//
	addParam(createParamCentered<PointyKnob10mm>(mm2px(Vec(CENTER_LEFT_MM, KNOB_AUX1_Y_MM)), pModule, ChainMixerExtChannelModule::ParamAux1));
	addParam(createParamCentered<PointyKnob10mm>(mm2px(Vec(CENTER_LEFT_MM, KNOB_AUX2_Y_MM)), pModule, ChainMixerExtChannelModule::ParamAux2));

	addParam(createParamCentered<PointyKnob10mm>(mm2px(Vec(CENTER_LEFT_MM, KNOB_PANBAL_Y_MM)), pModule, ChainMixerExtChannelModule::ParamPanBal));

	m_pFader = createParamCentered<GPaudioSlider44mm>(mm2px(Vec(CENTER_LEFT_MM, SLIDER_Y_MM)), pModule, ChainMixerExtChannelModule::ParamGain);
	addParam(m_pFader);

	addParam(createParamCentered<MuteSoloButton>(mm2px(Vec(CENTER_LEFT_MM, SOLO_Y_MM)), pModule, ChainMixerExtChannelModule::ParamSolo));
	addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(CENTER_LEFT_MM, SOLO_Y_MM)), pModule, ChainMixerExtChannelModule::LightSolo));
	addParam(createParamCentered<MuteSoloButton>(mm2px(Vec(CENTER_LEFT_MM, MUTE_Y_MM)), pModule, ChainMixerExtChannelModule::ParamMute));
	addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(CENTER_LEFT_MM, MUTE_Y_MM)), pModule, ChainMixerExtChannelModule::LightMute));

	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_LEFT_MM, SOCKET_L_Y_MM)), pModule, ChainMixerExtChannelModule::InputL));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_LEFT_MM, SOCKET_R_Y_MM)), pModule, ChainMixerExtChannelModule::InputR));

	//
	// Extended
	//
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_RIGHT_MM, GAINCV_Y_MM)), pModule, ChainMixerExtChannelModule::InputCVGain));

	addParam(createParamCentered<VCVLatch>(mm2px(Vec(CENTER_RIGHT_MM, KNOB_AUX1_Y_MM)), pModule, ChainMixerExtChannelModule::ParamAux1Pre));
	addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(CENTER_RIGHT_MM, KNOB_AUX1_Y_MM)), pModule, ChainMixerExtChannelModule::LightAux1Pre));
	addParam(createParamCentered<VCVLatch>(mm2px(Vec(CENTER_RIGHT_MM, KNOB_AUX2_Y_MM)), pModule, ChainMixerExtChannelModule::ParamAux2Pre));
	addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(CENTER_RIGHT_MM, KNOB_AUX2_Y_MM)), pModule, ChainMixerExtChannelModule::LightAux2Pre));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_RIGHT_MM, KNOB_PANBAL_Y_MM)), pModule, ChainMixerExtChannelModule::InputCVPanBal));

	addParam(createParamCentered<VCVLatch>(mm2px(Vec(CENTER_RIGHT_MM, APPLYMAINFADER_Y_MM)), pModule, ChainMixerExtChannelModule::ParamApplyMainFader));
	addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(CENTER_RIGHT_MM, APPLYMAINFADER_Y_MM)), pModule, ChainMixerExtChannelModule::LightApplyMainFader));
	addParam(createParamCentered<VCVLatch>(mm2px(Vec(CENTER_RIGHT_MM, MUTEMAIN_Y_MM)), pModule, ChainMixerExtChannelModule::ParamMuteMain));
	addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(CENTER_RIGHT_MM, MUTEMAIN_Y_MM)), pModule, ChainMixerExtChannelModule::LightMuteMain));

	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_RIGHT_MM, SOLO_Y_MM)), pModule, ChainMixerExtChannelModule::InputCVSolo));
	addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_RIGHT_MM, MUTE_Y_MM)), pModule, ChainMixerExtChannelModule::InputCVMute));

	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_RIGHT_MM, SOCKET_L_Y_MM)), pModule, ChainMixerExtChannelModule::OutputDirectL));
	addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(CENTER_RIGHT_MM, SOCKET_R_Y_MM)), pModule, ChainMixerExtChannelModule::OutputDirectR));
}

void ChainMixerExtChannelWidget::appendContextMenu(Menu* pMainMenu) /*override*/
{
	if (module == nullptr)
		return;
	auto pModule = dynamic_cast<ChainMixerModule*>(getModule());
	if (pModule == nullptr)
		return;
	pMainMenu->addChild(new MenuSeparator);
	auto pSlider = new MenuSlider(pModule, ChainMixerExtChannelModule::ParamTrim);
	pSlider->box.size.x = 200;
	pMainMenu->addChild(pSlider);
}

void ChainMixerExtChannelWidget::step() /*override*/
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
	auto pMixerModule = dynamic_cast<ChainMixerExtChannelModule*>(module);
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

Model* the_pChainMixerExtChannelModel = createModel<ChainMixerExtChannelModule, ChainMixerExtChannelWidget>("ChainMixerExtChannel");
