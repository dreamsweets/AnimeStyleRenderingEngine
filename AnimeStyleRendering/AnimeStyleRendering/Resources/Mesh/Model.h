#pragma once
#include "DX12/DX12Common.h"
#include "Resources/Animation/Skeleton.h"

class VertexBuffer;
class IndexBuffer;
class Material;
class Model;

struct Face {
	u32 index[3];
};

class Mesh {
	friend class Model;
	friend class RenderManager;
public:
	Mesh() = default;
	virtual ~Mesh();

	void CreateUserDefined(std::vector<Vertex_SkeletalMesh> vertices);
	void CreateUserDefined(std::vector<Vertex_SkeletalMesh> vertices, std::vector<u32> indices);
	void CreateUserDefined(std::vector<math::vec3> vertices, std::vector<u32> indices);

	//���� �����Ͱ� �ִ� ���¿��� ���� �����͸� �̿��� Vertex_Mesh�� �����ϴ� �Լ�
	void CreateVertices(u32 UVChannel = 0, u32 ColorChannel = 0);
	
	//VertexBuffer�� IndexBuffer ����, RT���ӱ���ü ����
	void CreateBuffers();
	void CreateBuffersByPositions();
	//VertexBuffer�� IndexBuffer�� CmdList�� ���ε�
	void Render();
	
	bool IsSkeletalMesh() { return m_Skeleton != nullptr; }

public:	
	void SetName(std::string value) { m_Name = value; }
	std::string GetName() { return m_Name; }
	Model* GetModel() { return m_pModel; }
	VertexBuffer* GetVertexBuffer() { return m_VertexBuffer; }
	IndexBuffer* GetIndexBuffer() { return m_IndexBuffer; }
	u32 GetVertexCount() { return (u32)m_Vertices.size(); }
	u32 GetIndexCount() { return (u32)m_Indices.size(); }
	std::vector<math::vec3> GetPositions() { return m_Positions; }
	std::vector<math::vec4>	GetWeights() { return m_BoneWeights; }
private:
	std::string m_Name{"Undefined Mesh"};
	unsigned int m_MaterialIndex{ 0 };

	//���� ������
	std::vector<std::vector<math::vec2>> m_vecUVChannel;
	std::vector<std::vector<math::vec4>> m_vecColorChannel;
	std::vector<math::vec3> m_Positions;
	std::vector<math::vec3> m_Normals;
	std::vector<math::vec3> m_Bitangents;
	std::vector<math::vec3> m_Tangents;
	std::vector<math::vec4>	m_BoneWeights;
	std::vector<math::vec4> m_BoneIndices;
	
	std::vector<Face> m_Faces;
	
	//���� �����ͷκ��� ������ Vertex, index ������ (CreatVertices�� ȣ���ϸ� ���ε����͸� ������ ����)
	std::vector<Vertex_SkeletalMesh> m_Vertices;
	std::shared_ptr<Skeleton> m_Skeleton;
	std::vector<u32> m_Indices;

	//GPU������
	VertexBuffer* m_VertexBuffer{ nullptr };
	IndexBuffer* m_IndexBuffer{ nullptr };

	//Parent Model : ���� ���� ����.
	Model* m_pModel{ nullptr };
};

class Model{
public:
	bool LoadModel(std::string FilePath);
	std::string GetName() { return m_Name; }
	const std::vector<Mesh*>& GetVecMesh() const { return m_vecMesh; }
	std::shared_ptr<Skeleton> GetSkeleton() { return m_Skeleton; }

private:
	void ReadSkeleton(const aiScene* pScene);

private:
	std::shared_ptr<Skeleton> m_Skeleton;

	std::string m_Name;
	std::vector<Mesh*> m_vecMesh;
	std::vector<Material*> m_vecMaterialSlot;
};