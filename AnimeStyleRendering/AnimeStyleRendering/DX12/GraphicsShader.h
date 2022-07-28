#pragma once
#include "DX12Common.h"
#include "ConstantBuffer.h"
#include "Common/IMGUIElement.h"
#include "imgui/IMGUIManager.h"


struct ReflectionShaderResource;
struct ReflectionVariable {
	std::string type;
	std::string name;
	ReflectionShaderResource* parent;
	u32 offset;
	u32 size;
};

struct ReflectionShaderResource {
	std::string name;
	u32 size;
	std::unordered_map<std::string, u32> mapVariableNames;
	std::vector<ReflectionVariable> vecVariable;
};

struct ShaderMetaData {
	std::string Name;
	D3D_SHADER_INPUT_TYPE Type;
	u32 BindPoint;
	u32 ByteStride;
	u32 DataSize;
	ReflectionShaderResource variableReflection;
};

struct ShaderData {
	ComPtr<ID3DBlob> ShaderBlob;
	std::unordered_map<std::string, ShaderMetaData> MapMetaData;
};

class GraphicsShader {
	friend class Material;
	friend class SkeletalGeometryMaterial;
public:
	void LoadByPath(std::string fullPath);
	~GraphicsShader();

	bool FindMetaDataVS(std::string key, ShaderMetaData& out);
	bool FindMetaDataPS(std::string key, ShaderMetaData& out);

private:
	void ReflectionVS(ID3DBlob* shader);
	void ReflectionGS(ID3DBlob* shader) {}
	void ReflectionPS(ID3DBlob* shader);

private:
	std::string m_Name;
	std::vector<char*>						m_vecSemantics;
	std::vector<D3D12_INPUT_ELEMENT_DESC>	m_InputElements;

	ShaderData m_VertexShader;
	ShaderData m_GeometryShader;
	ShaderData m_PixelShader;
};