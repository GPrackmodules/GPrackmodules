//
// MultiControl class
// ==================
// Declaration of template classses MultiContorlModule and MultiControlWidget
// Since this is a template class (actually a struct in the VCV spirit=)) it
// At the end it includes MultiControl.ipp where the implementaion happens
//

#pragma once

#include <array>

template <int NUM_INDEV>
struct MultiControlModule : Module
{
////////////////////////////////////////////////////////////////////////////////////////////
// Public enums
////////////////////////////////////////////////////////////////////////////////////////////

public:
	enum ParamId
	{
		ParamOutputChannels, // currently only set from menu
		ParamA,
		ParamB,
		ParamC,
		NumParams
	};
	enum InputId
	{
		InputVOctA = 0,		// must match OutputVOct
		InputGateA,
		InputVelocityA,
		InputAftertouchA,
		InputModulationA,
		InputPitchA,
		InputCV1A,
		InputCV2A,
		InputCV3A,	// 3 and 4 reserved for future expansion, don't remove or cabbling in patches will break
		InputCV4A,
		NumColumnInputs,
		// multiple values here for inputs B, C etc. if present
		InputEnableA = NUM_INDEV * NumColumnInputs,
		// multiple values here for inputs B, C etc. if present
		NumInputs = InputEnableA + NUM_INDEV
	};
	enum OutputId
	{
		OutputVOct = 0,		// must match InputVOctA
		OutputGate,
		OutputVelocity,
		OutputAftertouch,
		OutputModulation,
		OutputAPitch,
		OutputCV1,
		OutputCV2,
		OutputAftertouchM,
		OutputModulationM,
		// OutputCV3,	// 3 and 4 reserved for future expansion, don't remove or cabbling in patches will break
		// OutputCV4,
		NumOutputs
	};
	enum LightId
	{
		LightA,
		// multiple values here for inputs B, C etc. if present
		NumLights = NUM_INDEV
	};

////////////////////////////////////////////////////////////////////////////////////////////
// Construction
////////////////////////////////////////////////////////////////////////////////////////////

public:
	MultiControlModule();

////////////////////////////////////////////////////////////////////////////////////////////
// Public API
////////////////////////////////////////////////////////////////////////////////////////////

public:
	void NumOutputChannels(int nChannels) { m_nNewOutputChannels = nChannels; }
	void process(const ProcessArgs& args) override;
	std::string OutputChannels() const { return rack::string::f("%d", m_nOutputChannels); }
	int NumOutputChannels() const { return m_nOutputChannels; }

////////////////////////////////////////////////////////////////////////////////////////////
// Private methods
////////////////////////////////////////////////////////////////////////////////////////////

private:
	void HandleEnable(const ProcessArgs& args, bool bForce);
	bool MapInput(int nInChannel, int nDevice);
	bool ReleaseInput(int nInChannel, int nDevice, int64_t nFrame);	// Mapping persists, but channel will be added to map of release outputs
	void UpdateNumOutputChannels(int nChannels, int64_t nFrame);
	bool Enabled(int nDevice) const { return m_bEnable[nDevice] || m_bEnableCV[nDevice]; }

////////////////////////////////////////////////////////////////////////////////////////////
// Data
////////////////////////////////////////////////////////////////////////////////////////////

private:
	struct InputState
	{
		bool bGated = false;
		int nMappedOutChannel = -1;	// -1 => not mapped
	};

	struct OutputState
	{
		int nDevice = 0;				// mapped to A or B input
		int nMappedInChannel = -1;	// -1 => no input mapped, < NumCahnnels = A, >= NumCahnnels = B
	};

	bool m_bInitialized = false;

	int m_nOutputChannels = 0;
	int m_nNewOutputChannels = 0;

	const bool m_bStartOnGatesAfterEnable = true; // const for now

	std::array<bool, NUM_INDEV> m_bEnable;
	std::array<bool, NUM_INDEV> m_bEnableCV;

	InputState m_Inputs[NUM_INDEV][PORT_MAX_CHANNELS];
	OutputState m_Outputs[PORT_MAX_CHANNELS];
	bool m_bOutConnected[OutputModulationM + 1 - OutputVOct] = { false, false, false, false, false, false, false, false, false, false };

	// map containing outputs with inactive gate, sorted by time of gate release (in frame counters)
	// begin() points to the output that didn't have gate set for the longest time.
	multimap<int64_t, int> m_mapReleasedOutputs;
};

template <int NUM_INDEV>
struct MultiControlWidget : ModuleWidget
{
public:
	MultiControlWidget(MultiControlModule<NUM_INDEV>* pModule);
	void appendContextMenu(Menu* pMainMenu) override;
};

Model* the_pDualControlModel = createModel<MultiControlModule<2>, MultiControlWidget<2>>("DualControl");
Model* the_pTripleControlModel = createModel<MultiControlModule<3>, MultiControlWidget<3>>("TripleControl");

#include "MultiControl/MultiControl.ipp"

