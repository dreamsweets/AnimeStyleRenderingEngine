#include "RayTracingMaterial.h"
#include <sstream>
#include "Gfx.h"
#include "Resources/Mesh/Model.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Components/Primitive/RayTracingMeshComponent.h"

static dxc::DxcDllSupport gDxcDllHelper;
static bool dxcInitialized = false;
static IDxcCompiler* pCompiler = nullptr;
static IDxcLibrary* pLibrary = nullptr;

#define DXIL_FOURCC(ch0, ch1, ch2, ch3) (                            \
  (uint32_t)(uint8_t)(ch0)        | (uint32_t)(uint8_t)(ch1) << 8  | \
  (uint32_t)(uint8_t)(ch2) << 16  | (uint32_t)(uint8_t)(ch3) << 24   \
  )

ComPtr<ID3D12RootSignature> createRootSignature(ID3D12Device* pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
{
    ComPtr<ID3DBlob> pSigBlob;
    ComPtr<ID3DBlob> pErrorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSigBlob, &pErrorBlob);
    if (FAILED(hr))
    {
        std::string errorString = (const char*)pErrorBlob->GetBufferPointer();
        MSGBox("Signature 생성 실패 : \n" + errorString);
        return nullptr;
    }

    ComPtr<ID3D12RootSignature> pRootSig;
    D3DCall(pDevice->CreateRootSignature(0, pSigBlob->GetBufferPointer(), pSigBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSig)));
    return pRootSig;
}

std::string GetPureNameOfFunction(LPCSTR dirtyFuncName)
{
    std::string primary = dirtyFuncName;
    auto first = primary.find_first_of('?', 0) + 1;
    auto last = primary.find_first_of('@', first) - 1;
    auto size = last - first + 1;
    return primary.substr(first, size);
}
std::string GetPayloadName(LPCSTR hitShaderName)
{
    static const std::string identifier = "@YAXU";
    std::string primary = hitShaderName;

    auto first = primary.find(identifier) + identifier.size();
    auto last = primary.find_first_of('@', first) - 1;
    auto size = last - first + 1;
    return primary.substr(first, size);
}
ComPtr<IDxcBlob> compileLibrary(const WCHAR* filename, const WCHAR* targetString)
{
    // Initialize the helper
    /*if (!dxcInitialized)
    {
        D3DCall(gDxcDllHelper.Initialize());
        D3DCall(gDxcDllHelper.CreateInstance(CLSID_DxcCompiler, &pCompiler));
        D3DCall(gDxcDllHelper.CreateInstance(CLSID_DxcLibrary, &pLibrary));

        dxcInitialized = true;
    }*/

    D3DCall(gDxcDllHelper.Initialize());
    D3DCall(gDxcDllHelper.CreateInstance(CLSID_DxcCompiler, &pCompiler));
    D3DCall(gDxcDllHelper.CreateInstance(CLSID_DxcLibrary, &pLibrary));

    // Open and read the file
    std::ifstream shaderFile(filename);
    if (shaderFile.good() == false)
    {
        MSGBox("RayTracing Compile Library 에러 : 파일을 찾을 수 없습니다. \n" + wstring_2_string(std::wstring(filename)));
        return nullptr;
    }
    std::stringstream strStream;
    strStream << shaderFile.rdbuf();
    std::string shader = strStream.str();

    // Create blob from the string
    ComPtr<IDxcBlobEncoding> pTextBlob;
    D3DCall(pLibrary->CreateBlobWithEncodingOnHeapCopy((LPBYTE)shader.c_str(), (uint32_t)shader.size(), 0, &pTextBlob));

    // Compile
    ComPtr<IDxcOperationResult> pResult;
    D3DCall(pCompiler->Compile(pTextBlob.Get(), filename, L"", targetString, nullptr, 0, nullptr, 0, nullptr, &pResult));

    // Verify the result
    HRESULT resultCode;
    D3DCall(pResult->GetStatus(&resultCode));
    if (FAILED(resultCode))
    {
        ComPtr<IDxcBlobEncoding> pError;
        D3DCall(pResult->GetErrorBuffer(&pError));
        std::string log = (const char*)pError->GetBufferPointer();
        MSGBox("Raytracing Shader Compiler error:\n" + log);
        return nullptr;
    }

    ComPtr<IDxcBlob> pBlob;
    D3DCall(pResult->GetResult(&pBlob));

    pCompiler->Release();
    pLibrary->Release();

    return pBlob;
}

//디버깅용으로 비주얼스튜디오 아웃풋메세지에 파이프라인 스테이트 오브젝트가 어떻게 생겨먹었는지 표기해줌.
inline void PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC* desc)
{
    std::wstringstream wstr;
    wstr << L"\n";
    wstr << L"--------------------------------------------------------------------\n";
    wstr << L"| D3D12 State Object 0x" << static_cast<const void*>(desc) << L": ";
    if (desc->Type == D3D12_STATE_OBJECT_TYPE_COLLECTION) wstr << L"Collection\n";
    if (desc->Type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE) wstr << L"Raytracing Pipeline\n";

    auto ExportTree = [](UINT depth, UINT numExports, const D3D12_EXPORT_DESC* exports)
    {
        std::wostringstream woss;
        for (UINT i = 0; i < numExports; i++)
        {
            woss << L"|";
            if (depth > 0)
            {
                for (UINT j = 0; j < 2 * depth - 1; j++) woss << L" ";
            }
            woss << L" [" << i << L"]: ";
            if (exports[i].ExportToRename) woss << exports[i].ExportToRename << L" --> ";
            woss << exports[i].Name << L"\n";
        }
        return woss.str();
    };

    for (UINT i = 0; i < desc->NumSubobjects; i++)
    {
        wstr << L"| [" << i << L"]: ";
        switch (desc->pSubobjects[i].Type)
        {
        case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE:
            wstr << L"Global Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
            break;
        case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE:
            wstr << L"Local Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
            break;
        case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK:
            wstr << L"Node Mask: 0x" << std::hex << std::setfill(L'0') << std::setw(8) << *static_cast<const UINT*>(desc->pSubobjects[i].pDesc) << std::setw(0) << std::dec << L"\n";
            break;
        case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY:
        {
            wstr << L"DXIL Library 0x";
            auto lib = static_cast<const D3D12_DXIL_LIBRARY_DESC*>(desc->pSubobjects[i].pDesc);
            wstr << lib->DXILLibrary.pShaderBytecode << L", " << lib->DXILLibrary.BytecodeLength << L" bytes\n";
            wstr << ExportTree(1, lib->NumExports, lib->pExports);
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION:
        {
            wstr << L"Existing Library 0x";
            auto collection = static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(desc->pSubobjects[i].pDesc);
            wstr << collection->pExistingCollection << L"\n";
            wstr << ExportTree(1, collection->NumExports, collection->pExports);
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
        {
            wstr << L"Subobject to Exports Association (Subobject [";
            auto association = static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
            UINT index = static_cast<UINT>(association->pSubobjectToAssociate - desc->pSubobjects);
            wstr << index << L"])\n";
            for (UINT j = 0; j < association->NumExports; j++)
            {
                wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
            }
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
        {
            wstr << L"DXIL Subobjects to Exports Association (";
            auto association = static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
            wstr << association->SubobjectToAssociate << L")\n";
            for (UINT j = 0; j < association->NumExports; j++)
            {
                wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
            }
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG:
        {
            wstr << L"Raytracing Shader Config\n";
            auto config = static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(desc->pSubobjects[i].pDesc);
            wstr << L"|  [0]: Max Payload Size: " << config->MaxPayloadSizeInBytes << L" bytes\n";
            wstr << L"|  [1]: Max Attribute Size: " << config->MaxAttributeSizeInBytes << L" bytes\n";
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG:
        {
            wstr << L"Raytracing Pipeline Config\n";
            auto config = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(desc->pSubobjects[i].pDesc);
            wstr << L"|  [0]: Max Recursion Depth: " << config->MaxTraceRecursionDepth << L"\n";
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP:
        {
            wstr << L"Hit Group (";
            auto hitGroup = static_cast<const D3D12_HIT_GROUP_DESC*>(desc->pSubobjects[i].pDesc);
            wstr << (hitGroup->HitGroupExport ? hitGroup->HitGroupExport : L"[none]") << L")\n";
            wstr << L"|  [0]: Any Hit Import: " << (hitGroup->AnyHitShaderImport ? hitGroup->AnyHitShaderImport : L"[none]") << L"\n";
            wstr << L"|  [1]: Closest Hit Import: " << (hitGroup->ClosestHitShaderImport ? hitGroup->ClosestHitShaderImport : L"[none]") << L"\n";
            wstr << L"|  [2]: Intersection Import: " << (hitGroup->IntersectionShaderImport ? hitGroup->IntersectionShaderImport : L"[none]") << L"\n";
            break;
        }
        }
        wstr << L"|--------------------------------------------------------------------\n";
    }
    wstr << L"\n";
    OutputDebugStringW(wstr.str().c_str());
}

void RayTracingMaterial::Load(std::string ShaderPath)
{
	if (!Path::IsExist(ShaderPath))
	{
		MSGBox("RayTracingMaterial : 입력된 패스\n" + ShaderPath + "\n를 찾을 수 없습니다!");
		throw;
	}
	std::wstring widePath{ ShaderPath.begin(), ShaderPath.end() };


	//셰이더 컴파일
    m_ShaderBlob = compileLibrary(WideString{ ShaderPath }, L"lib_6_3");
    
    CreateAccelerationStructures();
    ReflectionLibrary(m_ShaderBlob.Get());
    
    CreateGlobalRootSignature();
    CreatePipelineState();
    CreateShaderTable();
}

void RayTracingMaterial::Render()
{
    auto TraceScene = [&](u32 raygenIndex)
    {
        //SetDescriptorHeaps
        ID3D12DescriptorHeap* ppHeaps[] = { m_DescriptorHeap.pDH.Get() };
        Gfx::GetRTCommandList()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
        Gfx::GetRTCommandList()->SetComputeRootSignature(m_GlobalRootSignature.Get());
        Gfx::GetRTCommandList()->SetPipelineState1(m_PipelineStateObject.Get());

        //RayDesc 초기화
        D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
        raytraceDesc.Width = Gfx::GetWidth();
        raytraceDesc.Height = Gfx::GetHeight();
        raytraceDesc.Depth = 1;

        uint32_t RayGenEntrySize = m_TableGenerator.GetRayGenEntrySize();
        uint32_t RayGenerationSectionSizeInBytes = m_TableGenerator.GetRayGenSectionSize();
        raytraceDesc.RayGenerationShaderRecord.StartAddress = m_ShaderTableStorage->GetGPUVirtualAddress() + RayGenEntrySize * raygenIndex;
        raytraceDesc.RayGenerationShaderRecord.SizeInBytes = RayGenEntrySize;

        uint32_t MissSectionSizeInBytes = m_TableGenerator.GetMissSectionSize();
        raytraceDesc.MissShaderTable.StartAddress = m_ShaderTableStorage->GetGPUVirtualAddress() + RayGenerationSectionSizeInBytes;
        raytraceDesc.MissShaderTable.SizeInBytes = MissSectionSizeInBytes;
        raytraceDesc.MissShaderTable.StrideInBytes = m_TableGenerator.GetMissEntrySize();

        uint32_t HitGroupsSectionSize = m_TableGenerator.GetHitGroupSectionSize();
        raytraceDesc.HitGroupTable.StartAddress = m_ShaderTableStorage->GetGPUVirtualAddress() + RayGenerationSectionSizeInBytes + MissSectionSizeInBytes;
        raytraceDesc.HitGroupTable.SizeInBytes = HitGroupsSectionSize;
        raytraceDesc.HitGroupTable.StrideInBytes = m_TableGenerator.GetHitGroupEntrySize();

        Gfx::GetRTCommandList()->DispatchRays(&raytraceDesc);
    };
    
    TraceScene(0);

    // 에러가 나서 그냥 첫번째에 있는 raygen만 사용하기로 함...
    //for (int i = 1; i < m_vecRaygen.size(); ++i)
    //{
    //    //Preresult 등록
    //    auto preResult = m_vecRaygen[(i - 1)].OutputTexure.Get();

    //    Gfx::ResourceBarrier(Gfx::GetRTCommandList(), preResult, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);

    //    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    //    SRVDesc.Texture2D.MipLevels = 1;
    //    SRVDesc.Texture2D.MostDetailedMip = 0;
    //    SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    //    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    //    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    //    Gfx::GetDevice()->CreateShaderResourceView(preResult, &SRVDesc, m_DescriptorHeap.hCPU(m_vecRaygen[i].GetHeapIndex("PrevResult")));

    //    TraceScene(i);

    //    Gfx::ResourceBarrier(Gfx::GetRTCommandList(), preResult, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    //}
}

void RayTracingMaterial::CreateAccelerationStructures()
{
    std::vector<RayTracingMeshComponent*> dummy;
    BuildTopLevelAS(dummy);
}

void RayTracingMaterial::CreateGlobalRootSignature()
{
    m_GlobalRootSignature = createRootSignature(Gfx::GetDevice(), m_GlobalRootSignatureDesc.desc);
}

void RayTracingMaterial::CreatePipelineState()
{
    CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };
    auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    
    //Register ShaderLibrary
    CD3DX12_SHADER_BYTECODE shaderBytecode;
    shaderBytecode.pShaderBytecode = m_ShaderBlob->GetBufferPointer();
    shaderBytecode.BytecodeLength = m_ShaderBlob->GetBufferSize();
    lib->SetDXILLibrary(&shaderBytecode);
    {
        for (auto& shader : m_vecShader)
        {
            lib->DefineExport(shader.Name);
        }
    }

    //Define HitGroup
    for (auto& [id, hits] : m_mapHitGroup)
    {
        auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
        hitGroup->SetHitGroupExport(hits.Name);
        
        if(hits.ChsShader) hitGroup->SetClosestHitShaderImport(hits.ChsShader->Name);
        if (hits.AnyHitShader) hitGroup->SetAnyHitShaderImport(hits.AnyHitShader->Name);
        if (hits.IntersectShader) hitGroup->SetIntersectionShaderImport(hits.IntersectShader->Name);
        
        hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
    }

    //Shader Config
    auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    shaderConfig->Config(m_MaxPayloadSize, m_MaxAttributeSize);

    //Global RootSignature 등록 - 아마 현재는 Empty일 거임. 나중에 가속 구조체, PerFrame Data, OutputTexture 정도는 여기다가 넣어도 무방하지 않나 싶긴 함.
    auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSignature->SetRootSignature(m_GlobalRootSignature.Get());

    //각 셰이더 별로 갖고 있는 Local RootSignatures 등록
    for (auto& raygen : m_vecRaygen)
    {
        auto localRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
        localRootSignature->SetRootSignature(raygen.LocalRootSignature.Get());

        auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
        rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
        rootSignatureAssociation->AddExport(raygen.RayGenerationShader->Name);
    }

    for (auto& [id, hits] : m_mapHitGroup)
    {
        auto localRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
        localRootSignature->SetRootSignature(hits.LocalRootSignature.Get());

        if (hits.AnyHitShader)
        {
            auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
            rootSignatureAssociation->AddExport(hits.AnyHitShader->Name);
        }
        if (hits.ChsShader)
        {
            auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
            rootSignatureAssociation->AddExport(hits.ChsShader->Name);
        }
        if (hits.IntersectShader)
        {
            auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
            rootSignatureAssociation->AddExport(hits.IntersectShader->Name);
        }
    }

    for (auto& miss : m_vecMiss)
    {
        auto localRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
        localRootSignature->SetRootSignature(miss.LocalRootSignature.Get());

        auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
        rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
        rootSignatureAssociation->AddExport(miss.MissShader->Name);
    }

    //Pipeline Config 등록 - 이걸 셰이더 리플렉션에서 구할 방법이 있는 지 모르겠네;
    auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    UINT maxRecursionDepth = 2;
    pipelineConfig->Config(maxRecursionDepth);

#if _DEBUG
    PrintStateObjectDesc(raytracingPipeline);
#endif
    D3DCall(Gfx::GetDevice()->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&m_PipelineStateObject)));
    D3DCall(m_PipelineStateObject.As(&m_StateObjectProperties));
}

void RayTracingMaterial::BuildTopLevelAS(std::vector<RayTracingMeshComponent*>& vecInstances, bool UpdateOnly)
{
    if (!UpdateOnly)
    {
        m_TopLevelASGenerator.Clear();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        
        for (auto& cpuHandle : m_vecTLASViewCPUHandle)
        {
            Gfx::GetDevice()->CreateShaderResourceView(nullptr, &srvDesc, cpuHandle);
        }


        for (size_t i = 0; i < vecInstances.size(); i++)
        {
            m_TopLevelASGenerator.AddInstance(vecInstances[i]->GetBottomLevelAS().pResult.Get(), vecInstances[i]->InstanseWorldMatrix(), static_cast<UINT>(i), static_cast<UINT>(0));
        }

        UINT64 scratchSize, resultSize, instanceDescsSize;
        m_TopLevelASGenerator.ComputeASBufferSizes(Gfx::GetDevice(), true, &scratchSize, &resultSize, &instanceDescsSize);

        if (instanceDescsSize == 0) instanceDescsSize = 256;

        m_PrevTopLevelAS = m_TopLevelAS;

        m_TopLevelAS.pScratch = Gfx::CreateBuffer(
            scratchSize,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            kDefaultHeapProps
        );
        m_TopLevelAS.pResult = Gfx::CreateBuffer(
            resultSize,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
            kDefaultHeapProps
        );
        m_TopLevelAS.pInstanceDesc = Gfx::CreateBuffer(
            instanceDescsSize,
            D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            kUploadHeapProps
        );
        
        srvDesc.RaytracingAccelerationStructure.Location = m_TopLevelAS.pResult->GetGPUVirtualAddress();

        for (auto& cpuHandle : m_vecTLASViewCPUHandle)
        {
            Gfx::GetDevice()->CreateShaderResourceView(nullptr, &srvDesc, cpuHandle);
        }
    }

    m_TopLevelASGenerator.Generate(Gfx::GetRTCommandList(),
        m_TopLevelAS.pScratch.Get(),
        m_TopLevelAS.pResult.Get(),
        m_TopLevelAS.pInstanceDesc.Get(),
        UpdateOnly,
        m_TopLevelAS.pResult.Get()
    );

    m_TopLevelAS.pResult->SetName(L"Ray Tracing acceleration structure");
    m_TopLevelAS.pScratch->SetName(L"Ray Tracing acceleration structure scratch");
    m_TopLevelAS.pInstanceDesc->SetName(L"Ray Tracing acceleration structure instance desc");
}

void RayTracingMaterial::RemovePrevFrameBuffers()
{
    if(m_PrevTopLevelAS.pResult) m_PrevTopLevelAS.pResult.Reset();
    if (m_PrevTopLevelAS.pScratch) m_PrevTopLevelAS.pScratch.Reset();
    if (m_PrevTopLevelAS.pInstanceDesc) m_PrevTopLevelAS.pInstanceDesc.Reset();
}

RayGen* RayTracingMaterial::GetRayGen(u32 index)
{
    if(index >= m_vecRaygen.size()) return nullptr;
    return &m_vecRaygen[index];
}

HitGroup* RayTracingMaterial::GetHitGroup(std::string hitgroupName)
{
    auto result = m_mapHitGroup.find(hitgroupName);
    if(result == m_mapHitGroup.end()) return nullptr;

    return &result->second;
}

Miss* RayTracingMaterial::GetMiss(u32 index)
{
    if (index >= m_vecMiss.size()) return nullptr;
    return &m_vecMiss[index];
}

void RayTracingMaterial::CreateShaderTable()
{
    m_TableGenerator.Reset();

    for (auto& raygen : m_vecRaygen)
    {
        if (raygen.HeapStartAddress.ptr != 0)
        {
            auto HeapPointer = reinterpret_cast<UINT64*>(raygen.HeapStartAddress.ptr);
            m_TableGenerator.AddRayGenerationProgram(raygen.RayGenerationShader->Name, { HeapPointer });
        }
        else
        {
            m_TableGenerator.AddRayGenerationProgram(raygen.RayGenerationShader->Name, {});
        }
    }
    for (auto& miss : m_vecMiss)
    {
        if (miss.HeapStartAddress.ptr != 0)
        {
            auto HeapPointer = reinterpret_cast<UINT64*>(miss.HeapStartAddress.ptr);
            m_TableGenerator.AddMissProgram(miss.MissShader->Name, { HeapPointer });
        }
        else
        {
            m_TableGenerator.AddMissProgram(miss.MissShader->Name, {});
        }
    }
    for (auto& [id, hits] : m_mapHitGroup)
    {
        if (hits.HeapStartAddress.ptr != 0)
        {
            auto HeapPointer = reinterpret_cast<UINT64*>(hits.HeapStartAddress.ptr);
            m_TableGenerator.AddHitGroup(hits.Name, { HeapPointer });
        }
        else
        {
            m_TableGenerator.AddHitGroup(hits.Name, {});
        }
    }

    uint32_t sbtSize = m_TableGenerator.ComputeSBTSize();

    m_ShaderTableStorage = Gfx::CreateBuffer(
        sbtSize,
        D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        kUploadHeapProps);

    m_TableGenerator.Generate(m_ShaderTableStorage.Get(), m_StateObjectProperties.Get());
}

void RayTracingMaterial::ReflectionLibrary(IDxcBlob* ShaderCode)
{
    ComPtr<ID3D12LibraryReflection> reflection;

    IDxcContainerReflection* ContainerReflection;
    D3DCall(gDxcDllHelper.CreateInstance(CLSID_DxcContainerReflection, &ContainerReflection));
    D3DCall(ContainerReflection->Load(ShaderCode));

    const u32 DxilPartKind = DXIL_FOURCC('D', 'X', 'I', 'L');
    u32 DxilPartIndex = ~0u;
    D3DCall(ContainerReflection->FindFirstPartKind(DxilPartKind, &DxilPartIndex));

    HRESULT Result = ContainerReflection->GetPartReflection(DxilPartIndex, IID_PPV_ARGS(&reflection));
    ContainerReflection->Release();

    D3D12_LIBRARY_DESC libDesc{};
    reflection->GetDesc(&libDesc);

    //함수 리플렉션
    for (int i = 0; i < (int)libDesc.FunctionCount; ++i)
    {
        auto funcReflection = reflection->GetFunctionByIndex(i);
        D3D12_FUNCTION_DESC funcDesc{};
        funcReflection->GetDesc(&funcDesc);

        RTShaderMetaData shader{};
        
        shader.Type = (ShaderType)D3D12_SHVER_GET_TYPE(funcDesc.Version);
        switch (shader.Type)
        {
        case ShaderType::AnyHit:
        case ShaderType::ClosestHit:
        case ShaderType::Miss:
            shader.payloadName = GetPayloadName(funcDesc.Name);
            auto iter = std::find(m_vecPayloadName.begin(), m_vecPayloadName.end(), shader.payloadName);
            if (iter == m_vecPayloadName.end()) m_vecPayloadName.push_back(shader.payloadName);
        }
        
        std::string s = GetPureNameOfFunction(funcDesc.Name);
        auto index = m_vecName.size();
        m_vecName.push_back(WideString{ s });
        shader.Name = m_vecName[index].c_str();

        for (int j = funcDesc.BoundResources -1; j >= 0; --j)
        {
            D3D12_SHADER_INPUT_BIND_DESC resDesc{};
            funcReflection->GetResourceBindingDesc(j, &resDesc);
            RTResourceMetaData res{};
            res.Type  =  resDesc.Type;
            if (res.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER)
            {
                auto reflCbuf = funcReflection->GetConstantBufferByName(resDesc.Name);
                D3D12_SHADER_BUFFER_DESC cbufDesc{};
                reflCbuf->GetDesc(&cbufDesc);
                res.DataSize = cbufDesc.Size;
            }
            res.BindPoint = resDesc.BindPoint;

            auto index = shader.vecResourceData.size();
            shader.mapResourceIndexing[resDesc.Name] = (u32)index;
            shader.vecResourceData.push_back(res);
        }

        m_vecShader.push_back(shader);
    }
    
    //Heap 생성
    m_HeapSize = 0;
    for (auto& shader : m_vecShader)
    {
        m_HeapSize += (u32)shader.vecResourceData.size();
    }
    m_DescriptorHeap.Create(Gfx::GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_HeapSize, true);

    //raygen, hitgroup, miss 셰이더 데이터 생성
    for (auto& shader : m_vecShader){
        switch (shader.Type){
        case ShaderType::RayGeneration:
        {
            m_vecRaygen.push_back(RayGen{});
            m_vecRaygen.back().RayGenerationShader = &shader;
            continue;
        }

        case ShaderType::Miss:
        {
            m_vecMiss.push_back(Miss{});
            m_vecMiss.back().MissShader = &shader;
            continue;
        }
        case ShaderType::AnyHit:
        case ShaderType::ClosestHit:
        {   
            auto& hits = m_mapHitGroup[shader.payloadName];

            if (!hits.RepresentationShader) hits.RepresentationShader = &shader;

            if (shader.Type == ShaderType::AnyHit)
            {
                hits.AnyHitShader = &shader;
            }
            else if (shader.Type == ShaderType::ClosestHit)
            {
                hits.ChsShader = &shader;
            }
            continue;
        }
        default:
            continue;
        }
    }

    for (auto& raygen : m_vecRaygen)
    {
        Init(raygen);
    }

    for (auto& miss : m_vecMiss)
    {
        Init(miss);
    }

    for (auto& [id, hits] : m_mapHitGroup)
    {
        //hitgroup 이름 지정 - 걍 id로 하자
        m_vecName.push_back(WideString{id});
        hits.Name = m_vecName.back().c_str();
        Init(hits);
    }

    m_OutputTexure = m_vecRaygen.back().OutputTexure;
}

void RayTracingMaterial::Init(RayGen& raygen)
{
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.DepthOrArraySize = 1;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    resDesc.Width = Gfx::GetWidth();
    resDesc.Height = Gfx::GetHeight();
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;

    D3DCall(Gfx::GetDevice()->CreateCommittedResource(&kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&raygen.OutputTexure)));

    raygen.EntrySize = 0;
    //Create Local RootSignature
    {
        GenerateLocalRootSignature(raygen.LocalRootSignatureDesc, *raygen.RayGenerationShader);
        raygen.LocalRootSignature = createRootSignature(Gfx::GetDevice(), raygen.LocalRootSignatureDesc.desc);
        
        if (!raygen.LocalRootSignatureDesc.rootParams.empty())
        {
            raygen.HeapStartAddress = m_DescriptorHeap.hGPU(m_HeapCurrentIndex);
        }
    }

    raygen.EntrySize += g_ShaderIdentifierSize;

    for (int i = 0; i < raygen.RayGenerationShader->vecResourceData.size(); ++i)
    {
        auto& res = raygen.RayGenerationShader->vecResourceData[i];
        switch (res.Type) {
        case D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER:
        {
            raygen.mapConstantBuffers[i].Init(Gfx::GetDevice(), res.DataSize);
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
            desc.BufferLocation = raygen.mapConstantBuffers[i].GetResource()->GetGPUVirtualAddress();
            desc.SizeInBytes = raygen.mapConstantBuffers[i].GetBufferSize();
            raygen.vecHeapOffsetIndex.push_back(m_HeapCurrentIndex++);
            Gfx::GetDevice()->CreateConstantBufferView(&desc, m_DescriptorHeap.hCPU(raygen.vecHeapOffsetIndex.back()));
            raygen.EntrySize += g_ShaderDescriptorOffset;
            break;
        }
        case D3D_SHADER_INPUT_TYPE::D3D11_SIT_UAV_RWTYPED:
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            raygen.vecHeapOffsetIndex.push_back(m_HeapCurrentIndex++);
            Gfx::GetDevice()->CreateUnorderedAccessView(raygen.OutputTexure.Get(), nullptr, &uavDesc, m_DescriptorHeap.hCPU(raygen.vecHeapOffsetIndex.back()));
            raygen.EntrySize += g_ShaderDescriptorOffset;
            break;
        }
        case D3D_SHADER_INPUT_TYPE::D3D_SIT_RTACCELERATIONSTRUCTURE:
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.RaytracingAccelerationStructure.Location = m_TopLevelAS.pResult->GetGPUVirtualAddress();

            raygen.vecHeapOffsetIndex.push_back(m_HeapCurrentIndex++);
            Gfx::GetDevice()->CreateShaderResourceView(nullptr, &srvDesc, m_DescriptorHeap.hCPU(raygen.vecHeapOffsetIndex.back()));
            m_vecTLASViewCPUHandle.push_back(m_DescriptorHeap.hCPU(raygen.vecHeapOffsetIndex.back()));
            raygen.EntrySize += g_ShaderDescriptorOffset;
            break;
        }
        default:
        {
            raygen.vecHeapOffsetIndex.push_back(m_HeapCurrentIndex++);
            raygen.EntrySize += g_ShaderDescriptorOffset;
            break;
        }
        }
    }
}

void RayTracingMaterial::Init(HitGroup& hitgroup)
{
    hitgroup.EntrySize = 0;
    //Create Local RootSignature
    {
        GenerateLocalRootSignature(hitgroup.LocalRootSignatureDesc, *hitgroup.RepresentationShader);
        hitgroup.LocalRootSignature = createRootSignature(Gfx::GetDevice(), hitgroup.LocalRootSignatureDesc.desc);
        if (!hitgroup.LocalRootSignatureDesc.rootParams.empty())
        {
            hitgroup.HeapStartAddress = m_DescriptorHeap.hGPU(m_HeapCurrentIndex);
        }
    }

    hitgroup.EntrySize += g_ShaderIdentifierSize;
    
    for (int i = 0; i < hitgroup.RepresentationShader->vecResourceData.size(); ++i)
    {
        auto& res = hitgroup.RepresentationShader->vecResourceData[i];
        switch (res.Type) {
        case D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER:
        {
            hitgroup.mapConstantBuffers[i].Init(Gfx::GetDevice(), res.DataSize);
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
            desc.BufferLocation = hitgroup.mapConstantBuffers[i].GetResource()->GetGPUVirtualAddress();
            desc.SizeInBytes = hitgroup.mapConstantBuffers[i].GetBufferSize();
            hitgroup.vecHeapOffsetIndex.push_back(m_HeapCurrentIndex);
            Gfx::GetDevice()->CreateConstantBufferView(&desc, m_DescriptorHeap.hCPU(hitgroup.vecHeapOffsetIndex.back()));
            hitgroup.EntrySize += g_ShaderDescriptorOffset;
            break;
        }
        case D3D_SHADER_INPUT_TYPE::D3D11_SIT_UAV_RWTYPED:
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            hitgroup.vecHeapOffsetIndex.push_back(m_HeapCurrentIndex++);
            Gfx::GetDevice()->CreateUnorderedAccessView(m_OutputTexure.Get(), nullptr, &uavDesc, m_DescriptorHeap.hCPU(hitgroup.vecHeapOffsetIndex.back()));
            hitgroup.EntrySize += g_ShaderDescriptorOffset;
            break;
        }
        case D3D_SHADER_INPUT_TYPE::D3D_SIT_RTACCELERATIONSTRUCTURE:
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.RaytracingAccelerationStructure.Location = m_TopLevelAS.pResult->GetGPUVirtualAddress();

            hitgroup.vecHeapOffsetIndex.push_back(m_HeapCurrentIndex++);
            Gfx::GetDevice()->CreateShaderResourceView(nullptr, &srvDesc, m_DescriptorHeap.hCPU(hitgroup.vecHeapOffsetIndex.back()));
            m_vecTLASViewCPUHandle.push_back(m_DescriptorHeap.hCPU(hitgroup.vecHeapOffsetIndex.back()));
            hitgroup.EntrySize += g_ShaderDescriptorOffset;
            break;
        }
        default:
        {
            hitgroup.vecHeapOffsetIndex.push_back(m_HeapCurrentIndex++);
            hitgroup.EntrySize += g_ShaderDescriptorOffset;
            break; 
        }
        }
    }
}

void RayTracingMaterial::Init(Miss& miss)
{
    miss.EntrySize = 0;
    //Create Local RootSignature
    {
        GenerateLocalRootSignature(miss.LocalRootSignatureDesc, *miss.MissShader);
        miss.LocalRootSignature = createRootSignature(Gfx::GetDevice(), miss.LocalRootSignatureDesc.desc);
        if (!miss.LocalRootSignatureDesc.rootParams.empty())
        {
            miss.HeapStartAddress = m_DescriptorHeap.hGPU(m_HeapCurrentIndex);
        }
    }
    miss.EntrySize += g_ShaderIdentifierSize;
    
    for (int i = 0; i < miss.MissShader->vecResourceData.size(); ++i)
    {
        auto& res = miss.MissShader->vecResourceData[i];
        switch (res.Type) {
        case D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER:
        {
            miss.mapConstantBuffers[i].Init(Gfx::GetDevice(), res.DataSize);
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
            desc.BufferLocation = miss.mapConstantBuffers[i].GetResource()->GetGPUVirtualAddress();
            desc.SizeInBytes = miss.mapConstantBuffers[i].GetBufferSize();
            miss.vecHeapOffsetIndex.push_back(m_HeapCurrentIndex);
            Gfx::GetDevice()->CreateConstantBufferView(&desc, m_DescriptorHeap.hCPU(miss.vecHeapOffsetIndex.back()));
            miss.EntrySize += g_ShaderDescriptorOffset;
            break;
        }
        case D3D_SHADER_INPUT_TYPE::D3D11_SIT_UAV_RWTYPED:
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            miss.vecHeapOffsetIndex.push_back(m_HeapCurrentIndex++);
            Gfx::GetDevice()->CreateUnorderedAccessView(m_OutputTexure.Get(), nullptr, &uavDesc, m_DescriptorHeap.hCPU(miss.vecHeapOffsetIndex.back()));
            miss.EntrySize += g_ShaderDescriptorOffset;
            break;
        }
        case D3D_SHADER_INPUT_TYPE::D3D_SIT_RTACCELERATIONSTRUCTURE:
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.RaytracingAccelerationStructure.Location = m_TopLevelAS.pResult->GetGPUVirtualAddress();

            miss.vecHeapOffsetIndex.push_back(m_HeapCurrentIndex++);
            Gfx::GetDevice()->CreateShaderResourceView(nullptr, &srvDesc, m_DescriptorHeap.hCPU(miss.vecHeapOffsetIndex.back()));
            m_vecTLASViewCPUHandle.push_back(m_DescriptorHeap.hCPU(miss.vecHeapOffsetIndex.back()));
            miss.EntrySize += g_ShaderDescriptorOffset;
            break;
        }
        default:
        {
            miss.vecHeapOffsetIndex.push_back(m_HeapCurrentIndex++);
            miss.EntrySize += g_ShaderDescriptorOffset;
            break;
        }
        }
    }
}

u32 RayTracingMaterial::CalculateHeapSize()
{
    /*u32 output = 0;

    for (auto& shader : m_vecShader)
    {
        output += shader.vecResourceData.size();
    }

    return output;*/
    return 32 + 8; //id + 1 descriptorTable
}

void RayTracingMaterial::GenerateLocalRootSignature(RootSignatureDesc& desc, RTShaderMetaData& shader)
{
    for (auto& resource : shader.vecResourceData)
    {
        CD3DX12_DESCRIPTOR_RANGE range;
        switch (resource.Type)
        {
        case D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER:
            range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, resource.BindPoint);
            break;
        case D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWTYPED:
            range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, resource.BindPoint);
            break;
        case D3D_SHADER_INPUT_TYPE::D3D_SIT_RTACCELERATIONSTRUCTURE:
        case D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE:
            range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, resource.BindPoint);
            break;
        }
        range.OffsetInDescriptorsFromTableStart = (u32)desc.range.size();
        desc.range.push_back(range);
    }

    if (!desc.range.empty())
    {
        CD3DX12_ROOT_PARAMETER param;
        param.InitAsDescriptorTable((UINT)desc.range.size(), desc.range.data());
        desc.rootParams.push_back(param);

        desc.desc.NumParameters = (UINT)desc.rootParams.size();
        desc.desc.pParameters = desc.rootParams.data();
    }

    desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
}
