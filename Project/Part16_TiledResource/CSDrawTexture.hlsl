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

cbuffer Param {
	float4 cbColor;
};

RWTexture2D<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint2 size;
	output.GetDimensions(size.x, size.y);

	[branch] if (DTid.x < size.x && DTid.y < size.y) {
		//チェッカー柄で塗りつぶす
		uint2 massSize = size / 10;
		uint2 pos = DTid / massSize;
		float4 color;
		if ((pos.y % 2 + pos.x) % 2 == 0) {
			color = cbColor;
		} else {
			color = float4(0, 0, 0, 1);
		}
		output[DTid.xy] = color;
	}
}
