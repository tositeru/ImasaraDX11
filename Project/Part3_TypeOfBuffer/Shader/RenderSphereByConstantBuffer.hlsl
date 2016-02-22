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

//
//	定数バッファを使った球体の描画
//

cbuffer Camera : register(b0)
{
	float4x4 cbInvProjection;
}

cbuffer Param : register(b1)
{
	float3 cbSpherePos;
	float cbSphereRange;
	float3 cbSphereColor;
	float pad;
};

RWTexture2D<float4> screen : register(u0);

float4x4 makeProjectionMatrix(in float2 zPlane, in float degree, in float aspect);

[numthreads(1, 1, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	float2 screenSize;
	screen.GetDimensions(screenSize.x, screenSize.y);
	float4 rayDir = float4(
		(DTid / screenSize) * float2(2, -2) + float2(-1, 1),
		1,
		1);
	rayDir = mul(rayDir, cbInvProjection);
	rayDir /= rayDir.w;
	rayDir.xyz = normalize(rayDir.xyz);

	//レイの方向に球体があるならその色を塗る
	float3 p = rayDir.xyz * dot(rayDir.xyz, cbSpherePos);
	float len = distance(p, cbSpherePos);
	[branch] if (len < cbSphereRange) {
		screen[DTid] = float4(cbSphereColor * ((1 - saturate(len / cbSphereRange))*0.8f + 0.2f), 1);
	} else {
		screen[DTid] = float4(0, 0.125f, 0.2f, 1);
	}
}
