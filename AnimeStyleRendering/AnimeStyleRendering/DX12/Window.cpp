#include "Window.h"
#include "DX12/DX12Common.h"

HWND Window::Create(std::string Title, unsigned int Width, unsigned int Height, WndProcFunc func)
{
	const WCHAR* className = L"AnimeWindowClass";
	DWORD winStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	
	WNDCLASS wc = {};
	wc.lpfnWndProc = func;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.lpszClassName = className;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	if (RegisterClass(&wc) == 0)
	{
		MSGBox("RegisterClass() failed");
		return nullptr;
	}
	RECT r{ 0, 0, (LONG)Width, (LONG)Height };
	AdjustWindowRect(&r, winStyle, false);

	int windowWidth = r.right - r.left;
	int windowHeight = r.bottom - r.top;
	
	int xPos = ((GetSystemMetrics(SM_CXSCREEN)) / 2) - (windowWidth / 2);
	int yPos = ((GetSystemMetrics(SM_CYSCREEN)) / 2) - (windowHeight / 2);

	r.left += xPos;
	r.top += yPos;
	r.right += xPos;
	r.bottom += yPos;

	// create the window
	std::wstring wTitle = string_2_wstring(Title);
	HWND hWnd = CreateWindowEx(0, className, wTitle.c_str(), winStyle, r.left, r.top, windowWidth, windowHeight, nullptr, nullptr, wc.hInstance, nullptr);
	if (hWnd == nullptr)
	{
		MSGBox("CreateWindowEx() failed");
		return nullptr;
	}

	return hWnd;
	
	//WNDCLASSEX wc{};
	//wc.cbSize = sizeof(WNDCLASSEX);
	//wc.style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX; //CS_CLASSDC/*CS_HREDRAW | CS_VREDRAW*/;
	//wc.lpfnWndProc = func;
	//wc.cbClsExtra = 0;
	//wc.cbWndExtra = 0;
	//wc.hInstance = GetModuleHandle(NULL);
	//wc.hbrBackground = CreateSolidBrush(RGB(26, 48, 76));
	//wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	//wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wc.lpszMenuName = NULL;
	//wc.lpszClassName = L"Anime Window";
	//wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	//RegisterClassEx(&wc);

	//RECT windowRect{ 0,0,Width, Height };

	//int xPos = ((GetSystemMetrics(SM_CXSCREEN)) / 2) - (Width / 2);
	//int yPos = ((GetSystemMetrics(SM_CYSCREEN)) / 2) - (Height / 2);

	//windowRect.left += xPos;
	//windowRect.top += yPos;
	//windowRect.right += xPos;
	//windowRect.bottom += yPos;

	//unsigned int style{};
	//style |= WS_OVERLAPPEDWINDOW;

	//auto clientArea = math::uvec4(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);
	//style = style;

	//AdjustWindowRect(&windowRect, style, false);

	//HWND hwnd = CreateWindowEx(
	//	0,
	//	wc.lpszClassName,
	//	std::wstring(Title.begin(), Title.end()).c_str(),
	//	style,
	//	windowRect.left, windowRect.top,
	//	windowRect.right - windowRect.left,
	//	windowRect.bottom - windowRect.top,
	//	nullptr,
	//	nullptr,
	//	nullptr,
	//	nullptr
	//);

	//if (hwnd)
	//{
	//	SetLastError(0);
	//	ShowWindow(hwnd, SW_SHOWNORMAL);
	//	UpdateWindow(hwnd);
	//	SetFocus(hwnd);
	//}

	//return hwnd;
}
