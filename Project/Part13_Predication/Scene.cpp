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
	{
		D3D11_QUERY_DESC queryDesc;
		queryDesc.Query = D3D11_QUERY_SO_STATISTICS_STREAM0;
		queryDesc.MiscFlags = 0;
		auto hr = this->mpDevice->CreateQuery(&queryDesc, this->mpQuery.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("ID3D11Predicateの作成に失敗。");
		}

		D3D11_QUERY_DESC predicateDesc;
		predicateDesc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
		predicateDesc.MiscFlags = D3D11_QUERY_MISC_PREDICATEHINT;
		//predicateDesc.MiscFlags = 0;
		hr = this->mpDevice->CreatePredicate(&predicateDesc, this->mpPredicate.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("ID3D11Predicateの作成に失敗。");
		}

		D3D11_COUNTER_DESC counterDesc;
		counterDesc.Counter = D3D11_COUNTER_DEVICE_DEPENDENT_0;
		counterDesc.MiscFlags = 0;
		hr = this->mpDevice->CreateCounter(&counterDesc, this->mpCounter.GetAddressOf());
		switch (hr) {
		case S_FALSE:					throw std::runtime_error("ID3D11Counterの作成に失敗。hr=S_FALSE");
		case E_OUTOFMEMORY:				throw std::runtime_error("ID3D11Counterの作成に失敗。hr=E_OUTOFMEMORY");
		case DXGI_ERROR_UNSUPPORTED:	throw std::runtime_error("ID3D11Counterの作成に失敗。hr=DXGI_ERROR_UNSUPPORTED");
		case DXGI_ERROR_NONEXCLUSIVE:	throw std::runtime_error("ID3D11Counterの作成に失敗。hr=DXGI_ERROR_NONEXCLUSIVE");
		case E_INVALIDARG:				throw std::runtime_error("ID3D11Counterの作成に失敗。hr=E_INVALIDARG");
		}

		hr = this->mpDevice->CreateCounter(&counterDesc, this->mpCounter2.GetAddressOf());

		D3D11_COUNTER_TYPE counterType;
		UINT activeCounters;
		char counterName[1024];
		UINT counterNameLength = 1024;
		char units[1024];
		UINT unitsLength = 1024;
		char description[1024];
		UINT descriptionLength = 1024;
		hr = this->mpDevice->CheckCounter(&counterDesc, &counterType, &activeCounters, counterName, &counterNameLength, units, &unitsLength, description, &descriptionLength);
		//hr = this->mpDevice->CheckCounter(&counterDesc, &counterType, &activeCounters, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

		//情報を記憶する
		this->mCounterType.type = counterType;
		this->mCounterType.name = counterName;
		this->mCounterType.units = units;
		this->mCounterType.decription = description;

		D3D11_COUNTER_TYPE;
		OutputDebugStringA("ID3D11Counter\n");
		OutputDebugStringA((std::string(" type = ") + std::to_string(counterType) + "\n").c_str());
		OutputDebugStringA((std::string(" active count = ") + std::to_string(activeCounters) + "\n").c_str());
		OutputDebugStringA((std::string(" name = ") + counterName + "\n").c_str());
		OutputDebugStringA((std::string(" units = ") + units + "\n").c_str());
		OutputDebugStringA((std::string(" description = ") + description + "\n").c_str());

		D3D11_COUNTER_INFO counterInfo;
		this->mpDevice->CheckCounterInfo(&counterInfo);
		OutputDebugStringA("D3D11_COUNTER_INFO\n");
		OutputDebugStringA((std::string(" LastDeviceDependentCounter = ") + std::to_string(counterInfo.LastDeviceDependentCounter) + "\n").c_str());
		OutputDebugStringA((std::string(" NumSimultaneousCounters = ") + std::to_string(counterInfo.NumSimultaneousCounters) + "\n").c_str());
		OutputDebugStringA((std::string(" NumDetectableParallelUnits = ") + std::to_string(counterInfo.NumDetectableParallelUnits) + "\n").c_str());
	}
	{//グラフィックスパイプラインの初期化
		std::vector<char> byteCode;
		createShader(this->mpVertexShader.GetAddressOf(), this->mpDevice.Get(), "VertexShader.cso", &byteCode);

		//入力レイアウトの作成
		std::array<D3D11_INPUT_ELEMENT_DESC, 2> elements = { {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		} };
		auto hr = this->mpDevice->CreateInputLayout(elements.data(), static_cast<UINT>(elements.size()), byteCode.data(), byteCode.size(), this->mpInputLayout.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("入力レイアウトの作成に失敗");
		}

		//ピクセルシェーダ
		createShader(this->mpPixelShader.GetAddressOf(), this->mpDevice.Get(), "PixelShader.cso", &byteCode);
	}
	{//ストリームアウトプット用シェーダの作成
		std::vector<char> byteCode;
		//頂点シェーダ
		createShader(this->mpVSStreamOutput.GetAddressOf(), this->mpDevice.Get(), "VSStreamOutput.cso", &byteCode);

		//ジオメトリシェーダ
		if (!loadBinaryFile(&byteCode, "GeometryShader.cso")) {
			throw std::runtime_error("GeometryShader.csoの読み込みに失敗");
		}

		std::array<D3D11_SO_DECLARATION_ENTRY, 2> soEntrys = { {
			{ 0, "POSITION", 0, 0, 3, 0 },
			{ 0, "TEXCOORD", 0, 0, 4, 0 },
		} };
		std::array<UINT, 1> strides = { { sizeof(float) * 3 + sizeof(float) * 4 } };
		auto hr = this->mpDevice->CreateGeometryShaderWithStreamOutput(
			byteCode.data(),
			static_cast<SIZE_T>(byteCode.size()),
			soEntrys.data(),
			static_cast<UINT>(soEntrys.size()),
			strides.data(),
			static_cast<UINT>(strides.size()),
			D3D11_SO_NO_RASTERIZED_STREAM,
			nullptr,
			this->mpGeometryShader.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("GeometryShaderの作成に失敗");
		}

	}
	{//IA用のバッファ作成
		std::array<Vertex, 3> data = { {
			{ {  0.0f,  0.5f, 0 } },
			{ {  0.5f, -0.5f, 0 } },
			{ { -0.5f, -0.5f, 0 } },
		} };
		CreateIABuffer(this->mpVertexBuffer.GetAddressOf(), this->mpDevice.Get(), static_cast<UINT>(data.size()), data.data(), D3D11_BIND_VERTEX_BUFFER);
	}
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(Vertex) * M_STREAM_OUTPUT_COUNT;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;
		auto hr = this->mpDevice->CreateBuffer(&desc, nullptr, this->mpStreamOutputBuffer.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("ストリームアウトプット用のバッファ作成に失敗");
		}
	}
	{//必ず、深度テストに失敗させる深度ステンシルステートを作成
		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = true;
		desc.DepthFunc = D3D11_COMPARISON_NEVER;
		this->mpDevice->CreateDepthStencilState(&desc, this->mpDSNotDepthTestPass.GetAddressOf());
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

	if (key == 'X') {
		this->mOnPredicate = !this->mOnPredicate;
	}
}

void Scene::onRender()
{
	this->mpImmediateContext->Begin(this->mpCounter.Get());
	float clearColor[] = { 0, 0.1f, 0.125f, 1 };
	this->mpImmediateContext->ClearRenderTargetView(this->mpBackBufferRTV.Get(), clearColor);
	this->mpImmediateContext->ClearDepthStencilView(this->mpZBufferDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
	this->mpImmediateContext->End(this->mpCounter.Get());

	this->mpImmediateContext->Flush();
	{
		CounterValue value;
		D3D11_COUNTER_TYPE;
		HRESULT hr;
		do {
			hr = this->mpImmediateContext->GetData(this->mpCounter.Get(), &value, value.stride(this->mCounterType.type), 0);
		} while (hr != S_OK);

		OutputDebugStringA((std::string("time = ") + value.toString(this->mCounterType.type) + " ").c_str());
	}


	this->mpImmediateContext->Begin(this->mpCounter2.Get());
	switch (this->mMode) {
	case eMODE_QUERY:
		this->renderQuery();
		break;
	case eMODE_PREDICATE:
		this->renderPredicate();
		break;
	}
	this->mpImmediateContext->End(this->mpCounter2.Get());

	this->mpImmediateContext->Flush();

	{
		CounterValue value;
		D3D11_COUNTER_TYPE;
		HRESULT hr;
		do {
			hr = this->mpImmediateContext->GetData(this->mpCounter2.Get(), &value, value.stride(this->mCounterType.type), 0);
		} while (hr != S_OK);

		OutputDebugStringA((std::string("time2 = ") + value.toString(this->mCounterType.type) + "\n").c_str());
	}
}

void Scene::renderQuery()
{
	this->mpImmediateContext->Begin(this->mpQuery.Get());
	{
		//三角形を生成する
		outputTriangles(4);
	}
	this->mpImmediateContext->End(this->mpQuery.Get());
	//this->mpImmediateContext->Flush();

	//生成した三角形の個数を取得する
	D3D11_QUERY_DATA_SO_STATISTICS soStatistics = {};
	UINT flag = 0;
	HRESULT hr;
	do {
		hr = this->mpImmediateContext->GetData(this->mpQuery.Get(), &soStatistics, sizeof(soStatistics), flag);
	} while (hr != S_OK);

	//生成した三角形を描画する
	renderTriangles(static_cast<UINT>(soStatistics.NumPrimitivesWritten));
}

void Scene::renderPredicate()
{
	//三角形を生成する
	this->outputTriangles(10);

	//わざと深度テストを失敗させる
	this->mpImmediateContext->Begin(this->mpPredicate.Get());
	{
		this->mpImmediateContext->OMSetDepthStencilState(this->mpDSNotDepthTestPass.Get(), 0);
		this->renderTriangles(-1);
	}
	this->mpImmediateContext->End(this->mpPredicate.Get());

	this->mpImmediateContext->Flush();

	//Predicateの効果を確認する
	if (this->mOnPredicate) {
		this->mpImmediateContext->SetPredication(this->mpPredicate.Get(), false);
	}

	this->mpImmediateContext->OMSetDepthStencilState(nullptr, 0);
	this->renderTriangles(-1);
	this->mpImmediateContext->SetPredication(nullptr, true);

}

void Scene::outputTriangles(UINT count)
{
	//三角形を生成する
	std::array<ID3D11Buffer*, 1> ppSOBufs = { {
			this->mpStreamOutputBuffer.Get(),
		} };
	std::array<UINT, 1> soOffsets = { { 0 } };
	this->mpImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	this->mpImmediateContext->VSSetShader(this->mpVSStreamOutput.Get(), nullptr, 0);
	this->mpImmediateContext->GSSetShader(this->mpGeometryShader.Get(), nullptr, 0);
	this->mpImmediateContext->SOSetTargets(static_cast<UINT>(ppSOBufs.size()), ppSOBufs.data(), soOffsets.data());

	this->mpImmediateContext->Draw(min(this->M_STREAM_OUTPUT_COUNT, count), 0);

	ppSOBufs[0] = nullptr;
	this->mpImmediateContext->SOSetTargets(static_cast<UINT>(ppSOBufs.size()), ppSOBufs.data(), soOffsets.data());
}

void Scene::renderTriangles(UINT count)
{
	//生成した三角形を描画する
	//入力アセンブラステージ
	std::array<ID3D11Buffer*, 1> ppSOBufs = { {
			this->mpStreamOutputBuffer.Get(),
		} };
	std::array<UINT, 1> soOffsets = { { 0 } };
	std::array<UINT, 1> strides = { { sizeof(Vertex) } };
	this->mpImmediateContext->IASetVertexBuffers(0, static_cast<UINT>(ppSOBufs.size()), ppSOBufs.data(), strides.data(), soOffsets.data());
	this->mpImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	this->mpImmediateContext->IASetInputLayout(this->mpInputLayout.Get());

	//頂点シェーダ
	this->mpImmediateContext->VSSetShader(this->mpVertexShader.Get(), nullptr, 0);
	//ジオメトリシェーダ
	this->mpImmediateContext->GSSetShader(nullptr, nullptr, 0);
	//ピクセルシェーダ
	this->mpImmediateContext->PSSetShader(this->mpPixelShader.Get(), nullptr, 0);

	if (count == -1) {
		this->mpImmediateContext->DrawAuto();
	} else {
		this->mpImmediateContext->Draw(count * 3, 0);
	}

	ppSOBufs[0] = nullptr;
	this->mpImmediateContext->IASetVertexBuffers(0, static_cast<UINT>(ppSOBufs.size()), ppSOBufs.data(), strides.data(), soOffsets.data());
}

void Scene::onDestroy()
{
}
