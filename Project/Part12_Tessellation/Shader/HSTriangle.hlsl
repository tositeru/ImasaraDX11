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

cbuffer Param : register(b0)
{
	float cbEdgeFactor;
	float cbInsideFactor;
};

struct VS_CONTROL_POINT_OUTPUT
{
	float3 pos : POSITION;
};

struct HS_CONTROL_POINT_OUTPUT
{
	float3 pos : POSITION;
};

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]			: SV_TessFactor;
	float InsideTessFactor			: SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 3

// パッチ定数関数
HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
	uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;

	//分割数の設定
	Output.EdgeTessFactor[0] = Output.EdgeTessFactor[1] = Output.EdgeTessFactor[2] = cbEdgeFactor;
	Output.InsideTessFactor = cbInsideFactor;

	[branch] if (false)
	{
		//分割数の調節を行ってくれる組み込み関数が用意されている
		float3 roundedEdgeFactor;
		float roundedInsideFactor;
		float unroundedInsideFactor;
		ProcessTriTessFactorsAvg(cbEdgeFactor, cbInsideFactor, roundedEdgeFactor, roundedInsideFactor, unroundedInsideFactor);
		//ProcessTriTessFactorsMax(cbEdgeFactor, cbInsideFactor, roundedEdgeFactor, roundedInsideFactor, unroundedInsideFactor);
		//ProcessTriTessFactorsMin(cbEdgeFactor, cbInsideFactor, roundedEdgeFactor, roundedInsideFactor, unroundedInsideFactor);
		Output.EdgeTessFactor[0] = roundedEdgeFactor.x;
		Output.EdgeTessFactor[1] = roundedEdgeFactor.y;
		Output.EdgeTessFactor[2] = roundedEdgeFactor.z;

		Output.InsideTessFactor = roundedInsideFactor;
		//Output.InsideTessFactor = unroundedInsideFactor;
	}
	return Output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("CalcHSPatchConstants")]
HS_CONTROL_POINT_OUTPUT main( 
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> inputPatch, 
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID )
{
	HS_CONTROL_POINT_OUTPUT Output;
	Output.pos = inputPatch[i].pos;
	return Output;
}
