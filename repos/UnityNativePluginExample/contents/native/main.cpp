#include "IUnityInterface.h"
#include <stdio.h>

// Unity plugin load event
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
    UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
}

// Unity plugin unload event
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
    UnityPluginUnload()
{
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
    UnityPluginCustomTest()
{
    return 3;
}
