# CMake file for Unity plugin

cmake_minimum_required(VERSION 3.0.0)
project(UnityPlugin)

# Settings

set(UNITY_PLUGIN_HEADERS )
set(CMAKE_INSTALL_LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/../Assets/Plugins)

# Parse UNITY_PLUGIN_HEADERS setting, or print an error if it has not been set

if (UNITY_PLUGIN_HEADERS)

	include_directories(${UNITY_PLUGIN_HEADERS})

else(UNITY_PLUGIN_HEADERS)

	message(FATAL_ERROR "The CMAKE_INSTALL_LIBDIR variable must be set (like cmake -DCMAKE_INSTALL_LIBDIR=/path) to the PluginAPI directory for your version of Unity. On the Mac this is inside of the Unity.app/, on Windows it is in Program Files.")

endif(UNITY_PLUGIN_HEADERS) 

# Build library

add_library(UnityPlugin MODULE
	main.cpp
)

# On OS X, CMake will want to name the library "libUnityPlugin.so", but Unity requires it to be "UnityPlugin.bundle"

if (UNIX AND APPLE) # TODO: iPhone
	set_target_properties(UnityPlugin PROPERTIES
		PREFIX ""
		SUFFIX ".bundle"
	)
endif(UNIX AND APPLE)

# Install library

install(TARGETS UnityPlugin
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RENAME ${UNITY_PLUGIN_NAME}
)
