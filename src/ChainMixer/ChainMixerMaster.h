#pragma once

#include "ChainMixerCommon.h"
#include "ChainMixerModule.h"
#include "Fade.h"

class ChainMixerMasterModule : public ChainMixerModule
{
public:
	enum ParamId
	{
		ParamAux1,
		ParamAux2,
		ParamGain,
		ParamMute,
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
		NumLights
	};

public:
	ChainMixerMasterModule();

public:
	void onSampleRateChange(const SampleRateChangeEvent &e) override;
	void process(const ProcessArgs& args) override;
	void SetWidget(struct ChainMixerMasterWidget* pWidget) { m_pWidget = pWidget; }
	bool Disabled() const override { return TypeInstance() > 1; }
	void ProcessAudioBusses(
		const ProcessArgs& args,
		float* pMainL, float* pMainR,
		float* pAux1L, float* pAux1R,
		float* pAux2L, float* pAux2R,
		bool bAnyChannelSolo,
		struct AuxInfo rInfo[2]) override;

private:
	void DetermineSolo(class ChainMixerAuxModule*& rpAuxModule);	// sts ptr to aux module if found, and read AuxInfo
	void SetupBusses();
	void ProcessChannelModules(const ProcessArgs& args);
	void ProcessAuxGain();
	struct ChainMixerMasterWidget* m_pWidget = nullptr;
	bool m_bInitialized = false;
	struct AuxInfo m_AuxInfo[2];
	bool m_bAnyChannelSolo = false;

	// Audio busses
	float m_fMainL = 0.0f;
	float m_fMainR = 0.0f;
	float m_fAux1L = 0.0f;
	float m_fAux1R = 0.0f;
	float m_fAux2L = 0.0f;
	float m_fAux2R = 0.0f;

	// Pointers to busses, can be null)
	float* m_pMainL = nullptr;
	float* m_pMainR = nullptr;
	float* m_pAux1L = nullptr;
	float* m_pAux1R = nullptr;
	float* m_pAux2L = nullptr;
	float* m_pAux2R = nullptr;

	// Master module;s own parameters
	float m_fFactorFader = 0.0f;				// fader only
	Fade m_fadeMain;

	float m_fFactorAux1;
	Fade m_fadeAux1;

	float m_fFactorAux2;
	Fade m_fadeAux2;
};

struct ChainMixerMasterWidget : ModuleWidget
{
public:
	ChainMixerMasterWidget(ChainMixerMasterModule* pModule);

protected:
	void step() override;

private:
	class GPaudioFader* m_pFader = nullptr;
};

extern Model* the_pChainMixerMasterModel;
