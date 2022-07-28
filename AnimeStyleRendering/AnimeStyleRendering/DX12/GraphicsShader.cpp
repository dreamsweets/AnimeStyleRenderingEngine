#include "GraphicsShader.h"

void GraphicsShader::LoadByPath(std::string fullPath)
{
	assert(Path::IsExist(fullPath));

	m_Name = Path::GetFileName(fullPath);

	std::wstring widePath{ fullPath.begin(), fullPath.end() };

	//셰이더 컴파일
	{
	#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	#else
		UINT compileFlags = 0;
	#endif

		ComPtr<ID3DBlob> error;
		if (FAILED(D3DCompileFromFile(widePath.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &m_VertexShader.ShaderBlob, &error))) {

			std::string errorString = (const char*)error->GetBufferPointer();
			MSGBox("VS 컴파일 에러 : " + errorString);
		}
		if (FAILED(D3DCompileFromFile(widePath.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &m_PixelShader.ShaderBlob, &error)))
		{
			std::string errorString = (const char*)error->GetBufferPointer();
			MSGBox("PS 컴파일 에러 : " + errorString);
		}
	}

	ReflectionVS(m_VertexShader.ShaderBlob.Get());
	ReflectionPS(m_PixelShader.ShaderBlob.Get());
}

GraphicsShader::~GraphicsShader()
{
	for (auto& ch : m_vecSemantics)
	{
		delete[] ch;
	}
}

bool GraphicsShader::FindMetaDataVS(std::string key, ShaderMetaData& out)
{
	auto result = m_VertexShader.MapMetaData.find(key);
	if (result == m_VertexShader.MapMetaData.end()) return false;
	
	out = result->second;

	return true;
}

bool GraphicsShader::FindMetaDataPS(std::string key, ShaderMetaData& out)
{
	auto result = m_PixelShader.MapMetaData.find(key);
	if (result == m_PixelShader.MapMetaData.end()) return false;

	out = result->second;

	return true;
}


void GraphicsShader::ReflectionVS(ID3DBlob* shader)
{
	ComPtr<ID3D12ShaderReflection> reflection;
	D3DCall(D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&reflection)));
	D3D12_SHADER_DESC shader_desc{};
	reflection->GetDesc(&shader_desc);

	//인풋레이아웃 파싱
	{
		auto& il{ m_InputElements };
		for (auto i = 0u; i < shader_desc.InputParameters; ++i)
		{
			D3D12_SIGNATURE_PARAMETER_DESC params{};
			reflection->GetInputParameterDesc(i, &params);

			auto elementIndex = il.size();
			il.emplace_back();
			auto& element{ il[elementIndex] };

			auto semanticNameIndex = m_vecSemantics.size();

			char* ch = new char[strlen(params.SemanticName) + 1];
			strcpy(ch, params.SemanticName);

			m_vecSemantics.push_back(ch);
			element.SemanticName = m_vecSemantics[semanticNameIndex];
			element.SemanticIndex = params.SemanticIndex;
			element.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
			element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;

			if (params.Mask == 1)
			{
				if (params.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element.Format = DXGI_FORMAT_R32_UINT;
				else if (params.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element.Format = DXGI_FORMAT_R32_SINT;
				else if (params.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element.Format = DXGI_FORMAT_R32_FLOAT;
			}
			else if (params.Mask <= 3)
			{
				if (params.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element.Format = DXGI_FORMAT_R32G32_UINT;
				else if (params.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element.Format = DXGI_FORMAT_R32G32_SINT;
				else if (params.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element.Format = DXGI_FORMAT_R32G32_FLOAT;
			}
			else if (params.Mask <= 7)
			{
				if (params.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element.Format = DXGI_FORMAT_R32G32B32_UINT;
				else if (params.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element.Format = DXGI_FORMAT_R32G32B32_SINT;
				else if (params.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			}
			else if (params.Mask <= 15)
			{
				if (params.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element.Format = DXGI_FORMAT_R32G32B32A32_UINT;
				else if (params.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element.Format = DXGI_FORMAT_R32G32B32A32_SINT;
				else if (params.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
		}
	}

	//리소스 파싱
	{
		for (int i = 0; i < (int)shader_desc.BoundResources; ++i)
		{
			D3D12_SHADER_INPUT_BIND_DESC inputBindDesc{};
			reflection->GetResourceBindingDesc(i, &inputBindDesc);

			m_VertexShader.MapMetaData.insert({ inputBindDesc.Name, ShaderMetaData{} });
			auto& metaData = m_VertexShader.MapMetaData[inputBindDesc.Name];
			metaData.Type = inputBindDesc.Type;
			metaData.Name = inputBindDesc.Name;
			metaData.BindPoint = inputBindDesc.BindPoint;

			if (inputBindDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER)
			{
				//상수버퍼 처리
				ID3D12ShaderReflectionConstantBuffer* reflCbuf =
					reflection->GetConstantBufferByName(inputBindDesc.Name);
				D3D12_SHADER_BUFFER_DESC cbufDesc{};
				reflCbuf->GetDesc(&cbufDesc);

				metaData.DataSize = cbufDesc.Size;
				metaData.ByteStride = metaData.DataSize;
				metaData.variableReflection.size = cbufDesc.Size;
				metaData.variableReflection.name = inputBindDesc.Name;

				for (int j = 0; j < (int)cbufDesc.Variables; ++j)
				{
					auto variable = reflCbuf->GetVariableByIndex(j);
					D3D12_SHADER_VARIABLE_DESC varDesc{};
					variable->GetDesc(&varDesc);
					ReflectionVariable var;
					var.parent = &metaData.variableReflection;
					var.name = varDesc.Name;
					var.offset = varDesc.StartOffset;
					var.size = varDesc.Size;

					D3D12_SHADER_TYPE_DESC typeDesc;
					variable->GetType()->GetDesc(&typeDesc);
					var.type = typeDesc.Name;

					auto index = metaData.variableReflection.vecVariable.size();
					metaData.variableReflection.vecVariable.push_back(var);
					metaData.variableReflection.mapVariableNames[var.name] = (u32)index;
				}
			}
		}
	}
}

void GraphicsShader::ReflectionPS(ID3DBlob* shader)
{
	ComPtr<ID3D12ShaderReflection> reflection;
	D3DCall(D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&reflection)));
	D3D12_SHADER_DESC shader_desc{};
	reflection->GetDesc(&shader_desc);

	//리소스 파싱
	{
		for (int i = 0; i < (int)shader_desc.BoundResources; ++i)
		{
			D3D12_SHADER_INPUT_BIND_DESC inputBindDesc{};
			reflection->GetResourceBindingDesc(i, &inputBindDesc);
			m_PixelShader.MapMetaData.insert({ inputBindDesc.Name, ShaderMetaData{} });
			auto& metaData = m_PixelShader.MapMetaData[inputBindDesc.Name];
			metaData.Type = inputBindDesc.Type;
			metaData.Name = inputBindDesc.Name;
			metaData.BindPoint = inputBindDesc.BindPoint;

			if (inputBindDesc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER)
			{
				//상수버퍼 처리
				ID3D12ShaderReflectionConstantBuffer* reflCbuf =
					reflection->GetConstantBufferByName(inputBindDesc.Name);
				D3D12_SHADER_BUFFER_DESC cbufDesc{};
				reflCbuf->GetDesc(&cbufDesc);

				metaData.DataSize = cbufDesc.Size;
				metaData.ByteStride = metaData.DataSize;
				metaData.variableReflection.size = cbufDesc.Size;
				metaData.variableReflection.name = inputBindDesc.Name;

				for (int j = 0; j < (int)cbufDesc.Variables; ++j)
				{
					auto variable = reflCbuf->GetVariableByIndex(j);
					D3D12_SHADER_VARIABLE_DESC varDesc{};
					variable->GetDesc(&varDesc);
					ReflectionVariable var;
					var.parent = &metaData.variableReflection;
					var.name = varDesc.Name;
					var.offset = varDesc.StartOffset;
					var.size = varDesc.Size;

					D3D12_SHADER_TYPE_DESC typeDesc;
					variable->GetType()->GetDesc(&typeDesc);
					var.type = typeDesc.Name;

					auto index = metaData.variableReflection.vecVariable.size();
					metaData.variableReflection.vecVariable.push_back(var);
					metaData.variableReflection.mapVariableNames[var.name] = (u32)index;
				}
			}
		}
	}
}
