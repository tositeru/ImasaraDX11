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
	float4x4 cbInvProjection;
	float4x4 cbRotaMatrix;
}

TextureCube<float4> cubemap : register(t0);

SamplerState linear_sampler : register(s0);

RWTexture2D<float4> screen : register(u0);

[numthreads(8, 8, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float2 screenSize;
	screen.GetDimensions(screenSize.x, screenSize.y);
	float4 rayDir = float4( (DTid.xy / screenSize) * float2(2, -2) + float2(-1, 1), 1,	1);
	rayDir = mul(rayDir, cbInvProjection);
	rayDir /= rayDir.w;
	rayDir.xyz = normalize(rayDir.xyz);

	//ÉåÉCÇÃï˚å¸Ç…ãÖëÃÇ™Ç†ÇÈÇ»ÇÁÇªÇÃêFÇìhÇÈ
	const float3 spherePos = float3(0, 0, 30.f);
	const float sphereRange = 10.f;
	float3 p = rayDir.xyz * dot(rayDir.xyz, spherePos);
	float len = distance(p, spherePos);
	[branch] if (len < sphereRange) {
		//ãÖëÃÇÃñ@ê¸ÇãÅÇﬂÇÈ
		float A = dot(rayDir.xyz, rayDir.xyz);
		float B = dot(rayDir.xyz, spherePos);
		float C = dot(spherePos, spherePos) - pow(sphereRange, 2);
		float3 nearIntersectionPos = (B - sqrt(pow(B, 2) - A*C)) / A * rayDir;
		float3 N = normalize(nearIntersectionPos - spherePos);
		N = mul(N, (float3x3)cbRotaMatrix);
		float3 reflectDir = reflect(N, rayDir.xyz);
		screen[DTid.xy] = cubemap.SampleLevel(linear_sampler, reflectDir, 0);
	} else {
		screen[DTid.xy] = float4(0, 0.125f, 0.2f, 1);
	}
}