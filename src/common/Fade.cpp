//
// Fade class
// ==========
// smooth fade for a single value
//

#include "Fade.h"

Fade::Fade(float& rCoefficient, float fMilliseconds, float fInitialValue) :
	m_rCoefficient(rCoefficient),
	m_fFadeMs(fMilliseconds)
{
	rCoefficient = fInitialValue;
}

void Fade::SetSamplerate(float fSamplerate)
{
	float fSteps = m_fFadeMs * fSamplerate / 1000.0f;
	m_fInvertedSteps = 1.0f / fSteps;
}

void Fade::Start(float fTarget)
{
	if (fTarget == m_rCoefficient)
		return;

	if (fTarget < m_rCoefficient)
	{
		m_bFadingDown = true;
		m_bFadingUp = false;
	}
	else if (fTarget > m_rCoefficient)
	{
		m_bFadingDown = false;
		m_bFadingUp = true;
	}
	m_fFadeStep = (fTarget - m_rCoefficient) * m_fInvertedSteps;
	m_fTarget = fTarget;
}

void Fade::CutShort()
{
	m_rCoefficient = m_fTarget;
	m_bFadingUp = false;
	m_bFadingDown = false;
}

void Fade::Stop()
{
	m_bFadingUp = false;
	m_bFadingDown = false;
}

bool Fade::Advance()
{
	bool bReturn = false;
	if (m_bFadingDown)
	{
		m_rCoefficient += m_fFadeStep;
		if (m_rCoefficient <= m_fTarget)
		{
			m_rCoefficient = m_fTarget;
			m_bFadingDown = false;
			bReturn = true;
		}
	}
	else if (m_bFadingUp)
	{
		m_rCoefficient += m_fFadeStep;
		if (m_rCoefficient >= m_fTarget)
		{
			m_rCoefficient = m_fTarget;
			m_bFadingUp = false;
			bReturn = true;
		}
	}
	return bReturn;
}
