//
// Rotor4 class
// ============
// Handles rotation with slow/fast/vari speeds and Ramp up/Ramp down
// Yields sine values (+/- 1.0f) for left and tight channels
//


#include "plugin.hpp"
#include "Rotor4.h"
#include "Rotary.h"

#define RAMP_DOWN_FACTOR				(1.25f)
#define Ramp_DOWN_FACTOR_FAST_TO_SLOW	(0.5f)

static float s_fRPM2Hz = 1.0f / 60.0f;

Rotor4::Rotor4(RotaryModule* pModule) :
	m_pModule(pModule)
{

}

void Rotor4::Samplerate(float fSamplerate)
{
	if (fSamplerate == m_fSamplerate)
		return;
	m_fSamplerate = fSamplerate;
	m_fInvertedSamplerate = 1.0f / fSamplerate;
}

void Rotor4::SlowRPM(int nChannel, float fRPM)
{
	m_fSlowRPM[nChannel] = fRPM;
	m_fSlowPhaseStep[nChannel] = fRPM * s_fRPM2Hz * m_fInvertedSamplerate;
	if (m_eTargetSpeed == SpeedTarget::Slow)
		Update(m_eTargetSpeed);
}

void Rotor4::FastRPM(int nChannel, float fRPM)
{
	// printf("FastRPM %d=%f\n", nChannel, fRPM);

	m_fFastRPM[nChannel] = fRPM;
	m_fFastPhaseStepPrevious[nChannel] = m_fFastPhaseStep[nChannel];	// used to calc ramping down differently (only once)
	m_fFastPhaseStep[nChannel] = fRPM * s_fRPM2Hz * m_fInvertedSamplerate;
	if (m_eTargetSpeed == SpeedTarget::Fast || m_eTargetSpeed == SpeedTarget::VariSpeed)
		Update(m_eTargetSpeed, m_fVariSpeed);
	m_fFastPhaseStepPrevious[nChannel] = 0.0f;
}

void Rotor4::RampupTime(int nChannel, float fSeconds)
{
	m_fRampupTime[nChannel] = fSeconds;
}

void Rotor4::TargetSpeed(SpeedTarget eTarget, float fVariSpeed /*= 0.0f*/)
{
	if (eTarget == m_eTargetSpeed && fVariSpeed == m_fVariSpeed)
		return;
	Update(eTarget, fVariSpeed);
}

void Rotor4::Frequency(int nChannel, float fHertz)
{
	m_f4Frequency[nChannel] = fHertz;
	m_f4PhaseStep[nChannel] = fHertz * m_fInvertedSamplerate;
}

void Rotor4::Frequency(const simd::float_4& f4Hertz, const simd::float_4& f4TargetHertz)
{
	m_f4Frequency = f4Hertz;
	m_f4TargetFrequency = f4TargetHertz;
	m_f4PhaseStep = f4Hertz * m_fInvertedSamplerate;
}

void Rotor4::Update(SpeedTarget eTarget, float fVariSpeed)
{
	switch (eTarget)
	{
		case SpeedTarget::Slow:
			for (int i = 0; i < 4; i++)
				m_fTargetPhaseStep[i] = m_fSlowPhaseStep[i];
			break;

		case SpeedTarget::Fast:
			for (int i = 0; i < 4; i++)
				m_fTargetPhaseStep[i] = m_fFastPhaseStep[i];
			break;

		case SpeedTarget::VariSpeed:
			for (int i = 0; i < 4; i++)
				m_fTargetPhaseStep[i] = fVariSpeed * m_fFastPhaseStep[i];
			break;

		default:
			for (int i = 0; i < 4; i++)
				m_fTargetPhaseStep[i] = 0.0f;
			break;
	}
	for (int i = 0; i < 4; i++)
		StartRamping(i, eTarget);
	m_eTargetSpeed = eTarget;
	m_fVariSpeed = fVariSpeed;
}

void Rotor4::StartRamping(int nChannel, SpeedTarget eTargetSpeed)
{
	m_f4FadeStartPhaseStep.s[nChannel] = m_f4PhaseStep.s[nChannel];
	m_nFadeStartFrame = m_pModule->CurrentFrame();
	if (m_fTargetPhaseStep[nChannel] == m_f4PhaseStep.s[nChannel])
	{
		m_eFadeMode[nChannel] = FadeMode::Stopped;
		m_f4PhaseStepDelta[nChannel] = 0.0f;
	}
	else if (m_fTargetPhaseStep[nChannel] < m_f4PhaseStep.s[nChannel])
	{
		m_eFadeMode[nChannel] = FadeMode::RampDown;
		float fTime;
		if (eTargetSpeed == SpeedTarget::Slow)
			fTime = m_fRampupTime[nChannel] * Ramp_DOWN_FACTOR_FAST_TO_SLOW;	// fast to slow Ramps down a little faster than parking completely
		else
			fTime = m_fRampupTime[nChannel] * RAMP_DOWN_FACTOR;
		// printf("TimeDown1 = %f, Setp = %f, TargetStep = %f, FastStep=%f\n", fTime, m_f4PhaseStep.s[nChannel], m_fTargetPhaseStep[nChannel], m_fFastPhaseStep[nChannel]);
		fTime *= (m_f4PhaseStep.s[nChannel] - m_fTargetPhaseStep[nChannel]);
		if (m_fFastPhaseStepPrevious[nChannel] > 0.0f)
			fTime /= m_fFastPhaseStepPrevious[nChannel];
		else
			fTime /= m_fFastPhaseStep[nChannel];
		// printf("TimeDown2 = %f\n", fTime);
		float fSamples = std::ceil(fTime * m_fSamplerate);
		if (fSamples < 1.0f)
			fSamples = 1.0f;
		m_f4PhaseStepDelta.s[nChannel] = (m_fTargetPhaseStep[nChannel] - m_f4PhaseStep.s[nChannel]) / fSamples;
		if (m_fTargetPhaseStep[nChannel] == 0.0f && m_f4PhaseStepDelta.s[nChannel] != 0.0f)
		{
			// recalculate Rampdown so the rotor will stop at the park position (as close as floating point precision allows)
			float fParkRotations = m_f4PhaseStep[nChannel] * (fSamples + 1.0f) / 2.0f;
			float fParkPosition = m_f4Phase.s[nChannel] + fParkRotations;
			fParkPosition -= floor(fParkPosition);
			if (fParkPosition < m_fParkPosition)
				fParkRotations += m_fParkPosition - fParkPosition;
			else
				fParkRotations += 1.0f + m_fParkPosition - fParkPosition;
			fSamples = 1.0f + 2.0f * fParkRotations / m_f4PhaseStep[nChannel];
			m_f4PhaseStepDelta.s[nChannel] = - m_f4PhaseStep.s[nChannel] / fSamples;
		}
	}
	else if (m_fTargetPhaseStep[nChannel] > m_f4PhaseStep.s[nChannel])
	{
		m_eFadeMode[nChannel] = FadeMode::RampUp;
		float fTime;
		if (eTargetSpeed == SpeedTarget::Slow)
			fTime = m_fRampupTime[nChannel] * (m_fTargetPhaseStep[nChannel] - m_f4PhaseStep.s[nChannel]) / m_fSlowPhaseStep[nChannel];
		else
			fTime = m_fRampupTime[nChannel] * (m_fTargetPhaseStep[nChannel] - m_f4PhaseStep.s[nChannel]) / m_fFastPhaseStep[nChannel];
		// printf("TimeUo = %f, Setp = %f, TargetStep = %f, FastStep=%f\n", fTime, m_f4PhaseStep.s[nChannel], m_fTargetPhaseStep[nChannel], m_fFastPhaseStep[nChannel]);
		float fSamples = std::ceil(fTime * m_fSamplerate);
		if (fSamples < 1.0f)
			fSamples = 1.0f;
		m_f4PhaseStepDelta[nChannel] = (m_fTargetPhaseStep[nChannel] - m_f4PhaseStep.s[nChannel]) / fSamples;
	}
}

void Rotor4::PhaseOffsetLR(float fDegree)
{
	m_fPhaseOffsetLR = fDegree / 360.0f;
	m_fParkPosition = 0.25 - (m_fPhaseOffsetLR / 2.0f);	// park in the middle between left and right mics
	if (m_fParkPosition < 0.0f)
		m_fParkPosition += 1.0f;
	for (int c = 0; c < ROTOR4_BANDS; c++)
	{
		if (m_fTargetPhaseStep[c] == 0.0f)
		{
			if (m_eFadeMode[c] == FadeMode::Stopped)
				m_f4Phase.s[c] = m_fParkPosition;
			else if (m_eFadeMode[c] == FadeMode::RampDown)
			 	StartRamping(c, SpeedTarget::Stop); // recalc Ramp down to park position
		}
	}
}

void Rotor4::Advance()
{
	// Rampup/Rampdown
	simd::float_4 f4FadeFrames = (float)(m_pModule->CurrentFrame() - m_nFadeStartFrame);
	m_f4PhaseStep = simd::ifelse(m_f4PhaseStepDelta != 0.0f, m_f4FadeStartPhaseStep + f4FadeFrames * m_f4PhaseStepDelta, m_f4PhaseStep);
	for (int c = 0; c < ROTOR4_BANDS; c++)
	{
		// have to do this w/o simd instructions, since some channels might be Rampning up while others Ramp down
		if (m_eFadeMode[c] == FadeMode::RampDown)
		{
			if (m_f4PhaseStep.s[c] <= m_fTargetPhaseStep[c])
			{
				m_f4PhaseStep.s[c] = m_fTargetPhaseStep[c];
				m_f4PhaseStepDelta.s[c] = 0.0f;
				m_eFadeMode[c] = FadeMode::Stopped;
				if (m_fTargetPhaseStep[c] == 0.0f)
				{
					// printf("%d: Stopped at %f\n", c + 1, m_f4Phase.s[c]); // !!!!!!!!!!!!!!!!!!!
					m_f4Phase.s[c] = m_fParkPosition;
				}
			}
		}
		else if (m_eFadeMode[c] == FadeMode::RampUp)
		{
			if (m_f4PhaseStep.s[c] >= m_fTargetPhaseStep[c])
			{
				m_f4PhaseStep.s[c] = m_fTargetPhaseStep[c];
				m_f4PhaseStepDelta.s[c] = 0.0f;
				m_eFadeMode[c] = FadeMode::Stopped;
			}
		}
	}
	m_f4Phase += m_f4PhaseStep;
	m_f4Phase = simd::ifelse(m_f4Phase >= 1.0f, m_f4Phase - 1.0f, m_f4Phase);
	m_f4OutputL = simd::sin(2.0f * M_PI * m_f4Phase);
	simd::float_4 f4PhaseR = m_f4Phase + m_fPhaseOffsetLR;
	f4PhaseR = simd::ifelse(f4PhaseR >= 1.0f, f4PhaseR - 1.0f, f4PhaseR);
	m_f4OutputR = simd::sin(2.0f * M_PI * f4PhaseR);
}
