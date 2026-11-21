#include "plugin.h"

#include "ChainMixerSeparator.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MODULE
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChainMixerSeparatorModule::ChainMixerSeparatorModule() :
	ChainMixerModule(ChainMixerModule::ModuleType::Separator)
{
	config(0, 0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// WIDGET
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChainMixerSeparatorWidget::ChainMixerSeparatorWidget(Module* pModule)
{
	setModule(pModule);
	setPanel(createPanel(asset::plugin(the_pPluginInstance, "res/ChainMixerSeparator.svg"), asset::plugin(the_pPluginInstance, "res/ChainMixerSeparator-dark.svg")));

	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
}

Model* the_pChainMixerSeparatorModel = createModel<ChainMixerSeparatorModule, ChainMixerSeparatorWidget>("ChainMixerSeparator");
