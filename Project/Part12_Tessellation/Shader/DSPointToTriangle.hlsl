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

uint2 random(uint stream, uint sequence)
{
	uint2 v = uint2(stream, sequence);
	uint s = 0x9E3779B9u;
	[unroll] for (int i = 0; i<5; ++i) {
		v.x += ((v.y << 4u) + 0xA341316Cu) ^ (v.y + s) ^ ((v.y >> 5u) + 0xC8013EA4u);
		v.y += ((v.x << 4u) + 0xAD90777Du) ^ (v.x + s) ^ ((v.x >> 5u) + 0x7E95761Eu);
		s += 0x9E3779B9u;
	}
	return v;
}
float4 calColor(float3 domain)
{
	float4 color = 1;
	uint rnd_seed = domain.x * 123123123 + domain.y * 456456456 + domain.z * 789789789;
	uint2 r = random(rnd_seed, rnd_seed << 3);
	r = random(r.x, r.y);
	color.r = r.x / (float)0xffffffff;
	r = random(r.x, r.y);
	color.g = r.x / (float)0xffffffff;
	r = random(r.x, r.y);
	color.b = r.x / (float)0xffffffff;
	return color;
}

struct DS_OUTPUT
{
	float4 pos  : SV_POSITION;
	float4 color : TEXCOORD0;
};

// 出力制御点
struct HS_CONTROL_POINT_OUTPUT
{
	float3 pos : POSITION;
};

// 出力パッチ定数データ。
struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]			: SV_TessFactor;
	float InsideTessFactor : SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 1

[domain("tri")]
DS_OUTPUT main(
	HS_CONSTANT_DATA_OUTPUT input,
	float3 domain : SV_DomainLocation,
	const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
	DS_OUTPUT output;

	static const float3 offsets[] = {
		{ 0.5f,  0.5f, 0.f },
		{ -0.5f,  0.5f, 0.f },
		{ 0.0f, -0.5f, 0.f },
	};
	output.pos = float4(offsets[0] * domain.x + offsets[1] * domain.y + offsets[2] * domain.z, 1);
	output.color = calColor(domain);

	return output;
}
