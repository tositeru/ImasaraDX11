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

#include <DirectXTK\Inc\WICTextureLoader.h>

Scene::Scene(UINT width, UINT height, std::wstring name)
	: DXSample(width, height, name)
{
}

void Scene::onInit()
{
	this->initMipmap();
	this->initArray(6);
	this->initCubemap();
	this->initMultisampling(8);

	{//サンプラー作成
		// 
		D3D11_SAMPLER_DESC desc = {};
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = 0.4f;
		desc.BorderColor[3] = 1.f;
		desc.MaxLOD = FLT_MAX;
		desc.MinLOD = FLT_MIN;
		//desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; //補間が聞いたサンプリング
		std::array<D3D11_TEXTURE_ADDRESS_MODE, 5> address = { {
			D3D11_TEXTURE_ADDRESS_WRAP,
			D3D11_TEXTURE_ADDRESS_MIRROR,
			D3D11_TEXTURE_ADDRESS_CLAMP,
			D3D11_TEXTURE_ADDRESS_BORDER,
			D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
		} };
		this->mpSamplers.reserve(address.size());
		for (auto mode : address) {
			desc.AddressU = mode;
			desc.AddressV = mode;
			desc.AddressW = mode;
			ID3D11SamplerState *pSampler;
			auto hr = this->mpDevice->CreateSamplerState(&desc, &pSampler);
			if (FAILED(hr)) {
				throw std::runtime_error("サンプラーの作成に失敗");
			}
			this->mpSamplers.emplace_back(pSampler);
			pSampler->Release();
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
	this->updateTitle();
}

void Scene::onUpdate()
{
}

void Scene::onKeyUp(UINT8 key)
{
	if (key == 'Z') {
		this->mMode = static_cast<decltype(this->mMode)>((this->mMode + 1) % eMODE_COUNT);
		this->updateTitle();
	}

	if (key == 'X') {
		this->mIsUseSampler = !this->mIsUseSampler;
		this->updateTitle();
	}

	if (key == VK_LEFT && 0 < this->mReadValue) {
		--this->mReadValue;
		this->updateTitle();
	}
	if (key == VK_RIGHT) {
		++this->mReadValue;
		this->updateTitle();
	}
	if (key == 'C') {
		this->mSamplerIndex = (this->mSamplerIndex + 1) % this->mpSamplers.size();
		this->updateTitle();
	}
}

void Scene::onRender()
{
	//GPUに必要なデータを設定する

	switch (this->mMode) {
	case eMODE_MIPMAP: this->runMipmap(); break;
	case eMODE_ARRAY: this->runArray(); break;
	case eMODE_CUBEMAP: this->runCubemap(); break;
	case eMODE_MULTISAMPLING: this->runMultisampling(); break;
	}

	if (this->mMode != eMODE_MULTISAMPLING) {
		//シェーダの結果をバックバッファーにコピーする
		this->mpImmediateContext->CopySubresourceRegion(this->mpBackBuffer.Get(), 0, 0, 0, 0, this->mpScreen.Get(), 0, nullptr);
	}
}

//ID3D11DeviceContext::GenerateMipsを使うときはtureにしてください
static const bool is_used_generatemips = false;
void Scene::initMipmap()
{//ミップマップ用のリソース初期化
	createShader(this->mMipmap.mpCSRead.GetAddressOf(), this->mpDevice.Get(), "CSMipmapRead.cso");
	createShader(this->mMipmap.mpCSWrite.GetAddressOf(), this->mpDevice.Get(), "CSMipmapWrite.cso");

	MipmapReadParam readParam;
	readParam.mipLevel = 0;
	readParam.isUseSampler = false;
	createConstantBuffer<MipmapReadParam>(this->mMipmap.mpCSReadParam.GetAddressOf(), this->mpDevice.Get(), &readParam);

	MipmapWriteParam writeParam;
	writeParam.baseColor = DirectX::SimpleMath::Vector4(1, 1, 0, 1);
	createConstantBuffer<MipmapWriteParam>(this->mMipmap.mpCSWriteParam.GetAddressOf(), this->mpDevice.Get(), &writeParam);

	auto desc = makeTex2DDesc(this->width(), this->height(), DXGI_FORMAT_R8G8B8A8_UNORM);
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	//ミップレベルを最大を指定している。使わない時は1を必ず指定すること
	desc.MipLevels = calMaxMipLevel(desc.Width, desc.Height);
	if(is_used_generatemips) {
		//　下のコードはID3D11DeviceContext::GenerateMipsを使ってミップマップを自動生成するときに使う
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET;
	}
	auto hr = this->mpDevice->CreateTexture2D(&desc, nullptr, this->mMipmap.mpResource.GetAddressOf());
	if (FAILED(hr)) {
		throw std::runtime_error("ミップマップ用のテクスチャ作成に失敗");
	}
	//シェーダリソースビューの場合は1つで全ミップレベルにアクセスできるが、設定で1つのミップレベルのみだけにアクセスできる
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = desc.Format;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	hr = this->mpDevice->CreateShaderResourceView(this->mMipmap.mpResource.Get(), &srvDesc, this->mMipmap.mpSRV.GetAddressOf());
	if (FAILED(hr)) {
		throw std::runtime_error("ミップマップ用のシェーダリソースビュー作成に失敗");
	}

	//各ミップレベルごとのアンオーダードアクセスビューを作成する必要がある
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Format = desc.Format;
	this->mMipmap.mpUAVs.reserve(desc.MipLevels);
	for (UINT level = 0; level < desc.MipLevels; ++level) {
		uavDesc.Texture2D.MipSlice = level;
		ID3D11UnorderedAccessView* pUAV = nullptr;
		hr = this->mpDevice->CreateUnorderedAccessView(this->mMipmap.mpResource.Get(), &uavDesc, &pUAV);
		if (FAILED(hr)) {
			throw std::runtime_error("ミップマップ用のアンオーダードアクセスビュー作成に失敗");
		}
		this->mMipmap.mpUAVs.emplace_back(pUAV);
		pUAV->Release();
	}


	//ミップマップにデータを書き込む
	this->mpImmediateContext->CSSetShader(this->mMipmap.mpCSWrite.Get(), nullptr, 0);
	std::array<DirectX::SimpleMath::Vector4, 6> colorTable = { {
			DirectX::SimpleMath::Vector4(1.0f, 0.7f, 0.7f, 1),
			DirectX::SimpleMath::Vector4(0.7f, 1.0f, 0.7f, 1),
			DirectX::SimpleMath::Vector4(0.7f, 0.7f, 1.0f, 1),
			DirectX::SimpleMath::Vector4(1.0f, 1.0f, 0.7f, 1),
			DirectX::SimpleMath::Vector4(1.0f, 0.7f, 1.0f, 1),
			DirectX::SimpleMath::Vector4(0.7f, 1.0f, 1.0f, 1),
		} };
	for (UINT mipLevel = 0; mipLevel < desc.MipLevels; ++mipLevel) {
		//定数バッファの設定
		MipmapWriteParam writeParam;
		writeParam.baseColor = colorTable[mipLevel % colorTable.size()];
		this->mpImmediateContext->UpdateSubresource(this->mMipmap.mpCSWriteParam.Get(), 0, nullptr, &writeParam, sizeof(writeParam), sizeof(writeParam));

		std::array<ID3D11Buffer*, 1> ppCBs = { { this->mMipmap.mpCSWriteParam.Get(), } };
		this->mpImmediateContext->CSSetConstantBuffers(0, static_cast<UINT>(ppCBs.size()), ppCBs.data());

		std::array<ID3D11UnorderedAccessView*, 1> ppUAVs = { { this->mMipmap.mpUAVs[mipLevel].Get(), } };
		std::array<UINT, 1> initCounts = { { 0u, } };
		this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());

		const auto dx = calDispatchCount(this->width() / (mipLevel + 1), 8);
		const auto dy = calDispatchCount(this->height() / (mipLevel + 1), 8);
		this->mpImmediateContext->Dispatch(dx, dy, 1);
	}
	std::array<ID3D11UnorderedAccessView*, 1> ppUAVs = { { nullptr } };
	std::array<UINT, 1> initCounts = { { 0u, } };
	this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());

	if (is_used_generatemips) {
		this->mpImmediateContext->GenerateMips(this->mMipmap.mpSRV.Get());
	}
}

void Scene::runMipmap()
{
	if (is_used_generatemips)
	{
		this->mpImmediateContext->CSSetShader(this->mMipmap.mpCSWrite.Get(), nullptr, 0);
		std::array<DirectX::SimpleMath::Vector4, 6> colorTable = { {
				DirectX::SimpleMath::Vector4(1.0f, 0.7f, 0.7f, 1),
				DirectX::SimpleMath::Vector4(0.7f, 1.0f, 0.7f, 1),
				DirectX::SimpleMath::Vector4(0.7f, 0.7f, 1.0f, 1),
				DirectX::SimpleMath::Vector4(1.0f, 1.0f, 0.7f, 1),
				DirectX::SimpleMath::Vector4(1.0f, 0.7f, 1.0f, 1),
				DirectX::SimpleMath::Vector4(0.7f, 1.0f, 1.0f, 1),
			} };
		D3D11_TEXTURE2D_DESC desc;
		this->mMipmap.mpResource->GetDesc(&desc);
		for (UINT mipLevel = 0; mipLevel < desc.MipLevels; ++mipLevel) {
			//定数バッファの設定
			MipmapWriteParam writeParam;
			writeParam.baseColor = colorTable[mipLevel % colorTable.size()];
			this->mpImmediateContext->UpdateSubresource(this->mMipmap.mpCSWriteParam.Get(), 0, nullptr, &writeParam, sizeof(writeParam), sizeof(writeParam));

			std::array<ID3D11Buffer*, 1> ppCBs = { { this->mMipmap.mpCSWriteParam.Get(), } };
			this->mpImmediateContext->CSSetConstantBuffers(0, static_cast<UINT>(ppCBs.size()), ppCBs.data());

			std::array<ID3D11UnorderedAccessView*, 1> ppUAVs = { { this->mMipmap.mpUAVs[mipLevel].Get(), } };
			std::array<UINT, 1> initCounts = { { 0u, } };
			this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());

			const auto dx = calDispatchCount(this->width()/ (mipLevel + 1), 8);
			const auto dy = calDispatchCount(this->height()/ (mipLevel + 1), 8);
			//this->mpImmediateContext->Dispatch(dx, dy, 1);
		}
		std::array<ID3D11UnorderedAccessView*, 1> ppUAVs = { { nullptr } };
		std::array<UINT, 1> initCounts = { { 0u, } };
		this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());
	}

	this->mpImmediateContext->CSSetShader(this->mMipmap.mpCSRead.Get(), nullptr, 0);

	MipmapReadParam param;
	param.isUseSampler = this->mIsUseSampler;
	param.mipLevel = this->mReadValue;
	this->mpImmediateContext->UpdateSubresource(this->mMipmap.mpCSReadParam.Get(), 0, nullptr, &param, sizeof(param), sizeof(param));
	std::array<ID3D11Buffer*, 1> ppCBs = { { this->mMipmap.mpCSReadParam.Get(), } };
	this->mpImmediateContext->CSSetConstantBuffers(0, static_cast<UINT>(ppCBs.size()), ppCBs.data());

	std::array<ID3D11ShaderResourceView*, 1> ppSRVs = { { this->mMipmap.mpSRV.Get(), } };
	this->mpImmediateContext->CSSetShaderResources(0, static_cast<UINT>(ppSRVs.size()), ppSRVs.data());

	std::array<ID3D11SamplerState*, 1> ppSamplers = { { this->mpSamplers[this->mSamplerIndex].Get(), } };
	this->mpImmediateContext->CSSetSamplers(0, static_cast<UINT>(ppSamplers.size()), ppSamplers.data());

	std::array<ID3D11UnorderedAccessView*, 1> ppUAVs = { { this->mpScreenUAV.Get(), } };
	std::array<UINT, 1> initCounts = { { 0u, } };
	this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());

	auto dx = calDispatchCount(this->width(), 8);
	auto dy = calDispatchCount(this->height(), 8);
	this->mpImmediateContext->Dispatch(dx, dy, 1);


	ppSRVs[0] = nullptr;
	this->mpImmediateContext->CSSetShaderResources(0, static_cast<UINT>(ppSRVs.size()), ppSRVs.data());
	ppUAVs[0] = nullptr;
	this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());
}

void Scene::initArray(UINT arrayLength)
{
	createShader(this->mArray.mpCSRead.GetAddressOf(), this->mpDevice.Get(), "CSArrayRead.cso");
	createShader(this->mArray.mpCSWrite.GetAddressOf(), this->mpDevice.Get(), "CSArrayWrite.cso");

	ArrayReadParam readParam;
	readParam.arrayIndex = 0;
	readParam.isUseSampler = false;
	createConstantBuffer<ArrayReadParam>(this->mArray.mpCSReadParam.GetAddressOf(), this->mpDevice.Get(), &readParam);

	ArrayWriteParam writeParam;
	writeParam.baseColor = DirectX::SimpleMath::Vector4(1, 1, 0, 1);
	createConstantBuffer<ArrayWriteParam>(this->mArray.mpCSWriteParam.GetAddressOf(), this->mpDevice.Get(), &writeParam);

	auto desc = makeTex2DDesc(this->width(), this->height(), DXGI_FORMAT_R8G8B8A8_UNORM);
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.ArraySize = arrayLength;
	auto hr = this->mpDevice->CreateTexture2D(&desc, nullptr, this->mArray.mpResource.GetAddressOf());
	if (FAILED(hr)) {
		throw std::runtime_error("テクスチャ配列用のテクスチャ作成に失敗");
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Format = desc.Format;
	srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = desc.ArraySize;
	hr = this->mpDevice->CreateShaderResourceView(this->mArray.mpResource.Get(), &srvDesc, this->mArray.mpSRV.GetAddressOf());
	if (FAILED(hr)) {
		throw std::runtime_error("テクスチャ配列用のシェーダリソースビュー作成に失敗");
	}

	//テクスチャ配列は1つのUnorderedAccessViewで全要素にアクセス可能(ミップマップは固定されるので注意)
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	uavDesc.Format = desc.Format;
	uavDesc.Texture2DArray.MipSlice = 0;
	uavDesc.Texture2DArray.FirstArraySlice = 0;
	uavDesc.Texture2DArray.ArraySize = desc.ArraySize;
	ID3D11UnorderedAccessView* pUAV = nullptr;
	hr = this->mpDevice->CreateUnorderedAccessView(this->mArray.mpResource.Get(), &uavDesc, &pUAV);
	if (FAILED(hr)) {
		throw std::runtime_error("テクスチャ配列用のアンオーダードアクセスビュー作成に失敗");
	}
	this->mArray.mpUAVs.emplace_back(pUAV);
	pUAV->Release();


	//テクスチャ配列にデータを書き込む
	this->mpImmediateContext->CSSetShader(this->mArray.mpCSWrite.Get(), nullptr, 0);
	std::array<ID3D11UnorderedAccessView*, 1> ppUAVs = { { this->mArray.mpUAVs[0].Get(), } };
	std::array<UINT, 1> initCounts = { { 0u, } };
	this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());
	std::array<DirectX::SimpleMath::Vector4, 6> colorTable = { {
			DirectX::SimpleMath::Vector4(1.0f, 0.7f, 0.7f, 1),
			DirectX::SimpleMath::Vector4(0.7f, 1.0f, 0.7f, 1),
			DirectX::SimpleMath::Vector4(0.7f, 0.7f, 1.0f, 1),
			DirectX::SimpleMath::Vector4(1.0f, 1.0f, 0.7f, 1),
			DirectX::SimpleMath::Vector4(1.0f, 0.7f, 1.0f, 1),
			DirectX::SimpleMath::Vector4(0.7f, 1.0f, 1.0f, 1),
	} };
	for (UINT index = 0; index < desc.ArraySize; ++index) {
		//定数バッファの設定
		ArrayWriteParam writeParam;
		writeParam.baseColor = colorTable[index % colorTable.size()];
		writeParam.arrayIndex = index;
		this->mpImmediateContext->UpdateSubresource(this->mArray.mpCSWriteParam.Get(), 0, nullptr, &writeParam, sizeof(writeParam), sizeof(writeParam));

		std::array<ID3D11Buffer*, 1> ppCBs = { { this->mArray.mpCSWriteParam.Get(), } };
		this->mpImmediateContext->CSSetConstantBuffers(0, static_cast<UINT>(ppCBs.size()), ppCBs.data());

		const auto dx = calDispatchCount(this->width(), 8);
		const auto dy = calDispatchCount(this->height(), 8);
		this->mpImmediateContext->Dispatch(dx, dy, 1);
	}
	ppUAVs = { { nullptr } };
	this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());
}

void Scene::runArray()
{
	this->mpImmediateContext->CSSetShader(this->mArray.mpCSRead.Get(), nullptr, 0);

	ArrayReadParam param;
	param.isUseSampler = this->mIsUseSampler;
	param.arrayIndex = this->mReadValue;
	this->mpImmediateContext->UpdateSubresource(this->mArray.mpCSReadParam.Get(), 0, nullptr, &param, sizeof(param), sizeof(param));
	std::array<ID3D11Buffer*, 1> ppCBs = { { this->mArray.mpCSReadParam.Get(), } };
	this->mpImmediateContext->CSSetConstantBuffers(0, static_cast<UINT>(ppCBs.size()), ppCBs.data());

	std::array<ID3D11ShaderResourceView*, 1> ppSRVs = { { this->mArray.mpSRV.Get(), } };
	this->mpImmediateContext->CSSetShaderResources(0, static_cast<UINT>(ppSRVs.size()), ppSRVs.data());

	std::array<ID3D11SamplerState*, 1> ppSamplers = { { this->mpSamplers[this->mSamplerIndex].Get(), } };
	this->mpImmediateContext->CSSetSamplers(0, static_cast<UINT>(ppSamplers.size()), ppSamplers.data());

	std::array<ID3D11UnorderedAccessView*, 1> ppUAVs = { { this->mpScreenUAV.Get(), } };
	std::array<UINT, 1> initCounts = { { 0u, } };
	this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());

	auto dx = calDispatchCount(this->width(), 8);
	auto dy = calDispatchCount(this->height(), 8);
	this->mpImmediateContext->Dispatch(dx, dy, 1);


	ppSRVs[0] = nullptr;
	this->mpImmediateContext->CSSetShaderResources(0, static_cast<UINT>(ppSRVs.size()), ppSRVs.data());
	ppUAVs[0] = nullptr;
	this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());

}

void Scene::initCubemap()
{
	{
		auto desc = makeTex2DDesc(1024, 1024, DXGI_FORMAT_R8G8B8A8_UNORM);
		desc.ArraySize = 6;
		desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		auto hr = this->mpDevice->CreateTexture2D(&desc, nullptr, this->mCubemap.mpResource.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("キューブマップの生成に失敗");
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Format = desc.Format;
		srvDesc.TextureCube.MipLevels = desc.MipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;
		hr = this->mpDevice->CreateShaderResourceView(this->mCubemap.mpResource.Get(), &srvDesc, this->mCubemap.mpSRV.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("キューブマップ用のシェーダリソースビュー作成に失敗");
		}

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Format = desc.Format;
		rtvDesc.Texture2DArray.ArraySize = 6;
		rtvDesc.Texture2DArray.FirstArraySlice = 0;
		rtvDesc.Texture2DArray.MipSlice = 0;
		hr = this->mpDevice->CreateRenderTargetView(this->mCubemap.mpResource.Get(), &rtvDesc, this->mCubemap.mpRTV.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("キューブマップ用のレンダーターゲットビュー作成に失敗");
		}

		//キューブマップに適当なものを描画する
		createShader(this->mCubemap.mpVSRenderCubemap.GetAddressOf(), this->mpDevice.Get(), "VSDummy.cso", nullptr);
		createShader(this->mCubemap.mpGSRenderCubemap.GetAddressOf(), this->mpDevice.Get(), "GSRenderCubemap.cso", nullptr);
		createShader(this->mCubemap.mpPSRenderCubemap.GetAddressOf(), this->mpDevice.Get(), "PSRenderCubemap.cso", nullptr);

		std::array<ID3D11RenderTargetView*, 1> ppRTVs = { {
				this->mCubemap.mpRTV.Get(),
			} };
		this->mpImmediateContext->OMSetRenderTargets(static_cast<UINT>(ppRTVs.size()), ppRTVs.data(), nullptr);
		this->mpImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		this->mpImmediateContext->VSSetShader(this->mCubemap.mpVSRenderCubemap.Get(), nullptr, 0);
		this->mpImmediateContext->GSSetShader(this->mCubemap.mpGSRenderCubemap.Get(), nullptr, 0);
		this->mpImmediateContext->PSSetShader(this->mCubemap.mpPSRenderCubemap.Get(), nullptr, 0);
		this->mpImmediateContext->Draw(1, 0);

		ppRTVs[0] = nullptr;
		this->mpImmediateContext->OMSetRenderTargets(static_cast<UINT>(ppRTVs.size()), ppRTVs.data(), nullptr);
	}
	{//描画用のシェーダ作成
		createShader(this->mCubemap.mpCSUseCubemap.GetAddressOf(), this->mpDevice.Get(), "CSUseCubemap.cso", nullptr);

		createConstantBuffer<Cubemap::CSUseCubemapParam>(this->mCubemap.mpCSUseCubemapParam.GetAddressOf(), this->mpDevice.Get(), nullptr);
	}
}

void Scene::runCubemap()
{
	this->mpImmediateContext->CSSetShader(this->mCubemap.mpCSUseCubemap.Get(), nullptr, 0);

	DirectX::SimpleMath::Matrix invPerspective;
	this->mCubemap.mCubemapParam.mInvProjection = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(tan(45.f * DirectX::XM_PI / 180.f), this->mWidth / static_cast<float>(this->mHeight), 0.1f, 100.f).Invert().Transpose();

	this->mCubemap.mRotaParam.x += 0.01f;
	this->mCubemap.mRotaParam.y += 0.02f;
	this->mCubemap.mRotaParam.z += 0.03f;
	this->mCubemap.mCubemapParam.mRotaMatrix = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(this->mCubemap.mRotaParam.x, this->mCubemap.mRotaParam.y, this->mCubemap.mRotaParam.z);

	this->mpImmediateContext->UpdateSubresource(this->mCubemap.mpCSUseCubemapParam.Get(), 0, nullptr, &this->mCubemap.mCubemapParam, sizeof(this->mCubemap.mCubemapParam), sizeof(this->mCubemap.mCubemapParam));
	std::array<ID3D11Buffer*, 1> ppCBs = { { this->mCubemap.mpCSUseCubemapParam.Get(), } };
	this->mpImmediateContext->CSSetConstantBuffers(0, static_cast<UINT>(ppCBs.size()), ppCBs.data());

	std::array<ID3D11ShaderResourceView*, 1> ppSRVs = { { this->mCubemap.mpSRV.Get(), } };
	this->mpImmediateContext->CSSetShaderResources(0, static_cast<UINT>(ppSRVs.size()), ppSRVs.data());

	std::array<ID3D11SamplerState*, 1> ppSamplers = { { this->mpSamplers[this->mSamplerIndex].Get(), } };
	this->mpImmediateContext->CSSetSamplers(0, static_cast<UINT>(ppSamplers.size()), ppSamplers.data());

	std::array<ID3D11UnorderedAccessView*, 1> ppUAVs = { { this->mpScreenUAV.Get(), } };
	std::array<UINT, 1> initCounts = { { 0u, } };
	this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());

	auto dx = calDispatchCount(this->width(), 8);
	auto dy = calDispatchCount(this->height(), 8);
	this->mpImmediateContext->Dispatch(dx, dy, 1);

	ppSRVs[0] = nullptr;
	this->mpImmediateContext->CSSetShaderResources(0, static_cast<UINT>(ppSRVs.size()), ppSRVs.data());
	ppUAVs[0] = nullptr;
	this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), initCounts.data());
}

void Scene::initMultisampling(UINT samplingCount)
{
	{
		auto desc = makeTex2DDesc(this->width(), this->height(), DXGI_FORMAT_R8G8B8A8_UNORM);
		desc.BindFlags = D3D11_BIND_RENDER_TARGET;
		//GPUによっては使える設定が異なるので、希望するサンプリングポイント数を指定し確認している。
		desc.SampleDesc.Count = samplingCount;
		this->mpDevice->CheckMultisampleQualityLevels(desc.Format, desc.SampleDesc.Count, &desc.SampleDesc.Quality);
		if (0 == desc.SampleDesc.Quality) {
			throw std::runtime_error("指定したフォーマットとサンプリング数はデバイスが対応していません");
		}
		desc.SampleDesc.Quality -= 1;//そのまま使うと上限を超えるので1引いている
		auto hr = this->mpDevice->CreateTexture2D(&desc, nullptr, this->mMultiSampling.mpResource.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("マルチサンプリング生成に失敗");
		}

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
		rtvDesc.Format = desc.Format;
		hr = this->mpDevice->CreateRenderTargetView(this->mMultiSampling.mpResource.Get(), &rtvDesc, this->mMultiSampling.mpRTV.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("マルチサンプリング用のレンダーターゲットビュー作成に失敗");
		}
	}
	{//効果があるのかわからなかった
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.MultisampleEnable = true;
		auto hr = this->mpDevice->CreateRasterizerState(&desc, this->mMultiSampling.mpRasterizerState.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("マルチサンプリング用のラスタライザーステートの失敗");
		}
	}
	{//描画用のシェーダ作成
		createShader(this->mMultiSampling.mpVSRenderCubemap.GetAddressOf(), this->mpDevice.Get(), "VSDummy.cso", nullptr);
		createShader(this->mMultiSampling.mpGSRenderCubemap.GetAddressOf(), this->mpDevice.Get(), "GSRenderMultisampling.cso", nullptr);
		createShader(this->mMultiSampling.mpPSRenderCubemap.GetAddressOf(), this->mpDevice.Get(), "PSRenderMultisampling.cso", nullptr);
	}
}

void Scene::runMultisampling()
{
	float clearColor[] = { 0, 0.1f, 0.125f, 1 };
	this->mpImmediateContext->ClearRenderTargetView(this->mMultiSampling.mpRTV.Get(), clearColor);

	std::array<ID3D11RenderTargetView*, 1> ppRTVs = { {
			this->mIsUseSampler ? this->mMultiSampling.mpRTV.Get() : this->mpBackBufferRTV.Get()
		} };
	this->mpImmediateContext->OMSetRenderTargets(static_cast<UINT>(ppRTVs.size()), ppRTVs.data(), nullptr);
	this->mpImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	this->mpImmediateContext->VSSetShader(this->mCubemap.mpVSRenderCubemap.Get(), nullptr, 0);
	this->mpImmediateContext->GSSetShader(this->mCubemap.mpGSRenderCubemap.Get(), nullptr, 0);
	this->mpImmediateContext->PSSetShader(this->mCubemap.mpPSRenderCubemap.Get(), nullptr, 0);
	this->mpImmediateContext->Draw(1, 0);

	ppRTVs[0] = nullptr;
	this->mpImmediateContext->OMSetRenderTargets(static_cast<UINT>(ppRTVs.size()), ppRTVs.data(), nullptr);
	this->mpImmediateContext->RSSetState(nullptr);

	//シェーダの結果をバックバッファーにコピーする
	if (this->mIsUseSampler) {
		D3D11_TEXTURE2D_DESC desc;
		this->mMultiSampling.mpResource->GetDesc(&desc);
		this->mpImmediateContext->ResolveSubresource(this->mpBackBuffer.Get(), 0, this->mMultiSampling.mpResource.Get(), 0, desc.Format);
	}
}

void Scene::onDestroy()
{
}

void Scene::updateTitle()
{
	std::wstring title = L"";
	switch (this->mMode) {
	case eMODE_MIPMAP:			title += L"eMODE_MIPMAP"; break;
	case eMODE_ARRAY:			title += L"eMODE_ARRAY"; break;
	case eMODE_CUBEMAP:			title += L"eMODE_CUBEMAP"; break;
	case eMODE_MULTISAMPLING:	title += L"eMODE_MULTISAMPLING"; break;
	default:
		assert(false && "未実装");
	}
	title += L"; Read Value=" + std::to_wstring(this->mReadValue);
	title += L"; Use Sampler=" + std::to_wstring(this->mIsUseSampler);
	title += L"; Sampler Index=" + std::to_wstring(this->mSamplerIndex);

	this->setCustomWindowText(title.c_str());
}
