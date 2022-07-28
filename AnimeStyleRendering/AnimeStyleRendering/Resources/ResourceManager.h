#pragma once
#include "Common/Common.h"
#include "Resources/Mesh/Model.h"
#include "Resources/Texture/Texture.h"
#include "Resources/Animation/AnimationClip.h"
#include "Resources/Animation/Animation.h"

class GraphicsShader;
class Material;

class ResourceManager{
	friend class ContentsBrowser;
public:
	void Init();
	void Destroy();

	Model* GetModel(std::string name)
	{
		auto it = m_mapModel.find(name);
		if (it == m_mapModel.end()) return nullptr;
		return it->second;
	}
	
	Mesh* GetMesh(std::string name)
	{
		auto it = m_mapMesh.find(name);
		if (it == m_mapMesh.end()) return nullptr;
		return it->second;
	}

	Texture* GetTexture(std::string name) {
		auto it = m_mapTexture.find(name);
		if (it == m_mapTexture.end()) return nullptr;
		return it->second;
	}

	void RegisterMesh(std::string key, Mesh* mesh)
	{
		m_mapMesh.insert({key, mesh});
	}
	
	GraphicsShader* RegisterShader(std::string ShaderPath);
	GraphicsShader* GetShader(std::string Key);

	ComputeShader* RegisterComputeShader(std::string ShaderPath);
	ComputeShader* GetComputeShader(std::string Key);

	void RegisterMaterial(std::string Key, Material* Material);
	Material* GetMaterial(std::string Key);

	AnimationClip* LoadAnimationClip(std::string path);
	AnimationClip* GetAnimationClip(std::string key);
	
	Animation* CreateAnimation(std::string identifier);
	Animation* GetAnimation(std::string key);

	std::string GetAssetDirectory();

public:
	bool IsModel(std::string path);
	bool IsTexture(std::string path);
	bool IsShader(std::string path);

private:
	void LoadResources(std::string directoryPath);
	void CreateSkySphere();

private:
	std::unordered_map<std::string, Model*>				m_mapModel;
	std::unordered_map<std::string, Mesh*>				m_mapMesh;
	std::unordered_map<std::string, Texture*>			m_mapTexture;
	std::unordered_map<std::string, GraphicsShader*>	m_mapShaders;
	std::unordered_map<std::string, ComputeShader*>		m_mapComputeShaders;
	std::unordered_map<std::string, Material*>			m_mapMaterial;
	std::unordered_map<std::string, AnimationClip*>		m_mapAnimationClip;
	std::unordered_map<std::string, Animation*>			m_mapAnimation;
	
	DECLARE_SINGLE(ResourceManager)
};