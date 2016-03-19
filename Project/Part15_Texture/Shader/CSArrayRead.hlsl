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

cbuffer Param :register(b0)
{
	uint cbArrayIndex;
	bool cbIsUseSampler;
	uint2 pad;
}

Texture2DArray<float4> image : register(t0);

SamplerState linear_sampler : register(s0);

RWTexture2D<float4> screen : register(u0);

[numthreads(8, 8, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	uint2 image_size;
	uint arrayCount;
	uint arrayLength;
	image.GetDimensions(image_size.x, image_size.y, arrayLength);

	[branch]
	if (cbIsUseSampler) {
		uint2 screen_size;
		screen.GetDimensions(screen_size.x, screen_size.y);
		float3 uv_and_index = float3((float2)DTid / (float2)screen_size, cbArrayIndex);
		screen[DTid] = image.SampleLevel(linear_sampler, uv_and_index, 0);
	} else {
		[branch]
		if (DTid.x < image_size.x && DTid.y < image_size.y) {
			screen[DTid] = image[uint3(DTid, min(cbArrayIndex, arrayLength - 1))];
			//screen[DTid] = image.mip[mipLevel][uint3(DTid, min(cbArrayIndex, arrayLength - 1))];
		} else {
			screen[DTid] = float4(0, 0, 0, 1);
		}
	}
}
