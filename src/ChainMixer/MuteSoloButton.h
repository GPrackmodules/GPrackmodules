//
// MuteSoloButton class
// ====================
// Buton class that does not modify a parameter directly, instead
// its queues an update  the ChainMixmermodule-dervied module class.class
// The update contains the state of modifier keys as well as the __cpp_nontype_template_parameter_auto
// is and button up/down info
//

#pragma once

struct MuteSoloEvent
{
	MuteSoloEvent() = default;
	MuteSoloEvent(bool bPress, int nParam, bool bCtrl, bool bShift);

	bool bPressed = false;
	int nParamId = 0;
	bool bCtrlKeyDown = false;
	bool bShiftKeyDown = false;
};

class MuteSoloButton : public VCVLatch
{
	using base = VCVLatch;
public:
	MuteSoloButton() = default;
	void onButton(const ButtonEvent& evBtn) override;
};

