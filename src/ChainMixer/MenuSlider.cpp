//
// MenuSlider class
// ================
// Slider to be used in the Channel/.ExtChannel context menu to dajust gain trim
// Also includes TrimQuantity
//

#include "plugin.h"
#include "MenuSlider.h"
#include "ChainMixerModule.h"

MenuSlider::MenuSlider(class ChainMixerModule *pModule, int nParamId) :
	m_pModule(pModule),
	m_nParamId(nParamId)
{
	assert(m_pModule != nullptr);
	quantity = m_pModule->paramQuantities[nParamId];
}
