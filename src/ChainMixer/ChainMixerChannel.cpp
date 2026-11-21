#include "plugin.h"
#include "Faders.h"

#include "ChainMixerChannel.h"
#include "ChainMixerCommon.h"
#include "Knobs.h"
#include "MenuSlider.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MODULE
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChainMixerChannelModule::ChainMixerChannelModule() :
	ChainMixerChannelBase(ChainMixerModule::ModuleType::Channel)
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
