//
// ChainMixerModule class
// ====================
// Base class for all chain mixer modules. Handles
// everything to do with chaining and offers generic
// handling of mute/solo or generic bool parameters.
//

#include "plugin.hpp"
#include "ChainMixerModule.h"

#include "ChainMixerChannel.h"
#include "ChainMixerMaster.h"
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
		case ModuleType::Master: return "Master";
		case ModuleType::Aux: return "Aux";
		default: return "Unknown";
	}
}

bool ChainMixerModule::HandleMute(int nParam, bool bForce /*= false*/)
{
	bool bMute = params[nParam].getValue() > 0.5f;
	bool bChanged = bMute != m_bMute;
	if (bChanged || bForce)
		m_bMute = bMute;
	return bChanged;
}

bool ChainMixerModule::HandleSolo(int nParam, bool bForce /*= false*/)
{
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

void ChainMixerModule::DetermineTypeInstance(Model* pModel)
{
	int nSameTypeCount = 0;
	for (Module* pModule = leftExpander.module; pModule != nullptr; pModule = pModule->leftExpander.module)
	{
		Model* pNeighborModel = pModule->model;
		if (pNeighborModel == pModel)
			nSameTypeCount++;
		else
		{
			if (pNeighborModel == the_pChainMixerChannelModel)
				continue;
			if (pNeighborModel == the_pChainMixerMasterModel)
				continue;
			if (pNeighborModel == the_pChainMixerAuxModel)
				continue;
			break;
		}
	}
	m_nTypeInstance = nSameTypeCount + 1;
}

