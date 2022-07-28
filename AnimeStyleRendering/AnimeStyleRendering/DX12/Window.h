#pragma once
#include "Common/Common.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // For HRESULT
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#if defined(CreateWindow)
#undef CreateWindow
#endif

class Window {

public:
	using WndProcFunc = LRESULT(*)(HWND, UINT, WPARAM, LPARAM);

	static HWND Create(std::string Title, unsigned int Width, unsigned int Height, WndProcFunc func);
};
