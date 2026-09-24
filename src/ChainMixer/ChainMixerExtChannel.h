#pragma once

#include "ChainMixerCommon.h"
#include "ChainMixerChannelBase.h"

#define MAX_CHAINMIXER_ExtChannelS		16

/////////////////////////////////////////////////////
/// Module
/////////////////////////////////////////////////////

class ChainMixerExtChannelModule : public ChainMixerChannelBase
{
	using base=ChainMixerChannelBase;

public:
	enum ParamId
	{
		ParamAux1,
		ParamAux2,
		ParamAux1Pre,
		ParamAux2Pre,
		ParamPanBal,
		ParamTrim,
		ParamGain,
		ParamMuteMain,
		ParamSolo,
		ParamMute,
		ParamApplyMainFader,
		NumParams
	};
	enum InputId
	{
		InputL,
		InputR,
		InputCVSolo,
		InputCVMute,
		InputCVGain,
		InputCVPanBal,
		NumInputs
	};
	enum OutputId
	{
		OutputDirectL,
		OutputDirectR,
		NumOutputs
	};
	enum LightId
	{
		LightAux1Pre,
		LightAux2Pre,
		LightMuteMain,
		LightSolo,
		LightMute,
		LightApplyMainFader,
		NumLights
	};

public:
	ChainMixerExtChannelModule();

public:
	//void SetSampleRate(float fSamplerate);
	void process(const ProcessArgs& args) override;
	void SetWidget(struct ChainMixerExtChannelWidget* pWidget) { m_pWidget = pWidget; }
	bool Disabled() const override { return TypeInstance() > MAX_CHAINMIXER_ExtChannelS; }
	void ProcessAudioBuses(	// called from main module's process() function
		const ProcessArgs& args,
		float* pMainL, float* pMainR,
		float* pAux1L, float* pAux1R,
		float* pAux2L, float* pAux2R,
		float fMainFactor,
		bool bMainMute,
		bool bAnyChannelSolo,
		struct AuxInfo rAuxInfo[2]) override;

	bool Solo() override;
	bool Mute() override;

protected:
	bool HandleMuteMain(bool bForce = false);	// true if changed
	int SoloParam() const override { return (int)ParamId::ParamSolo; }
	int MuteParam() const override { return (int)ParamId::ParamMute; }
	int TrimParam() const override { return (int)ParamId::ParamTrim; }
	int GainParam() const override { return (int)ParamId::ParamGain; }
	int PanBalParam() const override { return (int)ParamId::ParamPanBal; }
	int Aux1Param() const override { return (int)ParamId::ParamAux1; }
	int Aux2Param() const override { return (int)ParamId::ParamAux2; }
	int InputLId() const override { return InputL; }
	int InputRId() const override { return InputR; }
	float CVGainFactor() override;
	void ApplyPanBalCV(float & rParamValue) override;
	void AddMonoStereoRequirements(bool& rNeedMono, bool& rNeedStereo) override;

private:
	struct ChainMixerExtChannelWidget* m_pWidget = nullptr;
	bool m_bInitialized = false;

	// Extended
	bool m_bApplyMainFader = false;
};

/////////////////////////////////////////////////////
/// Widget
/////////////////////////////////////////////////////

struct ChainMixerExtChannelWidget : ModuleWidget
{
public:
	ChainMixerExtChannelWidget(ChainMixerExtChannelModule* pModule);

	void appendContextMenu(Menu* menu) override;
	void step() override;

private:
	class GPaudioFader* m_pFader = nullptr;
	int m_nTypeInstance = -1;
	bool m_bDarkMode;
	SvgWidget* m_pNumberWidget;
};

extern Model* the_pChainMixerExtChannelModel;
