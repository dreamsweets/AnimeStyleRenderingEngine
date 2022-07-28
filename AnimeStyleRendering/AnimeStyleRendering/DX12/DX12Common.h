#pragma once
#include "Window.h"

void MSGBox(const std::string& msg);
void D3DTraceHR(const std::string msg, HRESULT hr);

#define D3DCall(a) {HRESULT hr_ = a; if(FAILED(hr_)) { D3DTraceHR( #a, hr_); }}

template<class BlotType>
std::string convertBlobToString(BlotType* pBlob)
{
	std::vector<char> infoLog(pBlob->GetBufferSize() + 1);
	memcpy(infoLog.data(), pBlob->GetBufferPointer(), pBlob->GetBufferSize());
	infoLog[pBlob->GetBufferSize()] = 0;
	return std::string(infoLog.data());
}

static const D3D12_HEAP_PROPERTIES kUploadHeapProps =
{
	D3D12_HEAP_TYPE_UPLOAD,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0,
	0,
};

static const D3D12_HEAP_PROPERTIES kDefaultHeapProps =
{
	D3D12_HEAP_TYPE_DEFAULT,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0,
	0
};

#ifndef ROUND_UP
#define ROUND_UP(v, powerOf2Alignment) (((v) + (powerOf2Alignment)-1) & ~((powerOf2Alignment)-1))
#endif


struct PerObjectData {
	math::mat4x4 World;
};

struct PerFrameData {
	math::vec3 CamPosition;
	f32 padding;
	math::mat4x4 View;
	math::mat4x4 InverseView;
	math::mat4x4 Projection;
};

struct OutlineInfo {
	f32 Amount = 0.02f;
	math::vec3 Color = {1.f,0.f,0.f};
};

enum class LightType : unsigned int {
	Directional = 1,
	Point,
	Spot,
};

struct RTCamera {
	math::mat4x4 View;
	math::mat4x4 Proj;
	math::mat4x4 InverseView;
	math::mat4x4 InverseProj;

	math::vec3 CamPosition;
	f32 Near;
	f32 Far;
	math::vec3 CameraForward;
};

struct LightData {
	LightType lightType{ LightType::Directional };
	math::vec3 position{ 0.f, 0.f, 0.f };

	float range{ 0.f };
	math::vec3 direction{ 0.f,0.f,0.f };

	float spotAngle{ 0.f };
	math::vec3 color{ 1.f,1.f,0.f };
};

struct LightPassData {
	u32 lightCount{ 0 };
	math::vec3 padd1;
	LightData lights[32];
};

struct RaytracingPassData {
	u32 lightCount{ 0 };
	math::vec3 padd1;
	RTCamera camera;
	LightData lights[32];
};

struct DescriptorHeapWrapper {
	DescriptorHeapWrapper() { memset(this, 0, sizeof(*this)); }

	HRESULT Create(
		ID3D12Device* pDevice,
		D3D12_DESCRIPTOR_HEAP_TYPE Type,
		UINT NumDescriptors,
		bool bShaderVisible = false)
	{
		Desc.Type = Type;
		Desc.NumDescriptors = NumDescriptors;
		Desc.Flags = (bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : (D3D12_DESCRIPTOR_HEAP_FLAGS)0);

		HRESULT hr = pDevice->CreateDescriptorHeap(&Desc,
			__uuidof(ID3D12DescriptorHeap),
			(void**)&pDH);
		if (FAILED(hr)) return hr;

		hCPUHeapStart = pDH->GetCPUDescriptorHandleForHeapStart();
		if (bShaderVisible)
		{
			hGPUHeapStart = pDH->GetGPUDescriptorHandleForHeapStart();
		}
		else
		{
			hGPUHeapStart.ptr = 0;
		}
		HandleIncrementSize = pDevice->GetDescriptorHandleIncrementSize(Desc.Type);
		return hr;
	}
	operator ID3D12DescriptorHeap* () { return pDH.Get(); }

	inline void Destroy()
	{
		pDH.Reset();
	}

	SIZE_T MakeOffsetted_SizeT(SIZE_T ptr, UINT index)
	{
		SIZE_T offsetted;
		offsetted = ptr + static_cast<SIZE_T>(index * HandleIncrementSize);
		return offsetted;
	}

	UINT64 MakeOffsetted_Uint64(UINT64 ptr, UINT index)
	{
		UINT64 offsetted;
		offsetted = ptr + static_cast<UINT64>(index * HandleIncrementSize);
		return offsetted;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE hCPU(UINT index)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle;
		handle.ptr = MakeOffsetted_SizeT(hCPUHeapStart.ptr, index);
		return handle;
	}
	D3D12_GPU_DESCRIPTOR_HANDLE hGPU(UINT index)
	{
		assert(Desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		D3D12_GPU_DESCRIPTOR_HANDLE handle;
		handle.ptr = MakeOffsetted_Uint64(hGPUHeapStart.ptr, index);
		return handle;
	}
	D3D12_DESCRIPTOR_HEAP_DESC Desc;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pDH;
	D3D12_CPU_DESCRIPTOR_HANDLE hCPUHeapStart;
	D3D12_GPU_DESCRIPTOR_HANDLE hGPUHeapStart;
	UINT HandleIncrementSize;
};
