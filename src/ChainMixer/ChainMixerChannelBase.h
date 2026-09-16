#pragma once

#include "ChainMixerModule.h"
#include "Fade.h"
#include "Fade2.h"

#define TRIM_STEPS			180 // Parameter values go from -TRIM_STEPS to +TRIM_STEPS => +/- TRIM_STEPS/10 dB
#define TRIM_STEPS_F		(static_cast<float>(TRIM_STEPS))

struct TrimQuantity : public ParamQuantity
{
	TrimQuantity();

	static float GainFactor(float fParam);
	std::string getDisplayValueString() override;
	void setDisplayValueString(std::string s) override;
};

class ChainMixerChannelBase : public ChainMixerModule
{
public:
	ChainMixerChannelBase(ModuleType eType);

	void onSampleRateChange(const SampleRateChangeEvent &e) override;
	bool SetMuteExternal(bool bMute, MultiParamChange& rHistoryBuf) override;
	bool SetSoloExternal(bool bSolo, bool bSaveCurrent, MultiParamChange& rHistoryBuf) override;
	bool RestoreSoloExternal(MultiParamChange& rHistoryBuf) override;

	void ProcessAudioBuses(	// called from the main module's process() function. Always override in channel modules to at least call AdvanceFades!
	const ProcessArgs& args,
	float* pMainL, float* pMainR,
	float* pAux1L, float* pAux1R,
	float* pAux2L, float* pAux2R,
	float fMainFactor,
	bool bMainMute,
	bool bAnyChannelSolo,
	struct AuxInfo rAuxInfo[2]) override;

protected:
	// overrideables
	virtual int SoloParam() const = 0;
	virtual int MuteParam() const = 0;
	virtual int TrimParam() const = 0;
	virtual int GainParam() const = 0;
	virtual int PanBalParam() const = 0;
	virtual int Aux1Param() const = 0;
	virtual int Aux2Param() const = 0;
	virtual int InputLId() const = 0;
	virtual int InputRId() const = 0;
	virtual float CVGainFactor() { return 1.0f; }
	virtual void ApplyPanBalCV(float & rValue) { }
	virtual void FadeToZero(bool bAdvance);
	virtual void AddMonoStereoRequirements(bool& rNeedMono, bool& rNeedStereo) { } // overridden in ext channel for direct out needs

	void AdvanceFades();
	void HandleMuteSoloQueue();


protected:
	bool m_bFadesInitialized = false;

	//
	// Signals
	//

	// Only Trim applied
	float m_fPreL = 0.0f;
	float m_fPreR = 0.0f;
	float m_fPreMono = 0.0f;
	// Trim, PanBal, Fader, Mute/Solo applied. Without AnyAuxSolo and MuteMain
	float m_fPostL = 0.0f;
	float m_fPostR = 0.0f;
	float m_fPostMono = 0.0f;

	//
	// Factors and associatewd Fades
	//

	float m_fFaderFactorMono = 0.0f;				// Fader, Mute, Solo, for mono outputs
	Fade m_fadeMainMono;
	float m_fMainFactorsStereo[2] = { 0.0f, 0.0f };	// Fader, Mute, Solo, Pan/Balance
	Fade2 m_fadeMainStereo;

	bool m_bMuteMain = false;						// ony set in extended channel
	float m_fMainBusFactor = 1.0f;					// affected by Aux1 and 2 Solo and (extended channel only) MuteMain
	Fade m_fadeMainBus;

	bool m_bAux1Pre = false;
	float m_fAux1Factor = 0.0f;
	Fade m_fadeAux1;

	bool m_bAux2Pre = false;
	float m_fAux2Factor = 0.0f;
	Fade m_fadeAux2;

	// used to restore previous solo state on CTRL-key + Solo
	bool m_bOldSolo = false;
	bool m_bOldSoloValid = false;
};


