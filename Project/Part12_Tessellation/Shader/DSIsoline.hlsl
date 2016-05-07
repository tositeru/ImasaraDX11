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
float4 calColor(float2 domain)
{
	float4 color = 1;
	uint rnd_seed = domain.x * 123123123 + domain.y * 456456456;
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

// 出力されたパッチ定数データ。
struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[2]			: SV_TessFactor;
};

#define NUM_CONTROL_POINTS 2

[domain("isoline")]
DS_OUTPUT main(
	HS_CONSTANT_DATA_OUTPUT input,
	float2 domain : SV_DomainLocation,
	const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
	DS_OUTPUT output;

	//適当に位置をずらしている
	//実際にはパッチ定数関数の出力値を元に処理を行う形になると思う。
	output.pos = float4(lerp(patch[0].pos, patch[1].pos, domain.x), 1);
	output.pos.x += 0.5f * domain.y;
	//上に凸な曲線を描くためにyの位置をずらしている。
	const float RATE = abs(domain.x - 0.5f) * 2.f;
	const float PI = 3.14159265f;
	output.pos.y = cos(RATE * 0.5f * PI);

	output.color = calColor(domain);
	output.color.b = 0;

	return output;
}
