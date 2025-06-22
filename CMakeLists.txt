#
# CMakeLists.txt
#
# This file and the included RackSDK.cmake file can be used for the development of this VCV Rack2 plugin.
# There is a target build_plugin_quick_install which copies the plugin directly into the Rack plugin
# folders for testing and debugging.
#
# This CMakeLists.txt works nicely with CLion and allows you to have CMake profiles for Debug (with
# symbols, nothing 'optimized away') and Release (full optimization) builds.
#
# Unlike with the Makefile no .vcvplugin archive gets created. Use the Makefile and 'make dist' or
# 'make install' to create official pluyin archives.
#
# The CMakeLists.txt and RackSDK.cmake files are based on the VCV Plugin CMake Example
# here: https://github.com/qno/vcv-plugin-cmake-example. See LICENSE-dist for license information.
#

cmake_minimum_required(VERSION 3.16)

# set(CMAKE_CXX_STANDARD 20)
# default to release build if no build type given
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")

project(GPrackmodules)

# PLUGIN_NAME must match SLUG from plugin.json

set(PLUGIN_NAME GP)
set(ADDITIONAL_PLUGIN_DISTRIBUTABLES
	LICENSE
	LICENSE-dist
)
include(RackSDK.cmake)

include_directories(
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/common
	${CMAKE_SOURCE_DIR}/src/AB4
	${CMAKE_SOURCE_DIR}/src/ChainMixer
	${CMAKE_SOURCE_DIR}/src/StereoChorus
)

target_sources(${RACK_PLUGIN_LIB} PRIVATE
	src/common/Faders.cpp
	src/common/Knobs.cpp
	src/common/Fade.cpp
	src/common/Fade2.cpp
	src/AB4/AB4.cpp
	src/plugin.cpp
	src/ChainMixer/ChainMixerModule.cpp
	src/ChainMixer/ChainMixerChannel.cpp
	src/ChainMixer/ChainMixerMaster.cpp
	src/ChainMixer/ChainMixerAux.cpp
	src/StereoChorus/StereoChorus.cpp
	src/StereoChorus/ChorusDelayLine.cpp
)
