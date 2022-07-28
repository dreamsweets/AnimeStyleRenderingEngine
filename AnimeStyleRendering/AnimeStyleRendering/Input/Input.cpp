#include "Input.h"
#include "DX12/DX12Common.h"
namespace Input {
namespace {
HWND main_id;
math::vec2 mouse_raw{};
std::bitset<256> prev_state;
std::bitset<256> curr_state;
bool enable_capture;
} // anonymous namespace
LRESULT CALLBACK Input_WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

	if (hwnd != main_id) return DefWindowProc(hwnd, msg, wparam, lparam);
	switch (msg) {
	case WM_INPUT: {
		u32 dataSize = 0;
		GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam), RID_INPUT, nullptr, &dataSize, sizeof(RAWINPUTHEADER));
		if (dataSize > 0) {
			std::vector<byte> datas; datas.resize(dataSize);
			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam), RID_INPUT, datas.data(), &dataSize, sizeof(RAWINPUTHEADER)) == dataSize) {
				RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(datas.data());
				if (raw->header.dwType == RIM_TYPEMOUSE) {
					mouse_raw = { raw->data.mouse.lLastX, raw->data.mouse.lLastY };
				}
			}
		}
		else {
			mouse_raw = { 0,0 };
		}
		break;
	}
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Input::Init_Win32(HWND main_window)
{
	main_id = main_window;

	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = 0;
	rid.hwndTarget = nullptr;
	if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE)
	{
		MSGBox("Raw Device 등록 실패!");
		throw;
	}
}

void Input::Update_Win32()
{
	prev_state = curr_state;
	for (auto i = 0u; i < 256; ++i)
	{
		curr_state[i] = GetAsyncKeyState(i);
	}
}

math::vec2 Input::MousePosition()
{
	return math::vec2();
}

math::vec2 Input::MouseDelta() {
	return mouse_raw;
}

s32 Input::MouseWheel()
{
	return s32();
}

bool Input::MouseButtonDown(Mouse mouse) {
	return !prev_state[(u32)mouse] && curr_state[(u32)mouse];
}

bool Input::MouseButton(Mouse mouse) {
	return prev_state[(u32)mouse] && curr_state[(u32)mouse];
}

bool Input::MouseButtonUp(Mouse mouse) {
	return prev_state[(u32)mouse] && !curr_state[(u32)mouse];
}

bool Input::KeyButtonDown(Key key) {
	return !prev_state[(u32)key] && curr_state[(u32)key];
}

bool Input::KeyButton(Key key) {
	return prev_state[(u32)key] && curr_state[(u32)key];
}

bool Input::KeyButtonUp(Key key) {
	return prev_state[(u32)key] && !curr_state[(u32)key];
}

} // namespace anime