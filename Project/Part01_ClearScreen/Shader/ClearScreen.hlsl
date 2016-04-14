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

RWTexture2D<float4> screen : register(u0);

[numthreads(1, 1, 1)]
void main( uint2 DTid : SV_DispatchThreadID )
{
	screen[DTid] = float4(1, 1, 0, 1);
}

//
//	C++風な画面クリア
//		Dispatch関数の引数にはすべて1を渡してください
//		D3DCompileFromFileのマクロの動作テストも兼ねてます
//
[numthreads(1, 1, 1)]
void clearByOneThread(uint2 DTid : SV_DispatchThreadID)
{
	//1つのスレッドしか処理しないようする
	if (DTid.x != 0 || DTid.y != 0) {
		return;
	}

	//screenのサイズを取得している
	uint2 size;
	screen.GetDimensions(size.x, size.y);

	//全ピクセルクリアー
	for (uint y = 0; y < size.y; ++y) {
		for (uint x = 0; x < size.x; ++x) {
#ifdef DEFINE_MACRO
			screen[uint2(x, y)] = DEFINE_MACRO;
#else
			screen[uint2(x, y)] = float4(0.3f, 1, 0.3f, 1);
#endif
		}
	}
}