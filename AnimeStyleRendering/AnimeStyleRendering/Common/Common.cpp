#include "Common.h"
#include <locale>
#include <codecvt>

std::wstring string_2_wstring(const std::string& s)
{
    return WideString{ s };
}

std::string wstring_2_string(const std::wstring& ws)
{
    std::string s(ws.begin(), ws.end());
    return s;
}