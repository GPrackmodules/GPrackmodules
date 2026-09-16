#include "plugin.h"

#include "AB/AB4.h"
#include "AB/AB8.h"
#include "ChainMixer/ChainMixerChannel.h"
#include "ChainMixer/ChainMixerExtChannel.h"
#include "ChainMixer/ChainMixerMain.h"
#include "ChainMixer/ChainMixerAux.h"
#include "StereoChorus/StereoChorus.h"
#include "Rotary/Rotary.h"
#include "Clamp/Clamp.h"
#include "MultiControl/MultiControl.h" // contains Dual- and TripleControl models

Plugin* the_pPluginInstance;

int iround(float f)
{
	return static_cast<int>(lround(f));
}

int iround(double d)
{
	return static_cast<int>(lround(d));
}

int64_t i64round(float f)
{
	return static_cast<int64_t>(round(f));
}

int64_t i64round(double d)
{
	return static_cast<int64_t>(round(d));
}

bool StrToFloat(const std::string& s, float& f, const char** ppParsedUntil /*= nullptr*/)
{
	return StrToFloat(s.c_str(), f, ppParsedUntil);
}

bool StrToFloat(const char* psz, float& f, const char** ppParsedUntil /*= nullptr*/)
{
	char *pEnd = nullptr;
	f = strtof(psz, &pEnd);
	if (ppParsedUntil != nullptr)
		*ppParsedUntil = pEnd;
	return pEnd != psz && errno != ERANGE;
}

void init(Plugin* p)
{
	the_pPluginInstance = p;

	// Add modules here
	p->addModel(the_pAB4Model);
	p->addModel(the_pAB8Model);
	p->addModel(the_pChainMixerChannelModel);
	p->addModel(the_pChainMixerExtChannelModel);
	p->addModel(the_pChainMixerMainModel);
	p->addModel(the_pChainMixerAuxModel);
	p->addModel(the_pStereoChorusModel);
	p->addModel(the_pRotaryModel);
	p->addModel(the_pClampModel);
	p->addModel(the_pDualControlModel);
	p->addModel(the_pTripleControlModel);
}
