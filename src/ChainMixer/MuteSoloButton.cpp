//
// MuteSoloButton class
// ====================
// Buton class that does not modify a parameter directly, instead
// its queues an update  the ChainMixmermodule-dervied module class.class
// The update contains the state of modifier keys as well as the __cpp_nontype_template_parameter_auto
// is and button up/down info
//

#include "plugin.h"
#include "ChainMixer/MuteSoloButton.h"
#include "ChainMixer/ChainMixerModule.h"

MuteSoloEvent::MuteSoloEvent(bool bPress, int nParam, bool bCtrl, bool bShift) :
	bPressed(bPress),
	nParamId(nParam),
	bCtrlKeyDown(bCtrl),
	bShiftKeyDown(bShift)
{
}

void MuteSoloButton::onButton(const ButtonEvent& evBtn) /*override*/
{
	if (evBtn.button != GLFW_MOUSE_BUTTON_LEFT || (evBtn.mods & RACK_MOD_CTRL) == 0)
	{
		base::onButton(evBtn);
		return;
	}
	auto pModule = dynamic_cast<ChainMixerModule*>(module);
	if (pModule != nullptr)
	{
		pModule->QueueMuteSoloEvent(evBtn.action == GLFW_PRESS, paramId, (evBtn.mods & RACK_MOD_CTRL) != 0, (evBtn.mods & GLFW_MOD_SHIFT) != 0);
		evBtn.consume(nullptr);
		return;
	}
	base::onButton(evBtn);
}
