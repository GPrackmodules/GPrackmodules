#pragma once

#include "ChainMixerChannelBase.h"

#define MAX_CHAINMIXER_CHANNELS		16

/////////////////////////////////////////////////////
/// Module
/////////////////////////////////////////////////////

class ChainMixerChannelModule : public ChainMixerChannelBase
{
	using base=ChainMixerChannelBase;

public:
	enum ParamId
	{
		ParamAux1,
		ParamAux2,
		ParamPanBal,
		ParamGain,
		ParamSolo,
		ParamMute,
		ParamTrim,
		NumParams
	};
	enum InputId
	{
		InputL,
		InputR,
		NumInputs
	};
	enum LightId
	{
		LightSolo,
		LightMute,
		NumLights
	};

public:
	ChainMixerChannelModule();

public:
	//void SetSampleRate(float fSamplerate);
	void process(const ProcessArgs& args) override;
	bool Disabled() const override { return TypeInstance() > MAX_CHAINMIXER_CHANNELS; }
	void ProcessAudioBuses(	// called from main module's process() function
		const ProcessArgs& args,
		float* pMainL, float* pMainR,
		float* pAux1L, float* pAux1R,
		float* pAux2L, float* pAux2R,
		float fMainFactor,
		bool bMainMute,
		bool bAnyChannelSolo,
		struct AuxInfo rAuxInfo[2]) override;

protected:
	int SoloParam() const override { return (int)ParamId::ParamSolo; }
	int MuteParam() const override { return (int)ParamId::ParamMute; }
	int TrimParam() const override { return (int)ParamId::ParamTrim; }
	int GainParam() const override { return (int)ParamId::ParamGain; }
	int PanBalParam() const override { return (int)ParamId::ParamPanBal; }
	int Aux1Param() const override { return (int)ParamId::ParamAux1; }
	int Aux2Param() const override { return (int)ParamId::ParamAux2; }
	int InputLId() const override { return InputL; }
	int InputRId() const override { return InputR; }

private:
	bool m_bInitialized = false;
	Fade m_fadeMainMute;
};

/////////////////////////////////////////////////////
/// Widget
/////////////////////////////////////////////////////

struct ChainMixerChannelWidget : ModuleWidget
{
public:
	ChainMixerChannelWidget(ChainMixerChannelModule* pModule);

	void appendContextMenu(Menu* menu) override;
	void step() override;

private:
	class GPaudioFader* m_pFader = nullptr;
	int m_nTypeInstance = -1;
	bool m_bDarkMode;
	SvgWidget* m_pNumberWidget;

	int m_nRow = -1;
	int m_nColumn = -1;
};

extern Model* the_pChainMixerChannelModel;
