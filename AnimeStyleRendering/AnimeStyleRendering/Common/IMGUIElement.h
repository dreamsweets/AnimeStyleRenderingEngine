#pragma once
#include "imgui/imgui.h"
class IMGUIElement {
public:
	virtual void OnIMGUIRender() = 0;
};