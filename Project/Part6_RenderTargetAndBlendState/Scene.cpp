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

#include <random>

Scene::Scene(UINT width, UINT height, std::wstring name)
	: DXSample(width, height, name)
{
}

void Scene::onInit()
{
	auto instanceData = this->makeInstanceData();

	{//グラフィックスパイプラインの初期化
		std::vector<char> byteCode;
		CreateShader(this->mpVertexShader.GetAddressOf(), this->mpDevice.Get(), "VertexShader.cso", &byteCode);

		//入力レイアウトの作成
		std::array<D3D11_INPUT_ELEMENT_DESC, 2> elements = { {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		} };
		auto hr = this->mpDevice->CreateInputLayout(elements.data(), static_cast<UINT>(elements.size()), byteCode.data(), byteCode.size(), this->mpInputLayout.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("入力レイアウトの作成に失敗");
		}

		//ピクセルシェーダ
		CreateShader(this->mpPixelShader.GetAddressOf(), this->mpDevice.Get(), "PixelShader.cso", &byteCode);
	}
	{//インスタンス描画用のバッファ作成
		CreateStructuredBuffer(this->mpOffsetBuffer.GetAddressOf(), this->mpOffsetBufferSRV.GetAddressOf(), nullptr, this->mpDevice.Get(), M_INSTANCED_COUNT, instanceData.data());
	}
	{//IA用のバッファ作成
		std::array<Vertex, 6> data = { {
			{ {  0.0f,  0.5f, 0 } },
			{ {  0.5f, -0.5f, 0 } },
			{ { -0.5f, -0.5f, 0 } },

			{ { -0.5f,  0.5f, 0 } },
			{ {  0.5f,  0.5f, 0 } },
			{ {  0.0f, -0.5f, 0 } },
		} };
		CreateIABuffer(this->mpVertexBuffer.GetAddressOf(), this->mpDevice.Get(), static_cast<UINT>(data.size()), data.data(), D3D11_BIND_VERTEX_BUFFER);

		std::array<uint16_t, 3> indices = { {
			0, 1, 2,
		} };
		CreateIABuffer(this->mpIndexBuffer.GetAddressOf(), this->mpDevice.Get(), static_cast<UINT>(indices.size()), indices.data(), D3D11_BIND_INDEX_BUFFER);
	}
	{//レンダーターゲットの作成
		{//レンダーターゲット1の作成
			D3D11_TEXTURE2D_DESC desc = {};
			desc.Width = this->width();
			desc.Height = this->height();
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.BindFlags = D3D11_BIND_RENDER_TARGET;
			desc.SampleDesc.Count = 1;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			auto hr = this->mpDevice->CreateTexture2D(&desc, nullptr, this->mpRenderTarget1.GetAddressOf());
			if (FAILED(hr)) {
				throw std::runtime_error("レンダーターゲット1の作成に失敗");
			}
			hr = this->mpDevice->CreateRenderTargetView(this->mpRenderTarget1.Get(), nullptr, this->mpRenderTarget1RTV.GetAddressOf());
			if (FAILED(hr)) {
				throw std::runtime_error("レンダーターゲット1のレンダーターゲットビュー作成に失敗");
			}
		}
		{//レンダーターゲット2の作成
			D3D11_TEXTURE2D_DESC desc = {};
			desc.Width = this->width();
			desc.Height = this->height();
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.BindFlags = D3D11_BIND_RENDER_TARGET;
			desc.SampleDesc.Count = 1;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			auto hr = this->mpDevice->CreateTexture2D(&desc, nullptr, this->mpRenderTarget2.GetAddressOf());
			if (FAILED(hr)) {
				throw std::runtime_error("レンダーターゲット2の作成に失敗");
			}
			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Format = desc.Format;
			rtvDesc.Texture2D.MipSlice = 0;
			hr = this->mpDevice->CreateRenderTargetView(this->mpRenderTarget2.Get(), &rtvDesc, this->mpRenderTarget2RTV.GetAddressOf());
			if (FAILED(hr)) {
				throw std::runtime_error("レンダーターゲット2のレンダーターゲットビュー作成に失敗");
			}
		}
	}
	{//ブレンドステートの作成
		{//その1
			D3D11_BLEND_DESC desc = {};
			desc.AlphaToCoverageEnable = false;
			desc.IndependentBlendEnable = false;
			desc.RenderTarget[0].BlendEnable = true;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			auto hr = this->mpDevice->CreateBlendState(&desc, this->mpBlendState.GetAddressOf());
			if (FAILED(hr)) {
				throw std::runtime_error("ブレンドステートの作成に失敗");
			}
		}
		{//その2
			D3D11_BLEND_DESC desc = {};
			desc.AlphaToCoverageEnable = false;
			desc.IndependentBlendEnable = true;
			desc.RenderTarget[0].BlendEnable = true;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			desc.RenderTarget[1].BlendEnable = true;
			desc.RenderTarget[1].SrcBlend = D3D11_BLEND_ZERO;
			desc.RenderTarget[1].DestBlend = D3D11_BLEND_INV_SRC_COLOR;
			desc.RenderTarget[1].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[1].SrcBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[1].DestBlendAlpha = D3D11_BLEND_ONE;
			desc.RenderTarget[1].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			auto hr = this->mpDevice->CreateBlendState(&desc, this->mpBlendState2.GetAddressOf());
			if (FAILED(hr)) {
				throw std::runtime_error("ブレンドステート2の作成に失敗");
			}
		}
	}
}

void Scene::onUpdate()
{
}

void Scene::onKeyUp(UINT8 key)
{
	if (key == 'Z') {
		this->mBlendMode = static_cast<decltype(this->mBlendMode)>((this->mBlendMode + 1) % eBLEND_MODE_COUNT);
	}
	if (key == 'X') {
		this->mShowRT = static_cast<decltype(this->mShowRT)>((this->mShowRT + 1) % eSHOW_RT_COUNT);
	}
}

void Scene::onRender()
{
	//GPUに必要なデータを設定する

	//入力アセンブラステージ
	std::array<ID3D11Buffer*, 1> ppBufs = { {
			this->mpVertexBuffer.Get(),
	} };
	std::array<UINT, 1> strides = { { sizeof(Vertex) } };
	std::array<UINT, 1> offsets = { { 0 } };
	this->mpImmediateContext->IASetVertexBuffers(0, static_cast<UINT>(ppBufs.size()), ppBufs.data(), strides.data(), offsets.data());
	this->mpImmediateContext->IASetIndexBuffer(this->mpIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	this->mpImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	this->mpImmediateContext->IASetInputLayout(this->mpInputLayout.Get());

	//頂点シェーダ
	this->mpImmediateContext->VSSetShader(this->mpVertexShader.Get(), nullptr, 0);
	std::array<ID3D11ShaderResourceView*, 1> ppSRVs = { {
			this->mpOffsetBufferSRV.Get(),
		} };
	this->mpImmediateContext->VSSetShaderResources(0, static_cast<UINT>(ppSRVs.size()), ppSRVs.data());

	//ピクセルシェーダ
	this->mpImmediateContext->PSSetShader(this->mpPixelShader.Get(), nullptr, 0);

	//アウトプットマージャステージ
	std::array<float, 4> clearValue1 = { { 0, 0, 0, 1 } };
	this->mpImmediateContext->ClearRenderTargetView(this->mpRenderTarget1RTV.Get(), clearValue1.data());
	std::array<float, 4> clearValue2 = { { 1, 1, 1, 1 } };
	this->mpImmediateContext->ClearRenderTargetView(this->mpRenderTarget2RTV.Get(), clearValue2.data());

	std::array<ID3D11RenderTargetView*, 2> ppRTVs = { {
			this->mpRenderTarget1RTV.Get(),
			this->mpRenderTarget2RTV.Get(),
		} };
	this->mpImmediateContext->OMSetRenderTargets(static_cast<UINT>(ppRTVs.size()), ppRTVs.data(), nullptr);
	std::array<float, 4> factor = { {
		1, 1, 1, 1
	} };
	switch (this->mBlendMode) {
	case eBLEND_MODE_SAME_BLEND: 	this->mpImmediateContext->OMSetBlendState(this->mpBlendState.Get(), factor.data(), 0xffffffff); break;
	case eBLEND_MODE_SEPARATEBLEND: this->mpImmediateContext->OMSetBlendState(this->mpBlendState2.Get(), factor.data(), 0xffffffff); break;
	default:
		assert(false);
	}

	//実行
	this->mpImmediateContext->DrawIndexedInstanced(3, M_INSTANCED_COUNT, 0, 0, 0);

	//シェーダの結果をバックバッファーにコピーする
	switch (this->mShowRT) {
	case eSHOW_RT_1: this->mpImmediateContext->CopySubresourceRegion(this->mpBackBuffer.Get(), 0, 0, 0, 0, this->mpRenderTarget1.Get(), 0, nullptr); break;
	case eSHOW_RT_2: this->mpImmediateContext->CopySubresourceRegion(this->mpBackBuffer.Get(), 0, 0, 0, 0, this->mpRenderTarget2.Get(), 0, nullptr); break;
	default:
		assert(false);
	}
}

void Scene::onDestroy()
{
}

std::vector<Scene::InstancedParam> Scene::makeInstanceData()const
{
	float work = 0.f;
	auto calPos = [&work, this]() {
		const float start = -0.5f;
		const float interval = 1.f / this->M_INSTANCED_COUNT;
		auto r =  start + work;
		work += interval;
		return DirectX::SimpleMath::Vector3(r, 0, 0);
	};

	std::vector<InstancedParam> data;
	data.resize(this->M_INSTANCED_COUNT);
	data.at(0).offset = calPos();
	data.at(0).color = DirectX::SimpleMath::Vector4(1, 0, 0, 1);

	data.at(1).offset = calPos();
	data.at(1).color = DirectX::SimpleMath::Vector4(0, 1, 0, 1);

	data.at(2).offset = calPos();
	data.at(2).color = DirectX::SimpleMath::Vector4(0, 0, 1, 1);

	data.at(3).offset = calPos();
	data.at(3).color = DirectX::SimpleMath::Vector4(1, 1, 0, 1);

	data.at(4).offset = calPos();
	data.at(4).color = DirectX::SimpleMath::Vector4(0, 1, 1, 1);

	return data;
}