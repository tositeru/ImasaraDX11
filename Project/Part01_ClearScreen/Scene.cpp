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

#include "Scene.h"

#include <DirectXTK\Inc\SimpleMath.h>

Scene::Scene(UINT width, UINT height, std::wstring name)
	: DXSample(width, height, name)
{
}

void Scene::onInit()
{
	this->updateTitle();

	{
		//実行中にシェーダをコンパイルし、ID3D11ComputeShaderを作成する
		std::array<D3D_SHADER_MACRO, 2> macros = { {
			{"DEFINE_MACRO", "float4(0, 1, 1, 1)"},
			{nullptr, nullptr},
		} };
		UINT compileFlag = 0;
#ifdef _DEBUG
		compileFlag |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		std::wstring filepath = L"Shader/ClearScreen.hlsl";
		const char* entryPoint = "main";
		//const char* entryPoint = "clearByOneThread"; // c++風な画面クリアシェーダを使いたいときはこちらを使用してください
		const char* shaderTarget = "cs_5_0";
		Microsoft::WRL::ComPtr<ID3DBlob> pShaderBlob, pErrorMsg;
		//シェーダのコンパイル
		HRESULT hr = D3DCompileFromFile(
			filepath.c_str(),
			macros.data(),
			nullptr,
			entryPoint,
			shaderTarget,
			compileFlag,
			0,
			pShaderBlob.GetAddressOf(),
			pErrorMsg.GetAddressOf());
		if (FAILED(hr)) {
			if (pErrorMsg) {
				OutputDebugStringA(static_cast<char*>(pErrorMsg->GetBufferPointer()));
			}
			throw std::runtime_error("ClearScreen.hlslのコンパイルに失敗");
		}
		//ID3D11ComputeShaderの作成
		hr = this->mpDevice->CreateComputeShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, &this->mpCSClearScreen);
		if (FAILED(hr)) {
			throw std::runtime_error("ClearScreenWithConstantBuffer.hlslの作成に失敗");
		}
	}

	{//コンパイル済みのシェーダからID3D11ComputeShaderを作成するコード
		std::vector<char> byteCode;
		if (!loadBinaryFile(&byteCode, "ClearScreen.cso")) {
			throw std::runtime_error("シェーダファイルの読み込みに失敗");
		}

		Microsoft::WRL::ComPtr<ID3D11ComputeShader> pShader;
		HRESULT hr;
		hr = this->mpDevice->CreateComputeShader(byteCode.data(), static_cast<SIZE_T>(byteCode.size()), nullptr, &pShader);
		if (FAILED(hr)) {
			throw std::runtime_error("ID3D11ComputerShaderの作成に失敗");
		}
	}

	{//シェーダの出力先の作成
		D3D11_TEXTURE2D_DESC desc = { };
		desc.Width = this->mWidth;
		desc.Height = this->mHeight;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		auto hr = this->mpDevice->CreateTexture2D(&desc, nullptr, &this->mpScreen);
		if (FAILED(hr)) {
			throw std::runtime_error("コンピュータシェーダの出力先用のID3D11Texture2Dの作成に失敗");
		}

		hr = this->mpDevice->CreateUnorderedAccessView(this->mpScreen.Get(), nullptr, &this->mpScreenUAV);
		if (FAILED(hr)) {
			throw std::runtime_error("コンピュータシェーダの出力先用のID3D11Texture2DのUnorderedAccessViewの作成に失敗");
		}
	}
}

void Scene::onUpdate()
{
}

void Scene::onKeyUp(UINT8 key)
{
}


void Scene::onRender()
{
	//実行するシェーダをGPUに設定する
	this->mpImmediateContext->CSSetShader(this->mpCSClearScreen.Get(), nullptr, 0);

	//シェーダのscreenとして扱うリソースを設定する。
	std::array<ID3D11UnorderedAccessView*, 1> ppUAVs = { { this->mpScreenUAV.Get(), } };
	std::array<UINT, 1> initCounts = { { 0u, } };
	this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());

	//ClearScreen.hlslの実行
	this->mpImmediateContext->Dispatch(this->mWidth, this->mHeight, 1);

	//補足: シェーダを使わず画面をクリアするコード
	//float value[4] = {1, 0.7f, 1, 1};//左から赤、緑、青、アルファ
	//this->mpImmediateContext->ClearUnorderedAccessViewFloat(this->mpScreenUAV.Get(), value);

	//シェーダの結果をバックバッファーにコピーする
	this->mpImmediateContext->CopySubresourceRegion(this->mpBackBuffer.Get(), 0, 0, 0, 0, this->mpScreen.Get(), 0, nullptr);
}

void Scene::onDestroy()
{
}

void Scene::updateTitle()
{
}
