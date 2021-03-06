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

#include "Template/DXSample.h"

class Scene : public DXSample
{
public:
	Scene(UINT width, UINT height, std::wstring name);

	virtual void onInit()override;
	virtual void onUpdate()override;
	virtual void onRender()override;
	virtual void onDestroy()override;

	virtual void onKeyUp(UINT8 key)override;

private:
	void updateTitle();

private:
	enum SHADER_MODE {
		eMODE_INDEX,
		eMODE_SAMPLER,
		eMODE_COUNT,
	} mMode = eMODE_INDEX;

	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mpCSClearScreen;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mpCSClearScreenWithSampler;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> mpTex2D;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mpTex2DSRV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mpImage;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mpImageSRV;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> mpSampler;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> mpScreen;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mpScreenUAV;
};
