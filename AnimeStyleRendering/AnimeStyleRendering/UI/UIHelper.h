#pragma once
#include "imgui/Window/IMGUIWindow.h"

void DrawVec3Control(const std::string& label, math::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);