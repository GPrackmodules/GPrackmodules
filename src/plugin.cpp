#include "plugin.hpp"

#include "AB/AB4.h"
#include "AB/AB8.h"
#include "ChainMixer/ChainMixerChannel.h"
#include "ChainMixer/ChainMixerMaster.h"
#include "ChainMixer/ChainMixerAux.h"
#include "StereoChorus/StereoChorus.h"
#include "Rotary/Rotary.h"

Plugin* the_pPluginInstance;

void init(Plugin* p)
{
	the_pPluginInstance = p;

	// Add modules here
	p->addModel(the_pAB4Model);
	p->addModel(the_pAB8Model);
	p->addModel(the_pChainMixerChannelModel);
	p->addModel(the_pChainMixerMasterModel);
	p->addModel(the_pChainMixerAuxModel);
	p->addModel(the_pStereoChorusModel);
	p->addModel(the_pRotaryModel);

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}


