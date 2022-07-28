#pragma once
#include "DX12/DX12Common.h"

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

enum RTLightType : int {
	LT_Directional = 1,
	LT_Spot,
	LT_Point,
};

struct RTCameraData {
	math::mat4x4 cameraView{};
	math::mat4x4 cameraProj{};

	math::vec3 cameraPosition{ 0.f,0.f,-2.f };
	float cameraNearZ{ 0.01f };

	float cameraFarZ{ 100000.f };
	math::vec3 padd;
};

struct RTLightData {
	u32 lightType{ LT_Directional };
	math::vec3 position{ 0.f, 0.f, 0.f };

	float range{ 0.f };
	math::vec3 direction{ 0.f,0.f,0.f };

	float spotAngle{ 0.f };
	math::vec3 color{ 1.f,1.f,0.f };
};

struct RTPerFrameData {
	u32 lightCount{ 0 };
	math::vec3 padd1;
	RTCameraData camera;
	RTLightData lights[32];
};

struct ShaderTable {
	ComPtr<ID3D12Resource> table;
	uint32_t tableSize{ 0u };
	ShaderTable() {}
	ShaderTable(ComPtr<ID3D12Device5> pDevice, u32 ShaderDataSize);
};

struct RTShaderEntry {
	static constexpr const WCHAR* rayGen = L"rayGen";

	static constexpr const WCHAR* miss = L"miss";
	static constexpr const WCHAR* chs = L"chs";
	static constexpr const WCHAR* hitgroup = L"hitgroup";

	static constexpr const WCHAR* miss2 = L"miss2";
	static constexpr const WCHAR* chs2 = L"chs2";
	static constexpr const WCHAR* hitgroup2 = L"hitgroup2";
};
struct RootSignatureDesc
{
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	std::vector<D3D12_DESCRIPTOR_RANGE> range;
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
};

ComPtr<ID3D12RootSignature> createRootSignature(ComPtr<ID3D12Device5> pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc);

ID3DBlob* compileLibrary(const WCHAR* filename, const WCHAR* targetString);

struct DxilLibrary
{
	DxilLibrary(ID3DBlob* pBlob, const WCHAR* entryPoint[], uint32_t entryPointCount) : pShaderBlob(pBlob)
	{
		stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		stateSubobject.pDesc = &dxilLibDesc;

		dxilLibDesc = {};
		exportDesc.resize(entryPointCount);
		exportName.resize(entryPointCount);
		if (pBlob)
		{
			dxilLibDesc.DXILLibrary.pShaderBytecode = pBlob->GetBufferPointer();
			dxilLibDesc.DXILLibrary.BytecodeLength = pBlob->GetBufferSize();
			dxilLibDesc.NumExports = entryPointCount;
			dxilLibDesc.pExports = exportDesc.data();

			for (uint32_t i = 0; i < entryPointCount; i++)
			{
				exportName[i] = entryPoint[i];
				exportDesc[i].Name = exportName[i].c_str();
				exportDesc[i].Flags = D3D12_EXPORT_FLAG_NONE;
				exportDesc[i].ExportToRename = nullptr;
			}
		}
	};

	DxilLibrary() : DxilLibrary(nullptr, nullptr, 0) {}

	D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
	D3D12_STATE_SUBOBJECT stateSubobject{};
	ID3DBlob* pShaderBlob;
	std::vector<D3D12_EXPORT_DESC> exportDesc;
	std::vector<std::wstring> exportName;
};

struct ExportAssociation
{
	ExportAssociation(const WCHAR* exportNames[], uint32_t exportCount, const D3D12_STATE_SUBOBJECT* pSubobjectToAssociate)
	{
		association.NumExports = exportCount;
		association.pExports = exportNames;
		association.pSubobjectToAssociate = pSubobjectToAssociate;

		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		subobject.pDesc = &association;
	}

	D3D12_STATE_SUBOBJECT subobject = {};
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association = {};
};

struct LocalRootSignature
{
	LocalRootSignature(ComPtr<ID3D12Device5> pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
	{
		pRootSig = createRootSignature(pDevice, desc);
		pInterface = pRootSig.GetInterfacePtr();
		subobject.pDesc = &pInterface;
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	}
	ComPtr<ID3D12RootSignature> pRootSig;
	ID3D12RootSignature* pInterface = nullptr;
	D3D12_STATE_SUBOBJECT subobject = {};
};

struct GlobalRootSignature
{
	GlobalRootSignature(ComPtr<ID3D12Device5> pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
	{
		pRootSig = createRootSignature(pDevice, desc);
		pInterface = pRootSig.GetInterfacePtr();
		subobject.pDesc = &pInterface;
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	}
	ComPtr<ID3D12RootSignature> pRootSig;
	ID3D12RootSignature* pInterface = nullptr;
	D3D12_STATE_SUBOBJECT subobject = {};
};

struct HitProgram
{
	HitProgram(LPCWSTR ahsExport, LPCWSTR chsExport, const std::wstring& name) : exportName(name)
	{
		desc = {};
		desc.AnyHitShaderImport = ahsExport;
		desc.ClosestHitShaderImport = chsExport;
		desc.HitGroupExport = exportName.c_str();

		subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		subObject.pDesc = &desc;
	}

	std::wstring exportName;
	D3D12_HIT_GROUP_DESC desc;
	D3D12_STATE_SUBOBJECT subObject;
};

void PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC* desc);