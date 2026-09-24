//
// MultiControl class
// ==================
// Implementation of template classses MultiContorlModule and MultiControlWidget
// Will be included at the end of MultiControl.h
//
// Also defines DualoControlModel and tripleControlModel as template instances
// with NUM_INDEV 2 and 3.
//

#pragma once

template<int NUM_INDEV>
MultiControlModule<NUM_INDEV>::MultiControlModule()
{
	m_bEnable.fill(true);
	m_bEnableCV.fill(false);

	int nNumIn = NumInputs;
	config(NumParams, nNumIn, NumOutputs, NumLights);
	configParam(ParamOutputChannels, 1.0f, static_cast<float>(PORT_MAX_CHANNELS), static_cast<float>(PORT_MAX_CHANNELS), "Number of output channels");
	configParam(ParamA, 0.0f, 1.0f, 1.0f, "Enable Device A");
	paramQuantities[ParamA]->snapEnabled = true;
	configParam(ParamB, 0.0f, 1.0f, 1.0f, "Enable Device B");
	paramQuantities[ParamB]->snapEnabled = true;
	configParam(ParamC, 0.0f, 1.0f, 1.0f, "Enable Device C");
	paramQuantities[ParamC]->snapEnabled = true;

	char cDevice = 'A';
	for (int nDevice = 0; nDevice < NUM_INDEV; nDevice++)
	{
		int nOffset = nDevice * NumColumnInputs;
		configInput(nOffset + InputVOctA, std::string("V/Oct ")  + cDevice);
		configInput(nOffset + InputGateA, std::string("Gate ")  + cDevice);
		configInput(nOffset + InputVelocityA, std::string("Velocity ")  + cDevice);
		configInput(nOffset + InputAftertouchA, std::string("Aftertouch ")  + cDevice);
		configInput(nOffset + InputModulationA, std::string("Modulation ")  + cDevice);
		configInput(nOffset + InputPitchA, std::string("Pitchbend ")  + cDevice);
		configInput(nOffset + InputCV1A, std::string("Other CV1 ")  + cDevice);
		configInput(nOffset + InputCV2A, std::string("Other CV2 ")  + cDevice);
		configInput(nDevice + InputEnableA, std::string("Enable Device ") + cDevice + " CV");
		cDevice++;
	}

	configOutput(OutputVOct, "V/Oct");
	configOutput(OutputGate, "Gate");
	configOutput(OutputVelocity, "Velocity");
	configOutput(OutputAftertouch, "Aftertouch");
	configOutput(OutputModulation, "Modulation");
	configOutput(OutputAPitch, "Pitchbend");
	configOutput(OutputCV1, "Other CV 1");
	configOutput(OutputCV2, "Other CV 2");
	configOutput(OutputAftertouchM, "Monophonic Aftertouch");
	configOutput(OutputModulationM, "Monophonic Modulation");
}

template<int NUM_INDEV>
void MultiControlModule<NUM_INDEV>::process(const ProcessArgs& args) /*override*/
{
	if (!m_bInitialized)
	{
		int nOutputChannels = iround(params[ParamOutputChannels].getValue());
		UpdateNumOutputChannels(nOutputChannels, args.frame);
		HandleEnable(args, true);
		m_bInitialized = true;
	}
	else
		HandleEnable(args, false);

	if (m_nNewOutputChannels > 0)
	{
		UpdateNumOutputChannels(m_nNewOutputChannels, args.frame);
		m_nNewOutputChannels = 0;
	}

	// Handle Gate inputs to define mapping of channels
	for (int nDevice = 0; nDevice < NUM_INDEV; nDevice++)
	{
		if (!m_bStartOnGatesAfterEnable && !Enabled(nDevice))
			continue;
		Input& inGate = inputs[InputGateA + nDevice * NumColumnInputs];
		int nInChannels = inGate.getChannels();
		if (nInChannels == 1)
		{
			if (Enabled(nDevice))
			{
				InputState& inState = m_Inputs[nDevice][0];
				if (inState.nMappedOutChannel == -1)
					MapInput(0, nDevice);
			}
		}
		else
		{
			for (int nInChannel = 0; nInChannel < nInChannels; nInChannel++)
			{
				bool bGate = inGate.getVoltage(nInChannel) > 1.0f;
				if (bGate != m_Inputs[nDevice][nInChannel].bGated)
				{
					m_Inputs[nDevice][nInChannel].bGated = bGate;
					if (m_bStartOnGatesAfterEnable && !Enabled(nDevice))
						continue;
					if (bGate)
						MapInput(nInChannel, nDevice);
					else
						ReleaseInput(nInChannel, nDevice, args.frame);
				}
			}
		}
	}

	// refresh output channel count when cable is plugged in
	for (int nOutSocket = OutputVOct; nOutSocket <= OutputModulationM; ++nOutSocket)
	{
		bool bConnected = outputs[nOutSocket].isConnected();
		if (bConnected != m_bOutConnected[nOutSocket])
		{
			m_bOutConnected[nOutSocket] = bConnected;
			if (bConnected)
			{
				if (nOutSocket >= OutputAftertouchM)
					outputs[nOutSocket].setChannels(1);
				else
					outputs[nOutSocket].setChannels(m_nOutputChannels);
			}
		}
	}

	// go through outputs and forward voltages from associated inputs or set to zero if not mapped
	float fMaxAftertouch = 0.0f;
	float fMaxModulation = 0.0f;

	// always apply mono mod inputs to mono output, even if no channel is mapped from A or B respectivley
	for (int nDevice = 0; nDevice < NUM_INDEV; nDevice++)
	{
		if (Enabled(nDevice))
		{
			int nInput = InputModulationA + nDevice * NumColumnInputs;
			if (inputs[nInput].isConnected() && inputs[nInput].getChannels() >= 1)
			{
				float fIn = inputs[nInput].getVoltage(0);
				fMaxModulation = std::max(fMaxModulation, fIn);
			}
		}
	}

	// forward input voltages to associated output channel
	for (int nOutChn = 0; nOutChn < m_nOutputChannels; nOutChn++)
	{
		const OutputState& stOut = m_Outputs[nOutChn];
		if (stOut.nMappedInChannel < 0)
			continue;
		int nInputOffset = stOut.nDevice * NumColumnInputs;
		for (int nOutSocket = OutputVOct; nOutSocket <= OutputCV2; ++nOutSocket)
		{
			if (!outputs[nOutSocket].isConnected())
				continue;
			Input& input = inputs[nOutSocket + nInputOffset];
			float fVoltage;
			if (input.getChannels() <= 1)
				fVoltage = input.getVoltage(0);
			else
				fVoltage = input.getVoltage(stOut.nMappedInChannel);
			outputs[nOutSocket].setVoltage(fVoltage, nOutChn);
			if (nOutSocket == OutputAftertouch)
			{
				if (fVoltage > fMaxAftertouch)
					fMaxAftertouch = fVoltage;
			}
			else if (nOutSocket == OutputModulation)
			{
				if (fVoltage > fMaxModulation)
					fMaxModulation = fVoltage;
			}
		}
	}
	outputs[OutputAftertouchM].setVoltage(fMaxAftertouch);
	outputs[OutputModulationM].setVoltage(fMaxModulation);
}

template<int NUM_INDEV>
void MultiControlModule<NUM_INDEV>::HandleEnable(const ProcessArgs& args, bool bForce)
{
	for (int nDevice = 0; nDevice < NUM_INDEV; nDevice++)
	{
		bool bEnable = params[ParamA + nDevice].getValue() > 0.0f;
		bool bEnableCV = inputs[InputEnableA + nDevice].getVoltage() > 1.0f;
		if (bEnable == m_bEnable[nDevice] && bEnableCV == m_bEnableCV[nDevice] && !bForce)
			continue;
		m_bEnable[nDevice] = bEnable;
		m_bEnableCV[nDevice] = bEnableCV;
		lights[nDevice].setBrightness(m_bEnable[nDevice] ? 1.0f : 0.2f);
		if (!Enabled(nDevice))
		{
			// Clear gates and mappings for this device
			Input& inGate = inputs[InputGateA + nDevice * NumColumnInputs];
			int nInChannels = inGate.getChannels();
			for (int nInChannel = 0; nInChannel < nInChannels; nInChannel++)
			{
				if (m_Inputs[nDevice][nInChannel].bGated)
				{
					m_Inputs[nDevice][nInChannel].bGated = false;
					ReleaseInput(nInChannel, nDevice, args.frame);
				}
				if (m_Inputs[nDevice][nInChannel].nMappedOutChannel >= 0)
				{
					int nOut = m_Inputs[nDevice][nInChannel].nMappedOutChannel;
					outputs[OutputGate].setVoltage(0.0f, nOut);
					m_Inputs[nDevice][nInChannel].nMappedOutChannel = -1;
					m_Outputs[nOut].nMappedInChannel = -1;
				}
			}
		}
	}
}

template <int NUM_INDEV>
bool MultiControlModule<NUM_INDEV>::MapInput(int nInChannel, int nDevice)
{
	if (m_mapReleasedOutputs.empty()) // no channel available for use
		return false;
	auto itBegin = m_mapReleasedOutputs.begin();
	int nOutCh = itBegin->second;
	for (int o = 0; o < m_nOutputChannels; o++)
	{
		if (m_Outputs[o].nMappedInChannel == nInChannel && m_Outputs[o].nDevice == nDevice) // remove input from other channels
		{
			outputs[OutputGate].setVoltage(0.0f, o);
			outputs[OutputAftertouch].setVoltage(0.0f, o);
			m_Outputs[o].nMappedInChannel = -1;
		}
	}
	m_Outputs[nOutCh].nDevice = nDevice;
	m_Outputs[nOutCh].nMappedInChannel = nInChannel;
	m_Inputs[nDevice][nInChannel].nMappedOutChannel = nOutCh;
	m_mapReleasedOutputs.erase(itBegin);
	return true;
}

template <int NUM_INDEV>
bool MultiControlModule<NUM_INDEV>::ReleaseInput(int nInChannel, int nDevice, int64_t nFrame)
{
	InputState& inState = m_Inputs[nDevice][nInChannel];
	if (inState.nMappedOutChannel == -1)
		return false;
	for (auto it = m_mapReleasedOutputs.begin(); it != m_mapReleasedOutputs.end(); /*nothing here*/)
	{
		if (it->second == inState.nMappedOutChannel) // remove any other references to this output channel
			it = m_mapReleasedOutputs.erase(it);
		else
			++it;
	}
	m_mapReleasedOutputs.insert(pair<int64_t, int>(nFrame, inState.nMappedOutChannel));
	return true;
}

template <int NUM_INDEV>
void MultiControlModule<NUM_INDEV>::UpdateNumOutputChannels(int nChannels, int64_t nFrame)
{
	if (nChannels == m_nOutputChannels)
		return;
	if (nChannels > m_nOutputChannels)
	{
		for (int c = m_nOutputChannels; c < nChannels; c++)
			m_mapReleasedOutputs.insert(pair<int64_t, int>(nFrame, c));
	}
	else
	{
		for (int c = nChannels; c < m_nOutputChannels; ++c)
		{
			m_Outputs[c].nMappedInChannel = -1;
			for (int o = OutputVOct; o <= OutputCV2; ++o)
				outputs[o].setVoltage(0.0f, c);
			for (int nDevice = 0; nDevice < NUM_INDEV; nDevice++)
			{
				for (int i = 0; i < PORT_MAX_CHANNELS; ++i)
				{
					InputState& inState = m_Inputs[nDevice][i];
					if (inState.nMappedOutChannel >= nChannels)
						inState.nMappedOutChannel = -1;
				}
			}
		}
		for (auto it = m_mapReleasedOutputs.begin(); it != m_mapReleasedOutputs.end(); /*nothing here*/)
		{
			if (it->second >= nChannels)
				it = m_mapReleasedOutputs.erase(it);
			else
				++it;
		}
	}
	m_nOutputChannels = nChannels;
	paramQuantities[ParamOutputChannels]->setValue(static_cast<float>(nChannels));
	for (int nOutSocket = OutputVOct; nOutSocket <= OutputCV2; ++nOutSocket)
		outputs[nOutSocket].setChannels(nChannels);
}
template <int NUM_INDEV>
MultiControlWidget<NUM_INDEV>::MultiControlWidget(MultiControlModule<NUM_INDEV>* pModule)
{
	setModule(pModule);
	if (NUM_INDEV == 2)
		setPanel(createPanel(asset::plugin(the_pPluginInstance, "res/DualControl.svg"), asset::plugin(the_pPluginInstance, "res/DualControl-dark.svg")));
	else if (NUM_INDEV == 3)
		setPanel(createPanel(asset::plugin(the_pPluginInstance, "res/TripleControl.svg"), asset::plugin(the_pPluginInstance, "res/TripleControl-dark.svg")));
	else
		assert(false); // there are only front panels for Dual and TripleControl

	addChild(createWidget<ThemedScrew>(Vec(0, 0)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ThemedScrew>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	Vec vecBottom(0, RACK_GRID_HEIGHT);
	float fX = 5.5f;
	float fDx = 9.74f; // NOLINT enough is enough
	float fY = -15.0f - 9 * 10.16f;
	for (int nDevice = 0; nDevice < NUM_INDEV; nDevice++)
	{
		int nInput = MultiControlModule<NUM_INDEV>::InputVOctA + nDevice * MultiControlModule<NUM_INDEV>::NumColumnInputs;
		for (int i = MultiControlModule<NUM_INDEV>::InputVOctA; i <= MultiControlModule<NUM_INDEV>::InputCV2A; ++i, ++nInput)
		{
			addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY))), pModule, nInput));
			fY += 10.16f;
		}
		fY -= 1.0f;
		addParam(createParamCentered<VCVLatch>(vecBottom.plus(mm2px(Vec(fX, fY))), pModule, MultiControlModule<NUM_INDEV>::ParamA + nDevice));
		addChild(createLightCentered<MediumLight<BlueLight>>(vecBottom.plus(mm2px(Vec(fX, fY))), pModule, MultiControlModule<NUM_INDEV>::LightA + nDevice));
		fY += 8.2f;
		addInput(createInputCentered<ThemedPJ301MPort>(vecBottom.plus(mm2px(Vec(fX, fY))), pModule, MultiControlModule<NUM_INDEV>::InputEnableA + nDevice));

		fY = -15.0f - 9 * 10.16f;
		fX += fDx;
	}

	Vec vecBottomRight(box.size.x, RACK_GRID_HEIGHT);
	fX = 5.5f;
	for (int o = MultiControlModule<NUM_INDEV>::OutputVOct; o <= MultiControlModule<NUM_INDEV>::OutputModulationM; ++o)
	{
		if (o == MultiControlModule<NUM_INDEV>::OutputAftertouchM)
			fY += 1.0f;
		addOutput(createOutputCentered<ThemedPJ301MPort>(vecBottomRight.plus(mm2px(Vec(-fX, fY))), pModule, o));
		fY += 10.16f;
	}
}

template <int NUM_INDEV>
void MultiControlWidget<NUM_INDEV>::appendContextMenu(Menu* pMainMenu) /*override*/
{
	auto pModule = dynamic_cast<MultiControlModule<NUM_INDEV>*>(getModule());
	if (pModule == nullptr)
		return;
	pMainMenu->addChild(new MenuSeparator);
	ui::MenuItem* pSubMenuItem = createSubmenuItem(
		"Polyphonic output channels",
		pModule->OutputChannels(),
		[=](Menu* pMenu)
		{
			for (int nChannels = 1; nChannels <= PORT_MAX_CHANNELS; nChannels++)
			{
				std::string sLabel;
				if (nChannels == 1)
					sLabel = "1 (Monophonic)";
				else
					sLabel = rack::string::f("%d", nChannels);
				ui::MenuItem* pNumberItem = createCheckMenuItem(
					sLabel,
					"",
					[=]() { return pModule->NumOutputChannels() == nChannels; },
					[=]() { pModule->NumOutputChannels(nChannels); } );
				pMenu->addChild(pNumberItem);
			}
		} );
	pMainMenu->addChild(pSubMenuItem);
}
