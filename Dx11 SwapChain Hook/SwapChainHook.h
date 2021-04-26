#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <MinHook.h>

#pragma comment(lib, "d3d11.lib")

#include "FW1FontWrapper.h"

class SwapChainHook {
public:
	static void InitHook();
};