//
// Fade class
// ==========
// smooth fade for a single value
//

#pragma once

class Fade
{
///////////////////////////////////////////////////////////////////////////
// Construction
///////////////////////////////////////////////////////////////////////////

public:
	Fade(float& rCoefficient, float fMilliseconds, float fInitialValue);

///////////////////////////////////////////////////////////////////////////
// Public API
///////////////////////////////////////////////////////////////////////////

public:
	void SetSamplerate(float fSamplerate);
	void Start(float fTarget);
	void CutShort();
	void Stop();
	bool Advance(); // return true if done
	bool Fading() const { return m_bFadingDown || m_bFadingUp;}
	float Target() const { return m_fTarget; }

///////////////////////////////////////////////////////////////////////////
// Private data
///////////////////////////////////////////////////////////////////////////

private:
	float& m_rCoefficient;
	float m_fFadeMs = 20.0f;
	float m_fInvertedSteps = 1.0f;		// will be computer in SetSamplerate()
	float m_fTarget = 0.0f;
	float m_fFadeStep = 0.0f;
	bool m_bFadingUp = false;
	bool m_bFadingDown = false;
};
