//
// ChainMixerDefines.h
// ===================
// Common defines for ChainMixer modules
//

#pragma once
#include "Faders.h"

#define KNOB_TOP_MM			(14.5f)
#define KNOB_STEP_MM		(10.0f)
#define KNOB_AUX1_Y_MM		KNOB_TOP_MM
#define KNOB_AUX2_Y_MM		(KNOB_TOP_MM + KNOB_STEP_MM)
#define KNOB_PANBAL_Y_MM	(KNOB_TOP_MM + 2 * KNOB_STEP_MM)

#define SLIDER_Y_MM			((KNOB_PANBAL_Y_MM + KNOB_STEP_MM / 2.0f + SOLO_Y_MM - SWITCH_STEP_MM / 2.0f) / 2.0f)

#define SOCKET_BOTTOM_MM	(RACK_GRID_HEIGHT_MM - 14.0f)
#define SOCKET_STEP_MM		(9.0f)
#define SWITCH_STEP_MM		(8.0f)

#define SOLO_Y_MM			(MUTE_Y_MM - SWITCH_STEP_MM)
#define MUTE_Y_MM			(SOCKET_BOTTOM_MM - 2 * SOCKET_STEP_MM)
#define SOCKET_L_Y_MM		(SOCKET_BOTTOM_MM - SOCKET_STEP_MM)
#define SOCKET_R_Y_MM		SOCKET_BOTTOM_MM					// numbered from bottom upwards

#define CENTER_2U_MM		5.08f

// causes issue with MIDI mapping w/ MIDI feedback
#define CHAIN_MIXER_SNAPGAIN

