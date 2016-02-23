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
	this->createScreenTexture();

	struct SphereInfo {
		DirectX::SimpleMath::Vector3 pos;
		float range;
		DirectX::SimpleMath::Vector3 color;
		float pad;//構造体のサイズを16の倍数にするために宣言している
	};

	{//定数バッファを使った球体描画の初期化
		this->compileComputeShader(&this->mpCSRenderSphereByCB, "RenderSphereByConstantBuffer.cso");

		D3D11_BUFFER_DESC desc = {};
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		desc.ByteWidth = sizeof(SphereInfo);
		desc.Usage = D3D11_USAGE_DEFAULT;

		SphereInfo info;
		info.pos = DirectX::SimpleMath::Vector3(0, 0, 30.f);
		info.range = 10.f;
		info.color = DirectX::SimpleMath::Vector3(1, 1, 0.4f);
		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = &info;
		initData.SysMemPitch = sizeof(info);
		auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpConstantBuffer.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("定数バッファの作成に失敗");
		}
	}

	{//構造化バッファを使った球体描画の初期化
		this->compileComputeShader(&this->mpCSRenderSphereByStructured, "RenderSphereByStructuredBuffer.cso");

		D3D11_BUFFER_DESC desc = {};
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.StructureByteStride = sizeof(SphereInfo);

		desc.ByteWidth = sizeof(SphereInfo);
		desc.Usage = D3D11_USAGE_DEFAULT;

		SphereInfo info;
		info.pos = DirectX::SimpleMath::Vector3(10, 0, 30.f);
		info.range = 5.f;
		info.color = DirectX::SimpleMath::Vector3(0.4f, 1.f, 0.4f);
		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = &info;
		initData.SysMemPitch = sizeof(info);
		auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpStructuredBuffer.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("構造化バッファの作成に失敗");
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.BufferEx.FirstElement = 0;
		srvDesc.BufferEx.Flags = 0;
		srvDesc.BufferEx.NumElements = 1;
		hr = this->mpDevice->CreateShaderResourceView(this->mpStructuredBuffer.Get(), &srvDesc, this->mpStructuredBufferSRV.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("構造化バッファのShaderResourceViewの作成に失敗");
		}
	}

	{//バイトアドレスバッファを使った球体描画の初期化
		this->compileComputeShader(&this->mpCSRenderSphereByByteAddress, "RenderSphereByByteAddressBuffer.cso");

		D3D11_BUFFER_DESC desc = {};
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

		desc.ByteWidth = sizeof(SphereInfo);
		desc.Usage = D3D11_USAGE_DEFAULT;

		SphereInfo info;
		info.pos = DirectX::SimpleMath::Vector3(-15, 0, 45.f);
		info.range = 15.f;
		info.color = DirectX::SimpleMath::Vector3(0.4f, 0.4f, 1.f);
		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = &info;
		initData.SysMemPitch = sizeof(info);
		auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpByteAddressBuffer.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("バイトアドレスバッファの作成に失敗");
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		srvDesc.BufferEx.FirstElement = 0;
		srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		srvDesc.BufferEx.NumElements = sizeof(SphereInfo) / 4;
		hr = this->mpDevice->CreateShaderResourceView(this->mpByteAddressBuffer.Get(), &srvDesc, this->mpByteAddressBufferSRV.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("バイトアドレスバッファのShaderResourceViewの作成に失敗");
		}
	}

	{//カメラ関係の定数バッファの作成
		DirectX::SimpleMath::Matrix invPerspective;
		invPerspective = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(tan(45.f * DirectX::XM_PI / 180.f), this->mWidth /static_cast<float>(this->mHeight), 0.1f, 100.f).Invert().Transpose();

		D3D11_BUFFER_DESC desc = {};
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.ByteWidth = sizeof(invPerspective);
		desc.Usage = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = &invPerspective;
		initData.SysMemPitch = sizeof(invPerspective);
		auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpCBCamera.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("カメラ用定数バッファの作成に失敗");
		}
	}
}

void Scene::onUpdate()
{
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
	case eMODE_CONSTANT_BUFFER:
	{
		this->mpImmediateContext->CSSetShader(this->mpCSRenderSphereByCB.Get(), nullptr, 0);
		std::array<ID3D11Buffer*, 1> ppCBs = { {
				this->mpConstantBuffer.Get(),
			} };
		this->mpImmediateContext->CSSetConstantBuffers(1, static_cast<UINT>(ppCBs.size()), ppCBs.data());
	}
		break;
	case eMODE_STRUCTURED_BUFFER:
	{
		this->mpImmediateContext->CSSetShader(this->mpCSRenderSphereByStructured.Get(), nullptr, 0);

		std::array < ID3D11ShaderResourceView*, 1> ppSRVs = { {
				this->mpStructuredBufferSRV.Get(),
			} };
		this->mpImmediateContext->CSSetShaderResources(0, static_cast<UINT>(ppSRVs.size()), ppSRVs.data());
		break;
	}
	case eMODE_BYTE_ADDRESS_BUFFER:
	{
		this->mpImmediateContext->CSSetShader(this->mpCSRenderSphereByByteAddress.Get(), nullptr, 0);

		std::array < ID3D11ShaderResourceView*, 1> ppSRVs = { {
				this->mpByteAddressBufferSRV.Get(),
			} };
		this->mpImmediateContext->CSSetShaderResources(0, static_cast<UINT>(ppSRVs.size()), ppSRVs.data());
		break;
	}
	default:
		assert(false);
	}

	std::array<ID3D11Buffer*, 1> ppCBs = { {
			this->mpCBCamera.Get(),
		} };
	this->mpImmediateContext->CSSetConstantBuffers(0, static_cast<UINT>(ppCBs.size()), ppCBs.data());

	std::array<ID3D11UnorderedAccessView*, 1> ppUAVs = { { this->mpScreenUAV.Get(), } };
	std::array<UINT, 1> initCounts = { { 0u, } };
	this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());

	//実行
	this->mpImmediateContext->Dispatch(this->mWidth, this->mHeight, 1);


	//シェーダの結果をバックバッファーにコピーする
	this->mpImmediateContext->CopySubresourceRegion(this->mpBackBuffer.Get(), 0, 0, 0, 0, this->mpScreen.Get(), 0, nullptr);
}

void Scene::onDestroy()
{
}



void Scene::compileComputeShader(Microsoft::WRL::ComPtr<ID3D11ComputeShader>* pOut, const std::string& filepath)
{
	std::vector<char> byteCode;
	if (!loadBinaryFile(&byteCode, filepath.c_str())) {
		throw std::runtime_error(filepath + "の読み込みに失敗");
	}

	HRESULT hr;
	hr = this->mpDevice->CreateComputeShader(byteCode.data(), static_cast<SIZE_T>(byteCode.size()), nullptr, pOut->GetAddressOf());
	if (FAILED(hr)) {
		throw std::runtime_error(filepath + "の作成に失敗");
	}
}

void Scene::createScreenTexture()
{
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
