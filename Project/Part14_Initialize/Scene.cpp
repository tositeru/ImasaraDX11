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

#pragma comment(lib, "dxgi.lib")

Scene::Scene(UINT width, UINT height, std::wstring name)
	: DXSample(width, height, name)
{
}

void Scene::onInit()
{
	this->mpZBufferDSV.Reset();
	this->mpZBuffer.Reset();
	this->mpBackBufferRTV.Reset();
	this->mpBackBuffer.Reset();
	this->mpImmediateContext.Reset();
	this->mpDevice.Reset();
	this->mpSwapChain.Reset();

	//デバイスなどを作り直す
	HRESULT hr;
	Microsoft::WRL::ComPtr<IDXGIFactory1> pFactory;
	hr = CreateDXGIFactory1(IID_PPV_ARGS(pFactory.GetAddressOf()));
	if (FAILED(hr)) {
		throw std::runtime_error("IDXGIFactoryクラスの作成に失敗しました。");
	}

	//GPUアダプターを列挙して一番最初に見つかった使えるものを選ぶ
	Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapterIt;
	for (UINT adapterIndex = 0; S_OK == pFactory->EnumAdapters1(adapterIndex, pAdapterIt.GetAddressOf());  ++adapterIndex) {
		DXGI_ADAPTER_DESC1 desc;
		pAdapterIt->GetDesc1(&desc);
		OutputDebugStringA(  std::string("adapter " + std::to_string(adapterIndex) + "\n").c_str());
		OutputDebugStringW((std::wstring(L"  decription =  ") + desc.Description + L"\n").c_str());
		OutputDebugStringA(  std::string("  VemdorId =  " + std::to_string(desc.VendorId) + "\n").c_str());
		OutputDebugStringA(  std::string("  DeviceId =  " + std::to_string(desc.DeviceId) + "\n").c_str());
		OutputDebugStringA(  std::string("  SubSysId =  " + std::to_string(desc.SubSysId) + "\n").c_str());
		OutputDebugStringA(  std::string("  Revision =  " + std::to_string(desc.Revision) + "\n").c_str());
		OutputDebugStringA(  std::string("  DedicatedVideoMemory =  " + std::to_string(desc.DedicatedVideoMemory) + "\n").c_str());
		OutputDebugStringA(  std::string("  DedicatedSystemMemory =  " + std::to_string(desc.DedicatedSystemMemory) + "\n").c_str());
		OutputDebugStringA(  std::string("  SharedSystemMemory =  " + std::to_string(desc.SharedSystemMemory) + "\n").c_str());
		OutputDebugStringA(  std::string("  AdapterLuid =  high:" + std::to_string(desc.AdapterLuid.HighPart) + " low:" + std::to_string(desc.AdapterLuid.LowPart) + "\n").c_str());

		if (nullptr == this->mpAdapter) {
			if (desc.Flags ^= DXGI_ADAPTER_FLAG_SOFTWARE) {
				this->mpAdapter = pAdapterIt;
				OutputDebugStringA(std::string("このアダプターを使用します。 adapterIndex = " + std::to_string(adapterIndex) + "\n").c_str());
			}

			//期待通りに動作してくれなかった
			//LARGE_INTEGER version;
			//hr = pAdapterIt->CheckInterfaceSupport(__uuidof(ID3D11Device), &version);
			//DXGI_ERROR_UNSUPPORTED;
			//if (S_OK == hr) {
			//	pAdapter = pAdapterIt;
			//	OutputDebugStringA(std::string("このアダプターを使用します。 adapterIndex = " + std::to_string(adapterIndex) + "\n").c_str());
			//}
		}

		//使い終わったら必ずReleaseすること
		pAdapterIt.Reset();
	}

	//ID3D11Deviceの作成
	std::array<D3D_FEATURE_LEVEL, 3> featureLevels = { {
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0
		} };
	UINT flags = 0;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL usedLevel;
	hr = D3D11CreateDevice(
		this->mpAdapter.Get(),
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
		flags,
		featureLevels.data(),
		static_cast<UINT>(featureLevels.size()),
		D3D11_SDK_VERSION,
		this->mpDevice.GetAddressOf(),
		&usedLevel,
		this->mpImmediateContext.GetAddressOf()
		);

	if (FAILED(hr)) {
		throw std::runtime_error("ID3D11Deviceの作成に失敗。");
	}

	//IDXGISwapChainの作成
	DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
	swapChainDesc.OutputWindow = Win32Application::hwnd();
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	//swapChainDesc.Flags = 0;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	//フルスクリーンとウィンドモードの切り替えがしたい場合は、ウィンドウモードとして生成することを推奨しているみたい
	//https://msdn.microsoft.com/en-us/library/bb174579(v=vs.85).aspx
	swapChainDesc.Windowed = true;

	//希望する画面設定
	swapChainDesc.BufferDesc.Width = this->width();
	swapChainDesc.BufferDesc.Height = this->height();
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	//上の画面設定に一番近いものを調べる
	Microsoft::WRL::ComPtr<IDXGIOutput> pOutput;
	if (DXGI_ERROR_NOT_FOUND == this->mpAdapter->EnumOutputs(0, pOutput.GetAddressOf())) {
		throw std::runtime_error("アダプターの出力先が見つかりません。");
	}
	DXGI_MODE_DESC modeDesc;
	hr = pOutput->FindClosestMatchingMode(&swapChainDesc.BufferDesc, &modeDesc, this->mpDevice.Get());
	if (FAILED(hr)) {
		throw std::runtime_error("表示モードの取得に失敗");
	}

	swapChainDesc.BufferDesc = modeDesc;
	hr = pFactory->CreateSwapChain(this->mpDevice.Get(), &swapChainDesc, this->mpSwapChain.GetAddressOf());
	if (FAILED(hr)) {
		throw std::runtime_error("IDXGISwapChainの作成に失敗");
	}

	//ディスプレイの画面モードの一覧を取得する
	//IDXGIOutput* pOutput;
	//this->mpSwapChain->GetContainingOutput(&pOutput);
	//UINT num;
	//UINT flag = DXGI_ENUM_MODES_INTERLACED;
	//pOutput->GetDisplayModeList(swapChainDesc.BufferDesc.Format, flag, &num, nullptr);
	//std::vector<DXGI_MODE_DESC> modeDesces;
	//modeDesces.resize(num);
	//pOutput->GetDisplayModeList(swapChainDesc.BufferDesc.Format, flag, &num, &modeDesces[0]);
	//pOutput->Release();

	//
	//	後はバックバッファのレンダーターゲットビューの作成、必要ならZバッファの作成とビューポートの設定を行う
	//
	initRenderTargetAndDepthStencil(swapChainDesc.BufferDesc.Width, swapChainDesc.BufferDesc.Height);
}

void Scene::initRenderTargetAndDepthStencil(UINT width, UINT height)
{
	this->mpBackBuffer.Reset();
	this->mpBackBufferRTV.Reset();

	this->mpZBuffer.Reset();
	this->mpZBufferDSV.Reset();

	{//バックバッファのRenderTargetViewの作成
		HRESULT hr = this->mpSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&this->mpBackBuffer);
		if (FAILED(hr)) {
			throw std::runtime_error("バックバッファーのリソース取得に失敗");
		}
		this->mpDevice->CreateRenderTargetView(this->mpBackBuffer.Get(), nullptr, &this->mpBackBufferRTV);
	}
	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.Format = DXGI_FORMAT_D32_FLOAT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		auto hr = this->mpDevice->CreateTexture2D(&desc, nullptr, &this->mpZBuffer);
		if (FAILED(hr)) {
			throw std::runtime_error("Zバッファー作成に失敗");
		}

		hr = this->mpDevice->CreateDepthStencilView(this->mpZBuffer.Get(), nullptr, &this->mpZBufferDSV);
		if (FAILED(hr)) {
			throw std::runtime_error("ZバッファーのDepthStencilViewの作成に失敗");
		}
	}

	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = static_cast<float>(width);
	vp.Height = static_cast<float>(height);
	vp.MinDepth = 0.f;
	vp.MaxDepth = 1.f;
	this->mpImmediateContext->RSSetViewports(1, &vp);

	this->mWidth = width;
	this->mHeight = height;
	this->mAspectRatio = static_cast<float>(width) / static_cast<float>(height);

}

void Scene::onUpdate()
{
}

void Scene::onKeyUp(UINT8 key)
{
	if (key == 'Z') {
		DXGI_SWAP_CHAIN_DESC desc;
		this->mpSwapChain->GetDesc(&desc);
		auto hr = this->mpSwapChain->SetFullscreenState(desc.Windowed, nullptr);
		if (FAILED(hr)) {
			throw std::runtime_error("フルスリーンモードとウィンドウモードの切り替えに失敗。");
		}
	}
}


void Scene::onRender()
{
}

void Scene::onDestroy()
{
}

void Scene::onResizeWindow(WPARAM wParam, UINT width, UINT height)
{
	if (this->width() == width && this->height() == height) {
		return;
	}
	if (nullptr == this->mpSwapChain) {
		return;
	}

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	this->mpSwapChain->GetDesc(&swapChainDesc);
	if (!swapChainDesc.Windowed) {
		return;
	}

	this->mpBackBuffer.Reset();
	this->mpBackBufferRTV.Reset();

	auto hr = this->mpSwapChain->ResizeBuffers(swapChainDesc.BufferCount, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags);
	if (FAILED(hr)) {
		throw std::runtime_error("バックバッファのサイズ変更に失敗");
	}

	this->initRenderTargetAndDepthStencil(width, height);
}
