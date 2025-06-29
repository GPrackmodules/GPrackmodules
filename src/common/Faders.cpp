#include "plugin.hpp"
#include "Faders.h"

//
// Static stuff for FaderGainQuantity
//

#define N_FADER_CURVE		13

#define OFF_DB		(-144.0f)

//
// Static stuff fie GPaudioSlider
//

static struct _FaderCurve
{
	double dThresh;				// parameter value 0.0 .. 1.0
	double ddB;					// dB value at threshold
}
s_FaderCurve[N_FADER_CURVE] =
{
	{ 0.0,	-144.0 },
	{ 0.03008, -90.0 },
	{ 0.07519, -60.0 },
	{ 0.13534, -40.0 },
	{ 0.18797, -30.0 },
	{ 0.27820, -20.0 },
	{ 0.38346, -15.0 },
	{ 0.50376, -10.0 },
	{ 0.63158, -5.0 },
	{ 0.74788, -0.1 },
	{ 0.75588, 0.1 },
	{ 0.87970, 6.0 },
	{ 1.00000, 12.0 }
};
static bool s_bCurveInitialized = false;

static struct
{
	float fdB;
	float fFactor;
}
s_faderValues[FADER_STEPS + 1];

//
// Static stuff for PanBalQuantiry
//

static float s_fPanL[MAX_PANBAL - MIN_PANBAL + 1];
static float s_fPanR[MAX_PANBAL - MIN_PANBAL + 1];
static float s_fBalL[MAX_PANBAL - MIN_PANBAL + 1];
static float s_fBalR[MAX_PANBAL - MIN_PANBAL + 1];
bool s_bPanBalInitialized = false;

//
// Static stuff for SendQuantiry
//

static struct
{
	float fdB;
	float fFactor;
}
s_Send[SEND_STEPS + 1];

static bool s_bSendInitialized = false;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FaderGainQuantity
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

FaderGainQuantity::FaderGainQuantity()
{
	if (!s_bCurveInitialized)
	{
		s_bCurveInitialized = true;
		for (int i = 0; i < N_FADER_CURVE; i++)
			s_FaderCurve[i].dThresh *= (double)FADER_STEPS;
		for (int i = 0; i <= FADER_STEPS; i++)
		{
			s_faderValues[i].fdB = FaderParam2dB((float)i);
			s_faderValues[i].fFactor = pow(10.0f, s_faderValues[i].fdB / 20.0f);
			// printf("Step %d (rel %f), %.1f\n", i, (float)i, fdB);
		}
	}
}

/*static*/ float FaderGainQuantity::GainFactor(float fParam)
{
	int nParam = (int)(fParam + 0.5f);
	return s_faderValues[nParam].fFactor;
}

std::string FaderGainQuantity::getDisplayValueString()
{
	int nParam = (int)(getValue() + 0.5f);
	float fdB = s_faderValues[nParam].fdB;
	if (fdB == 0.0f)
		return "0 dB";
	if (fdB <= OFF_DB)
		return "Off";
	char szValue[32];
	snprintf(szValue, sizeof(szValue), "%.1f dB", fdB);
	return szValue;
}

void FaderGainQuantity::setDisplayValueString(std::string s) /*override*/
{
	float fdB;
	if (sscanf(s.c_str(), "%f", &fdB) != 1)
		fdB = OFF_DB;
	setDisplayValue(FaderdB2Param(fdB));
}

/*static*/ float FaderGainQuantity::FaderParam2dB(float fParam)
{
	if(fParam <= 0.0f)
		return OFF_DB;
	if(fParam >= FADER_STEPS_F)
		return (float)s_FaderCurve[N_FADER_CURVE - 1].ddB;
	double dParam = (double)fParam;
	for(int i = 1; i < N_FADER_CURVE; i++)
	{
		if(dParam > s_FaderCurve[i].dThresh)
			continue;
		double dRel = (dParam - s_FaderCurve[i - 1].dThresh) / (s_FaderCurve[i].dThresh - s_FaderCurve[i - 1].dThresh);
		double dRet = s_FaderCurve[i - 1].ddB + dRel * (s_FaderCurve[i].ddB - s_FaderCurve[i - 1].ddB);
		// round to .1 dB
		dRet *= 10.0f;
		dRet =  (dRet > 0.0) ? floor(dRet + 0.5) : ceil(dRet - 0.5);
		dRet /= 10;
		if(dRet > -0.1f && dRet < 0.1f)	// have a precise zero value, also with only 127 MIDI steps
			dRet = 0.0f;
		return (float)dRet;
	}
	return (float)s_FaderCurve[N_FADER_CURVE - 1].ddB;
}

/*static*/ float FaderGainQuantity::FaderdB2Param(float fdB)
{
	double ddB = (double)fdB;
	if(ddB <= s_FaderCurve[0].ddB)
		return 0.0f;
	if(ddB >= s_FaderCurve[N_FADER_CURVE -1].ddB)
		return FADER_STEPS_F;
	for(int i = 1; i < N_FADER_CURVE; i++)
	{
		if(ddB > s_FaderCurve[i].ddB)
			continue;
		double dRel = (ddB - s_FaderCurve[i - 1].ddB) / (s_FaderCurve[i].ddB - s_FaderCurve[i - 1].ddB);
		double dRet = s_FaderCurve[i - 1].dThresh + dRel * (s_FaderCurve[i].dThresh - s_FaderCurve[i - 1].dThresh);
		return (float)dRet;
	}
	return FADER_STEPS_F;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PanBalQuantity
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

PanBalQuantity::PanBalQuantity()
{
	snapEnabled = true;
	if (!s_bPanBalInitialized)
	{
		s_bPanBalInitialized = true;

		// Initialize the panning table
		// we go for a -3dB panning law, implemented as sine/cosine curves
		for (int i = 0; i <= MAX_PANBAL - MIN_PANBAL; i++)
		{
			float fAngle = M_PI_2 * (float)i / (float)(MAX_PANBAL - MIN_PANBAL);
			float fSine = sin(fAngle);
			s_fPanR[i] = fSine;
			s_fPanL[MAX_PANBAL - MIN_PANBAL - i] = fSine;
		}

		// Initialize the balance table
		// Values are all 1.0f on one side and droppen with a sine/cosine curve on the other
		for (int i = 0; i <= -MIN_PANBAL; i++)
		{
			s_fBalL[i] = 1.0f;
			s_fBalR[MAX_PANBAL - MIN_PANBAL - i] = 1.0f;
		}
		for (int i = -MIN_PANBAL; i <= MAX_PANBAL - MIN_PANBAL; i++)
		{
			s_fBalL[i] = cos((float)(i + MIN_PANBAL) * M_PI_2 / (float)MAX_PANBAL);
			s_fBalR[MAX_PANBAL - MIN_PANBAL - i] = s_fBalL[i];
		}
	}
}

/*static*/ float PanBalQuantity::GainFactorL(float fParam, bool bIsBalance)
{
	int nParam = ParamToIndex(fParam);
	if (bIsBalance)
		return s_fBalL[nParam];
	else
		return s_fPanL[nParam];
}

/*static*/ float PanBalQuantity::GainFactorR(float fParam, bool bIsBalance)
{
	int nParam = ParamToIndex(fParam);
	if (bIsBalance)
		return s_fBalR[nParam];
	else
		return s_fPanR[nParam];
}

std::string PanBalQuantity::getDisplayValueString() /*override*/
{
	int nParam = (int)getValue();

	char szValue[32];
	if (nParam < -1)
		snprintf(szValue, sizeof(szValue), "Left %d %%", -nParam);
	else if (getValue() > 1)
		snprintf(szValue, sizeof(szValue), "Right %d %%", nParam);
	else
		return "Center";
	return szValue;
}

/*static*/ int PanBalQuantity::ParamToIndex(float fParam)
{
	int nParam = (int)fParam;
	if (nParam >= -1 && nParam <= 1)	// catch middle position with 127 MIDI steps
		nParam = 0;
	return nParam - MIN_PANBAL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SendQuantity
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

SendQuantity::SendQuantity()
{
	if (!s_bSendInitialized)
	{
		s_bSendInitialized = true;
		for (int i = 1; i <= SEND_STEPS; i++) // avoid Value 0 = -infinity dB
		{
			float fFactor = (float)i / (float)(SEND_STEPS);
			s_Send[i].fFactor = fFactor * fFactor;
			s_Send[i].fdB = 20.0f * log10(s_Send[i].fFactor);
		}
	}
}

/*static*/ float SendQuantity::GainFactor(float fParam)
{
	int nParam = (int)(fParam + 0.5f);
	return s_Send[nParam].fFactor;
}

std::string SendQuantity::getDisplayValueString() /*override*/
{
	int nParam = (int)(getValue() + 0.5f);
	if (nParam == 0)
		return "Off";
	char szValue[32];
	snprintf(szValue, sizeof(szValue), "%.1f dB", s_Send[nParam].fdB);
	return szValue;
}

void SendQuantity::setDisplayValueString(std::string s) /*override*/
{
	float fdB;
	float fParam;
	if (s == "Off")
		fParam = 0.0f;
	else if (sscanf(s.c_str(), "%f", &fdB) != 1)
		fParam = 0.0f;
	else if (fdB >= 0.0f)
		fParam = 1.0f;
	else
	{
		float fFactor = pow(10.0f, fdB / 20.0f);
		if (fFactor == 0.0f)
			fParam = 0.0f;
		else
			fParam = sqrt(fFactor);
	}
	setDisplayValue(fParam * SEND_STEPS);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPaudioSlider
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define FADER_Y_OFFSET		(2.0f)

GPaudioFader::GPaudioFader(FaderLength eFaderLength) :
	m_bDarkMode(!settings::preferDarkPanels), // forces update on first step()
	m_nFaderLength(static_cast<int>(eFaderLength))
{
	std::stringstream ss;
	ss << "res/FaderBackground-" << m_nFaderLength << "mm";
	std::string sLight = ss.str() + ".svg";
	std::string sDark = ss.str() + "-dark.svg";
	m_pLightBackgroundSvg = Svg::load(asset::plugin(the_pPluginInstance, sLight));
	m_pDarkBackgroundSvg = Svg::load(asset::plugin(the_pPluginInstance, sDark));

	UpdateDarkMode();
	setHandleSvg(Svg::load(asset::plugin(the_pPluginInstance, "res/FaderKnob.svg")));

	float fYOffset = mm2px(FADER_Y_OFFSET);
	math::Vec vecMinPos(background->box.getWidth() / 2, background->box.getHeight() - fYOffset);
	math::Vec vecMaxPos(background->box.getWidth() / 2, fYOffset);

	//math::Vec margin = math::Vec(3.5, 3.5);
	setHandlePosCentered(vecMinPos, vecMaxPos);
	//background->box.pos = margin;
	box.size = background->box.size;
}

/*static*/ float GPaudioFader::GainFactor(float fParam)
{
	return s_faderValues[(int)(fParam + 0.5)].fFactor;
}

void GPaudioFader::UpdateDarkMode()
{
	if (settings::preferDarkPanels != m_bDarkMode)
	{
		m_bDarkMode = settings::preferDarkPanels;
		if (m_bDarkMode)
			setBackgroundSvg(m_pDarkBackgroundSvg);
		else
			setBackgroundSvg(m_pLightBackgroundSvg);
	}
}

void GPaudioFader::drawLayer(const widget::Widget::DrawArgs &args, int nLayer) /*override*/
{
	if (nLayer == 1)
		draw(args);
}
