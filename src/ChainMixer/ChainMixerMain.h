#pragma once

#include "ChainMixerModule.h"
#include "Fade.h"

class ChainMixerMainModule : public ChainMixerModule
{
public:
	enum ParamId
	{
		ParamAux1,
		ParamAux2,
		ParamGain,
		ParamMute,
		ParamOverdB,
		NumParams
	};
	enum OutputId
	{
		OutputL,
		OutputR,
		NumOutputs
	};
	enum LightId
	{
		LightMute,
		LightOver,
		NumLights
	};

public:
	ChainMixerMainModule();

public:
	void onSampleRateChange(const SampleRateChangeEvent &e) override;
	void OverThreshold(float fdB, bool bUpdateParamter = true);
	float OverThreshold() const { return m_fOverdB; }
	void process(const ProcessArgs& args) override;
	void SetWidget(struct ChainMixerMainWidget* pWidget) { m_pWidget = pWidget; }
	bool Disabled() const override { return TypeInstance() > 1; }
	void ProcessAudioBuses(
		const ProcessArgs& args,
		float* pMainL, float* pMainR,
		float* pAux1L, float* pAux1R,
		float* pAux2L, float* pAux2R,
		float fMainFactor,
		bool bMainMute,
		bool bAnyChannelSolo,
		struct AuxInfo rInfo[2]) override;

private:
	void DetermineSolo(class ChainMixerAuxModule*& rpAuxModule);	// sts ptr to aux module if found, and read AuxInfo
	void SetupBuses();
	void ProcessChannelModules(const ProcessArgs& args);
	void ProcessAuxGain();
	struct ChainMixerMainWidget* m_pWidget = nullptr;
	bool m_bInitialized = false;
	struct AuxInfo m_AuxInfo[2];
	bool m_bAnyChannelSolo = false;

	// Audio buses
	float m_fMainL = 0.0f;
	float m_fMainR = 0.0f;
	float m_fAux1L = 0.0f;
	float m_fAux1R = 0.0f;
	float m_fAux2L = 0.0f;
	float m_fAux2R = 0.0f;

	// Pointers to buses, possibly null
	float* m_pMainL = nullptr;
	float* m_pMainR = nullptr;
	float* m_pAux1L = nullptr;
	float* m_pAux1R = nullptr;
	float* m_pAux2L = nullptr;
	float* m_pAux2R = nullptr;

	// Main module's own parameters
	float m_fFactorFader = 0.0f;				// fader only
	Fade m_fadeMain;

	float m_fFactorAux1 = 1.0f;
	Fade m_fadeAux1;

	float m_fFactorAux2 = 1.0f;
	Fade m_fadeAux2;

	float m_fOverdB = 0.0f;
	float m_fOverVolt = 5.0f;

	bool m_bOver = false;	// state of over light (including during over hold)
	int64_t m_nOverFrame;	// insitalized in init list
	int64_t m_nOverHoldFrames = 0;
};

struct ChainMixerMainWidget : ModuleWidget
{
public:
	ChainMixerMainWidget(ChainMixerMainModule* pModule);

	void appendContextMenu(Menu* menu) override;
	void step() override;

private:
	class GPaudioFader* m_pFader = nullptr;
};

extern Model* the_pChainMixerMainModel;
