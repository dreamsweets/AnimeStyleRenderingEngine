#include "Entity.h"
#include "Scene/Scene.h"
#include "imgui/IMGUIManager.h"
#include "UI/SceneHierarchyWindow.h"

#include "Components/Transform/TransformComponent.h"

void Entity::Alive(bool value)
{
	m_isAlive = value;

	for (auto& child : m_vecChild)
	{
		child->m_isAlive = value;
	}
}

Entity::Entity(Scene* scene) : m_Scene(scene)
{
	m_TransformComponent = AddComponent<TransformComponent>();
}

Entity::~Entity()
{
	for (auto it = m_vecComponent.begin(); it != m_vecComponent.end(); ++it)
	{
		(*it)->m_isAlive = false;
		(*it)->m_isActive = false;
	}

	for (auto it = m_vecChild.begin(); it != m_vecChild.end(); ++it)
	{
		if (*it) delete *it;
	}
	m_vecChild.clear();
}

void Entity::OnIMGUIRender()
{
	auto hierarchyWindow = IMGUIManager::Inst().FindWindow<SceneHierarchyWindow>();
	
	ImGuiTreeNodeFlags flags = m_vecChild.empty() ? ImGuiTreeNodeFlags_Leaf : ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
	
	flags |= hierarchyWindow->GetSelectedEntity() == this ? ImGuiTreeNodeFlags_Selected : 0;

	const bool IsExpanded = ImGui::TreeNodeEx((void*)this, flags, this->m_Name.c_str());
	
	if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		ImGui::OpenPopup("SelectedEntityPopUp", ImGuiPopupFlags_NoOpenOverExistingPopup);
	}

	if (ImGui::BeginPopup("SelectedEntityPopUp"))
	{
		if (ImGui::MenuItem("Create Empty Entity As Child"))
		{
			auto entity = m_Scene->AddEntity(false);
			entity->Name("Empty Entity");
			entity->Parent(this);
		}
		if (ImGui::MenuItem("Remove This Entity"))
		{
			this->Alive(false);
		}
		ImGui::EndPopup();
	}

	if (ImGui::IsItemClicked())
	{
		if(hierarchyWindow->GetSelectedEntity() != this) hierarchyWindow->SetSelectedEntity(this);
	}

	if (IsExpanded)
	{
		for (auto& child : m_vecChild)
		{
			child->OnIMGUIRender();
		}
		ImGui::TreePop();
	}
}

void Entity::Start()
{
	for (auto& c : m_vecComponent)
	{
		c->Start();
	}

	for (auto& child : m_vecChild)
	{
		child->Start();
	}
}

void Entity::Update()
{
	for (auto& c : m_vecComponent)
	{
		c->Update();
	}

	for (auto& child : m_vecChild)
	{
		child->Update();
	}
}

void Entity::PostUpdate()
{
	if (!m_isAlive)
	{
		//컴포넌트들 모두 Destroy 호출
		for (auto& c : m_vecComponent)
		{
			c->Destroy();
		}
	}

	//현재 Entity가 죽은 상태면 Child도 죽은 상태로 되어있을테니 Child가 가진 Component에 대해서도 Destroy를 호출할 수 있도록 PostUpdate호출
	for (auto& child : m_vecChild)
	{
		child->PostUpdate();
	}

	//죽은 Child 삭제
	auto start = m_vecChild.begin();
	auto end = m_vecChild.end();
	for (; start != end; )
	{
		if (!(*start)->m_isAlive)
		{
			start = m_vecChild.erase(start);
			end = m_vecChild.end();
		}
		else {
			++start;
		}
	}
}
