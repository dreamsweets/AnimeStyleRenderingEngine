#include "Model.h"
#include "DX12/VertexBuffer.h"
#include "DX12/IndexBuffer.h"
#include "Render/RenderManager.h"
#include "Resources/Animation/Animation.h"
#include "Common/AssimpHelper.h"

bool Model::LoadModel(std::string FilePath)
{
	m_Skeleton.reset();
	m_Skeleton = std::make_shared<Skeleton>();
	Assimp::Importer importer;
	assert(Path::IsExist(FilePath));

	const aiScene* pScene = importer.ReadFile(FilePath,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_SortByPType |
		aiProcess_ConvertToLeftHanded |
		aiProcess_PopulateArmatureData |
		aiProcess_LimitBoneWeights
		//| aiProcess_FlipUVs
	);

	m_Name = Path::GetFileName(FilePath);
	if (pScene->HasMaterials())
	{
		m_vecMaterialSlot.resize(pScene->mNumMaterials);
	}

	//m_Skeleton->Init(pScene);
	ReadSkeleton(pScene);

	if (pScene->HasMeshes())
	{
		std::unordered_map<std::string, std::pair<int, glm::mat4>> boneInfo = {};

		for (u32 i{ 0 }; i < pScene->mNumMeshes; ++i)
		{
			bool hasBone = false;
			Mesh* mesh = new Mesh;
			auto& aimesh = pScene->mMeshes[i];
			assert(aimesh->HasPositions() && aimesh->HasNormals() && aimesh->HasTextureCoords(0) && aimesh->HasTangentsAndBitangents());
			mesh->m_Name = aimesh->mName.C_Str();
			mesh->m_MaterialIndex = aimesh->mMaterialIndex;
			mesh->m_vecUVChannel.resize(aimesh->GetNumUVChannels());
			mesh->m_vecColorChannel.resize(aimesh->GetNumColorChannels());
			mesh->m_Positions.resize(aimesh->mNumVertices);
			mesh->m_Normals.resize(aimesh->mNumVertices);
			mesh->m_Bitangents.resize(aimesh->mNumVertices);
			mesh->m_Tangents.resize(aimesh->mNumVertices);
			mesh->m_Faces.resize(aimesh->mNumFaces);

			//position
			memmove(mesh->m_Positions.data(), aimesh->mVertices, sizeof(math::vec3) * aimesh->mNumVertices);
			//normal
			memmove(mesh->m_Normals.data(), aimesh->mNormals, sizeof(math::vec3) * aimesh->mNumVertices);
			//tangents
			memmove(mesh->m_Tangents.data(), aimesh->mTangents, sizeof(math::vec3) * aimesh->mNumVertices);
			//bitangents
			memmove(mesh->m_Bitangents.data(), aimesh->mBitangents, sizeof(math::vec3) * aimesh->mNumVertices);
			//faces(Indices) : Triangulated
			for (u32 j{ 0 }; j < aimesh->mNumFaces; ++j)
			{
				memmove(&(mesh->m_Faces[j]), aimesh->mFaces[j].mIndices, sizeof(Face));
			}
			//indices
			mesh->m_Indices.resize(mesh->m_Faces.size() * 3u);
			memcpy(mesh->m_Indices.data(), mesh->m_Faces.data(), sizeof(Face) * mesh->m_Faces.size());
			//uvs
			for (u32 j{ 0 }; j < aimesh->GetNumUVChannels(); ++j)
			{
				mesh->m_vecUVChannel[j].resize(aimesh->mNumVertices);
				for (u32 k{ 0 }; k < aimesh->mNumVertices; ++k)
				{
					memmove(&(mesh->m_vecUVChannel[j][k]), &(aimesh->mTextureCoords[j][k]), sizeof(math::vec2));
				}
			}
			//colors
			for (u32 j{ 0 }; j < aimesh->GetNumColorChannels(); ++j)
			{
				mesh->m_vecColorChannel[j].resize(aimesh->mNumVertices);
				memmove(mesh->m_vecColorChannel[j].data(), aimesh->mColors[j], sizeof(math::vec4) * aimesh->mNumVertices);
			}

			
			if (aimesh->HasBones())
			{
				hasBone = true;

				mesh->m_BoneIndices.resize(mesh->m_Positions.size());
				mesh->m_BoneWeights.resize(mesh->m_Positions.size());

				for (u32 j{ 0 }; j < aimesh->mNumBones; ++j)
				{
					auto& aibone = *(aimesh->mBones[j]);
					
					math::mat4 offsetMatrix = assimpToGlmMatrix(aibone.mOffsetMatrix);
					std::string boneName = aibone.mName.C_Str();

					boneInfo[boneName] = {j, offsetMatrix };

					auto pBone = m_Skeleton->GetBone(boneName);
					if (!pBone) throw;
					pBone->OffsetMatrix = offsetMatrix;
					
					int boneIndex = m_Skeleton->GetBoneIndex(boneName);
					
					for (auto weight = 0; weight < (int)aibone.mNumWeights; ++weight)
					{
						auto& weightData = aibone.mWeights[weight];
						int vertexWeightIterator = 0;
						for (; vertexWeightIterator < 4; ++vertexWeightIterator)
						{
							if (mesh->m_BoneWeights[weightData.mVertexId][vertexWeightIterator] == 0)
								break;
						}
						mesh->m_BoneWeights[weightData.mVertexId][vertexWeightIterator] = weightData.mWeight;
						mesh->m_BoneIndices[weightData.mVertexId][vertexWeightIterator] = boneIndex;
					}
				}
			}

			//vertices
			mesh->CreateVertices();
			mesh->CreateBuffers();

			mesh->m_pModel = this;

			if (hasBone) mesh->m_Skeleton = m_Skeleton;
			m_vecMesh.emplace_back(mesh);
		}
		
		m_Skeleton->GenerateOffsetBuffer();
	}

    return true;
}

void Model::ReadSkeleton(const aiScene* pScene)
{
	if (pScene->HasMeshes())
	{
		std::unordered_map<std::string, std::pair<int, glm::mat4>> boneInfo = {};
		int index = 0;
		for (u32 i{ 0 }; i < pScene->mNumMeshes; ++i)
		{
			auto& aimesh = pScene->mMeshes[i];
			if (aimesh->HasBones())
			{
				for (u32 j{ 0 }; j < aimesh->mNumBones; ++j)
				{
					auto& aibone = *(aimesh->mBones[j]);

					math::mat4 offsetMatrix = assimpToGlmMatrix(aibone.mOffsetMatrix);
					std::string boneName = aibone.mName.C_Str();
					if (boneInfo.find(boneName) == boneInfo.end()) boneInfo.insert({ boneName, { index++, offsetMatrix } });
				}
			}
		}

		m_Skeleton->Init(pScene->mRootNode, boneInfo);

	}
}

void Mesh::CreateVertices(u32 UVChannel, u32 ColorChannel)
{
	m_Vertices.resize(m_Positions.size());
	for (u32 j{ 0 }; j < m_Vertices.size(); ++j)
	{
		m_Vertices[j].pos = m_Positions[j];
		if (!m_vecColorChannel.empty()) m_Vertices[j].color = m_vecColorChannel[ColorChannel][j];
		else m_Vertices[j].color = math::vec4{ 1,1,1,1 };
		m_Vertices[j].uv = m_vecUVChannel[UVChannel][j];
		m_Vertices[j].normal = m_Normals[j];
		m_Vertices[j].tangent = m_Tangents[j];
		m_Vertices[j].bitangent = m_Bitangents[j];
		if (!m_BoneWeights.empty())
		{
			m_Vertices[j].boneIndex = m_BoneIndices[j];
			m_Vertices[j].weight = m_BoneWeights[j];
		}
	}
}

void Mesh::CreateBuffers()
{
	SAFE_DELETE(m_VertexBuffer);
	SAFE_DELETE(m_IndexBuffer);

	m_VertexBuffer = new VertexBuffer{ m_Vertices };
	m_IndexBuffer = new IndexBuffer{ m_Indices };
}

void Mesh::CreateBuffersByPositions()
{
	SAFE_DELETE(m_VertexBuffer);
	SAFE_DELETE(m_IndexBuffer);

	m_VertexBuffer = new VertexBuffer{ m_Positions };
	m_IndexBuffer = new IndexBuffer{ m_Indices };
}

void Mesh::Render()
{
	auto commandList = Gfx::GetCommandList();
	
	m_VertexBuffer->SetVertexBuffer(commandList);
	m_IndexBuffer->SetIndexBuffer(commandList);

	commandList->DrawIndexedInstanced(m_IndexBuffer->GetNumIndices(), 1, 0, 0, 0);
}

Mesh::~Mesh()
{
	SAFE_DELETE(m_VertexBuffer);
	SAFE_DELETE(m_IndexBuffer);
}

void Mesh::CreateUserDefined(std::vector<Vertex_SkeletalMesh> vertices)
{
	m_Vertices = std::move(vertices);

	//Vertex밖에 없으니 Index는 자동 생성
	m_Indices.resize(m_Vertices.size());
	for(int i=0; i< m_Indices.size(); ++i)
	{
		m_Indices[i] = i;
	}

	CreateBuffers();
}

void Mesh::CreateUserDefined(std::vector<Vertex_SkeletalMesh> vertices, std::vector<u32> indices)
{
	m_Vertices  = std::move(vertices);
	m_Indices = std::move(indices);
	
	CreateBuffers();
}

void Mesh::CreateUserDefined(std::vector<math::vec3> vertices, std::vector<u32> indices)
{
	m_Positions = std::move(vertices);
	m_Indices = std::move(indices);

	CreateBuffersByPositions();
}