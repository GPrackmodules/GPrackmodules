//
// ChainMixerModule class
// ====================
// Base class for all chain mixer modules. Handles
// everything to do with chaining and offers generic
// handling of mute/solo or generic bool parameters.
//

#include "plugin.h"
#include "ChainMixerModule.h"

#include "ChainMixerChannel.h"
#include "ChainMixerExtChannel.h"
#include "ChainMixerMain.h"
#include "ChainMixerAux.h"

const float g_fMinus3dB = sqrt(0.5f);

ChainMixerModule::ChainMixerModule(ModuleType eType) :
	m_eType(eType)
{
}

/*static*/ std::string ChainMixerModule::TypeString(ModuleType eType)
{
	switch (eType)
	{
		case ModuleType::Channel: return "Channel";
		case ModuleType::ExtChannel: return	 "Extended Channel";
		case ModuleType::Main: return "Main";
		case ModuleType::Aux: return "Aux";
		default: return "Unknown";
	}
}

void ChainMixerModule::QueueMuteSoloEvent(bool bPressed, int nParanId, bool bShift, bool bCtrl)
{
	lock_guard<mutex> lg(m_mtxMuteSolo);
	m_queMuteSoloEvents.emplace(bPressed, nParanId, bShift, bCtrl);
}

void ChainMixerModule::ClearNeighborMuteSolo(bool bIsMute, bool bSaveCurrent, struct MultiParamChange& rHistoryBuf) const
{
	// Modules to the left
	auto pModule = dynamic_cast<ChainMixerModule*>(leftExpander.module);
	while (pModule != nullptr)
	{
		if (!pModule->Disabled())
		{
			if (bIsMute)
				pModule->SetMuteExternal(false, rHistoryBuf);
			else
				pModule->SetSoloExternal(false, bSaveCurrent, rHistoryBuf);
		}
		pModule = dynamic_cast<ChainMixerModule*>(pModule->leftExpander.module);
	}
	// Modules to the right
	pModule = dynamic_cast<ChainMixerModule*>(rightExpander.module);
	while (pModule != nullptr)
	{
		if (!pModule->Disabled())
		{
			if (bIsMute)
				pModule->SetMuteExternal(false, rHistoryBuf);
			else
				pModule->SetSoloExternal(false, bSaveCurrent, rHistoryBuf);
		}
		pModule = dynamic_cast<ChainMixerModule*>(pModule->rightExpander.module);
	}
}

void ChainMixerModule::RestoreNeighborSolo(MultiParamChange &rHistoryBuf) const
{
	// Modules to the left
	auto pModule = dynamic_cast<ChainMixerModule*>(leftExpander.module);
	while (pModule != nullptr)
	{
		if (!pModule->Disabled())
			pModule->RestoreSoloExternal(rHistoryBuf);
		pModule = dynamic_cast<ChainMixerModule*>(pModule->leftExpander.module);
	}
	// Modules to the right
	pModule = dynamic_cast<ChainMixerModule*>(rightExpander.module);
	while (pModule != nullptr)
	{
		if (!pModule->Disabled())
			pModule->RestoreSoloExternal(rHistoryBuf);
		pModule = dynamic_cast<ChainMixerModule*>(pModule->rightExpander.module);
	}
}

bool ChainMixerModule::MuteSoloEventFromQueue(MuteSoloEvent& event)
{
	if (m_queMuteSoloEvents.empty())
		return false;
	event = m_queMuteSoloEvents.front();
	m_queMuteSoloEvents.pop();
	return true;
}

bool ChainMixerModule::HandleMute(int nParam, bool bForce /*= false*/)
{
	lock_guard<mutex> lg(m_mtxMuteSolo);
	bool bMute = params[nParam].getValue() > 0.5f;
	bool bChanged = bMute != m_bMute;
	if (bChanged || bForce)
		m_bMute = bMute;
	return bChanged;
}

bool ChainMixerModule::HandleSolo(int nParam, bool bForce /*= false*/)
{
	lock_guard<mutex> lg(m_mtxMuteSolo);
	bool bSolo = params[nParam].getValue() > 0.5f;
	bool bChanged = bSolo != m_bSolo;
	if (bChanged || bForce)
		m_bSolo = bSolo;
	return bChanged;
}
bool ChainMixerModule::HandleBoolParam(bool& rValue, int nParam, bool bForce /*= false*/)
{
	bool bValue = params[nParam].getValue() > 0.5f;
	bool bChanged = bValue != rValue;
	if (bChanged || bForce)
		rValue = bValue;
	return bChanged;
}

void ChainMixerModule::DetermineTypeInstance(const Model* pModel)
{
	int nSameTypeCount = 0;
	for (Module* pModule = leftExpander.module; pModule != nullptr; pModule = pModule->leftExpander.module)
	{
		Model* pNeighborModel = pModule->model;
		if (pNeighborModel == pModel ||
			(pModel == the_pChainMixerChannelModel && pNeighborModel == the_pChainMixerExtChannelModel) || // Channel and ChnExt are nombered the same
			(pModel == the_pChainMixerExtChannelModel && pNeighborModel == the_pChainMixerChannelModel))
		{
			nSameTypeCount++;
		}
		else
		{
			if (pNeighborModel == the_pChainMixerChannelModel)
				continue;
			if (pNeighborModel == the_pChainMixerMainModel)
				continue;
			if (pNeighborModel == the_pChainMixerAuxModel)
				continue;
			break;
		}
	}
	m_nTypeInstance = nSameTypeCount + 1;
}

