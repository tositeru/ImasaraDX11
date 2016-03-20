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
		createShader(this->mpVertexShader.GetAddressOf(), this->mpDevice.Get(), "VertexShader.cso", &byteCode);

		//入力レイアウトの作成
		std::array<D3D11_INPUT_ELEMENT_DESC, 1> elements = { {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		} };
		auto hr = this->mpDevice->CreateInputLayout(elements.data(), static_cast<UINT>(elements.size()), byteCode.data(), byteCode.size(), this->mpInputLayout.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("入力レイアウトの作成に失敗");
		}
		//ピクセルシェーダ
		createShader(this->mpPixelShader.GetAddressOf(), this->mpDevice.Get(), "PixelShader.cso", &byteCode);
	}
	{//IA用のバッファ作成
		std::array<Vertex, 6> data = { {
			{ { -0.5f,  0.5f, 0 } },
			{ {  0.0f, -0.5f, 0 } },
			{ { -1.0f, -0.5f, 0 } },
			{ { 0.5f,  0.5f, 0 } },
			{ { 0.0f, -0.5f, 0 } },
			{ { 1.0f, -0.5f, 0 } },
		} };
		CreateIABuffer(this->mpVertexBuffer.GetAddressOf(), this->mpDevice.Get(), static_cast<UINT>(data.size()), data.data(), D3D11_BIND_VERTEX_BUFFER);
		std::array<uint16_t, 6> indices = { {
				0, 1, 2, 3, 4, 5
		} };
		CreateIABuffer(this->mpIndexBuffer.GetAddressOf(), this->mpDevice.Get(), static_cast<UINT>(indices.size()), indices.data(), D3D11_BIND_INDEX_BUFFER);
	}
	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.CullMode = D3D11_CULL_BACK;
		desc.FillMode = D3D11_FILL_SOLID;
		desc.FrontCounterClockwise = true;
		desc.ScissorEnable = false;
		desc.MultisampleEnable = false;
		auto hr = this->mpDevice->CreateRasterizerState(&desc, this->mpRSCulling.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("カリング用のラスタライザーステートの作成に失敗");
		}

		desc.FillMode = D3D11_FILL_WIREFRAME;
		hr = this->mpDevice->CreateRasterizerState(&desc, this->mpRSFill.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("カリング用のラスタライザーステートの作成に失敗");
		}

		desc.CullMode = D3D11_CULL_NONE;
		desc.FillMode = D3D11_FILL_SOLID;
		desc.ScissorEnable = true;
		hr = this->mpDevice->CreateRasterizerState(&desc, this->mpRSScissor.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("シザー矩形用のラスタライザーステートの作成に失敗");
		}
	}
	UINT count = 1;
	this->mpImmediateContext->RSGetViewports(&count, &this->mDefaultViewport);
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
	//ラスタライザーステージ
	this->mpImmediateContext->RSSetViewports(1, &this->mDefaultViewport);
	//ピクセルシェーダ
	this->mpImmediateContext->PSSetShader(this->mpPixelShader.Get(), nullptr, 0);

	switch (this->mMode) {
	case eMODE_NONE:
		this->mpImmediateContext->RSSetState(nullptr);
		break;
	case eMODE_CULLING:
		this->mpImmediateContext->RSSetState(this->mpRSCulling.Get());
		break;
	case eMODE_FILL:
		this->mpImmediateContext->RSSetState(this->mpRSFill.Get());
		break;
	case eMODE_VIEWPORT:
	{
		D3D11_VIEWPORT vp;
		vp.Width = static_cast<float>(this->width() / 2);
		vp.Height = static_cast<float>(this->height() / 2);
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.MinDepth = 0.f;
		vp.MaxDepth = 1.f;
		this->mpImmediateContext->RSSetViewports(1, &vp);
		this->mpImmediateContext->RSSetState(this->mpRSCulling.Get());
		break;
	}
	case eMODE_SCISSOR:
	{
		this->mpImmediateContext->RSSetState(this->mpRSScissor.Get());
		UINT count = 1;
		D3D11_RECT rect;
		rect.left = static_cast<LONG>(this->width() / 4.f);
		rect.right= static_cast<LONG>(this->width() * 3.f / 4.f);
		rect.top = static_cast<LONG>(this->height() / 4.f);
		rect.bottom = static_cast<LONG>(this->height() * 3.f / 4.f);
		this->mpImmediateContext->RSSetScissorRects(count, &rect);
		break;
	}
	default:
		assert(false);
	}
	this->mpImmediateContext->Draw(6, 0);
}

void Scene::onDestroy()
{
}

std::vector<Scene::InstancedParam> Scene::makeInstanceData()const
{
	int i = 0;
	float work = 0.f;
	auto calPos = [&i, &work, this]() {
		const float start = -0.5f;
		const float interval = 1.f / this->M_INSTANCED_COUNT;
		auto r =  start + work;
		work += interval;
		return DirectX::SimpleMath::Vector3(r, 0, ((i++ % 2) == 0) ? 0 : 0.5f);
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