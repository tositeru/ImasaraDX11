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
	float EdgeTessFactor[4]	: SV_TessFactor;
	float InsideTessFactor[2]  : SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 4

// パッチ定数関数
HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
	uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;

	[unroll] for (uint i = 0; i < NUM_CONTROL_POINTS; ++i) {
		Output.EdgeTessFactor[i] = cbEdgeFactor;
	}
	Output.InsideTessFactor[0] = Output.InsideTessFactor[1] = cbInsideFactor;

	[branch] if (false) {
		float4 roundedEdgeFactors;
		float2 roundedInsideFactor;
		float2 unroundedInsideFactor;
		ProcessQuadTessFactorsAvg(cbEdgeFactor, cbInsideFactor, roundedEdgeFactors, roundedInsideFactor, unroundedInsideFactor);
		//ProcessQuadTessFactorsMax(cbEdgeFactor, cbInsideFactor, roundedEdgeFactors, roundedInsideFactor, unroundedInsideFactor);
		//ProcessQuadTessFactorsMin(cbEdgeFactor, cbInsideFactor, roundedEdgeFactors, roundedInsideFactor, unroundedInsideFactor);
		//Process2DQuadTessFactorsAvg(cbEdgeFactor, cbInsideFactor, roundedEdgeFactors, roundedInsideFactor, unroundedInsideFactor);
		//Process2DQuadTessFactorsMax(cbEdgeFactor, cbInsideFactor, roundedEdgeFactors, roundedInsideFactor, unroundedInsideFactor);
		//Process2DQuadTessFactorsMin(cbEdgeFactor, cbInsideFactor, roundedEdgeFactors, roundedInsideFactor, unroundedInsideFactor);

		Output.EdgeTessFactor[0] = roundedEdgeFactors.x;
		Output.EdgeTessFactor[1] = roundedEdgeFactors.y;
		Output.EdgeTessFactor[2] = roundedEdgeFactors.z;
		Output.EdgeTessFactor[3] = roundedEdgeFactors.w;
		Output.InsideTessFactor[0] = roundedInsideFactor.x;
		Output.InsideTessFactor[1] = roundedInsideFactor.y;
	}

	return Output;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("CalcHSPatchConstants")]
[maxtessfactor(16.f)]
HS_CONTROL_POINT_OUTPUT main(
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> inputPatch,
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID)
{
	HS_CONTROL_POINT_OUTPUT Output;
	Output.pos = inputPatch[i].pos;
	return Output;
}
