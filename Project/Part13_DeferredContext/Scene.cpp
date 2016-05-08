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
	{//ディファードコンテキストの作成
		this->mpDevice->CreateDeferredContext(0, this->mpDeferedContext.GetAddressOf());

		//ビューポートとレンダーターゲットの設定コマンドを記憶する
		float clearColor[] = { 0, 0.1f, 0.125f, 1 };
		this->mpDeferedContext->ClearRenderTargetView(this->mpBackBufferRTV.Get(), clearColor);
		this->mpDeferedContext->ClearDepthStencilView(this->mpZBufferDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
		this->mpDeferedContext->FinishCommandList(false, this->mpCLRBindTAndVP.GetAddressOf());
	}

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
		createShader(this->mpPixelShader.GetAddressOf(), this->mpDevice.Get(), "PixelShader.cso", &byteCode);
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
	}

	//三角形を描画するコマンドを記憶するスレッドを走らせる
	this->mEnableExecuting = false;
	this->mIsRunRecordingThread = true;
	std::thread record([&]() {
		while (this->mIsRunRecordingThread) {
			Sleep(2);
			std::unique_lock<std::mutex> lock(this->mMutex);

			//記録したコマンドをクリアしている
			this->mpCommandLists.Reset();
			//別のディファードコンテキストが記録したものをコピーすることも出来る
			//グラフィックスパイプラインに関係するものは複数のディファードコンテキストにまたがって記録することはできないので注意
			this->mpDeferedContext->ExecuteCommandList(this->mpCLRBindTAndVP.Get(), false);

			//グラフィックスパイプラインを実行するために必要なものを位置から設定している。
			D3D11_VIEWPORT vp;
			vp.TopLeftX = 0;
			vp.TopLeftY = 0;
			vp.Width = static_cast< float >(this->width());
			vp.Height = static_cast< float >(this->height());
			vp.MinDepth = 0.f;
			vp.MaxDepth = 1.f;
			this->mpDeferedContext->RSSetViewports(1, &vp);

			std::array<ID3D11RenderTargetView*, 1> RTs = { {
					this->mpBackBufferRTV.Get()
				} };
			this->mpDeferedContext->OMSetRenderTargets(static_cast<UINT>(RTs.size()), RTs.data(), this->mpZBufferDSV.Get());

			//ピクセルシェーダ
			this->mpDeferedContext->PSSetShader(this->mpPixelShader.Get(), nullptr, 0);

			//入力アセンブラステージ
			std::array<ID3D11Buffer*, 1> ppVertexBuffers = { {
					this->mpTriangleBuffer.Get(),
				} };
			std::array<UINT, 1> strides = { { sizeof(Vertex) } };
			std::array<UINT, 1> offsets = { { 0 } };
			this->mpDeferedContext->IASetVertexBuffers(0, static_cast<UINT>(ppVertexBuffers.size()), ppVertexBuffers.data(), strides.data(), offsets.data());
			this->mpDeferedContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			UINT drawCount = 0;
			drawCount = 3;
			//入力アセンブラステージ
			this->mpDeferedContext->IASetInputLayout(this->mpInputLayout.Get());

			//頂点シェーダ
			this->mpDeferedContext->VSSetShader(this->mpVertexShader.Get(), nullptr, 0);

			//実行
			this->mpDeferedContext->Draw(3, 0);

			//ここまでのコマンドを記録する
			this->mpDeferedContext->FinishCommandList(true, this->mpCommandLists.GetAddressOf());

			this->mEnableExecuting = true;
			this->mEnableExecuteFlag.notify_one();
		}
	});
	this->mRecordDrawTriangle = std::move(record);
}

void Scene::onUpdate()
{
}

void Scene::onKeyUp(UINT8 key)
{
}

void Scene::onRender()
{
	std::unique_lock<std::mutex> lock(this->mMutex);
	while (!this->mEnableExecuting) {
		this->mEnableExecuteFlag.wait(lock);
	}

	this->mpImmediateContext->ExecuteCommandList(this->mpCommandLists.Get(), false);

	this->mEnableExecuting = false;
}

void Scene::onDestroy()
{
	this->mIsRunRecordingThread = false;
	this->mRecordDrawTriangle.join();
}
