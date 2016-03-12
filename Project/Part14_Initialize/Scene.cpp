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
	Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter, pAdapterIt;
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

		if (nullptr == pAdapter) {
			if (desc.Flags ^= DXGI_ADAPTER_FLAG_SOFTWARE) {
				pAdapter = pAdapterIt;
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
		pAdapter.Get(),
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
	swapChainDesc.BufferDesc.Width = this->width();
	swapChainDesc.BufferDesc.Height = this->height();
	swapChainDesc.Windowed = true;

	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;

	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.Flags = 0;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	hr = pFactory->CreateSwapChain(this->mpDevice.Get(), &swapChainDesc, this->mpSwapChain.GetAddressOf());
	if (FAILED(hr)) {
		throw std::runtime_error("IDXGISwapChainの作成に失敗");
	}

	//
	//	後はバックバッファのレンダーターゲットビューの作成、必要ならZバッファの作成とビューポートの設定を行う
	//
	{
		{//バックバッファのRenderTargetViewの作成
			HRESULT hr = this->mpSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&this->mpBackBuffer);
			if (FAILED(hr)) {
				throw std::runtime_error("バックバッファーのリソース取得に失敗");
			}
			this->mpDevice->CreateRenderTargetView(this->mpBackBuffer.Get(), nullptr, &this->mpBackBufferRTV);
		}
		{
			D3D11_TEXTURE2D_DESC desc = {};
			desc.Width = this->width();
			desc.Height = this->height();
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
		vp.Width = static_cast<float>(this->width());
		vp.Height = static_cast<float>(this->height());
		vp.MinDepth = 0.f;
		vp.MaxDepth = 1.f;
		this->mpImmediateContext->RSSetViewports(1, &vp);
	}
}

void Scene::onUpdate()
{
}

void Scene::onKeyUp(UINT8 key)
{
}


void Scene::onRender()
{
}

void Scene::onDestroy()
{
}

