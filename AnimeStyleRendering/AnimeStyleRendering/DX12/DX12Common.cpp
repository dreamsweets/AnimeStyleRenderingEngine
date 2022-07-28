#include "DX12Common.h"
void MSGBox(const std::string& msg)
{
	MessageBoxA(nullptr, msg.c_str(), "Error", MB_OK);
}

void D3DTraceHR(const std::string msg, HRESULT hr)
{
    char hr_msg[512];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hr, 0, hr_msg, ARRAYSIZE(hr_msg), nullptr);
    std::string error_msg = msg + ".\nError! " + hr_msg;
    MSGBox(error_msg);
}
