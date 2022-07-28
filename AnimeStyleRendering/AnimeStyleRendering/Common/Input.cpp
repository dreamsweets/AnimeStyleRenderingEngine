#include "Input.h"

IMPLEMENT_SINGLE(Input)

void Input::Init()
{
	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = 0;
	rid.hwndTarget = nullptr;

	if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE)
	{
		throw std::exception{ "Raw Device 등록 실패!" };
	}
}

void Input::Update()
{
	prev_state = curr_state;
	for (auto i = 0u; i < 256; ++i)
	{
		curr_state[i] = GetAsyncKeyState(i);
	}
}

void Input::PostUpdate()
{
	mouse_raw = { 0,0 };
}

math::vec2 Input::MousePosition()
{
	return math::vec2();
}

math::vec2 Input::MouseDelta()
{
	return mouse_raw;
}

s32 Input::MouseWheel()
{
	return s32();
}

bool Input::MouseButtonDown(Mouse mouse)
{
	return (!prev_state[mouse]) && curr_state[mouse];
}

bool Input::MouseButton(Mouse mouse)
{
	return curr_state[mouse];
}

bool Input::MouseButtonUp(Mouse mouse)
{
	return prev_state[mouse] && (!curr_state[mouse]);
}

bool Input::KeyButtonDown(Key key)
{
	return (!prev_state[key]) && (curr_state[key]);
}

bool Input::KeyButton(Key key)
{
	return curr_state[key];
}

bool Input::KeyButtonUp(Key key)
{
	return prev_state[key] && !curr_state[key];
}

LRESULT Input::Input_WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case WM_INPUT: {
		u32 dataSize = 0;
		GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam), RID_INPUT, nullptr, &dataSize, sizeof(RAWINPUTHEADER));
		if (dataSize > 0) {
			std::vector<byte> datas; datas.resize(dataSize);
			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam), RID_INPUT, datas.data(), &dataSize, sizeof(RAWINPUTHEADER)) == dataSize) {
				RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(datas.data());
				if (raw->header.dwType == RIM_TYPEMOUSE) {
					Input::Inst().mouse_raw = { raw->data.mouse.lLastX, raw->data.mouse.lLastY };
				}
			}
		}
		else {
			Input::Inst().mouse_raw = { 0,0 };
		}
		break;
	}
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}
