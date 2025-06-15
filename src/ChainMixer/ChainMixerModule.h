//
// ChainMixerModule class
// ====================
// Base class for all chain mixer modules. Handles
// everything to do with chaining and offers generic
// handling of mute/solo or generic bool parameters.
//

#pragma once

#define DIM_BRIGHTNESS	(0.1f)	// tone down green Solo LED to match red Mute light
#define SOLO_BRIGHTNESS (0.75f)	// tone down green Solo LED to match red Mute light
#define MUTE_BRIGHTNESS (1.0f)	// tone down green Solo LED to match red Mute light

#define FADE_MS			(30.0f)	// fade time in ms

extern const float g_fMinus3dB;

struct AuxInfo
{
	bool bConnected;
	bool bMono;
	bool bSolo;
	bool bMute;
};

class ChainMixerModule : public Module
{
public:
	enum class ModuleType : int
	{
		Channel,
		Master,
		Aux
	};

	typedef struct _Message
	{
		class ChainMixerModule* m_pOriginator;
		int m_nModulesSoFar = 0;
		int m_nTotalModules = 0;
		int m_nChannelModulesSoFar = 0;
		int m_nTotalChannelModules;
		int m_nMasterModulesSoFar = 0;
		int m_nTotalMasterModules;
		int m_nAuxModulesSoFar = 0;
		int m_nTotalAuxModules;
	}
	Message;

/////////////////////////////////////
/// Construction
/////////////////////////////////////

protected:
	ChainMixerModule(ModuleType eType);

/////////////////////////////////////
/// Public API
/////////////////////////////////////

public:
	int TypeInstance() const { return m_nTypeInstance; }
	ModuleType Type() const { return m_eType; }

	virtual bool Disabled() const = 0;

	virtual bool Solo() const { return m_bSolo; }
	bool Mute() const { return m_bMute; }

	// process channel/aux/master and add to audio busses
	// If a bus is mono, the right channel pointer is null. if an output is not connected, both pointers are null.
	virtual void ProcessAudioBusses(
		const ProcessArgs& args,
		float* pMainL, float* pMainR,
		float* pAux1L, float* pAux1R,
		float* pAux2L, float* pAux2R,
		bool bAnyChannelSolo,
		struct AuxInfo rInfo[2]) = 0;

	// Only for logging and debug printfs
	static std::string TypeString(ModuleType eType);
	std::string TypeString() const { return TypeString(m_eType); }

/////////////////////////////////////
/// Private and protected methods
/////////////////////////////////////

protected:
	// void process(const ProcessArgs &args) override;
	bool HandleMute(int nParam, bool bForce = false);	// true if changed
	bool HandleSolo(int nParam, bool bForce = false);	// true if changed
	bool HandleBoolParam(bool& rValue, int nParam, bool bForce = false);	// true if changed
	void DetermineTypeInstance(Model* pModel);	// walk left and count neighbors of same type


/////////////////////////////////////
/// Private Data
/////////////////////////////////////

private:
	const ModuleType m_eType;
	int m_nTypeInstance = 0;		// number of identical modules to the left plus 1

	bool m_bMute = false;
	bool m_bSolo = false;
};