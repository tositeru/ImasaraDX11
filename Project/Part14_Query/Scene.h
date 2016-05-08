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
		DirectX::SimpleMath::Vector4 color;
	};

	struct CounterType
	{
		D3D11_COUNTER_TYPE type;
		std::string name;
		std::string units;
		std::string decription;
	};

	union CounterValue {
		float f;
		UINT16 u16;
		UINT32 u32;
		UINT64 u64;

		UINT stride(D3D11_COUNTER_TYPE type)const
		{
			switch (type) {
			case D3D11_COUNTER_TYPE_FLOAT32:	return sizeof(float);
			case D3D11_COUNTER_TYPE_UINT16:		return sizeof(UINT16);
			case D3D11_COUNTER_TYPE_UINT32:		return sizeof(UINT32);
			case D3D11_COUNTER_TYPE_UINT64:		return sizeof(UINT64);
			default:							return 0;
			}
		}

		std::string toString(D3D11_COUNTER_TYPE type)const
		{
			switch (type) {
			case D3D11_COUNTER_TYPE_FLOAT32:	return std::to_string(this->f);
			case D3D11_COUNTER_TYPE_UINT16:		return std::to_string(this->u16);
			case D3D11_COUNTER_TYPE_UINT32:		return std::to_string(this->u32);
			case D3D11_COUNTER_TYPE_UINT64:		return std::to_string(this->u64);
			default:							return "NOT HAVE VALUE";
			}
		}
	};

private:
	void udpateTitle();

	void renderQuery();
	void renderPredicate();

	void outputTriangles(UINT count);
	void renderTriangles(UINT count);

private:
	enum MODE {
		eMODE_QUERY,
		eMODE_PREDICATE,
		eMODE_COUNT,
	} mMode = eMODE_QUERY;

	static const UINT M_STREAM_OUTPUT_COUNT = 100 * 3;

	CounterType mCounterType;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> mpVSStreamOutput;
	Microsoft::WRL::ComPtr<ID3D11GeometryShader> mpGeometryShader;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> mpVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> mpPixelShader;

	Microsoft::WRL::ComPtr<ID3D11Buffer> mpVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> mpInputLayout;

	Microsoft::WRL::ComPtr<ID3D11Buffer> mpStreamOutputBuffer;

	bool mOnPredicate = true;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mpDSNotDepthTestPass;

	Microsoft::WRL::ComPtr<ID3D11Query> mpQuery;
	Microsoft::WRL::ComPtr<ID3D11Predicate> mpPredicate;
	Microsoft::WRL::ComPtr<ID3D11Counter> mpCounter;
	Microsoft::WRL::ComPtr<ID3D11Counter> mpCounter2;
};
