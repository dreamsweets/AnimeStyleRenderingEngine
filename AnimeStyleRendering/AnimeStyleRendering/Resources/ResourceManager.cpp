#include "ResourceManager.h"
#include "DX12/Material.h"
#include "imgui/IMGUIManager.h"

IMPLEMENT_SINGLE(ResourceManager)

void ResourceManager::Init()
{
	CreateSkySphere();
	RegisterComputeShader("Shaders/ComputeShader/AnimationComputeShader.fx");
	
	LoadResources(Path::GetCurrentDirectory() + "/Assets");
}

void ResourceManager::Destroy()
{
	for (auto& [key, model] : m_mapModel)
	{
		delete model;
	}
	for (auto& [key, mesh] : m_mapMesh)
	{
		delete mesh;
	}
	for (auto& [key, texture] : m_mapTexture)
	{
		delete texture;
	}
	for (auto& [key, mat] : m_mapMaterial)
	{
		delete mat;
	}
	for (auto& [key, shader] : m_mapShaders)
	{
		delete shader;
	}
}

GraphicsShader* ResourceManager::RegisterShader(std::string ShaderPath)
{
	if (!Path::IsExist(ShaderPath)) return nullptr;
	
	GraphicsShader* Shader = new GraphicsShader;
	Shader->LoadByPath(ShaderPath);
	m_mapShaders[Path::GetFileName(ShaderPath)] = Shader;

	return Shader;
}

GraphicsShader* ResourceManager::GetShader(std::string Key)
{
	auto iter = m_mapShaders.find(Key);
	if (iter == m_mapShaders.end()) return nullptr;

	return iter->second;
}

ComputeShader* ResourceManager::RegisterComputeShader(std::string ShaderPath)
{
	if (!Path::IsExist(ShaderPath)) return nullptr;

	auto computeShader = new ComputeShader;
	computeShader->LoadByPath("Shaders/ComputeShader/BasicComputeShader.fx");
	m_mapComputeShaders.insert({ Path::GetFileName(ShaderPath), computeShader });
	
	return computeShader;
}

ComputeShader* ResourceManager::GetComputeShader(std::string Key)
{
	auto iter = m_mapComputeShaders.find(Key);
	if (iter == m_mapComputeShaders.end()) return nullptr;

	return iter->second;
}

void ResourceManager::RegisterMaterial(std::string Key, Material* Material)
{
	auto iter = m_mapMaterial.find(Key);
	
	if (iter != m_mapMaterial.end())
	{
		delete iter->second;
	}

	m_mapMaterial[iter->first] = Material;
}

Material* ResourceManager::GetMaterial(std::string Key)
{
	auto iter = m_mapMaterial.find(Key);

	if (iter == m_mapMaterial.end()) return nullptr;

	return iter->second;
}

AnimationClip* ResourceManager::LoadAnimationClip(std::string path)
{
	if (!Path::IsExist(path)) return nullptr;
	
	std::string fileName = Path::GetFileName(path);

	auto iter = m_mapAnimationClip.find(fileName);

	if (iter == m_mapAnimationClip.end())
	{
		AnimationClip* clip = new AnimationClip;
		clip->Load(path);
		m_mapAnimationClip.insert({ fileName, clip });
		return GetAnimationClip(fileName);
	}

	return iter->second;
}

AnimationClip* ResourceManager::GetAnimationClip(std::string key)
{
	auto iter = m_mapAnimationClip.find(key);

	if(iter == m_mapAnimationClip.end()) return nullptr;

	return iter->second;
}

Animation* ResourceManager::CreateAnimation(std::string identifier)
{
	auto iter = m_mapAnimation.find(identifier);

	if (iter == m_mapAnimation.end())
	{
		Animation* ani = new Animation;
		ani->Init();
		m_mapAnimation.insert({ identifier, ani });

		return GetAnimation(identifier);
	}

	return iter->second;
}

Animation* ResourceManager::GetAnimation(std::string key)
{
	auto iter = m_mapAnimation.find(key);

	if (iter == m_mapAnimation.end()) return nullptr;

	return iter->second;
}

std::string ResourceManager::GetAssetDirectory()
{
	return Path::GetCurrentDirectory() + "/Assets";
}

bool ResourceManager::IsModel(std::string path)
{
	auto extension = Path::GetExtension(path);
	
	return extension == ".fbx" || extension == ".FBX" ? true : extension == ".obj"? true : false;
}

bool ResourceManager::IsTexture(std::string path)
{
	auto extension = Path::GetExtension(path);

	return extension == ".tga" ? true : extension == ".png" ? true : extension == ".dds" ? true : extension == ".jpg" ? true : false;
}

bool ResourceManager::IsShader(std::string path)
{
	auto extension = Path::GetExtension(path);

	return extension == ".fx" ? true : extension == ".hlsl" ? true : false;
}

void ResourceManager::LoadResources(std::string directoryPath)
{
	auto pathes = Path::SearchDirectory(directoryPath);

	for (auto& path : pathes)
	{
		if (Path::IsFile(path))
		{
			if (IsModel(path))
			{
				Model* model = new Model;
				model->LoadModel(path);

				m_mapModel.insert({ Path::GetFileName(path), model });
			}

			else if (IsTexture(path))
			{
				Texture* texture = new Texture;
				texture->LoadTexture(path);

				m_mapTexture.insert({ Path::GetFileName(path), texture });
			}

			else if (IsShader(path))
			{
				RegisterShader(path);
			}
		}

		else if (Path::IsDirectory(path))
		{
			LoadResources(path);
		}
	}
}

void Subdivide(std::vector<math::vec3>& vecVertices, std::vector<u32>& vecIndices)
{
	// Save a copy of the input geometry.
	std::vector<math::vec3>	vecCopyVertex = vecVertices;
	std::vector<u32>	vecCopyIndex = vecIndices;


	vecVertices.resize(0);
	vecIndices.resize(0);

	//       v1
	//       *
	//      / \
					//     /   \
	//  m0*-----*m1
//   / \   / \
	//  /   \ /   \
	// *-----*-----*
// v0    m2     v2

	UINT numTris = vecCopyIndex.size() / 3;
	for (UINT i = 0; i < numTris; ++i)
	{
		math::vec3 v0 = vecCopyVertex[vecCopyIndex[i * 3 + 0]];
		math::vec3 v1 = vecCopyVertex[vecCopyIndex[i * 3 + 1]];
		math::vec3 v2 = vecCopyVertex[vecCopyIndex[i * 3 + 2]];

		//
		// Generate the midpoints.
		//

		math::vec3 m0, m1, m2;

		// For subdivision, we just care about the position component.  We derive the other
		// vertex components in CreateGeosphere.

		m0 = math::vec3(
			0.5f * (v0.x + v1.x),
			0.5f * (v0.y + v1.y),
			0.5f * (v0.z + v1.z));

		m1 = math::vec3(
			0.5f * (v1.x + v2.x),
			0.5f * (v1.y + v2.y),
			0.5f * (v1.z + v2.z));

		m2 = math::vec3(
			0.5f * (v0.x + v2.x),
			0.5f * (v0.y + v2.y),
			0.5f * (v0.z + v2.z));

		//
		// Add new geometry.
		//

		vecVertices.push_back(v0); // 0
		vecVertices.push_back(v1); // 1
		vecVertices.push_back(v2); // 2
		vecVertices.push_back(m0); // 3
		vecVertices.push_back(m1); // 4
		vecVertices.push_back(m2); // 5

		vecIndices.push_back(i * 6 + 0);
		vecIndices.push_back(i * 6 + 3);
		vecIndices.push_back(i * 6 + 5);

		vecIndices.push_back(i * 6 + 3);
		vecIndices.push_back(i * 6 + 4);
		vecIndices.push_back(i * 6 + 5);

		vecIndices.push_back(i * 6 + 5);
		vecIndices.push_back(i * 6 + 4);
		vecIndices.push_back(i * 6 + 2);

		vecIndices.push_back(i * 6 + 3);
		vecIndices.push_back(i * 6 + 1);
		vecIndices.push_back(i * 6 + 4);
	}
}

float AngleFromXY(float x, float y)
{
	float theta = 0.0f;

	// Quadrant I or IV
	if (x >= 0.0f)
	{
		// If x = 0, then atanf(y/x) = +pi/2 if y > 0
		//                atanf(y/x) = -pi/2 if y < 0
		theta = atanf(y / x); // in [-pi/2, +pi/2]

		if (theta < 0.0f)
			theta += 2.0f * math::pi<float>(); // in [0, 2*pi).
	}

	// Quadrant II or III
	else
		theta = atanf(y / x) + math::pi<float>(); // in [0, 2*pi).

	return theta;
}

bool CreateSphere(std::vector<math::vec3>& vecVertex, std::vector<u32>& vecIndex, float Radius, unsigned int SubDivision)
{
	// Put a cap on the number of subdivisions.
	SubDivision = math::min(SubDivision, 5u);

	// Approximate a sphere by tessellating an icosahedron.
	const float X = 0.525731f;
	const float Z = 0.850651f;

	math::vec3 pos[12] =
	{
		math::vec3(-X, 0.0f, Z),  math::vec3(X, 0.0f, Z),
		math::vec3(-X, 0.0f, -Z), math::vec3(X, 0.0f, -Z),
		math::vec3(0.0f, Z, X),   math::vec3(0.0f, Z, -X),
		math::vec3(0.0f, -Z, X),  math::vec3(0.0f, -Z, -X),
		math::vec3(Z, X, 0.0f),   math::vec3(-Z, X, 0.0f),
		math::vec3(Z, -X, 0.0f),  math::vec3(-Z, -X, 0.0f)
	};

	DWORD k[60] =
	{
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
	};

	vecVertex.resize(12);
	vecIndex.resize(60);

	for (UINT i = 0; i < 12; ++i)
		vecVertex[i] = pos[i];

	for (UINT i = 0; i < 60; ++i)
		vecIndex[i] = k[i];

	for (UINT i = 0; i < SubDivision; ++i)
		Subdivide(vecVertex, vecIndex);

	// Project vertices onto sphere and scale.
	for (UINT i = 0; i < vecVertex.size(); ++i)
	{
		// Project onto unit sphere.
		math::vec3	vN = vecVertex[i];
		vN = math::normalize(vN);

		// Project onto sphere.
		math::vec p = vN * Radius;

		vecVertex[i] = p;
		// Normal이 있을 경우 따로 저장한다.

		// Derive texture coordinates from spherical coordinates.
		//float theta = AngleFromXY(
			/*vecVertex[i].x,
			vecVertex[i].z);*/

			//float phi = acosf(vecVertex[i].y / Radius);

			//vecVertex[i].UV.x = theta / XM_2PI;
			//vecVertex[i].UV.y = phi / XM_PI;

			//// Partial derivative of P with respect to theta
			//vecVertex[i].Tangent.x = -Radius * sinf(phi) * sinf(theta);
			//vecVertex[i].Tangent.y = 0.0f;
			//vecVertex[i].Tangent.z = Radius * sinf(phi) * cosf(theta);

			//vecVertex[i].Binormal = vecVertex[i].Normal.Cross(vecVertex[i].Tangent);
			//vecVertex[i].Binormal.Normalize();
	}

	return true;
}

void ResourceManager::CreateSkySphere()
{
	Mesh* mesh = new Mesh;
	std::vector<math::vec3> positions;
	std::vector<u32> indices;
	CreateSphere(positions, indices, 1, 1);

	mesh->CreateUserDefined(positions, indices);
	
	RegisterMesh("SkySphere", mesh);
}
