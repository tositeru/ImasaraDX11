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
	float4 cbBaseColor;
}

RWTexture2D<float4> output : register(u0);

float noise_(float x, float y);

[numthreads(8, 8, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	uint2 image_size;
	output.GetDimensions(image_size.x, image_size.y);

	[branch]
	if(DTid.x == 0) {
		output[DTid] = float4(1, 0, 0, 1);
	} else if(DTid.y == 0) {
		output[DTid] = float4(0, 1, 0, 1);
	} else if(DTid.x+1 == image_size.x) {
		output[DTid] = float4(0, 1, 1, 1);
	} else if(DTid.y+1 == image_size.y) {
		output[DTid] = float4(1, 0, 1, 1);
	} else if(DTid.x < image_size.x && DTid.y < image_size.y) {
		float2 pos = DTid / (float2)image_size;
		pos += dot(cbBaseColor, float4(1.2f, 4.5f, 10.9f, 3.01f));
		pos *= 25;
		float factor = (noise_(pos.x, pos.y) * 0.5f + 0.5f);
		output[DTid] = cbBaseColor * (1 + sin((factor / 2) * 50.f)) / 2.f;
	}
}

//以下のコードは http://c5h12.hatenablog.com/entry/2014/07/06/084125 を参考した
static const uint hashSeed[256 * 2] = {
 36, 102, 45, 194, 188, 241, 32, 141, 115, 97, 117, 82, 143, 209, 1, 112, 158, 169,
 213, 77, 223, 253, 43, 133, 238, 76, 40, 90, 222, 177, 139, 95, 83, 219, 55, 191, 144, 26,
 203, 37, 232, 221, 0, 17, 100, 59, 138, 11, 204, 134, 38, 71, 207, 84, 114, 235, 210, 23,
 248, 251, 130, 81, 183, 201, 145, 93, 31, 151, 9, 6, 152, 94, 127, 99, 176, 61, 54, 212,
 51, 22, 142, 192, 33, 19, 208, 189, 74, 157, 88, 24, 60, 147, 64, 50, 202, 181, 53, 250,
 215, 186, 228, 150, 105, 30, 69, 140, 35, 200, 224, 107, 27, 57, 185, 225, 92, 155, 226,
 220, 78, 164, 87, 66, 172, 132, 116, 67, 126, 42, 246, 217, 146, 70, 108, 171, 2, 242,
 166, 96, 52, 62, 44, 121, 240, 167, 89, 214, 16, 124, 129, 197, 41, 216, 49, 8, 211, 72,
 120, 46, 170, 48, 122, 174, 153, 104, 68, 5, 125, 101, 230, 205, 187, 179, 58, 182, 21,
 65, 249, 137, 12, 243, 252, 165, 85, 245, 86, 254, 123, 7, 154, 47, 4, 28, 136, 34, 14,
 15, 161, 135, 79, 218, 29, 25, 131, 10, 56, 156, 234, 119, 63, 229, 233, 91, 103, 39, 190,
 118, 3, 198, 113, 75, 244, 163, 80, 178, 160, 173, 227, 106, 196, 149, 148, 175, 255, 236,
 18, 206, 168, 128, 231, 247, 111, 13, 110, 180, 73, 109, 162, 193, 199, 98, 184, 195, 237,
 20, 239, 159,
 36, 102, 45, 194, 188, 241, 32, 141, 115, 97, 117, 82, 143, 209, 1, 112, 158, 169,
 213, 77, 223, 253, 43, 133, 238, 76, 40, 90, 222, 177, 139, 95, 83, 219, 55, 191, 144, 26,
 203, 37, 232, 221, 0, 17, 100, 59, 138, 11, 204, 134, 38, 71, 207, 84, 114, 235, 210, 23,
 248, 251, 130, 81, 183, 201, 145, 93, 31, 151, 9, 6, 152, 94, 127, 99, 176, 61, 54, 212,
 51, 22, 142, 192, 33, 19, 208, 189, 74, 157, 88, 24, 60, 147, 64, 50, 202, 181, 53, 250,
 215, 186, 228, 150, 105, 30, 69, 140, 35, 200, 224, 107, 27, 57, 185, 225, 92, 155, 226,
 220, 78, 164, 87, 66, 172, 132, 116, 67, 126, 42, 246, 217, 146, 70, 108, 171, 2, 242,
 166, 96, 52, 62, 44, 121, 240, 167, 89, 214, 16, 124, 129, 197, 41, 216, 49, 8, 211, 72,
 120, 46, 170, 48, 122, 174, 153, 104, 68, 5, 125, 101, 230, 205, 187, 179, 58, 182, 21,
 65, 249, 137, 12, 243, 252, 165, 85, 245, 86, 254, 123, 7, 154, 47, 4, 28, 136, 34, 14,
 15, 161, 135, 79, 218, 29, 25, 131, 10, 56, 156, 234, 119, 63, 229, 233, 91, 103, 39, 190,
 118, 3, 198, 113, 75, 244, 163, 80, 178, 160, 173, 227, 106, 196, 149, 148, 175, 255, 236,
 18, 206, 168, 128, 231, 247, 111, 13, 110, 180, 73, 109, 162, 193, 199, 98, 184, 195, 237,
 20, 239, 159,
};
uint hash(uint x, uint y) {
    x &= 0xff;
    y &= 0xff;
    return hashSeed[x + hashSeed[y]]; // Pで同じものを繰り返しておくとここが簡潔になる
}

float value(float x, float y) {
    return hash(x, y) / 255.0;
}
float interpolate(float t) {
    return t * t * t * (10.0 + t * (-15.0 + 6.0 * t));
}

float mix(float a, float b, float t) {
    return a * (1.0 - t) + b * t;
}

// 適当なベクトル
static const float2 G[4] = {
    float2( 1.0,  1.0),
    float2(-1.0,  1.0),
    float2(-1.0, -1.0),
    float2( 1.0, -1.0)
};

float gradient(uint ix, uint iy, float x, float y) {
	float2 v = float2(x - ix, y - iy);
	return dot(G[ hash(ix, iy) % 4 ], v); // 内積をとる
}

float noise_(float x, float y)
{
	uint ix = floor(x); // 整数部を取り出す
	uint iy = floor(y);
	
	// 注目する点を囲む格子の頂点の値
    float v00 = gradient(ix    , iy    , x, y);
    float v10 = gradient(ix + 1, iy    , x, y);
    float v01 = gradient(ix    , iy + 1, x, y);
    float v11 = gradient(ix + 1, iy + 1, x, y);
	
    float tx = x - ix; // 入力から整数部を引いて少数部を取り出す
    float ty = y - iy;
    
    tx = interpolate(tx); // 滑らかになるように曲線に変換
    ty = interpolate(ty);
    
    float v0010 = mix(v00, v10, tx);
    float v0111 = mix(v01, v11, tx);
	
	return mix(v0010, v0111, ty);
}

