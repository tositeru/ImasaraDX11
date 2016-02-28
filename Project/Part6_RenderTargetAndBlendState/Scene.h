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
#include <DirectXTK\Inc\SimpleMath.h>

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
	struct Vertex {
		DirectX::SimpleMath::Vector3 pos;
	};

	struct InstancedParam
	{
		DirectX::SimpleMath::Vector3 offset;
		float pad;
		DirectX::SimpleMath::Vector4 color;
	};
	std::vector<InstancedParam> makeInstanceData()const;

private:
	enum BLEND_MODE {
		eBLEND_MODE_SAME_BLEND,
		eBLEND_MODE_SEPARATEBLEND,
		eBLEND_MODE_COUNT,
	} mBlendMode = eBLEND_MODE_SAME_BLEND;

	enum SHOW_RENDER_TARGET {
		eSHOW_RT_1,
		eSHOW_RT_2,
		eSHOW_RT_COUNT,
	}mShowRT = eSHOW_RT_1;
	static const UINT M_INSTANCED_COUNT = 5;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> mpVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> mpPixelShader;

	Microsoft::WRL::ComPtr<ID3D11Buffer> mpVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> mpIndexBuffer;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> mpInputLayout;

	Microsoft::WRL::ComPtr<ID3D11Buffer> mpOffsetBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mpOffsetBufferSRV;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> mpRenderTarget1;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mpRenderTarget2;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mpRenderTarget1RTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mpRenderTarget2RTV;

	Microsoft::WRL::ComPtr<ID3D11BlendState> mpBlendState;
	Microsoft::WRL::ComPtr<ID3D11BlendState> mpBlendState2;

};
