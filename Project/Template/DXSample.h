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

#pragma once

#include "Common.h"

#include "Win32Application.h"

class DXSample
{
public:
	DXSample(UINT width, UINT height, std::wstring name);
	virtual ~DXSample();

	void init();
	void update();
	void render();
	void destroy();

	// Samples override the event handlers to handle specific messages.
	virtual void onKeyDown(UINT8 /*key*/)   {}
	virtual void onKeyUp(UINT8 /*key*/)     {}

	// Accessors.
	UINT width() const           { return this->mWidth; }
	UINT height() const          { return this->mHeight; }
	const WCHAR* title() const   { return this->mTitle.c_str(); }

	void parseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

	Microsoft::WRL::ComPtr<ID3D11Device>&			device() { return this->mpDevice; }
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>&	context() { return this->mpImmediateContext; }
	Microsoft::WRL::ComPtr<IDXGISwapChain>&			swapChain() { return this->mpSwapChain; }

protected:
	virtual void onInit() = 0;
	virtual void onUpdate() = 0;
	virtual void onRender() = 0;
	virtual void onDestroy() = 0;

	void initD3D11();

	void setCustomWindowText(LPCWSTR text);

	// Viewport dimensions.
	UINT mWidth;
	UINT mHeight;
	float mAspectRatio;

	Microsoft::WRL::ComPtr<ID3D11Device> mpDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> mpImmediateContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mpSwapChain;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mpBackBuffer;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mpBackBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>	mpZBuffer;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	mpZBufferDSV;

private:
	// Window title.
	std::wstring mTitle;
};
