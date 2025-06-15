#pragma once

#include "ChainMixerCommon.h"
#include "ChainMixerModule.h"
#include "Fade.h"
#include "Fade2.h"

#define MAX_CHAINMIXER_CHANNELS		16

/////////////////////////////////////////////////////
/// Module
/////////////////////////////////////////////////////

class ChainMixerChannelModule : public ChainMixerModule
{
public:
	enum ParamId
	{
		ParamAux1,
		ParamAux2,
		ParamPanBal,
		ParamGain,
		ParamSolo,
		ParamMute,
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
	void onSampleRateChange(const SampleRateChangeEvent &e) override;
	void process(const ProcessArgs& args) override;
	void SetWidget(struct ChainMixerChannelWidget* pWidget) { m_pWidget = pWidget; }
	bool Disabled() const override { return TypeInstance() > MAX_CHAINMIXER_CHANNELS; }
	void ProcessAudioBusses(	// called from master channel's process() function
		const ProcessArgs& args,
		float* pMainL, float* pMainR,
		float* pAux1L, float* pAux1R,
		float* pAux2L, float* pAux2R,
		bool bAnyChannelSolo,
		struct AuxInfo rAuxInfo[2]) override;

private:
	void FadeToZeroAndAdvance();

private:
	struct ChainMixerChannelWidget* m_pWidget = nullptr;
	bool m_bInitialized = false;

	bool m_bFadesInitialized = false;

	float m_fFaderFactor = 0.0f;				// fader only
	float m_fMainFactors[2] = { 0.0f, 0.0f };	// fader plus pan/balance, for mixes to mono use fader factor
	Fade m_fadeMainFader;
	Fade2 m_fadeMain;

	float m_fAux1Factor = 0.0f;
	Fade m_fadeAux1;

	float m_fAux2Factor = 0.0f;
	Fade m_fadeAux2;
};

/////////////////////////////////////////////////////
/// Widget
/////////////////////////////////////////////////////

struct ChainMixerChannelWidget : ModuleWidget
{
public:
	ChainMixerChannelWidget(ChainMixerChannelModule* pModule);

protected:
	void step() override;

private:
	class GPaudioFader* m_pFader = nullptr;
	int m_nTypeInstance = -1;
	bool m_bDarkMode;
	SvgWidget* m_pNumberWidget;
};

extern Model* the_pChainMixerChannelModel;
