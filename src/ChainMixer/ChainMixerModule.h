//
// ChainMixerModule class
// ====================
// Base class for all chain mixer modules. Handles
// everything to do with chaining and offers generic
// handling of mute/solo or generic bool parameters.
//

#pragma once

#include "queue"
#include "ChainMixer/MuteSoloButton.h"
#include "common/MultiParamChange.h"

#define DIM_BRIGHTNESS	(0.1f)
#define AUXPRE_BRIGHTNESS (1.0f)	// tone down green Solo LED to match red Mute light
#define SOLO_BRIGHTNESS (0.85f)	// tone down green Solo LED to match red Mute light
#define MUTE_BRIGHTNESS (1.0f)
#define MAINFADER_BRIGHTNESS (1.0f)

#define FADE_MS			(30.0f)	// fade time in ms

extern const float g_fMinus3dB;

struct AuxInfo
{
	bool bConnected = false;
	bool bMono = false;
	bool bSolo = false;
	bool bMute = false;
};

class ChainMixerModule : public Module
{
public:
	enum class ModuleType : int
	{
		Channel,
		ExtChannel,
		Main,
		Aux
	};

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

	virtual bool Solo() { return m_bSolo; }
	virtual bool Mute() { return m_bMute; }

	// process (ext)channel/aux/main and add to audio buses.
	// If a bus is mono, the right channel pointer is null. if an output is not connected, both pointers are null.
	virtual void ProcessAudioBuses(
		const ProcessArgs& args,
		float* pMainL, float* pMainR,
		float* pAux1L, float* pAux1R,
		float* pAux2L, float* pAux2R,
		float fMainFactor,
		bool bMainMute,
		bool bAnyChannelSolo,
		struct AuxInfo rInfo[2])
	{
	}

	// Only for logging and debug printfs
	static std::string TypeString(ModuleType eType);
	std::string TypeString() const { return TypeString(m_eType); }

	void QueueMuteSoloEvent(bool bPressed, int nParanId, bool bShift, bool bCtrl);
	virtual bool SetMuteExternal(bool bMute, MultiParamChange& rHistoryBuf) { return false; } // affects all mutes in a module
	virtual bool SetSoloExternal(bool bSolo, bool bSaveCurrent, MultiParamChange& rHistoryBuf) { return false; } // affects all solos in a module
	virtual bool RestoreSoloExternal(MultiParamChange& rHistoryBuf) { return false; } // affects all solos in a module
	mutex& MuteSoloMutex() const { return m_mtxMuteSolo; }

/////////////////////////////////////
/// Private and protected methods
/////////////////////////////////////

protected:
	void ClearNeighborMuteSolo(bool bIsMute, bool bSaveCurrent, MultiParamChange& rHistoryBuf) const; // bSaveCurrent only applies to solo
	void RestoreNeighborSolo(MultiParamChange& rHistoryBuf) const; // bSaveCurrent only applies to solo
	bool MuteSoloEventFromQueue(MuteSoloEvent& event); // Must be called from inside MuteSoloMutex() lock
	bool HandleMute(int nParam, bool bForce = false);	// true if changed
	bool HandleSolo(int nParam, bool bForce = false);	// true if changed
	bool HandleBoolParam(bool& rValue, int nParam, bool bForce = false);	// true if changed
	void DetermineTypeInstance(const Model* pModel);	// walk left and count neighbors of same type

/////////////////////////////////////
/// Private Data
/////////////////////////////////////

private:
	const ModuleType m_eType;
	int m_nTypeInstance = 0;		// number of identical modules to the left plus 1

	mutable mutex m_mtxMuteSolo;
	bool m_bMute = false;
	bool m_bSolo = false;

	queue<MuteSoloEvent> m_queMuteSoloEvents;
};