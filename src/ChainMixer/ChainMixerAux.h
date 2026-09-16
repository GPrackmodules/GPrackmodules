#pragma once

#include <Fade.h>

#include "ChainMixerCommon.h"
#include "ChainMixerModule.h"

class ChainMixerAuxModule : public ChainMixerModule
{
public:
	enum ParamId
	{
		ParamGain1,
		ParamGain2,
		ParamSolo1,
		ParamSolo2,
		ParamMute1,
		ParamMute2,
		ParamMuteSendsOnMainMute,
		NumParams
	};
	enum InputId
	{
		Return1L,
		Return1R,
		Return2L,
		Return2R,
		NumInputs
	};
	enum OutputId
	{
		Send1L,
		Send1R,
		Send2L,
		Send2R,
		NumOutputs
	};
	enum LightId
	{
		LightSolo1,
		LightSolo2,
		LightMute1,
		LightMute2,
		NumLights
	};

public:
	ChainMixerAuxModule();

public:
	void onSampleRateChange(const SampleRateChangeEvent &e) override;
	void process(const ProcessArgs& args) override;
	void SetWidget(struct ChainMixerAuxWidget* pWidget) { m_pWidget = pWidget; }
	bool Solo() override { return m_AuxInfo[0].bSolo || m_AuxInfo[1].bSolo; }
	bool MuteSendsOnMainMute() const { return m_bMuteSendsOnMainMute; }
	void MuteSendsOnMainMute(bool bMuteSendsOnMainMute);
	void GetAuxInfo(struct AuxInfo rInfo[2]);
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

protected:
	void HandleMuteSoloQueue();


private:
	struct ChainMixerAuxWidget* m_pWidget = nullptr;
	struct AuxInfo m_AuxInfo[2];
	bool m_bInitialized = false;
	bool m_bFadesInitialized = false;

	float m_fFactorReturn1 = 0.0f;
	float m_fFactorReturn2 = 0.0f;
	Fade m_fadeReturn1;
	Fade m_fadeReturn2;
	bool m_bMuteSendsOnMainMute = false;

	bool m_bOldSolo[2] = { false, false };
	bool m_bOldSoloValid[2] = { false, false };
};

struct ChainMixerAuxWidget : ModuleWidget
{
public:
	ChainMixerAuxWidget(ChainMixerAuxModule* pModule);

	void appendContextMenu(Menu* menu) override;
	void step() override;

private:
	class GPaudioFader* m_pFader1 = nullptr;
	class GPaudioFader* m_pFader2 = nullptr;
};

extern Model* the_pChainMixerAuxModel;
