---
layout: default
title: "ID3D11Bufferのバリエーション"
categories: part
description: "今パートでは球体を表示するシェーダと単純なデータの受け渡しを行うシェーダを元に様々なバッファのバリエーションについて見ていきます。"
---
<h1 class="under-bar">ID3D11Bufferのバリエーション</h1>
定数バッファを使う際に<span class="keyward">ID3D11Buffer</span>を使いましたが、<span class="important">このクラスが表すことが出来るものにはまだ他にいくつかのバリエーションがあります。</span>
ここではそれについて見ていきます。

ちなみに<span class="keyward">ID3D11Buffer</span>のことをバッファと呼んだりします。
以後、バッファと呼んでいきますので、出てきたときは<span class="keyward">ID3D11Buffer</span>のことだと思ってください。

<h1 class="under-bar">概要</h1>
今パートでは球体を表示するシェーダと単純なデータの受け渡しを行うシェーダの２種類を元に様々なバッファのバリエーションについて見ていきます。
対応するプロジェクトは<span class="important">Part04_TypeOfBuffer</span>になります。

<div class="summary">
  <ol>
    <li><a href="#CONSTANT_BUFFER">球体を描画するシェーダ</a></li>
    <li><a href="#STRUCTURED_BUFFER"><span class="keyward">StructuredBuffer</span></a>
    </li>
    <li><a href="#BYTE_ADDRESS_BUFFER"><span class="keyward">ByteAddressBuffer</span></a>
    </li>
    <li><a href="#STACK_BUFFER">スタック操作を行うバッファ</a>
    </li>
    <li><a href="#SUMMARY">まとめ</a></li>
    <li><a href="#SUPPLEMENTAL">補足</a>
      <ul>
        <li>レイトレースとラスタライズ法</li>
      </ul>
    </li>
  </ol>
</div>

<h1 class="under-bar">1.球体を描画するシェーダ</h1>
<a name="CONSTANT_BUFFER"></a>

まず、比較するために定数バッファを使った球体描画シェーダのソースを見てみます。

{% highlight c++%}
// RenderSphereByConstantBuffer.hlsl
#include "Common.hlsli"
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
[numthreads(1, 1, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
  float2 screenSize;
  screen.GetDimensions(screenSize.x, screenSize.y);
  float4 rayDir = calRayDir(DTid, screenSize, cbInvProjection);
  //レイの方向に球体があるならその色を塗る
  screen[DTid] = calColor(rayDir.xyz, cbSpherePos, cbSphereRange, cbSphereColor);
}
{% endhighlight %}

シェーダ自体は単純なもので、球体を描画をする関数に定数バッファとして宣言したパラメータを渡しているだけです。
球体を描画している部分は<span class="keyward">Common.hlsli</span>で定義していますが、本筋ではないので省略します。

このパートの<span class="keyward">StructuredBuffer</span>と<span class="keyward">ByteAddressBuffer</span>はこのコードをベースにしています。
<span class="important">注目して見てもらいたい部分は定数バッファのParamの部分がどのように変わり、どのようにデータにアクセスしているかです。</span>

<h1 class="under-bar">2.StructuredBuffer</h1>
<a name="STRUCTURED_BUFFER"></a>
それでは<span class="keyward">StructuredBuffer</span>について見ていきましょう。
<span class="important"><span class="keyward">StructuredBuffer</span>は名前の通りCPU側の構造体をシェーダ内で直接読み込むことができるものになります。</span>

<h3>シェーダ側</h3>
{% highlight c++%}
// RenderSphereByStructuredBuffer.hlsl
#include "Common.hlsli"
cbuffer Camera : register(b0)
{
  float4x4 cbInvProjection;
}
//sphereInfoの要素となる構造体定義
struct Param
{
  float3 pos;
  float range;
  float3 color;
  float pad;
};
//StructuredBufferはシェーダリソースビューとして設定する
StructuredBuffer<Param> sphereInfo : register(t0);
RWTexture2D<float4> screen : register(u0);
[numthreads(1, 1, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
  float2 screenSize;
  screen.GetDimensions(screenSize.x, screenSize.y);
  float4 rayDir = calRayDir(DTid, screenSize, cbInvProjection);
  //レイの方向に球体があるならその色を塗る
  Param sphere = sphereInfo[0];
  screen[DTid] = calColor(rayDir.xyz, sphere.pos, sphere.range, sphere.color);
}
{% endhighlight %}

シェーダ内でも構造体を定義することができ、StructuredBufferの<span class="keyward">"<...>"</span>には型名を指定してください。
<span class="important">データにアクセスするときは配列のように添え字を使ってアクセスします。</span>
あとは、C++の構造体と同じように使います。

ドキュメント:
[StructuredBuffer(日本語)][STRUCTURED_BUFFER_JP]
[StructuredBuffer(英語)][STRUCTURED_BUFFER_EN]

[STRUCTURED_BUFFER_JP]: https://msdn.microsoft.com/ja-jp/library/ee422384(v=vs.85).aspx
[STRUCTURED_BUFFER_EN]: https://msdn.microsoft.com/en-us/library/windows/desktop/ff471514(v=vs.85).aspx


<span class="important"><span class="keyward">StructuredBuffer</span>はテクスチャと同じようにシェーダリソースビューとして扱われます。
なので、スロットの指定はテクスチャと同じで"<span class="keyward">register(t0)</span>"みたいに行います。</span>

<h3>CPU側</h3>

{% highlight c++ %}
D3D11_BUFFER_DESC desc = {};
//StructuredBufferとして扱うときはMiscFlagにD3D11_RESOURCE_MISC_BUFFER_STRUCTUREDを指定する必要がある
desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
//構造体のサイズも一緒に設定すること
desc.StructureByteStride = sizeof(SphereInfo);
//後は他のバッファと同じ
desc.ByteWidth = sizeof(SphereInfo);
desc.Usage = D3D11_USAGE_DEFAULT;
desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

//初期データの設定
SphereInfo info;
info.pos = DirectX::SimpleMath::Vector3(10, 0, 30.f);
info.range = 5.f;
info.color = DirectX::SimpleMath::Vector3(0.4f, 1.f, 0.4f);
D3D11_SUBRESOURCE_DATA initData;
initData.pSysMem = &info;
initData.SysMemPitch = sizeof(info);
//バッファ作成
auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpStructuredBuffer.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("構造化バッファの作成に失敗");
}
//シェーダリソースビューも作る必要がある
//また、Formatは必ずDXGI_FORMAT_UNKNOWNにしないといけない
D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
srvDesc.Format = DXGI_FORMAT_UNKNOWN;
srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
srvDesc.Buffer.FirstElement = 0;
srvDesc.Buffer.NumElements = 1;
// 次のコードでも設定出来る
//srvDesc.Buffer.ElementOffset = 0;
//srvDesc.Buffer.ElementWidth = 1;
// ViewDimensionにD3D11_SRV_DIMENSION_BUFFEREXを指定してもOK
//srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
//srvDesc.BufferEx.FirstElement = 0;
//srvDesc.BufferEx.Flags = 0;
//srvDesc.BufferEx.NumElements = 1;
hr = this->mpDevice->CreateShaderResourceView(this->mpStructuredBuffer.Get(), &srvDesc, this->mpStructuredBufferSRV.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("構造化バッファのShaderResourceViewの作成に失敗");
}
{% endhighlight %}

定数バッファとの作成の違いはいくつか設定する項目が増えただけです。
<br>ドキュメント: [D3D11_BUFFER_DESC(日本語)][MSDN_BUFFER_DESC_JP] [D3D11_BUFFER_DESC(英語)][MSDN_BUFFER_DESC_EN]

[MSDN_BUFFER_DESC_JP]: https://msdn.microsoft.com/ja-jp/library/ee416048(v=vs.85).aspx
[MSDN_BUFFER_DESC_EN]: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476092(v=vs.85).aspx

<ul>
  <li><span class="keyward">StructureByteStride</span>
    <p>構造体1個当たりのサイズを設定します。</p>
  </li>
  <li><span class="keyward">MiscFlags</span>
    <p>
      様々なフラグを設定するときに使用します。
      <span class="keyward">StructuredBuffer</span>を使う際は<span class="keyward">D3D11_RESOURCE_MISC_BUFFER_STRUCTURED</span>を指定する必要があります。
      <br>ドキュメント:
      <a href="https://msdn.microsoft.com/ja-jp/library/ee416267(v=vs.85).aspx">D3D11_RESOURCE_MISC_FLAG(日本語)</a>
      <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476203(v=vs.85).aspx">D3D11_RESOURCE_MISC_FLAG(英語)</a>      
    </p>
  </li>
</ul>

また、GPUに設定するときはシェーダリソースビューとして扱うため<span class="keyward">BindFlags</span>に<span class="keyward">D3D11_BIND_SHADER_RESOURCE</span>を設定する必要があります。

次にシェーダリソースビューを生成する際は、<span class="keyward">ViewDimension</span>に<span class="keyward">D3D11_SRV_DIMENSION_BUFFER</span>か<span class="keyward">D3D11_SRV_DIMENSION_BUFFEREX</span>を設定する必要があります。
各々設定したときのパラメータについてはサンプルを参考にしてください。
データを読み取り始めるオフセットと個数を設定する必要があります。
<span class="important">また、<span class="keyward">Format</span>には必ず、<span class="keyward">DXGI_FORMAT_UNKNOWN</span>を設定する必要があります。</span>


効率的な<span class="keyward">StructuredBuffer</span>の使い方は<span class="keyward">nVidia　GameWorks</span>のブログにて解説されています。
日本語訳もあった気がするのですが、見つからなかったので元記事のリンクを張っておきます。
端的に言うと構造体のサイズを16byteの倍数にすると効率よくデータにアクセスできるようです。
<br>
[Understanding Structured Buffer Performance][NVIDIA_STRUCTURED_PART1]<br>
[Redundancy and Latency in Structured Buffer Use][NVIDIA_STRUCTURED_PART2]<br>
[How About Constant Buffers?][NVIDIA_STRUCTURED_PART3]<br>

[NVIDIA_STRUCTURED_PART1]:https://developer.nvidia.com/content/understanding-structured-buffer-performance
[NVIDIA_STRUCTURED_PART2]:https://developer.nvidia.com/content/redundancy-and-latency-structured-buffer-use
[NVIDIA_STRUCTURED_PART3]:https://developer.nvidia.com/content/how-about-constant-buffers

<h1 class="under-bar">3.ByteAddressBuffer</h1>
<a name="BYTE_ADDRESS_BUFFER"></a>
<h3>シェーダ側</h3>
次に、<span class="keyward">ByteAddressBuffer</span>について見ていきます。<span class="important"><span class="keyward">ByteAddressBuffer</span>は4バイト単位でデータを読み込むことが出来るものになります。</span>

{% highlight c++ %}
//RenderSphereByByteAddressBuffer.hlsl
#include "Common.hlsli"
cbuffer Camera : register(b0)
{
  float4x4 cbInvProjection;
}
//float3 pos;
//float range;
//float3 color;
#define POS (0 * 4)
#define RANGE (3 * 4)
#define COLOR (4 * 4)
ByteAddressBuffer sphereInfo : register(t0);
RWTexture2D<float4> screen : register(u0);
[numthreads(1, 1, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
  float2 screenSize;
  screen.GetDimensions(screenSize.x, screenSize.y);
  float4 rayDir = calRayDir(DTid, screenSize, cbInvProjection);

  //レイの方向に球体があるならその色を塗る
  float3 spherePos = asfloat(sphereInfo.Load3(POS));
  float sphereRange = asfloat(sphereInfo.Load(RANGE));
  float3 color = asfloat(sphereInfo.Load3(COLOR));
  screen[DTid] = calColor(rayDir.xyz, spherePos, sphereRange, color);
}
{% endhighlight %}

<span class="keyward">ByteAddressBuffer</span>は<span class="keyward">uint</span>しか読み込むことしかできません。
<span class="important">なので、<span class="keyward">float</span>など別の型を読み込みたい場合は<span class="keyward">asfloat関数</span>などを利用する必要があります。</span>
<span class="important">また、複数の<span class="keyward">uint</span>を一度に読み込むことが出来る関数も用意されています。</span>

構造体のようにデータを読み込むにはこちらで並びを意識する必要があるので手間がかかるものになります。
<span class="important">基本的には<span class="keyward">StructuredBuffer</span>を使った方が手軽なのですが、頂点バッファというグラフィックスパイプラインで使うバッファをシェーダリソースビューとして扱う際はこれを使う必要が出てきます。</span>
[こちら][BAB_EXANPLE]のサイトでも<span class="keyward">ByteAddressBuffer</span>を扱っているので参考にしてください。

[BAB_EXANPLE]:http://sygh.hatenadiary.jp/entry/2014/05/03/010530

ドキュメント:
[ByteAddressBuffer(日本語)][BYTE_ADDRESS_BUFFER_JP]
[ByteAddressBuffer(英語)][BYTE_ADDRESS_BUFFER_EN]
[asfloat(日本語)][ASFLOAT_JP]
[asfloat(英語)][ASFLOAT_EN]

[BYTE_ADDRESS_BUFFER_JP]: https://msdn.microsoft.com/ja-jp/library/ee422328(v=vs.85).aspx
[BYTE_ADDRESS_BUFFER_EN]: https://msdn.microsoft.com/en-us/library/windows/desktop/ff471453(v=vs.85).aspx
[ASFLOAT_JP]:https://msdn.microsoft.com/ja-jp/library/ee418198(v=vs.85).aspx
[ASFLOAT_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/bb509570(v=vs.85).aspx

<h3>CPU側</h3>
{% highlight c++ %}
D3D11_BUFFER_DESC desc = {};
//ByteAddressBufferとして扱うときはMiscFlagにD3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWSを指定する必要がある
desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
//後は他のバッファと同じ
desc.ByteWidth = sizeof(SphereInfo);
desc.Usage = D3D11_USAGE_DEFAULT;
desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
//初期データの設定
SphereInfo info;
info.pos = DirectX::SimpleMath::Vector3(-15, 0, 45.f);
info.range = 15.f;
info.color = DirectX::SimpleMath::Vector3(0.4f, 0.4f, 1.f);
D3D11_SUBRESOURCE_DATA initData;
initData.pSysMem = &info;
initData.SysMemPitch = sizeof(info);
auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpByteAddressBuffer.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("バイトアドレスバッファの作成に失敗");
}
//シェーダリソースビューの作成
//Formatは必ず、DXGI_FORMAT_R32_TYPELESSにする必要がある
D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
srvDesc.BufferEx.FirstElement = 0;
srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
srvDesc.BufferEx.NumElements = sizeof(SphereInfo) / 4;
hr = this->mpDevice->CreateShaderResourceView(this->mpByteAddressBuffer.Get(), &srvDesc, this->mpByteAddressBufferSRV.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("バイトアドレスバッファのShaderResourceViewの作成に失敗");
}
{% endhighlight %}

<span class="important">定数バッファとの違いは、MiscFlagsにD3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWSを設定する必要があるだけです。</span>

<span class="important">また、シェーダリソースビューを生成する場合、<span class="keyward">Format</span>には<span class="keyward">DXGI_FORMAT_R32_TYPELESS</span>を設定する必要があります。</span>
<span class="important"><span class="keyward">ViewDimension</span>には<span class="keyward">D3D11_SRV_DIMENSION_BUFFEREX</span>を指定し、<span class="keyward">BufferEx.Flags</span>に<span class="keyward">D3D11_BUFFEREX_SRV_FLAG_RAW</span>を設定してください。</span>

<h1 class="under-bar">4.スタック操作を行うバッファ</h1>
<a name="STACK_BUFFER"></a>

DX11からシェーダ内でスタック操作を行うために<span class="keyward">AppendStructuredBuffer</span>と<span class="keyward">ConsumeStructuredBuffer</span>が追加されました。
ここではそれらについて見ていきます。
<span class="important">なお、スタックへのプッシュとポップは同時にはできませんので、別のシェーダをそれぞれ行ってください。</span>

<h4>プッシュ操作</h4>
{% highlight c++ %}
// PushStack.hlsl
AppendStructuredBuffer<float4> stack : register(u0);
#define COLOR_COUNT 10
static const float4 tblColor[COLOR_COUNT] = {
  float4(1, 0, 0, 1),
  float4(0, 1, 0, 1),
  float4(0, 0, 1, 1),
  float4(1, 1, 0, 1),
  float4(1, 0, 1, 1),
  float4(0, 1, 1, 1),
  float4(0, 0, 0, 1),
  float4(1, 1, 1, 1),
  float4(1, 0.5f, 0, 1),
  float4(1, 0, 0.5f, 1),
};
[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
  uint index = DTid.x % COLOR_COUNT;
  stack.Append(tblColor[index]);
}
{% endhighlight %}

<h4>ポップ操作</h4>
{% highlight c++ %}
// PopStack.hlsl
ConsumeStructuredBuffer<float4> stack : register(u0);
RWStructuredBuffer<float4> buffer : register(u1);
[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
  [branch] if (DTid.x < 10) {
    buffer[DTid.x] = stack.Consume();
  }
}
{% endhighlight %}

上2つのシェーダがやっていることはとても単純です。
GPU上で用意したデータをスタックに積んで、別のバッファに格納しているだけです。
ただGPU上では処理が並列に実行されているため、データの積まれる順番がどうなるかまでは制御できません。
なので、正しい順序が必要となる場合での使用は避けた方がいいでしょう。

スタック操作とは関係ありませんが、ポップ操作で使用してる<span class="keyward">RWStructuredBuffer</span>は書き込みができる<span class="keyward">StructuredBuffer</span>になります。

ドキュメント：<br>
[AppendStructuredBuffer(日本語)][APPEND_JP]
[AppendStructuredBuffer(英語)][APPEND_EN]<br>
[ConsumeStructuredBuffer(日本語)][CONSUME_JP]
[ConsumeStructuredBuffer(英語)][CONSUME_EN]<br>
[RWStructuredBuffer(日本語)][RW_STRUTURED_JP]
[RWStructuredBuffer(英語)][RW_STRUTURED_EN]

[APPEND_JP]:https://msdn.microsoft.com/ja-jp/library/ee422322(v=vs.85).aspx
[APPEND_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471448(v=vs.85).aspx
[CONSUME_JP]:https://msdn.microsoft.com/ja-jp/library/ee422335(v=vs.85).aspx
[CONSUME_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471459(v=vs.85).aspx
[RW_STRUTURED_JP]:https://msdn.microsoft.com/ja-jp/library/ee422364(v=vs.85).aspx
[RW_STRUTURED_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471494(v=vs.85).aspx

<h3>CPU側</h3>
{% highlight c++ %}
//Scene::runStackBuffer関数　一部
//AppendStructuredBufferとConsumeStructuredBufferはStructuredBufferと同じ設定でバッファを作成する
D3D11_BUFFER_DESC desc = {};
desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
desc.ByteWidth = sizeof(Data) * count;//スタックの上限を決めている
desc.StructureByteStride = sizeof(Data);
desc.Usage = D3D11_USAGE_DEFAULT;
auto hr = this->mpDevice->CreateBuffer(&desc, nullptr, pStackBuffer.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("スタック操作用のバッファ作成に失敗");
}
//ビューは
D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
uavDesc.Format = DXGI_FORMAT_UNKNOWN;
uavDesc.Buffer.FirstElement = 0;
uavDesc.Buffer.NumElements = count;
uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
hr = this->mpDevice->CreateUnorderedAccessView(pStackBuffer.Get(), &uavDesc, pStackBufferUAV.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("スタック操作用のバッファのUAVの作成に失敗");
}
{% endhighlight %}

<span class="keyward">AppendStructuredBuffer</span>と<span class="keyward">ConsumeStructuredBuffer</span>は<span class="keyward">StructuredBuffer</span>と同じ設定でバッファを作成します。
<span class="important">ビューはアンオーダードアクセスビューになり、<span class="keyward">Format</span>には<span class="keyward">DXGI_FORMAT_UNKNOWN</span>を指定し、</span>
<span class="important"><span class="keyward">Buffer.Flags</span>に<span class="keyward">D3D11_BUFFER_UAV_FLAG_APPEND</span>を指定してください。</span>
後はビューは異なりますが<span class="keyward">StructuredBuffer</span>と同じ要領で設定します。

<span class="important">注意点として、シェーダ内でのスタック操作は可能ですが動的にメモリを確保するわけではありません。</span>
<span class="important">作成したときに確保したメモリ量が上限となり、それを超えてプッシュしても追加されませんので注意してください。</span>

スタック操作については以上になります。
サンプルコードのScene::runStackBuffer関数ではGPUからCPUへのデータの転送を行うコードもあるので一度目を通してください。

<h1 class="under-bar">まとめ</h1>
<a name="SUMMARY"></a>

今回はバッファのバリエーションについて見てきました。
ここで上げたもの以外にもまだありますが、似たような内容になるので省略します。

次回からは本題ともいえるグラフィックスパイプラインについて見ていきます。
グラフィックスパイプラインを使うとモデルなど三角形で表現されたものを自由に画面に描画できるようになりますが、
いろいろな決まりごとや裏で行っていることがあるので少しずつ説明していきたいと思います。

<h1 class="under-bar">補足</h1>
<a name="SUPPLEMENTAL"></a>

<div class="supplemental">
  <h4>レイトレースとラスタライズ法</h4>
  <p>
    今回、球体を描画するのに各画面のピクセルからカメラの方向に合うようなレイを飛ばして球体と当たっていたら球体を描画するといった手順を踏んでいます。
    <span class="important">このレイを飛ばして画面を描画する手法はレイトレースと呼ばれています。</span>
    レイトレースは映画やCGの研究などで使わており非常にリアルな絵を描画することができる手法です。
    ただ、それ相応に重たい処理でもあるので、ゲームなどリアルタイムに描画する必要がある場合はラスタライズ法と呼ばれる手法が使われています。
    ラスタライズ法では三角形を使って物体を表現しており、複雑な物体を描画しようとするとそれだけ大量の三角形を描画する必要が出てきます。
    GPUはこのラスタライズ法を高速に処理するためつまり、大量の三角形を高速に処理するために作られたハードウェアといえます。
    <span class="important">これまで言葉だけが何回か出てきたグラフィックスパイプラインはこのラスタライズ法を行うための工程みたいなもので、ここまでの内容を踏まえますと、三角形を画面に描画するための工程だと言い換えることが出来るでしょう。</span>
  </p>
  <p>
    あと、今回のものはかなり粗末なものですが一応レイトレースと呼んでも差し支えないものになっていると思います。
    レイトレースについては詳しくは知らないのですが、レイマーチングと呼ばれる手法を使うと手軽にレイトレースできるそうなので興味がある人は調べてみて下さい。
    <br>
    <a href="http://qiita.com/gam0022/items/03699a07e4a4b5f2d41f">これがGPUの力！Three.jsによる“リアルタイム”なレイトレーシング</a><br>
    また、レイマーチングを使ったデモシーンが投稿されているサイトもあるようです。<br>
    <a href="http://qiita.com/gam0022/items/03699a07e4a4b5f2d41f">shadertoy</a><br>
    このサイトではGLSLと呼ばれるシェーダ言語を使ったデモシーンを投稿できるサイトになります。
    GLSLは文法自体はHLSLと似ているのでそこまで問題にはならないでしょう。<br>
    またレイマーチングをする際<a href="http://iquilezles.org/index.html">よく参考にされているサイト</a>もあるので一度目を通してみてください。
  </p>
</div>

<table class="table table-condensed">
  <tbody>
    <tr>
      <td class="left"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/texture">＜前</a></td>
      <td class="center"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}">トップ</a></td>
      <td class="right"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/graphics-pipeline">次＞</a></td>
    </tr>
  </tbody>
</table>
