//
// class Crossover
// ===============
// 2 way or 3 way crossover
// Currently usinhg 12 dB/oct filters
// Summing of outputs results in a flat frequncy response without dips.
//
// Filter math is optimixrf for simd instructions (128 bit = 4 * float)
//

#pragma once
#include <cmath>

class Crossover
{
////////////////////////////////////////////////////////////
/// Constructor
////////////////////////////////////////////////////////////

public:
	Crossover(bool b24dBPerOct);

////////////////////////////////////////////////////////////
/// Public API
////////////////////////////////////////////////////////////

public:
	void TwoWay(float fSplitHz);
	void ThreeWay(float fSplitHzLo, float fSplitHzHi);
	void Samplerate(float fSamplerate);
	void Reset();
	void Process(float fInput);
	float Bass() const;
	float Mid() const;
	float Treble() const;

////////////////////////////////////////////////////////////
/// Private methods
////////////////////////////////////////////////////////////

private:
	void Frequency(int nChannel, float fHz);
	void CalcCoeffs(int nChannel);

////////////////////////////////////////////////////////////
/// Data
////////////////////////////////////////////////////////////

private:
	enum FilterIndex : int
	{
		BassLoPass = 0,
		MidHiPass,	// goes into MidLoPass
		MidLoPass,
		TrebleHiPass,
		CrossoverBands	// number of bands
	};

	const bool m_b24dBPerOct;
	float m_fSamplerate = 0.0f;

	bool m_bThreeWay = false;
	float m_fFrequency[CrossoverBands] = { 0.0f, 0.0f, 0.0f, 0.0f };

	// Filter coefficients
	// Feedback
	simd::float_4 m_f4A1;
	simd::float_4  m_f4A2;
	// // Feedworward
	simd::float_4 m_f4B0;
	simd::float_4 m_f4B1;
	simd::float_4 m_f4B2;

	// Filter state variables
	// Input history
	simd::float_4 m_f4X1 = { 0.0f, 0.0f, 0.0f, 0.0f };
	simd::float_4 m_f4X2 = { 0.0f, 0.0f, 0.0f, 0.0f };
	// // Output history
	simd::float_4 m_f4Y1 = { 0.0f, 0.0f, 0.0f, 0.0f };
	simd::float_4 m_f4Y2 = { 0.0f, 0.0f, 0.0f, 0.0f };

	simd::float_4 m_f4Output = { 0.0f, 0.0f, 0.0f, 0.0f };
};

