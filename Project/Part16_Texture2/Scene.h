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
	struct Texture2D {
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> mpCSRead;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> mpCSWrite;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mpCSReadParam;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mpCSWriteParam;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> mpResource;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mpSRV;
		std::vector<Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>> mpUAVs;

	};

	struct MipmapReadParam {
		uint32_t mipLevel;
		uint32_t isUseSampler;
		float pad[2];
	};

	struct MipmapWriteParam {
		DirectX::SimpleMath::Vector4 baseColor;
	};

	struct ArrayReadParam {
		uint32_t arrayIndex;
		uint32_t isUseSampler;
		float pad[2];
	};

	struct ArrayWriteParam {
		DirectX::SimpleMath::Vector4 baseColor;
		uint32_t arrayIndex;
		float pad[3];
	};

	struct Cubemap {
		Microsoft::WRL::ComPtr<ID3D11VertexShader>		mpVSRenderCubemap;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>	mpGSRenderCubemap;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>		mpPSRenderCubemap;

		Microsoft::WRL::ComPtr<ID3D11ComputeShader> mpCSUseCubemap;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mpCSUseCubemapParam;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> mpResource;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mpSRV;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mpRTV;

		struct CSUseCubemapParam {
			DirectX::SimpleMath::Matrix mInvProjection;
			DirectX::SimpleMath::Matrix mRotaMatrix;
		}mCubemapParam;
		DirectX::SimpleMath::Vector3 mRotaParam;
	};

	struct MultiSampling {
		Microsoft::WRL::ComPtr<ID3D11VertexShader>		mpVSRenderCubemap;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>	mpGSRenderCubemap;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>		mpPSRenderCubemap;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> mpResource;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mpRTV;

		Microsoft::WRL::ComPtr<ID3D11RasterizerState> mpRasterizerState;
	};

private:
	void initMipmap();
	void runMipmap();

	void initArray(UINT arrayLength);
	void runArray();

	void initCubemap();
	void runCubemap();

	void initMultisampling(UINT samplingCount);
	void runMultisampling();

	void updateTitle();

private:
	enum SHADER_MODE {
		eMODE_MIPMAP,
		eMODE_ARRAY,
		eMODE_CUBEMAP,
		eMODE_MULTISAMPLING,
		eMODE_COUNT,
	} mMode = eMODE_MIPMAP;

	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mpCSClearScreen;

	UINT mReadValue = 0;
	bool mIsUseSampler = false;

	Texture2D mMipmap;
	Texture2D mArray;
	Cubemap mCubemap;
	MultiSampling mMultiSampling;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> mpImage;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mpImageSRV;

	UINT mSamplerIndex = 0;
	std::vector<Microsoft::WRL::ComPtr<ID3D11SamplerState>> mpSamplers;

	Microsoft::WRL::ComPtr<ID3D11Buffer> mpWriteParam;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> mpScreen;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mpScreenUAV;
};
