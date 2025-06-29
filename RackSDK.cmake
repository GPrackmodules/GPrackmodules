# Mapping of plugin build definitions from the Rack-SDK arch.mk, compile.mk, dep.mk and plugin.mk to CMake.
if (NOT RACK_SDK_DIR)
    set(RACK_SDK_DIR $ENV{RACK_DIR})
endif()

set(RACK_SDK_VERSION 2.4.0)
message(STATUS "Load RackSDK.cmake (mapping based on Rack-SDK-${RACK_SDK_VERSION})")

if ("${RACK_SDK_DIR}" STREQUAL "")
  message(FATAL_ERROR "Path to Rack SDK is missing! Add -DRACK_SDK_DIR=<PATH> to the cmake call.")
elseif (EXISTS "${RACK_SDK_DIR}/include/rack.hpp")
  message(STATUS "Using Rack-SDK in '${RACK_SDK_DIR}'")
else ()
  message(FATAL_ERROR "Couldn't find 'include/rack.hpp' in '${RACK_SDK_DIR}'")
endif ()

if ("${PLUGIN_SLUG}" STREQUAL "")
  message(FATAL_ERROR "PLUGIN_SLUG variable not set! Add PLUGIN_SLUG variable to the project CMakeLists.txt before including RackSDK.cmake.\
 The PLUGIN_SLUG must correspond to the plugin slug, as defined in plugin.json.")
else ()
  message(STATUS "Using PLUGIN_SLUG '${PLUGIN_SLUG}'")
endif ()

if ("${ADDITIONAL_PLUGIN_DISTRIBUTABLES}" STREQUAL "")
  message(WARNING "ADDITIONAL_PLUGIN_DISTRIBUTABLES variable not set. For installing additional files into '${PLUGIN_SLUG}'\
   folder add ADDITIONAL_PLUGIN_DISTRIBUTABLES variable to the project CMakeLists.txt before including RackSDK.cmake.")
endif ()

# Do not change the RACK_PLUGIN_LIB!
set(RACK_PLUGIN_LIB plugin)

file(GLOB LICENSE LICENSE*)
set(PLUGIN_DISTRIBUTABLES plugin.json res ${LICENSE} ${ADDITIONAL_PLUGIN_DISTRIBUTABLES})

message(STATUS "PLUGIN_DISTRIBUTABLES: ${PLUGIN_DISTRIBUTABLES}")

# This is needed for Rack for DAWs.
# Static libs don't usually compiled with -fPIC, but since we're including them in a shared library, it's needed.
add_compile_options(-fPIC)
# Generate dependency files alongside the object files
# Not required with CMake
#add_compile_options(-MMD -MP)

# Optimization
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Skipping Optimizations for Debug Build")
  add_compile_options(-D_DEBUG)
else ()
  message(STATUS "Enabling Optimizations for Non-Debug Build")
  add_compile_options(-O3 -DNDEBUG -funsafe-math-optimizations -fno-omit-frame-pointer)
endif ()
# Warnings
add_compile_options(-Wall -Wextra -Wno-unused-parameter)
# C++ standard
if (DEFINED CMAKE_CXX_STANDARD)
  message(STATUS "Retaining CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}")
else ()
  set(CMAKE_CXX_STANDARD 11)
  message(STATUS "Defaulting CMAKE_CXX_STANDARD to ${CMAKE_CXX_STANDARD}")
endif ()

add_library(${RACK_PLUGIN_LIB} MODULE)
set_target_properties(${RACK_PLUGIN_LIB} PROPERTIES PREFIX "")

# Since the plugin's compiler could be a different version than Rack's compiler, link libstdc++ and libgcc statically to avoid ABI issues.
target_link_options(${RACK_PLUGIN_LIB} PRIVATE $<$<CXX_COMPILER_ID:GNU>:-static-libstdc++> $<$<PLATFORM_ID:Linux>:-static-libgcc>)
target_compile_options(${RACK_PLUGIN_LIB} PRIVATE $<IF:$<STREQUAL:${CMAKE_OSX_ARCHITECTURES},arm64>,-march=armv8-a+fp+simd,-march=nehalem>)

add_library(RackSDK INTERFACE)
target_include_directories(RackSDK INTERFACE ${RACK_SDK_DIR}/include ${RACK_SDK_DIR}/dep/include)
target_link_directories(RackSDK INTERFACE ${RACK_SDK_DIR})
target_link_libraries(RackSDK INTERFACE Rack)
target_compile_definitions(RackSDK INTERFACE $<IF:$<STREQUAL:${CMAKE_OSX_ARCHITECTURES},arm64>,ARCH_ARM64,ARCH_X64>)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  if (NOT MINGW)
    message(FATAL_ERROR "Rack plugin development environment is only supported for MSYS2/MinGW")
  endif ()
  target_compile_definitions(RackSDK INTERFACE ARCH_WIN _USE_MATH_DEFINES)
  target_compile_options(RackSDK INTERFACE -municode -Wsuggest-override -Wno-parentheses)
  # For now ARM on Windows
  set(ARCH_CPU x64)
  # set(LOCAL_APP_DATA_WIN $ENV{LOCALAPPDATA})
  # message("LocalAppData ${LOCAL_APP_DATA}")
  set(USER_PLUGIN_DIR "$ENV{LOCALAPPDATA}\\Rack2\\plugins-win-x64\\${PLUGIN_SLUG}")
  set(USER_PLUGIN_RES_DIR "${USER_PLUGIN_DIR}\\res")
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  message(STATUS "Build Mac OSX Plugin for architecture ${CMAKE_OSX_ARCHITECTURES}")
  target_compile_definitions(RackSDK INTERFACE ARCH_MAC)
  if (${CMAKE_OSX_ARCHITECTURES} MATCHES "x86_64")
    add_compile_options(-arch x86_64)
  endif ()
  if (${CMAKE_OSX_ARCHITECTURES} MATCHES "arm64")
    add_compile_options(-arch arm64)
  endif ()
  set_target_properties(${RACK_PLUGIN_LIB} PROPERTIES SUFFIX ".dylib")
  set_target_properties(${RACK_PLUGIN_LIB} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
  set(USER_PLUGIN_DIR "$ENV(HOME)/Library/Application Support/Rack2/${PLUGIN_SLUG}")
  set(USER_PLUGIN_RES_DIR "${USER_PLUGIN_DIR}/res")
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  target_compile_definitions(RackSDK INTERFACE ARCH_LIN)
  # This prevents static variables in the DSO (dynamic shared object) from being preserved after dlclose().
  target_compile_options(RackSDK INTERFACE -fno-gnu-unique)
  # When Rack loads a plugin, it symlinks /tmp/Rack2 to its system dir, so the plugin can link to libRack.
  target_compile_options(RackSDK INTERFACE -Wl,-rpath=/tmp/Rack2)
  set(USER_PLUGIN_DIR $ENV{HOME}/.local/share/Rack2/plugins-lin-x64/${PLUGIN_SLUG})
  set(USER_PLUGIN_RES_DIR "${USER_PLUGIN_DIR}/res")
endif ()

message("-- USER_PLUGIN_DIR: ${USER_PLUGIN_DIR}")

target_link_libraries(${RACK_PLUGIN_LIB} PRIVATE RackSDK)

install(TARGETS ${RACK_PLUGIN_LIB} LIBRARY DESTINATION ${PROJECT_BINARY_DIR}/${PLUGIN_SLUG} OPTIONAL)
install(DIRECTORY ${PROJECT_BINARY_DIR}/${PLUGIN_SLUG}/ DESTINATION ${PLUGIN_SLUG})
file(COPY ${PLUGIN_DISTRIBUTABLES} DESTINATION ${PLUGIN_SLUG})

set(STABLE_PLUGIN_BUILD_TARGET build_plugin)
add_custom_target(${STABLE_PLUGIN_BUILD_TARGET})
add_dependencies(${STABLE_PLUGIN_BUILD_TARGET} ${RACK_PLUGIN_LIB})

# A quick installation target to copy the plugin library and plugin.json into VCV Rack plugin folder for development.
# USER_PLUGIN_DIR needs to point to the VCV Rack plugin folder in user documents.

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(STRIP strip $<TARGET_FILE:${RACK_PLUGIN_LIB}>)
endif()
add_custom_target(${STABLE_PLUGIN_BUILD_TARGET}_quick_install
        COMMAND ${STRIP}
        COMMAND rm -rf ${USER_PLUGIN_DIR}
        COMMAND mkdir ${USER_PLUGIN_DIR}
        COMMAND mkdir ${USER_PLUGIN_RES_DIR}
        COMMAND cmake -E copy $<TARGET_FILE:${RACK_PLUGIN_LIB}> ${USER_PLUGIN_DIR}
        COMMAND cmake -E copy ${CMAKE_SOURCE_DIR}/plugin.json ${USER_PLUGIN_DIR}
        COMMAND cmake -E copy ${CMAKE_SOURCE_DIR}/res/* ${USER_PLUGIN_RES_DIR}
        )
add_dependencies(${STABLE_PLUGIN_BUILD_TARGET}_quick_install ${RACK_PLUGIN_LIB})
