//
// Knobs classes for GPaudio modules
// =================================
// Collection of SVG knobs:
// * FilledKnow classes have a solid border under the arrow
// * PointyKnobs have no filled border under the arrow
//

#pragma once

#include "plugin.h"

//
// Knob base classes
//

class FilledKnob : public RoundKnob
{
protected:
	FilledKnob(int nMillimeter);

	// Nothing else here, since FilledKnob (unlike PointyKnob) doesn't have different SVGs for light and dark mode
};

class PointyKnob : public RoundKnob
{
protected:
	PointyKnob(int nMillimeter);

public:
	void step() override;

private:
	const int m_nMillimeter;
	bool m_bDarkMode;
};

//
// Different types and sizes of knobs
//

class FilledKnob16mm : public FilledKnob
{
public:
	FilledKnob16mm() : FilledKnob(16) {};
};

class FilledKnob14mm : public FilledKnob
{
public:
	FilledKnob14mm() : FilledKnob(14) {};
};

class PointyKnob8mm : public PointyKnob
{
public:
	PointyKnob8mm() : PointyKnob(8) {};
};

class PointyKnob10mm : public PointyKnob
{
public:
	PointyKnob10mm() : PointyKnob(10) {};
};

class PointyKnob12mm : public PointyKnob
{
public:
	PointyKnob12mm() : PointyKnob(12) {};
};
