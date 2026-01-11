//
// Rotor4 class
// ============
// Handles rotation with slow/fast/vari speeds and Ramp up/Ramp down
// Yields sine values (+/- 1.0f) for left and tight channels
//

#pragma once

#define ROTOR4_BANDS	4

class Rotor4
{
////////////////////////////////////////////////////////
/// enum for target speeds
////////////////////////////////////////////////////////

public:
	enum class SpeedTarget : int
	{
		Stop = 0,
		Slow = 1,
		Fast = 2,
		VariSpeed = 3
	};

////////////////////////////////////////////////////////
/// Construction
////////////////////////////////////////////////////////

public:
	Rotor4(class RotaryModule* pModule);

////////////////////////////////////////////////////////
/// Public API
////////////////////////////////////////////////////////

public:
	// parameters
	void Samplerate(float fSamplerate);
	void SlowRPM(int nChannel, float fRPM);
	void FastRPM(int nChannel, float fRPM);
	void RampupTime(int nChannel, float fSeconds);			// Rampdown is determined from Rampup
	void PhaseOffsetLR(float fDegree);

	// operation
	void TargetSpeed(SpeedTarget eTarget, float fVariSpeed	= 0.0f);
	const float& OutputL(int nChannel) const { return m_f4OutputL.s[nChannel]; }
	const float& OutputR(int nChannel) const { return m_f4OutputR.s[nChannel]; }
	const simd::float_4& OutputL4() const { return m_f4OutputL; }
	const simd::float_4& OutputR4() const { return m_f4OutputR; }

	void Advance();

	// obsolete
	void Frequency(int nChannel, float fHertz);
	void Frequency(const simd::float_4& f4Hertz, const simd::float_4& f4TargetHertz);

////////////////////////////////////////////////////////
/// Internal methods
////////////////////////////////////////////////////////

private:
	void Update(SpeedTarget eTarget, float fVariSpeed = 0.0f);
	void StartRamping(int nChannel, SpeedTarget eTargetSpeed);

////////////////////////////////////////////////////////
/// Data
////////////////////////////////////////////////////////

private:
	enum class FadeMode : int
	{
		Stopped,
		RampUp,
		RampDown
	};

	// constructor
	const class RotaryModule* const m_pModule;
	// function parameters
	float m_fSamplerate = 1.0f;
	float m_fSlowRPM[ROTOR4_BANDS] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float m_fSlowPhaseStep[ROTOR4_BANDS] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float m_fFastRPM[ROTOR4_BANDS] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float m_fFastPhaseStepPrevious[ROTOR4_BANDS] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float m_fFastPhaseStep[ROTOR4_BANDS] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float m_fRampupTime[ROTOR4_BANDS] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float m_fPhaseOffsetLR = 0.0f;	// offset between left and right channels
	SpeedTarget m_eTargetSpeed = SpeedTarget::Stop;
	float m_fVariSpeed = 0.0f;	// 0..1 corresponds to Stop..Fast

	// derived
	float m_fInvertedSamplerate;

	// Operation
	FadeMode m_eFadeMode[ROTOR4_BANDS] = { FadeMode::Stopped, FadeMode::Stopped, FadeMode::Stopped, FadeMode::Stopped };
	float m_fTargetPhaseStep[ROTOR4_BANDS] = { 0.0f, 0.0f, 0.0f, 0.0f };
	simd::float_4 m_f4PhaseStepDelta = { 0.0f, 0.0f, 0.0f, 0.0f };	// increment/decrement of PhaseStep while Rampning up or down
	simd::float_4 m_f4FadeStartPhaseStep = { 0.0f, 0.0f, 0.0f, 0.0f };	// position of the rotors when fade started
	int64_t m_nFadeStartFrame = 0;

	// Phase values here are normalized to 0.1 = 360 degrees = 2 * PI
	simd::float_4 m_f4Phase = { 0.0f, 0.0f, 0.0f, 0.0f };		// runs from 0..1.0f
	simd::float_4 m_f4Frequency = { 1.0f, 1.0f, 1.0f, 1.0f };	// frequency in Hz
	simd::float_4 m_f4TargetFrequency = { 1.0f, 1.0f, 1.0f, 1.0f };	// frequency in Hz
	simd::float_4 m_f4PhaseStep = { 0.0f, 0.0f, 0.0f, 0.0f };	// runs from 0..1.0f
	// simd::float_4 m_f4LastPhaseStep = { 0.0f, 0.0f, 0.0f, 0.0f };	// runs from 0..1.0f
	simd::float_4 m_f4OutputL;
	simd::float_4 m_f4OutputR;									// only valid if m_bStereo
	float m_fParkPosition = 0.0f;								// offset between left and right channels
	float m_fParkStep = 0.0f;									// final phase step befor reaching park position
	float m_fStopPosition = 0.0f;								// LFO runs to this position if frequency is set to zero while running
};