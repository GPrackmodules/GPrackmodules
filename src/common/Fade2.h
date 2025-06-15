//
// Fade2 class
// ===========
// smooth fade for a two values (stereo. crossface)
//

#pragma once

class Fade2
{
	///////////////////////////////////////////////////////////////////////////
	// Construction
	///////////////////////////////////////////////////////////////////////////

public:
	Fade2(float& rCoefficient1, float& rCoefficient2, float fMilliseconds, float fInitialValue);
	Fade2(float& rCoefficient1, float& rCoefficient2, float fMilliseconds, float fInitialValue1, float fInitialValue2);

	///////////////////////////////////////////////////////////////////////////
	// Public API
	///////////////////////////////////////////////////////////////////////////

public:
	void SetSamplerate(float fSamplerate);
	void Start(float fTarget) { Start(fTarget, fTarget); }
	void Start(float fTarget1, float fTarget2);
	void CutShort();
	void Stop();
	bool Advance();	// return true if at least one fade is down

	///////////////////////////////////////////////////////////////////////////
	// Private data
	///////////////////////////////////////////////////////////////////////////

private:
	float& m_rCoefficient1;
	float& m_rCoefficient2;
	float m_fFadeMs = 20.0f;
	float m_fInvertedSteps = 1.0f;		// will be computer in SetSamplerate()
	float m_fTarget1 = 0.0f;
	float m_fTarget2 = 0.0f;
	float m_fFadeStep1 = 0.0f;
	float m_fFadeStep2 = 0.0f;
	bool m_bFadingUp1 = false;
	bool m_bFadingUp2 = false;
	bool m_bFadingDown1 = false;
	bool m_bFadingDown2 = false;
};
