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

struct DummyInput {
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
};

static const float4 trianglePos[3] = {
	float4(0.f, 0.5f, 0.f, 1.f),
	float4(0.5f, -0.5f, 0.f, 1.f),
	float4(-0.5f, -0.5f, 0.f, 1.f),
};


[maxvertexcount(3)]
void main(
	point DummyInput input[1] : POSITION,
	in uint id : SV_PrimitiveID,
	inout TriangleStream< GSOutput > output
	)
{
	[unroll] for (uint i = 0; i < 3; i++) {
		GSOutput element;
		element.pos = trianglePos[i];
		element.color = float4(0.3f, 1.0f, 1.0f, 1.f);
		output.Append(element);
	}
}