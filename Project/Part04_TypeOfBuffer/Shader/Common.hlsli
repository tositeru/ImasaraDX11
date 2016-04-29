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

float4 calRayDir(in uint2 DTid, in float2 screenSize, in float4x4 invProjection)
{
	float4 rayDir;
	rayDir.xy = (DTid / screenSize) * float2(2, -2) + float2(-1, 1);
	rayDir.zw = 1;

	rayDir = mul(rayDir, invProjection);
	rayDir /= rayDir.w;
	rayDir.xyz = normalize(rayDir.xyz);
	return rayDir;
}

float4 calColor(float3 rayDir, float3 spherePos, float sphereRange, float3 color)
{
	float3 p = rayDir.xyz * dot(rayDir.xyz, spherePos);
	float len = distance(p, spherePos);
	[branch] if (len < sphereRange) {
		return float4(color * ((1 - saturate(len / sphereRange))*0.8f + 0.2f), 1);
	} else {
		return float4(0, 0.125f, 0.2f, 1);
	}
}
