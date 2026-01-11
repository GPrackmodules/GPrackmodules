<h1>AB4 - Simple four way A/B switcher</h1>

**AB4** is a module in the GP plugin for VCV Rack 2

![AB4 Module](AB4.png "The AB4 Module") &nbsp; &nbsp; &nbsp;![AB4 Module](AB4-dark.png "The AB4 Module")

This module is a simple polyphonic A/B switcher. The four output signals are either fed from
the four A inputs signals or the four B inputs.

A button controls the A/B selection, and an
indicator shows which group of inputs is selected. There is also a control voltage input to
control A/B switching. The "CV A/B" input and the parameter button are combined with a logical OR
operation, so AB4 switches to B when either is active.

The state of the A/B switch is reflected on the "CV A/B" control voltage output
and can be used to control more AB4 modules or other modules.

All channels of polyphonic input cables are
carried from the selected input to the output sockets.

AB4 does not distinguish between audio, CV or trigger signals. Please note that AB4 does not
understand which signals are triggers, which might need to be ended before switching. This
can lead to short bleeps when switching GATE signals through AB4.