//
// ChainMixerChannelBase class
// ===========================
// Base class for ChainMixerChannel and Chain<MixerExtChannel classes
//

#include "plugin.h"
#include "ChainMixerChannelBase.h"
#include "Faders.h"

static bool s_bTrimInitialized = false;
static struct
{
	float fdB;
	float fFactor;
}
s_Trim[2 * TRIM_STEPS + 1];

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrimQuantity
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

TrimQuantity::TrimQuantity()
{
	if (!s_bTrimInitialized)
	{
		s_bTrimInitialized = true;
		for (int i = -TRIM_STEPS; i <= TRIM_STEPS; i++)
		{
			s_Trim[i + TRIM_STEPS].fdB = static_cast<float>(i) / 10.0f;
			s_Trim[i + TRIM_STEPS].fFactor = pow(10.0f, s_Trim[i + TRIM_STEPS].fdB / 20.0f);
		}
	}
}

/*static*/ float TrimQuantity::GainFactor(float fParam)
{
	int nParam = iround(fParam);
	return s_Trim[nParam + TRIM_STEPS].fFactor;
}

std::string TrimQuantity::getDisplayValueString() /*override*/
{
	float fParam = getValue();
	int nParam = iround(fParam);
	char szValue[32];
	snprintf(szValue, sizeof(szValue), "%.1f dB", s_Trim[nParam + TRIM_STEPS].fdB);
	return szValue;
}

void TrimQuantity::setDisplayValueString(std::string s) /*override*/
{
	float fdB;
	float fParam;
	if (!StrToFloat(s, fdB))
		fParam = 0.0f;
	else if (fdB < -TRIM_STEPS_F / 10.0f)
		fParam = -TRIM_STEPS_F;
	else if (fdB > TRIM_STEPS_F / 10.0f)
		fParam = TRIM_STEPS_F;
	else
		fParam = fdB * 10.f;
	setDisplayValue(fParam);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChainMixerChannelBase
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChainMixerChannelBase::ChainMixerChannelBase(ModuleType eType) :
	ChainMixerModule(eType),
	m_fadeMainMono(m_fFaderFactorMono, FADE_MS, 0.0f),
	m_fadeMainStereo(m_fMainFactorsStereo[0], m_fMainFactorsStereo[1], FADE_MS, 0.0f),
	m_fadeMainBus(m_fMainBusFactor, FADE_MS, 1.0f),
	m_fadeAux1(m_fAux1Factor, FADE_MS, 0.0f),
	m_fadeAux2(m_fAux2Factor, FADE_MS, 0.0f)
{
}

void ChainMixerChannelBase::onSampleRateChange(const SampleRateChangeEvent &e) /*override*/
{
	m_fadeMainMono.SetSamplerate(e.sampleRate);
	m_fadeMainStereo.SetSamplerate(e.sampleRate);
	m_fadeAux1.SetSamplerate(e.sampleRate);
	m_fadeAux2.SetSamplerate(e.sampleRate);
	m_fadeMainBus.SetSamplerate(e.sampleRate);
	m_bFadesInitialized = true;
}

bool ChainMixerChannelBase::SetMuteExternal(bool bMute, MultiParamChange& rHistoryBuf) // override
{
	lock_guard<mutex> lg(MuteSoloMutex());
	float fValue = bMute ? 1.0f : 0.0f;
	if (fValue != params[MuteParam()].getValue())
	{
		history::ParamChange chg;
		chg.moduleId = id;
		chg.paramId = MuteParam();
		chg.oldValue = params[MuteParam()].getValue();
		chg.newValue = fValue;
		rHistoryBuf.vChanges.emplace_back(chg);
	}
	params[MuteParam()].setValue(fValue);
	return true;
}

bool ChainMixerChannelBase::SetSoloExternal(bool bSolo, bool bSaveCurrent, MultiParamChange& rHistoryBuf) // override
{
	lock_guard<mutex> lg(MuteSoloMutex());
	m_bOldSolo = Solo();
	m_bOldSoloValid = bSaveCurrent;
	float fValue = bSolo ? 1.0f : 0.0f;
	if (fValue != params[SoloParam()].getValue())
	{
		history::ParamChange chg;
		chg.moduleId = id;
		chg.paramId = SoloParam();
		chg.oldValue = params[SoloParam()].getValue();
		chg.newValue = fValue;
		rHistoryBuf.vChanges.emplace_back(chg);
	}
	params[SoloParam()].setValue(fValue);
	return true;
}

bool ChainMixerChannelBase::RestoreSoloExternal(MultiParamChange& rHistoryBuf) // override
{
	if (!m_bOldSoloValid)
		return false;
	m_bOldSoloValid = false;
	SetSoloExternal(m_bOldSolo, false, rHistoryBuf);
	return true;
}

// Audio processing for all buses
// Called from the main module's process() function

void ChainMixerChannelBase::ProcessAudioBuses(
	const ProcessArgs& args,
	float* pMainL, float* pMainR,
	float* pAux1L, float* pAux1R,
	float* pAux2L, float* pAux2R,
	float fMainFactor,
	bool bMainMute,
	bool bAnyChannelSolo,
	struct AuxInfo rAuxInfo[2])
{
	if (!m_bFadesInitialized)
	{
		m_bFadesInitialized = true;
		m_fadeMainMono.SetSamplerate(args.sampleRate);
		m_fadeMainStereo.SetSamplerate(args.sampleRate);
		m_fadeAux1.SetSamplerate(args.sampleRate);
		m_fadeAux2.SetSamplerate(args.sampleRate);
		m_fadeMainBus.SetSamplerate(args.sampleRate);
	}
	bool bAnyAuxSolo = (rAuxInfo[0].bSolo || rAuxInfo[1].bSolo);
	bool bHaveMono = false;
	bool bHaveStereo = false;
	bool bNeedMono = false;
	bool bNeedStereo = false;

	//
	// Input
	//
	float fTrimFactor = TrimQuantity::GainFactor(params[TrimParam()].getValue());
	float fInL = 0.0f;	// fInL also used for mono, regardless of connected mono ExtChannel
	float fInR = 0.0f;
	if (inputs[InputLId()].isConnected())
	{
		fInL = inputs[InputLId()].getVoltageSum() * fTrimFactor;
		if (inputs[InputRId()].isConnected())
		{
			fInR = inputs[InputRId()].getVoltageSum() * fTrimFactor;
			bHaveStereo = true;
		}
		else
			bHaveMono = true;
	}
	else
	{
		if (inputs[InputRId()].isConnected())
		{
			bHaveMono = true;
			fInL = inputs[InputRId()].getVoltageSum() * fTrimFactor;
		}
	}

	//
	// determine required output signals
	//
	if (m_fMainBusFactor > 0.0f)
	{
		if (pMainL != nullptr)
		{
			if (pMainR != nullptr)
				bNeedStereo = true;
			else
				bNeedMono = true;
		}
	}
	if (pAux1L != nullptr)
	{
		if (pAux1R != nullptr)
			bNeedStereo = true;
		else
			bNeedMono = true;
	}
	if (pAux2L != nullptr)
	{
		if (pAux2R != nullptr)
			bNeedStereo = true;
		else
			bNeedMono = true;
	}
	AddMonoStereoRequirements(bNeedMono, bNeedStereo); // handles the needs of direct out

	if (!(bHaveMono || bHaveStereo) || !(bNeedMono || bNeedStereo))
	{
		m_fPreL = 0.0f;
		m_fPreR = 0.0f;
		m_fPreMono = 0.0f;
		m_fPostL = 0.0f;
		m_fPostR = 0.0f;
		m_fPostMono = 0.0f;
		FadeToZero(true);
		return;
	}
	float fFaderFactor = 0.0f;
	if (!Mute() && (Solo() || !bAnyChannelSolo))
		fFaderFactor = GPaudioFader::GainFactor(params[GainParam()].getValue()) * CVGainFactor();

	if (bHaveMono)
	{
		//
		// mono input, audio in fInL
		//
		if (bNeedMono)
		{
			m_fadeMainMono.Start(fFaderFactor);
			m_fPreMono = fInL;
			m_fPostMono = fInL * m_fFaderFactorMono;
		}
		else
			m_fadeMainMono.Start(0.0f);

		if (bNeedStereo)
		{
			float fPanL = PanBalQuantity::GainFactorL(params[PanBalParam()].getValue(), false);
			float fPanR = PanBalQuantity::GainFactorR(params[PanBalParam()].getValue(), false);
			m_fadeMainStereo.Start(fFaderFactor * fPanL, fFaderFactor * fPanR);

			m_fPreL = m_fPreR = fInL;
			m_fPostL = fInL * m_fMainFactorsStereo[0];
			m_fPostR = fInL * m_fMainFactorsStereo[1];
		}
		else
			m_fadeMainStereo.Start(0.0f);
	} // bHaveMono
	else
	{
		//
		// stereo input, audio in fInL/fInR
		//
		m_fadeMainMono.Start(0.0);	// not using this, since mono output is left + right (including balance)
		float fParamPanBal = params[PanBalParam()].getValue();
		ApplyPanBalCV(fParamPanBal);
		float fBalL = PanBalQuantity::GainFactorL(fParamPanBal, true);
		float fBalR = PanBalQuantity::GainFactorR(fParamPanBal, true);
		m_fadeMainStereo.Start(fFaderFactor * fBalL, fFaderFactor * fBalR);

		m_fPreL = fInL;
		m_fPreR = fInR;
		m_fPreMono = (fInL + fInL) * g_fMinus3dB;
		m_fPostL = fInL * m_fMainFactorsStereo[0];
		m_fPostR = fInR * m_fMainFactorsStereo[1];
		m_fPostMono = (m_fPostL + m_fPostR) * g_fMinus3dB;
	}

	//
	// Mix to main bus
	//
	if (bAnyAuxSolo || m_bMuteMain)
		m_fadeMainBus.Start(0.0f);
	else
		m_fadeMainBus.Start(1.0f);

	if (pMainL != nullptr)
	{
		if (pMainR != nullptr)
		{
			*pMainL += m_fPostL * m_fMainBusFactor;
			*pMainR += m_fPostR * m_fMainBusFactor;
		}
		else
			*pMainL += m_fPostMono * m_fMainBusFactor;
	}

	//
	// Aux sends
	//
	if (pAux1L != nullptr)
	{
		float fParam = params[Aux1Param()].getValue();
		float fFactor = SendQuantity::GainFactor(fParam);
		m_fadeAux1.Start(fFactor);
		if (m_bAux1Pre)
		{
			if (pAux1R != nullptr)
			{
				*pAux1L += m_fPreL * fFactor;
				*pAux1R += m_fPreR * fFactor;
			}
			else
				*pAux1L += m_fPreMono * fFactor;
		}
		else
		{
			if (pAux1R != nullptr)
			{
				*pAux1L += m_fPostL * fFactor;
				*pAux1R += m_fPostR * fFactor;
			}
			else
				*pAux1L += m_fPostMono * fFactor;
		}
	}
	else
		m_fadeAux1.Start(0.0f);

	if (pAux2L != nullptr)
	{
		float fFactor = SendQuantity::GainFactor(params[Aux2Param()].getValue());
		m_fadeAux2.Start(fFactor);
		if (m_bAux2Pre)
		{
			if (pAux2R != nullptr)
			{
				*pAux2L += m_fPreL * fFactor;
				*pAux2R += m_fPreR * fFactor;
			}
			else
				*pAux2L += m_fPreMono * fFactor;
		}
		else
		{
			if (pAux2R != nullptr)
			{
				*pAux2L += m_fPostL * fFactor;
				*pAux2R += m_fPostR * fFactor;
			}
			else
				*pAux2L += m_fPostMono * fFactor;
		}
	}
	else
		m_fadeAux2.Start(0.0f);
}

void ChainMixerChannelBase::AdvanceFades()
{
	m_fadeMainMono.Advance();
	m_fadeMainStereo.Advance();
	m_fadeAux1.Advance();
	m_fadeAux2.Advance();
	m_fadeMainBus.Advance();
}

void ChainMixerChannelBase::HandleMuteSoloQueue()
{
	MuteSoloEvent ev;
	unique_lock<mutex> lock(MuteSoloMutex());
	while (MuteSoloEventFromQueue(ev))
	{
		if (!ev.bPressed)
			continue;

		const bool bCurrentValue = params[ev.nParamId].getValue() > 0.5f;
		float fNewValue = bCurrentValue ? 0.0f : 1.0f;
		const bool bIsMute = ev.nParamId == MuteParam();

		auto pChanges = new MultiParamChange();
		history::ParamChange chg;
		chg.moduleId = id;
		chg.paramId = ev.nParamId;
		chg.oldValue = params[ev.nParamId].getValue();
		chg.newValue = fNewValue;
		pChanges->vChanges.emplace_back(chg);
		params[ev.nParamId].setValue(fNewValue);
		if (ev.bCtrlKeyDown)
		{
			lock.unlock(); // release mutex to avoid deadlock when another thread does the same thing in another channel (very theoretical, you'd have to be really quick with the mouse)
			if (bIsMute)
				ClearNeighborMuteSolo(true, false, *pChanges);
			else
			{
				if (ev.bShiftKeyDown)
					ClearNeighborMuteSolo(false, false, *pChanges);
				else
				{
					if (bCurrentValue)
						RestoreNeighborSolo(*pChanges);
					else
						ClearNeighborMuteSolo(false, true, *pChanges);
				}
			}
			lock.lock();
		}
		APP->history->push(pChanges);
	} // while (MuteSoloEventFromQueue(ev))
}

void ChainMixerChannelBase::FadeToZero(bool bAdvance)
{
	m_fadeMainMono.Start(0.0f);
	m_fadeMainStereo.Start(0.0f);
	m_fadeAux1.Start(0.0f);
	m_fadeAux2.Start(0.0f);
	m_fadeMainBus.Start(0.0f);
	if (bAdvance)
		AdvanceFades();
}
