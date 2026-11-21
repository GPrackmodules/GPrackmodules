#include "plugin.h"

#include "AB/AB4.h"
#include "AB/AB8.h"
#include "ChainMixer/ChainMixerChannel.h"
#include "ChainMixer/ChainMixerExtChannel.h"
#include "ChainMixer/ChainMixerMain.h"
#include "ChainMixer/ChainMixerAux.h"
#include "ChainMixer/ChainMixerSeparator.h"
#include "StereoChorus/StereoChorus.h"
#include "Rotary/Rotary.h"
#include "Clamp/Clamp.h"
#include "MultiControl/MultiControl.h" // contains Dual- and TripleControl models, template class
#include "PhraseSelect/PhraseSelect.h"

Plugin* the_pPluginInstance;

shared_ptr<Svg> NumberSvg(int nNumber)
{
	static shared_ptr<Svg> s_SvgsLight[NUMBER_SVG_NUMBERS + 1];
	static shared_ptr<Svg> s_SvgsDark[NUMBER_SVG_NUMBERS + 1];
	mutex s_mtxNumberSvgs;

	lock_guard<mutex> lock(s_mtxNumberSvgs);
	if (s_SvgsLight[0] == nullptr)
	{
		for (int i = 0; i < NUMBER_SVG_NUMBERS; i++)
		{
			std::stringstream ssLight;
			ssLight << "res/Number" << i + 1 << ".svg";
			s_SvgsLight[i] = Svg::load(asset::plugin(the_pPluginInstance, ssLight.str()));
			std::stringstream ssDark;
			ssDark << "res/Number" << i + 1 << "-dark.svg";
			s_SvgsDark[i] = Svg::load(asset::plugin(the_pPluginInstance, ssDark.str()));
		}
		s_SvgsLight[NUMBER_SVG_NUMBERS] = Svg::load(asset::plugin(the_pPluginInstance, "res/NoNumber.svg"));
		s_SvgsLight[NUMBER_SVG_NUMBERS] = Svg::load(asset::plugin(the_pPluginInstance, "res/NoNumber-dark.svg"));
	}
	nNumber--;
	if (nNumber < 0 || nNumber >= NUMBER_SVG_NUMBERS || s_SvgsLight[nNumber] == nullptr)
		return settings::preferDarkPanels ? s_SvgsDark[NUMBER_SVG_NUMBERS] : s_SvgsLight[NUMBER_SVG_NUMBERS];
	return settings::preferDarkPanels ? s_SvgsDark[nNumber] : s_SvgsLight[nNumber];
}

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

void init(Plugin* plugin)
{
	the_pPluginInstance = plugin;

	auto pInitSingleton = NumberSvg(1);

	// Add modules here
	plugin->addModel(the_pAB4Model);
	plugin->addModel(the_pAB8Model);
	plugin->addModel(the_pChainMixerChannelModel);
	plugin->addModel(the_pChainMixerExtChannelModel);
	plugin->addModel(the_pChainMixerMainModel);
	plugin->addModel(the_pChainMixerAuxModel);
	plugin->addModel(the_pChainMixerSeparatorModel);
	plugin->addModel(the_pStereoChorusModel);
	plugin->addModel(the_pRotaryModel);
	plugin->addModel(the_pClampModel);
	plugin->addModel(the_pDualControlModel);
	plugin->addModel(the_pTripleControlModel);
	plugin->addModel(the_pPhraseSelectModel);
}
