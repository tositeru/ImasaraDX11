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

#include "DXSample.h"

#include <d3d11.h>
#include <dxgi.h>
#include <D3Dcompiler.h>

using namespace Microsoft::WRL;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

DXSample::DXSample(UINT width, UINT height, std::wstring name)
	: mWidth(width)
	, mHeight(height)
	, mTitle(name)
	, mAspectRatio(static_cast<float>(width) / static_cast<float>(height))
{
}

DXSample::~DXSample()
{
}

void DXSample::init()
{
	this->initD3D11();
	this->onInit();
}

void DXSample::initD3D11()
{
	auto hwnd = Win32Application::hwnd();
	//	デバイス作成フラグ
	auto deviceFlag = 0;
#ifdef _DEBUG
	deviceFlag |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
	swapChainDesc.OutputWindow = hwnd;
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

	std::array<D3D_DRIVER_TYPE, 3> driverTypes = { {
			D3D_DRIVER_TYPE_HARDWARE,
			D3D_DRIVER_TYPE_REFERENCE,
			D3D_DRIVER_TYPE_SOFTWARE
		} };

	std::array<D3D_FEATURE_LEVEL, 3> featureLevels = { {
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0
		} };

	HRESULT hr = E_FAIL;
	D3D_DRIVER_TYPE usedDriverType;
	ID3D11DeviceContext* pContext = nullptr;
	for (auto driverIndex = 0U; driverIndex < driverTypes.size(); driverIndex++) {
		usedDriverType = driverTypes[driverIndex];
		hr = D3D11CreateDeviceAndSwapChain(
			NULL, usedDriverType, NULL, deviceFlag,
			&featureLevels[0], static_cast<UINT>(featureLevels.size()),
			D3D11_SDK_VERSION, &swapChainDesc,
			&this->mpSwapChain, &this->mpDevice, nullptr, &pContext
			);

		if (SUCCEEDED(hr))break;
	}

	if (FAILED(hr)) {
		throw std::runtime_error("DX11の初期化に失敗しました。");
	}

	this->mpImmediateContext = pContext;
	pContext->Release();

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
	vp.Width = static_cast< float >(this->width());
	vp.Height = static_cast< float >(this->height());
	vp.MinDepth = 0.f;
	vp.MaxDepth = 1.f;
	this->mpImmediateContext->RSSetViewports(1, &vp);
}

void DXSample::update()
{
	this->onUpdate();
}

void DXSample::render()
{
	float clearColor[] = {0, 0.1f, 0.125f, 1};
	this->mpImmediateContext->ClearRenderTargetView(this->mpBackBufferRTV.Get(), clearColor);
	this->mpImmediateContext->ClearDepthStencilView(this->mpZBufferDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
	std::array<ID3D11RenderTargetView*, 1> RTs = {{
			this->mpBackBufferRTV.Get()
	} };
	this->mpImmediateContext->OMSetRenderTargets(static_cast<UINT>(RTs.size()), RTs.data(), this->mpZBufferDSV.Get());

	this->onRender();

	this->mpSwapChain->Present(1, 0);
}

void DXSample::destroy()
{
	this->onDestroy();
}

// Helper function for setting the window's title text.
void DXSample::setCustomWindowText(LPCWSTR text)
{
	std::wstring windowText = this->mTitle + L": " + text;
	SetWindowText(Win32Application::hwnd(), windowText.c_str());
}

// Helper function for parsing any supplied command line args.
_Use_decl_annotations_
void DXSample::parseCommandLineArgs(WCHAR* argv[], int argc)
{
}
