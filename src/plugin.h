#pragma once

#define RACK_GRID_WIDTH_MM	(5.08f)
#define RACK_GRID_HEIGHT_MM	(128.5f)

#if __GNUC__
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#endif
#endif

#include <rack.hpp>

#if __GNUC__
#ifndef __clang__
#pragma GCC diagnostic pop
#endif
#endif

#ifndef M_PIf
# define M_PIf		3.14159265358979323846f	/* pi */
#endif
#ifndef M_PI_2f
# define M_PI_2f	1.57079632679489661923f	/* pi/2 */
#endif

using namespace std;
using namespace rack;
using namespace rack::engine;

// Helpers
#define NUMBER_SVG_NUMBERS	16
extern shared_ptr<Svg> NumberSvg(int nNumber);

extern int iround(float f);
extern int iround(double d);
extern int64_t i64round(float f);
extern int64_t i64round(double d);
extern bool StrToFloat(const std::string& s, float& f, const char** ppParsedUntil = nullptr);
extern bool StrToFloat(const char* psz, float& f, const char** ppParsedUntil = nullptr);

// Declare the Plugin, defined in plugin.cpp
extern Plugin* the_pPluginInstance;
