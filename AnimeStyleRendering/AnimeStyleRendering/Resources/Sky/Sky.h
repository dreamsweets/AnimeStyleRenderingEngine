#pragma once
#include "Common/Common.h"
class Model;
class Material;

class Sky{
	friend class RenderManager;

public:
	bool Init();
	void Render();
	
	Sky() = default;
	~Sky();

private:
	Model* m_SkySphere{ nullptr };
	Material* m_Material{ nullptr };
};