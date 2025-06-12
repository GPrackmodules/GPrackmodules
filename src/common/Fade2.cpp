//
// Fade2 class
// ===========
// smooth fade for a two values (stereo. crossface)
//

#include "Fade2.h"

Fade2::Fade2(float& rCoefficient1, float& rCoefficient2, float fMilliseconds, float fInitialValue) :
	Fade2(rCoefficient1, rCoefficient2, fMilliseconds, fInitialValue, fInitialValue)
{
}

Fade2::Fade2(float& rCoefficient1, float& rCoefficient2, float fMilliseconds, float fInitialValue1, float fInitialValue2) :
	m_rCoefficient1(rCoefficient1),
	m_rCoefficient2(rCoefficient2),
	m_fFadeMs(fMilliseconds)
{
	rCoefficient1 = fInitialValue1;
	rCoefficient2 = fInitialValue2;
}

void Fade2::SetSamplerate(float fSamplerate)
{
	float fSteps = m_fFadeMs * fSamplerate / 1000.0f;
	m_fInvertedSteps = 1.0f / fSteps;
}

void Fade2::Start(float fTarget1, float fTarget2)
{
	bool bNoChange1 = false;
	if (fTarget1 < m_rCoefficient1)
	{
		m_bFadingDown1 = true;
		m_bFadingUp1 = false;
	}
	else if (fTarget1 > m_rCoefficient1)
	{
		m_bFadingDown1 = false;
		m_bFadingUp1 = true;
	}
	else
		bNoChange1 = true;
	if (!bNoChange1)
	{
		m_fFadeStep1 = (fTarget1 - m_rCoefficient1) * m_fInvertedSteps;
		m_fTarget1 = fTarget1;
	}

	bool bNoChange2 = false;
	if (fTarget2 < m_rCoefficient2)
	{
		m_bFadingDown2 = true;
		m_bFadingUp2 = false;
	}
	else if (fTarget2 > m_rCoefficient2)
	{
		m_bFadingDown2 = false;
		m_bFadingUp2 = true;
	}
	else
	{
		bNoChange2 = true;
		if (bNoChange1)
			return;
	}
	if (!bNoChange2)
	{
		m_fFadeStep2 = (fTarget2 - m_rCoefficient2) * m_fInvertedSteps;
		m_fTarget2 = fTarget2;
	}
}

void Fade2::CutShort()
{
	m_rCoefficient1 = m_fTarget1;
	m_bFadingUp1 = false;
	m_bFadingDown1 = false;
	m_rCoefficient2 = m_fTarget2;
	m_bFadingUp2 = false;
	m_bFadingDown2 = false;
}

void Fade2::Stop()
{
	m_bFadingUp1 = false;
	m_bFadingDown1 = false;
	m_bFadingUp2 = false;
	m_bFadingDown2 = false;
}

bool Fade2::Advance()
{
	bool bReturn = false;
	if (m_bFadingDown1)
	{
		m_rCoefficient1 += m_fFadeStep1;
		if (m_rCoefficient1 <= m_fTarget1)
		{
			m_rCoefficient1 = m_fTarget1;
			m_bFadingDown1 = false;
			bReturn = true;
		}
	}
	else if (m_bFadingUp1)
	{
		m_rCoefficient1 += m_fFadeStep1;
		if (m_rCoefficient1 >= m_fTarget1)
		{
			m_rCoefficient1 = m_fTarget1;
			m_bFadingUp1 = false;
			bReturn = true;
		}
	}

	if (m_bFadingDown2)
	{
		m_rCoefficient2 += m_fFadeStep2;
		if (m_rCoefficient2 <= m_fTarget2)
		{
			m_rCoefficient2 = m_fTarget2;
			m_bFadingDown2 = false;
			bReturn = true;
		}
	}
	else if (m_bFadingUp2)
	{
		m_rCoefficient2 += m_fFadeStep2;
		if (m_rCoefficient2 >= m_fTarget2)
		{
			m_rCoefficient2 = m_fTarget2;
			m_bFadingUp2 = false;
			bReturn = true;
		}
	}
	return bReturn;
}
