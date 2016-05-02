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
	this->updateTitle();
	auto instanceData = this->makeInstanceData();

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
	{//インスタンス描画用の頂点シェーダの作成(入力スロット利用)
		std::vector<char> byteCode;
		createShader(this->mpVertexShaderByInstanced1.GetAddressOf(), this->mpDevice.Get(), "VertexShaderWithSlot.cso", &byteCode);

		//入力レイアウトの作成
		std::array<D3D11_INPUT_ELEMENT_DESC, 2> elements = { {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "OFFSET", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 2 },
		} };
		auto hr = this->mpDevice->CreateInputLayout(elements.data(), static_cast<UINT>(elements.size()), byteCode.data(), byteCode.size(), this->mpInputLayoutWithSlot2.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("インスタンス描画用の入力レイアウトの作成に失敗");
		}

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(DirectX::SimpleMath::Vector3)  * M_INSTANCED_COUNT;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = instanceData.data();
		initData.SysMemPitch = desc.ByteWidth;
		hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpOffsetBufferBySlot.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("インスタンス描画用のオフセットバッファの作成に失敗");
		}
	}
	{//インスタンス描画用の頂点シェーダの作成(StructuredBuffer利用)
		std::vector<char> byteCode;
		createShader(this->mpVertexShaderByInstanced2.GetAddressOf(), this->mpDevice.Get(), "VertexShaderWithStructuredBuffer.cso", &byteCode);

		//入力レイアウトはthis->mpVertexShaderと同じなのでそれを使い回す

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(DirectX::SimpleMath::Vector3)  * M_INSTANCED_COUNT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.StructureByteStride = sizeof(DirectX::SimpleMath::Vector3);
		desc.MiscFlags = desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = instanceData.data();
		initData.SysMemPitch = desc.ByteWidth;
		auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpOffsetBufferByStructured.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("インスタンス描画用のオフセットバッファStructuredBuffer用)の作成に失敗");
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		srvDesc.BufferEx.FirstElement = 0;
		srvDesc.BufferEx.NumElements = static_cast<UINT>(instanceData.size());
		srvDesc.BufferEx.Flags = 0;
		hr = this->mpDevice->CreateShaderResourceView(this->mpOffsetBufferByStructured.Get(), nullptr, this->mpOffsetBufferByStructuredSRV.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("インスタンス描画用のオフセットバッファStructuredBuffer用)のビュー作成に失敗");
		}
	}
	{//ピクセルシェーダの作成
		createShader(this->mpPixelShader.GetAddressOf(), this->mpDevice.Get(), "PixelShader.cso");
	}
	{//頂点バッファの作成
		{//三角形
			std::array<Vertex, 6> data = { {
				{ {  0.0f,  0.5f, 0 } },
				{ {  0.5f, -0.5f, 0 } },
				{ { -0.5f, -0.5f, 0 } },
				{ { -0.5f,  0.5f, 0 } },
				{ {  0.5f,  0.5f, 0 } },
				{ {  0.0f, -0.5f, 0 } },
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
		{//インディックスバッファ
			std::array<uint16_t, 6> data = { {
				0, 1, 2, 5, 3, 4,
			} };
			D3D11_BUFFER_DESC desc = {};
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			desc.ByteWidth = sizeof(data);
			D3D11_SUBRESOURCE_DATA initData = {};
			initData.pSysMem = &data;
			initData.SysMemPitch = sizeof(data);
			auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpIndexBuffer.GetAddressOf());
			if (FAILED(hr)) {
				throw std::runtime_error("インディックスバッファの作成に失敗");
			}
		}
	}
	{//インダイレクト描画用のバッファ作成
		struct {
			UINT IndexCountPerInstance;
			UINT InstanceCount;
			UINT StartIndexLocation;
			INT BaseVertexLocation;
			UINT StartInstanceLocation;
		}data;
		data.IndexCountPerInstance = 3;
		data.InstanceCount = 10;
		data.StartIndexLocation = 3;
		data.BaseVertexLocation = 0;
		data.StartInstanceLocation = 0;

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(data);
		desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
		desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = &data;
		initData.SysMemPitch = sizeof(data);
		auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpIndirectDrawBuffer.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("インダイレクト用のバッファ作成に失敗");
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

	//ピクセルシェーダ
	this->mpImmediateContext->PSSetShader(this->mpPixelShader.Get(), nullptr, 0);

	//入力アセンブラステージ
	std::array<ID3D11Buffer*, 1> ppVertexBuffers = { {
			this->mpTriangleBuffer.Get(),
	} };
	std::array<UINT, 1> strides = { { sizeof(Vertex) } };
	std::array<UINT, 1> offsets = { { 0 } };
	this->mpImmediateContext->IASetVertexBuffers(0, static_cast<UINT>(ppVertexBuffers.size()), ppVertexBuffers.data(), strides.data(), offsets.data());
	this->mpImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT drawCount = 0;
	drawCount = 3;
	switch (this->mMode) {
	case eMODE_DRAW:
		//入力アセンブラステージ
		this->mpImmediateContext->IASetInputLayout(this->mpInputLayout.Get());

		//頂点シェーダ
		this->mpImmediateContext->VSSetShader(this->mpVertexShader.Get(), nullptr, 0);

		//実行
		this->mpImmediateContext->Draw(3, 0);

		break;
	case eMODE_DRAW_INDEXED:
		//入力アセンブラステージ
		this->mpImmediateContext->IASetInputLayout(this->mpInputLayout.Get());
		this->mpImmediateContext->IASetIndexBuffer(this->mpIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

		//頂点シェーダ
		this->mpImmediateContext->VSSetShader(this->mpVertexShader.Get(), nullptr, 0);

		//実行
		this->mpImmediateContext->DrawIndexed(3, 3, 0);

		break;
	case eMODE_DRAW_INSTANCED:
	{
		//入力アセンブラステージ
		this->mpImmediateContext->IASetInputLayout(this->mpInputLayoutWithSlot2.Get());
		std::array<ID3D11Buffer*, 1> ppOffsetBuffers = { {
				this->mpOffsetBufferBySlot.Get(),
			} };
		std::array<UINT, 1> strides = { { sizeof(DirectX::SimpleMath::Vector3) } };
		std::array<UINT, 1> offsets = { { 0 } };
		this->mpImmediateContext->IASetVertexBuffers(1, static_cast<UINT>(ppOffsetBuffers.size()), ppOffsetBuffers.data(), strides.data(), offsets.data());

		//頂点シェーダ
		this->mpImmediateContext->VSSetShader(this->mpVertexShaderByInstanced1.Get(), nullptr, 0);

		//実行
		this->mpImmediateContext->DrawInstanced(3, this->M_INSTANCED_COUNT, 0, 0);
		break;
	}
	case eMODE_DRAW_INDEXED_INSTANCED:
	{
		//入力アセンブラステージ
		this->mpImmediateContext->IASetInputLayout(this->mpInputLayout.Get());
		this->mpImmediateContext->IASetIndexBuffer(this->mpIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

		//頂点シェーダ
		this->mpImmediateContext->VSSetShader(this->mpVertexShaderByInstanced2.Get(), nullptr, 0);

		std::array<ID3D11ShaderResourceView*, 1> ppSRVs = { {
				this->mpOffsetBufferByStructuredSRV.Get(),
			} };
		this->mpImmediateContext->VSSetShaderResources(0, static_cast<UINT>(ppSRVs.size()), ppSRVs.data());

		//実行
		this->mpImmediateContext->DrawIndexedInstanced(3, this->M_INSTANCED_COUNT, 3, 0, 0);
		break;
	}
	case eMODE_DRAW_INDEXED_INDIRECT:
	{
		//入力アセンブラステージ
		this->mpImmediateContext->IASetInputLayout(this->mpInputLayout.Get());
		this->mpImmediateContext->IASetIndexBuffer(this->mpIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

		//頂点シェーダ
		this->mpImmediateContext->VSSetShader(this->mpVertexShaderByInstanced2.Get(), nullptr, 0);

		std::array<ID3D11ShaderResourceView*, 1> ppSRVs = { {
				this->mpOffsetBufferByStructuredSRV.Get(),
			} };
		this->mpImmediateContext->VSSetShaderResources(0, static_cast<UINT>(ppSRVs.size()), ppSRVs.data());

		//実行
		this->mpImmediateContext->DrawIndexedInstancedIndirect(this->mpIndirectDrawBuffer.Get(), 0);
		break;
	}
	default:
		assert(false);
	}

}

void Scene::onDestroy()
{
}

void Scene::updateTitle()
{
	std::wstring title;
	switch (this->mMode) {
	case eMODE_DRAW:					title = L"eMODE_DRAW"; break;
	case eMODE_DRAW_INDEXED:			title = L"eMODE_DRAW_INDEXED"; break;
	case eMODE_DRAW_INSTANCED:			title = L"eMODE_DRAW_INSTANCED"; break;
	case eMODE_DRAW_INDEXED_INSTANCED:	title = L"eMODE_DRAW_INDEXED_INSTANCED"; break;
	case eMODE_DRAW_INDEXED_INDIRECT:	title = L"eMODE_DRAW_INDEXED_INDIRECT"; break;
	default:
		assert(false && "未実装");
	}
	this->setCustomWindowText(title.c_str());
}

std::vector<DirectX::SimpleMath::Vector3> Scene::makeInstanceData()const
{
	std::mt19937 engine;
	std::uniform_real_distribution<float> distribution(-1, 1);
	auto rnd = [&]() { return distribution(engine); };

	std::vector<DirectX::SimpleMath::Vector3> data;
	data.resize(this->M_INSTANCED_COUNT);
	for (auto& d : data) {
		d.x = rnd();
		d.y = rnd();
		d.z = 0.f;
	}
	data[0].x = 0;
	data[0].y = 0;

	data[1].x = 0.5f;
	data[1].y = 0.5f;

	return data;
}