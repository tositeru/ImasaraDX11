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

	{//定数バッファの作成
		Param param;	//ID3D11Bufferに設定するデータ
		param.clearColor = DirectX::SimpleMath::Vector4(1, 0.7f, 0.7f, 1.f);
		param.screenSize = DirectX::SimpleMath::Vector2(static_cast<float>(this->width()), static_cast<float>(this->height()));

		//作成するID3D11Bufferの設定情報
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(param) + (sizeof(param) % 16 == 0 ? 0 : 16 - sizeof(param) % 16);	//サイズは16の倍数でないといけない
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;	//ID3D11Bufferを定数バッファとして使うよう宣言している
		desc.Usage = D3D11_USAGE_DEFAULT;				//GPU上からしかID3D11Bufferの内容にアクセスできないよう宣言している
		desc.CPUAccessFlags = 0;						//CPUからのアクセスフラグの指定。今回はアクセスしないので何も設定しない

		//CPUとGPU間のデータ転送の時に使う構造体
		//ここではID3D11Bufferの初期データを設定するために使っている
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = &param;
		initData.SysMemPitch = sizeof(param);
		auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpCB.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("定数バッファの作成に失敗");
		}

	}
	{//マップ可能な定数バッファの作成
		Param param;	//ID3D11Bufferに設定するデータ
		param.clearColor = DirectX::SimpleMath::Vector4(0.7f, 1.f, 0.7f, 1.f);
		param.screenSize = DirectX::SimpleMath::Vector2(static_cast<float>(this->width()), static_cast<float>(this->height()));
		
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(param) + (sizeof(param) % 16 == 0 ? 0 : 16 - sizeof(param) % 16);//サイズは16の倍数でないといけない
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;	//ID3D11Bufferを定数バッファとして使うよう宣言している
		desc.Usage = D3D11_USAGE_DYNAMIC;				//GPU上では読み込みだけをCPUから書き込みだけをできるように宣言している
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		//CPUとGPU間のデータ転送の時に使う構造体
		//ここではID3D11Bufferの初期データを設定するために使っている
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = &param;
		initData.SysMemPitch = sizeof(param);

		auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpCBMappable.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("マップ可能な定数バッファの作成に失敗");
		}
	}

	{//画面クリアーするシェーダの作成
		createShader(this->mpCSClearScreen.GetAddressOf(), this->mpDevice.Get(), "ClearScreen.cso");
	}

	{//シェーダの出力先の作成
		D3D11_TEXTURE2D_DESC desc = {};
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
	static float t = 0.0f;
	t += 0.01f;

	{//ID3D11DeviceCOntext::UpdateSubresource関数を使った定数バッファの更新
		Param param;
		param.clearColor = DirectX::SimpleMath::Vector4(abs(sin(t)), (cos(t)), (sin(t)), 1.f);
		param.screenSize.x = static_cast<float>(this->width());
		param.screenSize.y = static_cast<float>(this->height());

		this->mpImmediateContext->UpdateSubresource(this->mpCB.Get(), 0, nullptr, &param, 0, 0);
	}

	{//ID3D11DeviceContext::Map関数を使った定数バッファの更新
		UINT subresourceIndex = 0;
		D3D11_MAPPED_SUBRESOURCE mapped;
		auto hr = this->mpImmediateContext->Map(this->mpCBMappable.Get(), subresourceIndex, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		if (SUCCEEDED(hr)) {
			Param* p = static_cast<Param*>(mapped.pData);
			p->clearColor = DirectX::SimpleMath::Vector4(abs(sin(t*2.f)), (cos(t)), (sin(t)), 1.f);
			p->screenSize.x = static_cast<float>(this->width());
			p->screenSize.y = static_cast<float>(this->height());
			//this->mpImmediateContext->Unmap(this->mpCBMappable.Get(), subresourceIndex);
		}
	}
}

void Scene::onKeyUp(UINT8 key)
{
}


void Scene::onRender()
{
	//実行するシェーダをGPUに設定する
	this->mpImmediateContext->CSSetShader(this->mpCSClearScreen.Get(), nullptr, 0);

	std::array<ID3D11Buffer*, 1> ppCBs = { {
		this->mpCBMappable.Get(),
	} };
	this->mpImmediateContext->CSSetConstantBuffers(0, static_cast<UINT>(ppCBs.size()), ppCBs.data());

	//シェーダのscreenとして扱うリソースを設定する。
	std::array<ID3D11UnorderedAccessView*, 1> ppUAVs = { { this->mpScreenUAV.Get(), } };
	std::array<UINT, 1> initCounts = { { 0u, } };
	this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());

	//ClearScreen.hlslの実行
	this->mpImmediateContext->Dispatch(this->mWidth, this->mHeight, 1);

	//シェーダの結果をバックバッファにコピーする
	this->mpImmediateContext->CopySubresourceRegion(this->mpBackBuffer.Get(), 0, 0, 0, 0, this->mpScreen.Get(), 0, nullptr);
}

void Scene::onDestroy()
{
}

void Scene::updateTitle()
{
}
