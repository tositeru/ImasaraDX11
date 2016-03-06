//*********************************************************
// The MIT License(MIT)
// Copyright(c) 2016 tositeru
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files(the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions :
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//*********************************************************

#pragma once

#include <string>
#include <array>
#include <fstream>
#include <vector>

#include <windows.h>
#include <wrl.h>
#include <shellapi.h>

#include <d3d11.h>
#include <dxgi.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>

inline bool loadBinaryFile(std::vector<char>* pOut, const char* filepath)
{
	std::ifstream file(filepath, std::ifstream::binary);
	if (!file.good()) {
		return false;
	}
	file.seekg(0, file.end);
	size_t fileLength = file.tellg();
	file.seekg(0, file.beg);

	if (-1 == fileLength) {
		return false;
	}

	pOut->resize(fileLength);
	file.read(pOut->data(), fileLength);
	return true;
}

template<typename ShaderType>
void CreateShader(ShaderType** pOut, ID3D11Device* pDevice, const std::string& filepath, std::vector<char>* pOutByteCode = nullptr);

template<>
inline void CreateShader<ID3D11VertexShader>(ID3D11VertexShader** ppOut, ID3D11Device* pDevice, const std::string& filepath, std::vector<char>* pOutByteCode)
{
	std::vector<char> byteCode;
	auto* pTarget = pOutByteCode ? pOutByteCode : &byteCode;
	if (!loadBinaryFile(pTarget, filepath.c_str())) {
		throw std::runtime_error(filepath + "の読み込みに失敗");
	}

	HRESULT hr;
	hr = pDevice->CreateVertexShader(pTarget->data(), static_cast<SIZE_T>(pTarget->size()), nullptr, ppOut);
	if (FAILED(hr)) {
		throw std::runtime_error(filepath + "の作成に失敗");
	}
}

template<>
inline void CreateShader<ID3D11GeometryShader>(ID3D11GeometryShader** ppOut, ID3D11Device* pDevice, const std::string& filepath, std::vector<char>* pOutByteCode)
{
	std::vector<char> byteCode;
	auto* pTarget = pOutByteCode ? pOutByteCode : &byteCode;
	if (!loadBinaryFile(pTarget, filepath.c_str())) {
		throw std::runtime_error(filepath + "の読み込みに失敗");
	}

	HRESULT hr;
	hr = pDevice->CreateGeometryShader(pTarget->data(), static_cast<SIZE_T>(pTarget->size()), nullptr, ppOut);
	if (FAILED(hr)) {
		throw std::runtime_error(filepath + "の作成に失敗");
	}
}

template<>
inline void CreateShader<ID3D11HullShader>(ID3D11HullShader** ppOut, ID3D11Device* pDevice, const std::string& filepath, std::vector<char>* pOutByteCode)
{
	std::vector<char> byteCode;
	auto* pTarget = pOutByteCode ? pOutByteCode : &byteCode;
	if (!loadBinaryFile(pTarget, filepath.c_str())) {
		throw std::runtime_error(filepath + "の読み込みに失敗");
	}

	HRESULT hr;
	hr = pDevice->CreateHullShader(pTarget->data(), static_cast<SIZE_T>(pTarget->size()), nullptr, ppOut);
	if (FAILED(hr)) {
		throw std::runtime_error(filepath + "の作成に失敗");
	}
}

template<>
inline void CreateShader<ID3D11DomainShader>(ID3D11DomainShader** ppOut, ID3D11Device* pDevice, const std::string& filepath, std::vector<char>* pOutByteCode)
{
	std::vector<char> byteCode;
	auto* pTarget = pOutByteCode ? pOutByteCode : &byteCode;
	if (!loadBinaryFile(pTarget, filepath.c_str())) {
		throw std::runtime_error(filepath + "の読み込みに失敗");
	}

	HRESULT hr;
	hr = pDevice->CreateDomainShader(pTarget->data(), static_cast<SIZE_T>(pTarget->size()), nullptr, ppOut);
	if (FAILED(hr)) {
		throw std::runtime_error(filepath + "の作成に失敗");
	}
}

template<>
inline void CreateShader<ID3D11PixelShader>(ID3D11PixelShader** ppOut, ID3D11Device* pDevice, const std::string& filepath, std::vector<char>* pOutByteCode)
{
	std::vector<char> byteCode;
	auto* pTarget = pOutByteCode ? pOutByteCode : &byteCode;
	if (!loadBinaryFile(pTarget, filepath.c_str())) {
		throw std::runtime_error(filepath + "の読み込みに失敗");
	}

	HRESULT hr;
	hr = pDevice->CreatePixelShader(pTarget->data(), static_cast<SIZE_T>(pTarget->size()), nullptr, ppOut);
	if (FAILED(hr)) {
		throw std::runtime_error(filepath + "の作成に失敗");
	}
}

template<typename T>
inline void CreateIABuffer(ID3D11Buffer** ppOut, ID3D11Device* pDevice, UINT count, const T* pInitData, D3D11_BIND_FLAG bindFlag, D3D11_USAGE usage = D3D11_USAGE_DEFAULT, UINT cpuAccessFlags = 0u)
{
	D3D11_BUFFER_DESC desc = {};
	desc.BindFlags = bindFlag;
	desc.ByteWidth = sizeof(T) * count;
	desc.Usage = usage;
	desc.CPUAccessFlags = cpuAccessFlags;
	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pInitData;
	initData.SysMemPitch = desc.ByteWidth;
	auto hr = pDevice->CreateBuffer(&desc, &initData, ppOut);
	if (FAILED(hr)) {
		throw std::runtime_error("入力アセンブラステージ用のバッファ作成に失敗");
	}
}

template<typename T>
void CreateStructuredBuffer(ID3D11Buffer** ppOut, ID3D11ShaderResourceView** ppSRV, ID3D11UnorderedAccessView** ppUAV, ID3D11Device* pDevice, UINT count, const T* pInitData)
{
	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = sizeof(T)  * count;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.StructureByteStride = sizeof(T);
	desc.MiscFlags = desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pInitData;
	initData.SysMemPitch = desc.ByteWidth;
	auto hr = pDevice->CreateBuffer(&desc, &initData, ppOut);
	if (FAILED(hr)) {
		throw std::runtime_error("構造化バッファの作成に失敗");
	}

	if (ppSRV) {
		hr = pDevice->CreateShaderResourceView(*ppOut, nullptr, ppSRV);
		if (FAILED(hr)) {
			throw std::runtime_error("構造化バッファのシェーダリソースビュー作成に失敗");
		}
	}

	if (ppUAV) {
		hr = pDevice->CreateUnorderedAccessView(*ppOut, nullptr, ppUAV);
		if (FAILED(hr)) {
			throw std::runtime_error("構造化バッファのアンオーダードアクセスビュー作成に失敗");
		}
	}
}