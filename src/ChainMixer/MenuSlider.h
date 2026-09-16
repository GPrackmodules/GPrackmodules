//
// MenuSlider class
// ================
// Slider to be used in the Channel/.ExtChannel context menu to dajust gain trim
// Also includes TrimQuantity
//

#pragma once

class MenuSlider : public ui::Slider
{
public:
	MenuSlider(class ChainMixerModule* pModule, int nParamId);

private:
	class ChainMixerModule* m_pModule = nullptr;
	int m_nParamId = 0;
};
