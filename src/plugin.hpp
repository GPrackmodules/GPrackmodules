#pragma once

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

using namespace std;
using namespace rack;
using namespace rack::engine;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* the_pPluginInstance;
