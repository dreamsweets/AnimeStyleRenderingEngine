#include "Component.h"

void Component::OnIMGUIRender()
{
	std::type_index type = typeid(*this);

	if (ImGui::TreeNodeEx(this, ImGuiTreeNodeFlags_DefaultOpen, type.name()))
	{
		ImGui::TreePop();
	}
}
