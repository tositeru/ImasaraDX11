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
	//タイルリソースに対応しているかチェック
	D3D11_FEATURE_DATA_D3D11_OPTIONS1 featureData;
	this->mpDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS1, &featureData, sizeof(featureData));
	if (D3D11_TILED_RESOURCES_NOT_SUPPORTED == featureData.TiledResourcesTier) {
		throw std::runtime_error("使用しているデバイスはタイルリソースに対応していません。");
	}

	auto hr = this->mpImmediateContext.Get()->QueryInterface(IID_PPV_ARGS(&mpContext2));
	if (FAILED(hr)) {
		throw std::runtime_error("DeviceContext2が使えません");
	}

	{//タイルリソースの作成
		auto desc = makeTex2DDesc(this->width(), this->height(), DXGI_FORMAT_R8G8B8A8_UNORM);
		desc.MiscFlags = D3D11_RESOURCE_MISC_TILED;
		desc.Width = this->width();
		desc.Height = this->height();
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		//desc.MipLevels = calMaxMipLevel(desc.Width, desc.Height);
		auto create = [&](TileTexture* pOut) {
			auto hr = this->mpDevice->CreateTexture2D(&desc, nullptr, pOut->mpResource.GetAddressOf());
			if (FAILED(hr)) {
				throw std::runtime_error("タイルリソースの作成に失敗");
			}

			hr = this->mpDevice->CreateShaderResourceView(pOut->mpResource.Get(), nullptr, pOut->mpSRV.GetAddressOf());
			if (FAILED(hr)) {
				throw std::runtime_error("タイルリソースのシェーダリソースビューの作成に失敗");
			}
			hr = this->mpDevice->CreateUnorderedAccessView(pOut->mpResource.Get(), nullptr, pOut->mpUAV.GetAddressOf());
			if (FAILED(hr)) {
				throw std::runtime_error("タイルリソースのアンオーダードアクセスビューの作成に失敗");
			}
		};

		this->mTextures.resize(6);
		for (auto& tex : this->mTextures) {
			create(&tex);
		}
		create(&this->mTargetTex);
	}

	{//タイルプールの作成
		Microsoft::WRL::ComPtr<ID3D11Device2> pDevice2;
		auto hr = this->mpDevice.Get()->QueryInterface(IID_PPV_ARGS(&pDevice2));
		if (FAILED(hr)) {
			throw std::runtime_error("Device2が使えません。");
		}

		//タイルリソースの情報を取得する
		TileInfo tileInfo;
		tileInfo.subresourceTileCount = 1;
		tileInfo.firstSubresourceTile = 0;
		pDevice2->GetResourceTiling(this->mTargetTex.mpResource.Get(), &tileInfo.tileCount, &tileInfo.packedMipDesc, &tileInfo.tileShape, &tileInfo.subresourceTileCount, tileInfo.firstSubresourceTile, &tileInfo.subresourceTiling);
		for (auto& tex : this->mTextures) {
			tex.mInfo = tileInfo;
		}
		this->mTargetTex.mInfo = tileInfo;

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = 64 * 1024;//64KBの倍数でないといけない
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.MiscFlags = D3D11_RESOURCE_MISC_TILE_POOL;
		hr = this->mpDevice->CreateBuffer(&desc, nullptr, this->mpTilePool.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("タイルプールの作成に失敗");
		}

		UINT64 unitSize = tileInfo.tileCount * (64 * 1024);
		hr = this->mpContext2->ResizeTilePool(this->mpTilePool.Get(), unitSize * this->mTextures.size());
		if (FAILED(hr)) {
			throw std::runtime_error("タイルプールのリサイズに失敗");
		}
	}
	{//タイルリソースにタイルプールを設定する
		D3D11_TEXTURE2D_DESC desc;
		this->mTargetTex.mpResource->GetDesc(&desc);
		auto& tileInfo = this->mTargetTex.mInfo;

		std::array<D3D11_TILE_REGION_SIZE, 1 > regionSizes;
		regionSizes[0].Width = tileInfo.subresourceTiling.WidthInTiles;
		regionSizes[0].Height = tileInfo.subresourceTiling.HeightInTiles;
		regionSizes[0].Depth = tileInfo.subresourceTiling.DepthInTiles;
		regionSizes[0].bUseBox = true;
		regionSizes[0].NumTiles = regionSizes[0].Width * regionSizes[0].Height * regionSizes[0].Depth;

		for (UINT i = 0; i < this->mTextures.size(); ++i) {
			auto& tex = this->mTextures[i];

			//タイルリソース単位の座標値
			std::array<D3D11_TILED_RESOURCE_COORDINATE, 1> coordinates;
			coordinates[0].Subresource = 0;
			coordinates[0].X = 0;
			coordinates[0].Y = 0;
			coordinates[0].Z = 0;

			std::array<UINT, 1> ranges = { { 0 } };
			std::array<UINT, 1> offsets = { { i * tileInfo.tileCount } };
			std::array<UINT, 1> rangeTileCounts = { { tileInfo.tileCount } };
			UINT flags = 0;


			hr = this->mpContext2->UpdateTileMappings(
				tex.mpResource.Get(), static_cast<UINT>(coordinates.size()), coordinates.data(), regionSizes.data(),
				this->mpTilePool.Get(), static_cast<UINT>(ranges.size()), /*ranges.data()*/nullptr, offsets.data(), rangeTileCounts.data(), flags);
			if (FAILED(hr)) {
				throw std::runtime_error("タイルリソースがさすタイルプールの場所の設定に失敗");
			}
		}

		//タイルプールの同じ場所をマップすることもできる
		std::array<D3D11_TILED_RESOURCE_COORDINATE, 1> coordinates;
		coordinates[0].Subresource = 0;
		coordinates[0].X = 0;
		coordinates[0].Y = 0;
		coordinates[0].Z = 0;

		std::array<UINT, 1> ranges = { { 0 } };
		std::array<UINT, 1> offsets = { { 0 } };
		std::array<UINT, 1> rangeTileCounts = { { tileInfo.tileCount } };
		UINT flags = 0;
		hr = this->mpContext2->UpdateTileMappings(
			this->mTargetTex.mpResource.Get(), static_cast<UINT>(coordinates.size()), coordinates.data(), regionSizes.data(),
			this->mpTilePool.Get(), static_cast<UINT>(ranges.size()), /*ranges.data()*/nullptr, offsets.data(), rangeTileCounts.data(), flags);
		if (FAILED(hr)) {
			throw std::runtime_error("タイルリソースがさすタイルプールの場所の設定に失敗");
		}
	}

	{
		createShader(this->mpCSDrawTexture.GetAddressOf(), this->mpDevice.Get(), "CSDrawTexture.cso");

		DirectX::SimpleMath::Vector4 color(1,1, 0, 1);
		createConstantBuffer(this->mpCBParam.GetAddressOf(), this->mpDevice.Get(), &color);

		//this->mpContext2->Flush();
		std::array<DirectX::SimpleMath::Vector4, 5> colorTable = { {
			{ 1, 0, 0, 1 },
			{ 0, 1, 0, 1 },
			{ 0, 0, 1, 1 },
			{ 1, 1, 0, 1 },
			{ 1, 0, 1, 1 },
		} };
		for (UINT i = 0; i < this->mTextures.size()-1; ++i) {
			auto& tex = this->mTextures[i];

			this->mpImmediateContext->UpdateSubresource(this->mpCBParam.Get(), 0, nullptr, &colorTable[i], sizeof(colorTable[i]), sizeof(colorTable[i]));

			//初めて使うのでGPUに伝える必要はない
			//this->mpContext2->TiledResourceBarrier(tex.mpResource.Get(), tex.mpUAV.Get());

			this->mpImmediateContext->CSSetShader(this->mpCSDrawTexture.Get(), nullptr, 0);
			std::array<ID3D11Buffer*, 1> ppCBs = { { this->mpCBParam.Get() } };
			this->mpImmediateContext->CSSetConstantBuffers(0, static_cast<UINT>(ppCBs.size()), ppCBs.data());
			std::array<ID3D11UnorderedAccessView*, 1> ppUAVs = { { tex.mpUAV.Get() } };
			std::array<UINT, 1> pInitValues = { { 0 } };
			this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), pInitValues.data());

			auto dx = calDispatchCount(this->width(), 8);
			auto dy = calDispatchCount(this->height(), 8);
			this->mpImmediateContext->Dispatch(dx, dy, 1);

			ppUAVs[0] = nullptr;
			this->mpImmediateContext->CSSetUnorderedAccessViews(0, static_cast<UINT>(ppUAVs.size()), ppUAVs.data(), pInitValues.data());

			//コピー元として扱っていいようGPUに伝える
			//これがなければ、EXECUTION WARNING #3146139: NEED_TO_CALL_TILEDRESOURCEBARRIERと警告が出る
			this->mpContext2->TiledResourceBarrier(tex.mpUAV.Get(), tex.mpResource.Get());
		}

		{//UpdateSubresourceのようにCPUからデータを送ることもできる
			auto& last = *this->mTextures.rbegin();
			auto& tileInfo = last.mInfo;
			std::vector<uint32_t> src;
			src.resize(tileInfo.tileCount * (64 * 1024 / sizeof(uint32_t)));//必ず64KBの倍数のデータ長になるようにする

			D3D11_TEXTURE2D_DESC desc;
			this->mTargetTex.mpResource->GetDesc(&desc);
			UINT size = sqrt(64 * 1024 / sizeof(uint32_t));
			for (UINT tileIndex = 0; tileIndex < tileInfo.tileCount; ++tileIndex) {
				//タイル単位にデータを設定する必要がある
				UINT offset = tileIndex * (128 * 128);
				for (UINT y = 0; y < size; ++y) {
					for (UINT x = 0; x < size; ++x) {
						UINT index = y*size + x + offset;
						uint32_t color;
						if (((x + y) / 60) % 2 == 0) {
							color = 0xffffff00;
						} else {
							color = 0xff000000;
						}
						src[index] = color;
					}
				}
			}
			std::array<D3D11_TILED_RESOURCE_COORDINATE, 1> coordinates;
			coordinates[0].Subresource = 0;
			coordinates[0].X = 0;
			coordinates[0].Y = 0;
			coordinates[0].Z = 0;
			std::array<D3D11_TILE_REGION_SIZE, 1 > regionSizes;
			regionSizes[0].Width = tileInfo.subresourceTiling.WidthInTiles;
			regionSizes[0].Height = tileInfo.subresourceTiling.HeightInTiles;
			regionSizes[0].Depth = tileInfo.subresourceTiling.DepthInTiles;
			regionSizes[0].bUseBox = true;
			regionSizes[0].NumTiles = regionSizes[0].Width * regionSizes[0].Height * regionSizes[0].Depth;
			this->mpContext2->UpdateTiles(last.mpResource.Get(), coordinates.data(), regionSizes.data(), src.data(), 0);
			//CPUからデータを送り終えたことをGPUに伝える
			this->mpContext2->TiledResourceBarrier(last.mpResource.Get(), last.mpResource.Get());
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
}

void Scene::onKeyUp(UINT8 key)
{
	if (key == 'Z') {
		this->mMode = static_cast<decltype(this->mMode)>((this->mMode + 1) % eMODE_COUNT);
	}
	bool isUpdateMapping = false;
	if (key == VK_LEFT && 0 < this->mTargetIndex) {
		--this->mTargetIndex;
		isUpdateMapping = true;
	}
	if (key == VK_RIGHT && this->mTargetIndex < this->mTextures.size()-1) {
		++this->mTargetIndex;
		isUpdateMapping = true;
	}

	if (isUpdateMapping) {
		auto& tileInfo = this->mTargetTex.mInfo;
		//マップ情報をコピーする
		std::array<D3D11_TILED_RESOURCE_COORDINATE, 1> coordinates;
		coordinates[0].Subresource = 0;
		coordinates[0].X = 0;
		coordinates[0].Y = 0;
		coordinates[0].Z = 0;
		std::array<D3D11_TILE_REGION_SIZE, 1 > regionSizes;
		regionSizes[0].Width = tileInfo.subresourceTiling.WidthInTiles;
		regionSizes[0].Height = tileInfo.subresourceTiling.HeightInTiles;
		regionSizes[0].Depth = tileInfo.subresourceTiling.DepthInTiles;
		regionSizes[0].bUseBox = true;
		regionSizes[0].NumTiles = regionSizes[0].Width * regionSizes[0].Height * regionSizes[0].Depth;

		std::array<UINT, 1> ranges = { { 0 } };
		std::array<UINT, 1> offsets = { { 0 } };
		std::array<UINT, 1> rangeTileCounts = { { tileInfo.tileCount } };
		UINT flags = 0;
		auto hr = this->mpContext2->CopyTileMappings(
			this->mTargetTex.mpResource.Get(), coordinates.data(),
			this->mTextures[this->mTargetIndex].mpResource.Get(), coordinates.data(), regionSizes.data(), flags);
		if (FAILED(hr)) {
			throw std::runtime_error("タイルリソースのマッピング情報のコピーに失敗");
		}
	}
}

void Scene::onRender()
{
	//GPUに必要なデータを設定する

	//シェーダの結果をバックバッファーにコピーする
	this->mpImmediateContext->CopySubresourceRegion(this->mpBackBuffer.Get(), 0, 0, 0, 0, this->mTargetTex.mpResource.Get(), 0, nullptr);
}

void Scene::onDestroy()
{
}

