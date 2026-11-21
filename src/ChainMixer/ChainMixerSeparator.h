#pragma once

#include "ChainMixerModule.h"

#define MAX_CHAINMIXER_CHANNELS		NUMBER_SVG_NUMBERS

/////////////////////////////////////////////////////
/// Module
/////////////////////////////////////////////////////

class ChainMixerSeparatorModule : public ChainMixerModule
{
public:
	ChainMixerSeparatorModule();

public:
	bool Disabled() const { return false; }

protected:

private:
};

/////////////////////////////////////////////////////
/// Widget
/////////////////////////////////////////////////////

struct ChainMixerSeparatorWidget : ModuleWidget
{
public:
	explicit ChainMixerSeparatorWidget(Module* pModule);

private:
};

extern Model* the_pChainMixerSeparatorModel;
