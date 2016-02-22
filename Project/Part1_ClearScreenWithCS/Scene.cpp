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
	{
		std::vector<char> byteCode;
		if (!loadBinaryFile(&byteCode, "ClearScreen.cso")) {
			throw std::runtime_error("シェーダファイルの読み込みに失敗");
		}

		HRESULT hr;
		hr = this->mpDevice->CreateComputeShader(byteCode.data(), static_cast<SIZE_T>(byteCode.size()), nullptr, &this->mpCSClearScreen);
		if (FAILED(hr)) {
			throw std::runtime_error("ID3D11ComputerShaderの作成に失敗");
		}

		if (!loadBinaryFile(&byteCode, "ClearScreenWithConstantBuffer.cso")) {
			throw std::runtime_error("シェーダファイルの読み込みに失敗");
		}
		hr = this->mpDevice->CreateComputeShader(byteCode.data(), static_cast<SIZE_T>(byteCode.size()), nullptr, &this->mpCSClearScreenWithConstantBuffer);
		if (FAILED(hr)) {
			throw std::runtime_error("ClearScreenWithConstantBuffer.csoの作成に失敗");
		}
	}

	{//画面クリア用の定数バッファの作成
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(float) * 4;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.Usage = D3D11_USAGE_DEFAULT;

		auto hr = this->mpDevice->CreateBuffer(&desc, nullptr, this->mpClearColor.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("画面クリアー用の定数バッファの作成に失敗");
		}
	}
	{//ClearScreen.hlslの出力先の作成
		D3D11_TEXTURE2D_DESC desc = { 0 };
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
	static float t = 0.f;
	t += 0.01f;
	if (1 < t) { t = 0.f; }

	DirectX::SimpleMath::Vector4 setValue(1, t, 0, 1);
	this->mpImmediateContext->UpdateSubresource(this->mpClearColor.Get(), 0, nullptr, &setValue, sizeof(setValue), sizeof(setValue));
	//todo Map/Unmapについて説明する？
}

void Scene::onKeyUp(UINT8 key)
{
	if (key == 'Z') {
		this->mMode = static_cast<decltype(this->mMode)>((this->mMode + 1) % eMODE_COUNT);
	}
}


void Scene::onRender()
{
	//GPUに必要なデータを設定する
	switch (this->mMode) {
	case eMODE_SOLID:					this->mpImmediateContext->CSSetShader(this->mpCSClearScreen.Get(), nullptr, 0); break;
	case eMODE_WITH_CONSTANT_BUFFER:	this->mpImmediateContext->CSSetShader(this->mpCSClearScreenWithConstantBuffer.Get(), nullptr, 0); break;
	default:
		assert(false);
	}

	std::array<ID3D11Buffer*, 1> ppCBs = { { this->mpClearColor.Get(), } };
	this->mpImmediateContext->CSSetConstantBuffers(0, static_cast<UINT>(ppCBs.size()), ppCBs.data());

	std::array<ID3D11UnorderedAccessView*, 1> ppUAVs = { { this->mpScreenUAV.Get(), } };
	std::array<UINT, 1> initCounts = { { 0u, } };
	this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());

	//ClearScreen.hlslの実行
	this->mpImmediateContext->Dispatch(this->mWidth, this->mHeight, 1);


	//シェーダの結果をバックバッファーにコピーする
	this->mpImmediateContext->CopySubresourceRegion(this->mpBackBuffer.Get(), 0, 0, 0, 0, this->mpScreen.Get(), 0, nullptr);
}

void Scene::onDestroy()
{
}

