#pragma once
#define _CRT_SECURE_NO_WARNINGS

// STL Headers
#include <algorithm>
#include <cassert>
#include <chrono>
#include <string>
#include <vector>
#include <list>
#include <typeindex>
#include <memory>
#include <future>
#include <mutex>
#include <map>
#include <deque>
#include <queue>
#include <stack>
#include <functional>
#include <unordered_map>
#include <exception>
#include <bitset>
#include <array>
#include <fstream>
#include <wrl.h>

using namespace Microsoft::WRL;

//DirectX 12
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgidebug.h>
#include "d3dx12.h"

#pragma comment(lib,"d3d12")
#pragma comment(lib,"dxgi")
#pragma comment(lib,"d3dcompiler")
#pragma comment(lib, "dxguid.lib")
// __DirectX 12

//Types, Interface, ...etc
#include "TypeDefinitions.h"
#include "IMGUIElement.h"
#include "AnimeMath.h"
#include "Path.h"

//Singleton Pattern Helpers
#define DECLARE_SINGLE(CLASS)	\
public:							\
static CLASS& Inst();			\
private:						\
	CLASS() = default;			

#define IMPLEMENT_SINGLE(CLASS) \
CLASS& CLASS::Inst(){			\
static CLASS inst;				\
return inst;					\
}

#define SAFE_DELETE(x) if(x) delete x; x = nullptr;
#define arraysize(a) (sizeof(a)/sizeof(a[0]))
#define align_to(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

//Common Class
//
#include "Time.h"
#include "Input.h"
//

#pragma region Assimp
#ifdef _DEBUG
#pragma comment(lib, "assimp-vc142-mtd")
#else
#pragma comment(lib, "assimp-vc142-mt")
#endif

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#pragma endregion // region Assimp

#pragma region DirectXTex

#pragma comment(lib, "DirectXTex.lib")

#include <DirectXTex/DirectXTex.h>
#pragma endregion // region DirectXTex

class WideString
{
    std::wstring m_wstr;
public:
    WideString(std::string str) : m_wstr(str.begin(), str.end()) {}
    operator const wchar_t* () { return m_wstr.c_str(); }
    operator std::wstring& () { return m_wstr; }
};

std::wstring string_2_wstring(const std::string& s);

std::string wstring_2_string(const std::wstring& ws);

struct Vertex_SkeletalMesh
{
    math::vec3          pos{ 0,0,0 };
    math::vec4          color{ 0,0,0,0 };
    math::vec2          uv{ 0,0 };
    math::vec3          normal{ 0,0,0 };
    math::vec3          bitangent{ 0,0,0 };
    math::vec3          tangent{ 0,0,0 };
    math::vec4          weight{ 0, 0, 0, 0 };
    math::vec4          boneIndex{ -1, -1, -1, -1 };
};

struct Bone {
    std::string Name;
    int Identifier;
    math::mat4 OffsetMatrix = math::identity<math::mat4>();      // Static Mesh -> Bone 위치로 메쉬의 좌표계를 이동시키기 위한 오프셋 매트릭스
    std::vector<Bone> children;
};

struct KeyFrame {
    double time;
    math::vec3 position;
    math::quat rotation;
    math::vec3 scale;
};

template<typename T>
void EraseUnordered(std::vector<T>& v, size_t index)
{
    if (v.size() > 1)
    {
        std::iter_swap(v.begin() + index, v.end() - 1);
        v.pop_back();
    }
    else
    {
        v.clear();
    }
}