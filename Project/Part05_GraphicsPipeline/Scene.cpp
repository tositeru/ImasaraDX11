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

Scene::Scene(UINT width, UINT height, std::wstring name)
	: DXSample(width, height, name)
{
}

void Scene::onInit()
{
	this->updateTitle();
	//グラフィックスパイプラインの初期化
	{//頂点シェーダの作成
		std::vector<char> byteCode;
		createShader(this->mpVertexShader.GetAddressOf(), this->mpDevice.Get(), "VertexShader.cso", &byteCode);


		//入力レイアウトの作成
		std::array<D3D11_INPUT_ELEMENT_DESC, 1> elements = { {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		} };
		auto hr = this->mpDevice->CreateInputLayout(elements.data(), static_cast<UINT>(elements.size()), byteCode.data(), byteCode.size(), this->mpInputLayout.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("入力レイアウトの作成に失敗");
		}
	}
	{//ピクセルシェーダの作成
		createShader(this->mpPixelShader.GetAddressOf(), this->mpDevice.Get(), "PixelShader.cso");
	}
	{//頂点バッファの作成
		{//三角形
			std::array<Vertex, 3> data = { {
				{ {  0.0f,  0.5f, 0 } },
				{ {  0.5f, -0.5f, 0 } },
				{ { -0.5f, -0.5f, 0 } },
			} };
			D3D11_BUFFER_DESC desc = {};
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.ByteWidth = sizeof(data);
			D3D11_SUBRESOURCE_DATA initData = {};
			initData.pSysMem = &data;
			initData.SysMemPitch = sizeof(data);
			auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpTriangleBuffer.GetAddressOf());
			if (FAILED(hr)) {
				throw std::runtime_error("三角形用の頂点バッファの作成に失敗");
			}
		}
		{//線分
			std::array<Vertex, 4> data = { {
				{ { 1.0f,  1.0f, 0 } },
				{ {-1.0f, -1.0f, 0 } },
				{ { 1.0f, -1.0f, 0 } },
				{ {-1.0f,  1.0f, 0 } },
			} };
			D3D11_BUFFER_DESC desc = {};
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.ByteWidth = sizeof(data);
			D3D11_SUBRESOURCE_DATA initData = {};
			initData.pSysMem = &data;
			initData.SysMemPitch = sizeof(data);
			auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpLineBuffer.GetAddressOf());
			if (FAILED(hr)) {
				throw std::runtime_error("線分用の頂点バッファの作成に失敗");
			}
		}
		{//点
			std::array<Vertex, 3> data = { {
				{ { 0.0f,  0.5f, 0 } },
				{ { 0.5f, -0.5f, 0 } },
				{ { -0.5f, -0.5f, 0 } },
				} };
			D3D11_BUFFER_DESC desc = {};
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.ByteWidth = sizeof(data);
			D3D11_SUBRESOURCE_DATA initData = {};
			initData.pSysMem = &data;
			initData.SysMemPitch = sizeof(data);
			auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpPointBuffer.GetAddressOf());
			if (FAILED(hr)) {
				throw std::runtime_error("点用の頂点バッファの作成に失敗");
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
		this->mMode = static_cast<decltype(this->mMode)>((this->mMode + 1) % eMODE_COUNT);
		this->updateTitle();
	}
}

void Scene::onRender()
{
	//GPUに必要なデータを設定する

	//入力アセンブラステージ
	this->mpImmediateContext->IASetInputLayout(this->mpInputLayout.Get());

	std::array<ID3D11Buffer*, 1> ppVertexBuffers;
	std::array<UINT, 1> strides = { { sizeof(Vertex) } };
	std::array<UINT, 1> offsets = { { 0 } };
	UINT vertexCount = 0;
	switch (this->mMode) {
	case eMODE_TRIANGLE:
		ppVertexBuffers[0] = this->mpTriangleBuffer.Get();
		vertexCount = 3;
		this->mpImmediateContext->IASetVertexBuffers(0, static_cast<UINT>(ppVertexBuffers.size()), ppVertexBuffers.data(), strides.data(), offsets.data());
		this->mpImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		break;
	case eMODE_LINE:
		ppVertexBuffers[0] = this->mpLineBuffer.Get();
		vertexCount = 4;
		this->mpImmediateContext->IASetVertexBuffers(0, static_cast<UINT>(ppVertexBuffers.size()), ppVertexBuffers.data(), strides.data(), offsets.data());
		this->mpImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		break;
	case eMODE_POINT:
		ppVertexBuffers[0] = this->mpPointBuffer.Get();
		vertexCount = 3;
		this->mpImmediateContext->IASetVertexBuffers(0, static_cast<UINT>(ppVertexBuffers.size()), ppVertexBuffers.data(), strides.data(), offsets.data());
		this->mpImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		break;
	default:
		assert(false);
	}

	//頂点シェーダ
	this->mpImmediateContext->VSSetShader(this->mpVertexShader.Get(), nullptr, 0);

	//ピクセルシェーダ
	this->mpImmediateContext->PSSetShader(this->mpPixelShader.Get(), nullptr, 0);

	//実行
	this->mpImmediateContext->Draw(vertexCount, 0);
}

void Scene::onDestroy()
{
}

void Scene::updateTitle()
{
	std::wstring title;
	switch (this->mMode) {
	case eMODE_TRIANGLE:		title = L"D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST"; break;
	case eMODE_LINE:	title = L"D3D11_PRIMITIVE_TOPOLOGY_LINELIST"; break;
	case eMODE_POINT: title = L"D3D11_PRIMITIVE_TOPOLOGY_POINTLIST"; break;
	default:
		assert(false && "未実装");
	}
	this->setCustomWindowText(title.c_str());
}