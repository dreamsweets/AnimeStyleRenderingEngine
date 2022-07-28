#pragma once
#include "Common/Common.h"

enum class Mouse : byte {
	LButton = 0x01,
	RButton = 0x02,
	MButton = 0x04,
	X1Button = 0x05,
	X2Button = 0x06,
};
enum class Key {
	Space = VK_SPACE,
	BackSpace = VK_BACK,
	Ctrl = VK_CONTROL,
	Alt = VK_MENU,
	Shift = VK_SHIFT,
	LShift = VK_LSHIFT,
	RSHIFT = VK_RSHIFT,
	Tab = VK_TAB,
	Enter = VK_RETURN,
	ESC = VK_ESCAPE,
	PageUp = VK_PRIOR,
	PageDown = VK_NEXT,
	End = VK_END,
	Home = VK_HOME,
	LeftArrow = VK_LEFT,
	UpArrow = VK_UP,
	RightArrow = VK_RIGHT,
	DownArrow = VK_DOWN,
	Insert = VK_INSERT,
	Delete = VK_DELETE,

	_0 = 0x30,
	_1 = 0x31,
	_2 = 0x32,
	_3 = 0x33,
	_4 = 0x34,
	_5 = 0x35,
	_6 = 0x36,
	_7 = 0x37,
	_8 = 0x38,
	_9 = 0x39,

	A = 0x41,
	B = 0x42,
	C = 0x43,
	D = 0x44,
	E = 0x45,
	F = 0x46,
	G = 0x47,
	H = 0x48,
	I = 0x49,
	J = 0x4A,
	K = 0x4B,
	L = 0x4C,
	M = 0x4D,
	N = 0x4E,
	O = 0x4F,
	P = 0x50,
	Q = 0x51,
	R = 0x52,
	S = 0x53,
	T = 0x54,
	U = 0x55,
	V = 0x56,
	W = 0x57,
	X = 0x58,
	Y = 0x59,
	Z = 0x5A,

	NumPad0 = 0x60,
	NumPad1 = 0x61,
	NumPad2 = 0x62,
	NumPad3 = 0x63,
	NumPad4 = 0x64,
	NumPad5 = 0x65,
	NumPad6 = 0x66,
	NumPad7 = 0x67,
	NumPad8 = 0x68,
	NumPad9 = 0x69,

	F1 = 0x70,
	F2 = 0x71,
	F3 = 0x72,
	F4 = 0x73,
	F5 = 0x74,
	F6 = 0x75,
	F7 = 0x76,
	F8 = 0x77,
	F9 = 0x78,
	F10 = 0x79,
	F11 = 0x80,
	F12 = 0x81,
};

namespace Input {
	void Init_Win32(HWND main_window);
	void Update_Win32();
	math::vec2 MousePosition();
	math::vec2 MouseDelta();
	s32 MouseWheel();

	bool MouseButtonDown(Mouse mouse);
	bool MouseButton(Mouse mouse);
	bool MouseButtonUp(Mouse mouse);

	bool KeyButtonDown(Key key);
	bool KeyButton(Key key);
	bool KeyButtonUp(Key key);

	LRESULT CALLBACK Input_WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
} // namespace Input