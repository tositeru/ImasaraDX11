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

		createShader(this->mpVSDummy.GetAddressOf(), this->mpDevice.Get(), "VertexShader.cso", &byteCode);
		//ジオメトリシェーダ
		createShader(this->mpGeometryShader.GetAddressOf(), this->mpDevice.Get(), "GeometryShader.cso", &byteCode);
		createShader(this->mpGeometryShader2.GetAddressOf(), this->mpDevice.Get(), "GeometryShader2.cso", &byteCode);
		//ピクセルシェーダ
		createShader(this->mpPixelShader.GetAddressOf(), this->mpDevice.Get(), "PixelShader.cso", &byteCode);
	}
	{//IA用のバッファ作成
		std::array<Vertex, 3> data = { {
			{ {  0.0f,  0.5f, 0 } },
			{ {  0.5f, -0.5f, 0 } },
			{ { -0.5f, -0.5f, 0 } },
		} };
		CreateIABuffer(this->mpVertexBuffer.GetAddressOf(), this->mpDevice.Get(), static_cast<UINT>(data.size()), data.data(), D3D11_BIND_VERTEX_BUFFER);
		std::array<uint16_t, 6> indices = { {
				0, 1, 2, 3, 4, 5
		} };
		CreateIABuffer(this->mpIndexBuffer.GetAddressOf(), this->mpDevice.Get(), static_cast<UINT>(indices.size()), indices.data(), D3D11_BIND_INDEX_BUFFER);
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

	//入力アセンブラステージ
	std::array<ID3D11Buffer*, 1> ppBufs = { {
			this->mpVertexBuffer.Get(),
	} };
	std::array<UINT, 1> strides = { { sizeof(Vertex) } };
	std::array<UINT, 1> offsets = { { 0 } };
	this->mpImmediateContext->IASetVertexBuffers(0, static_cast<UINT>(ppBufs.size()), ppBufs.data(), strides.data(), offsets.data());
	this->mpImmediateContext->IASetIndexBuffer(this->mpIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	this->mpImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	this->mpImmediateContext->IASetInputLayout(this->mpInputLayout.Get());

	//頂点シェーダ
	this->mpImmediateContext->VSSetShader(this->mpVertexShader.Get(), nullptr, 0);
	//ピクセルシェーダ
	this->mpImmediateContext->PSSetShader(this->mpPixelShader.Get(), nullptr, 0);

	switch (this->mMode) {
	case eMODE_POINT_TO_TRIANGLE:
		this->mpImmediateContext->GSSetShader(this->mpGeometryShader.Get(), nullptr, 0);
		this->mpImmediateContext->Draw(3, 0);
		break;
	case eMODE_NO_INPUT_TO_TRIANGLE:
		this->mpImmediateContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
		this->mpImmediateContext->VSSetShader(this->mpVSDummy.Get(), nullptr, 0);
		this->mpImmediateContext->GSSetShader(this->mpGeometryShader2.Get(), nullptr, 0);
		this->mpImmediateContext->Draw(1, 0);
		break;
	default:
		assert(false);
	}
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