//
// Knobs classes for GPaudio modules
// =================================
// Collection of SVG knobs:
// * FilledKnow classes have a solid border under the arrow
// * PointyKnobs have no filled border under the arrow
//

#include "Knobs.h"

FilledKnob::FilledKnob(int nMillimeter)
{
	minAngle = -0.83 * M_PI;
	maxAngle = 0.83 * M_PI;

	std::stringstream ssSvg;
	ssSvg << "res/FilledKnob-" << nMillimeter << "mm";
	setSvg(Svg::load(asset::plugin(the_pPluginInstance , ssSvg.str() + ".svg")));
	bg->setSvg(Svg::load(asset::plugin(the_pPluginInstance , ssSvg.str() + "-bg.svg")));
	shadow->opacity = 0.1f;
}

PointyKnob::PointyKnob(int nMillimeter) :
	m_nMillimeter(nMillimeter),
	m_bDarkMode(settings::preferDarkPanels)
{
	minAngle = -0.75 * M_PI;
	maxAngle = 0.75 * M_PI;

	std::stringstream ssSvg;
	ssSvg << "res/PointyKnob-" << nMillimeter << "mm";
	if (m_bDarkMode)
		setSvg(Svg::load(asset::plugin(the_pPluginInstance , ssSvg.str() + "-dark.svg")));
	else
		setSvg(Svg::load(asset::plugin(the_pPluginInstance , ssSvg.str() + ".svg")));
	bg->setSvg(Svg::load(asset::plugin(the_pPluginInstance , ssSvg.str() + "-bg.svg")));
	shadow->box.size = mm2px(static_cast<float>(nMillimeter) - 4.0f);
	// Move shadow downward by 10%
	shadow->box.pos = mm2px(math::Vec(2.0f, 2.0f)).plus(math::Vec(0.0f, sw->box.size.y * 0.10f));
	shadow->opacity = 0.1f;
}

void PointyKnob::step() /*override*/
{
	if (m_bDarkMode != settings::preferDarkPanels)
	{
		std::stringstream ssSvg;
		ssSvg << "res/PointyKnob-" << m_nMillimeter << "mm";
		m_bDarkMode = settings::preferDarkPanels;
		if (m_bDarkMode)
			setSvg(Svg::load(asset::plugin(the_pPluginInstance , ssSvg.str() + "-dark.svg")));
		else
			setSvg(Svg::load(asset::plugin(the_pPluginInstance , ssSvg.str() + ".svg")));
		shadow->box.size = mm2px(static_cast<float>(m_nMillimeter) - 4.0f);
		// Move shadow downward by 10%
		shadow->box.pos = mm2px(math::Vec(2.0f, 2.0f)).plus(math::Vec(0.0f, sw->box.size.y * 0.10f));
		shadow->opacity = 0.1f;
	}
	RoundKnob::step();
}


