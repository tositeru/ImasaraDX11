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

struct Input
{
	uint id : ID;
};

struct GSOutput
{
	float4 pos : POSITION;
	float4 color : TEXCOORD0;
};

static const float4 trianglePos[3] = {
	float4(0.f, 0.5f, 0.1f, 1.f),
	float4(0.5f, -0.5f, 0.1f, 1.f),
	float4(-0.5f, -0.5f, 0.1f, 1.f),
};

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

[maxvertexcount(3)]
void main(
	point Input input[1],
	inout TriangleStream< GSOutput > output
	)
{
	uint2 r = random(input[0].id, input[0].id*input[0].id);
	float4 offset = 0;
	offset.x = r.x / (float)0xffffffff * 2.f - 1;
	r = random(r.x, r.y);
	offset.y = r.x / (float)0xffffffff * 2.f - 1;

	[unroll] for (uint i = 0; i < 3; i++) {
		GSOutput element;
		element.pos = trianglePos[i]*0.15f + offset;

		element.color = 1;
		r = random(r.x, r.y);
		element.color.r = r.x / (float)0xffffffff;
		r = random(r.x, r.y);
		element.color.g = r.x / (float)0xffffffff;
		r = random(r.x, r.y);
		element.color.b = r.x / (float)0xffffffff;

		output.Append(element);
	}
}