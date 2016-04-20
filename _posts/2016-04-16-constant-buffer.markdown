---
layout: default
title: "定数バッファ"
categories: part
description: "まず初めに画面全体をGPUを使って単色で塗りつぶす単純なプログラムを作ることを題材に、DX11を使う上で最も重要なシェーダとその使い方について説明していきます。"
---
<h3 class="under-bar">前書き</h3>

前のパートではシェーダを使った画面クリアを行いました。
が、あのままだとクリアする色を変えたくなったとき、その色に合わせて同じようなシェーダを作る必要が出てきます。
{% highlight hlsl %}
//色ごとに画面をクリアするシェーダの例
RWTexture2D<float4> screen :register(u0);
[numthreads(1,1,1)]
void clearRed(uint2 DTid : SV_DispatchThreadID) {
  screen[DTid] = float4(1, 0, 0, 1);//赤色でクリアする
}
[numthreads(1,1,1)]
void clearGreen(uint2 DTid : SV_DispatchThreadID) {
  screen[DTid] = float4(0, 1, 0, 1);//緑色でクリアする
}
  ... 以下、必要な色分エントリポイントを用意する
{% endhighlight %}

誰の目が見ても上のやり方は非効率的でしょう。
C++の関数の引数のようにシェーダのエントリポイントに任意の値を渡すようにしたいですが、シェーダではできません。

{% highlight hlsl %}
//c++の引数みたいにエントリポイントに値を渡したいが、シェーダではこの書き方はできない
RWTexture2D<float4> screen :register(u0);
[numthreads(1,1,1)]
void clearRed(uint2 DTid : SV_DispatchThreadID, float4 clearColor/*<- コンパイルエラー*/) {
  screen[DTid] = clearColor;//任意の色でクリアする
}
{% endhighlight %}

ですが、代わりとなる方法がシェーダでは用意されています。
このパートではそれについて説明していきます。

<h1 class="under-bar">定数バッファ</h1>
DX11ではシェーダ実行時に自由に使うことができる値として、定数バッファ(英訳:ConstantBuffer)というものが用意されています。
<br>今パートに対応しているサンプルプロジェクトはPart02_ConstantBufferになります。

<h4>概要</h4>
<div class="overview">
  <ol>
    <li>
      <a href="#USE_IN_SHADER">シェーダ内での使い方</a>
      <ul><li>cbufferキーワード</li></ul>
    </li>
    <li>
      <a href="#DX11_Buffer">ID3D11Buffer</a>
      <ul>
        <li>
          設定と作成<br>
          ID3D11DeviceContext::CSSetConstantBuffers関数
          ID3D11Device::CreateBuffer関数
        </li>
        <li>
          更新処理<br>
          ID3D11DeviceContext::UpdateSubresource関数<br>
          ID3D11DeviceContext::Map関数
        </li>
      </ul>
    </li>
    <li><a href="#Reference">参考サイト</a></li>
  </ol>
</div>

<h1 class="under-bar">1.シェーダ内での使い方</h1>
<a name="USE_IN_SHADER"></a>
{% highlight hlsl %}
//定数バッファの定義
cbuffer Param : register(b0) {
  float4 clearColor;
  uint2 screenSize;
};

RWTexture2D<float4> screen : register(u0);
[numthreads(1, 1, 1)]
void main( uint2 DTid : SV_DispatchThreadID ) {
  [branch]
  if(DTid.x < screenSize.x && DTid.y < screenSize.y) {
    screen[DTid] = clearColor;//定数バッファの変数はグローバル変数のように使える
  }
}
{% endhighlight %}

定数バッファを使うときはcbufferキーワード使い、構造体のように宣言します。
あと、アンオーダードアクセスビューと同じでスロット番号も指定可能です。
定数バッファの場合は"b0"のように接頭語にbをつけて指定してください。
詳細は参考サイトのMSDNをご覧ください

上のコードでは"Param"と名付けた定数バッファを宣言し、クリアしたい色(clearColor)と画面サイズ(screenSize)の2つの変数を中で宣言しています。

定数バッファとして宣言された変数はシェーダ内のどこからでも使うことが出来ます。
いわば、c++のグローバル変数のようなものです。
グローバル変数との違いは代入ができないというだけなので、const宣言されたグローバル変数だという認識でいいでしょう。

使い方は以上です。
次はCPU側の説明になります。

<h1 class="under-bar">2.ID3D11Buffer</h1>
<a name="DX11_Buffer"></a>

<h3>設定と作成</h3>
まず、定数バッファをGPUへ設定する方法について見ていきます。

{% highlight c++ %}
std::array<ID3D11Buffer*, 1> ppCBs = { {
  this->mpCB.Get(),
} };
this->mpImmediateContext->CSSetConstantBuffers(0, static_cast<UINT>(ppCBs.size()), ppCBs.data());
{% endhighlight %}

CPU側での定数バッファはID3D11Bufferとして扱います。
ID3D11BufferとはGPUのメモリを表すものになります。
CPUからGPU上のメモリにアクセスするにはこれかまたはテクスチャ(英訳:Texture)を介して行います。

ID3D11DeviceContext::CSSetConstantBuffers関数で定数バッファの設定を行っており、
引数の数が異なるだけで、各引数は前パートで使ったID3D11DeviceContext::CSSetUnorderedAccessViews関数と同じ意味合いになります。

設定した後は、前パートと同じくID3D11DeviceContext::Dispatch関数でシェーダを実行すれば、シェーダ内で定数バッファが使われます。

それでは次に、ID3D11Bufferの作成について見ていきましょう。
ID3D11Bufferを作成することはGPUメモリを確保することであり、C++でいうnew演算子と同じ意味合いになります。

{% highlight c++ %}
Param param;	//ID3D11Bufferに設定するデータ
param.clearColor = DirectX::SimpleMath::Vector4(1, 0.7f, 0.7f, 1.f);
param.screenSize = DirectX::SimpleMath::Vector2(static_cast<float>(this->width()), static_cast<float>(this->height()));

//作成するID3D11Bufferの情報
D3D11_BUFFER_DESC desc = {};
desc.ByteWidth = sizeof(param) + (sizeof(param) % 16 == 0 ? 0 : 16 - sizeof(param) % 16);//サイズは16の倍数でないといけない
desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;//ID3D11Bufferを定数バッファとして使うよう宣言している
desc.Usage = D3D11_USAGE_DEFAULT;//GPU上からしかID3D11Bufferの内容にアクセスできないよう宣言している
desc.CPUAccessFlags = 0;//CPUからのアクセスフラグの指定。今回はアクセスしないので何も設定しない

//CPUとGPU間のデータ転送の時に使う構造体
//ここではID3D11Bufferの初期データを設定するために使っている
D3D11_SUBRESOURCE_DATA initData = {};
initData.pSysMem = &param;
initData.SysMemPitch = sizeof(param);//設定するデータのサイズ
auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpCB.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("定数バッファの作成に失敗");
}
{% endhighlight %}

上のコードのthis->mpDevice->CreateBuffer関数で定数バッファを作成しています。<br>
ドキュメント: [ID3D11Device::CreateBuffer(日本語)][MSDN_CREATE_BUFFER_JP] [ID3D11Device::CreateBuffer(英語)][MSDN_CREATE_BUFFER_EN]

引数は前から順に
<ol>
  <li>作成するID3D11Bufferの情報</li>
  <li>初期データ</li>
  <li>作成したID3D11Bufferを受け取る変数</li>
</ol>
となります。

この中で重要となるのは第1引数のD3D11_BUFFER_DESC構造体になります。
メンバ変数の数が多いですが、定数バッファを作成する上で必要となるものは上のコードで使っているものです。
D3D11_BUFFER_DESCのドキュメントは以下になります。<br>
ドキュメント: [D3D11_BUFFER_DESC(日本語)][MSDN_BUFFER_DESC_JP] [D3D11_BUFFER_DESC(英語)][MSDN_BUFFER_DESC_EN]

ByteWidthはID3D11Bufferが確保するGPU上のメモリサイズになります。
単位はbyteになり、定数バッファとして扱う場合は必ず、値を16の倍数でなければなりません。
16の倍数でない場合は作成に失敗しますので注意してください。

BindFlagはGPU上でどのようにID3D11Bufferを使うか指定するものです。
今回は定数バッファとして使うので、D3D11_BIND_CONSTANT_BUFFERを指定しています。<br>
ドキュメント: [D3D11_BIND_FLAG(日本語)][MSDN_BIND_FLAG_JP] [D3D11_BIND_FLAG(英語)][MSDN_BIND_FLAG_EN]

Usageはどのようなメモリの読み書きを行うかを指定します。
今回は定数バッファはGPU上でしか読み書きしないので、D3D11_USAGE_DEFAULTを指定しています。
メモリ読み書きの種類は以下のリンクを参照してください<br>
ドキュメント: [D3D11_USAGE(日本語)][MSDN_USAGE_JP] [D3D11_USAGE(英語)][MSDN_USAGE_EN]

CPUAccessFlagsはUsageでCPUからアクセスするときどのようなアクセスを行うかを指定します。
Usageに設定したものによって使えるフラグが変わりますので注意してください。<br>
ドキュメント: [D3D11_CPU_ACCESS_FLAG(日本語)][MSDN_CPU_ACCESS_JP] [D3D11_CPU_ACCESS_FLAG(英語)][MSDN_CPU_ACCESS_EN]

第2引数には初期データを表すD3D11_SUBRESOURCE_DATA渡します。
使い方はコードを見てもらえれば十分でしょう。
初期データのアドレスとそのデータ長を設定するだけです。
この構造体はテクスチャの作成時にも使用し、また後述するID3D11DeviceContext::Map関数でも使用します。<br>
ドキュメント: [D3D11_SUBRESOURCE_DATA(日本語)][MSDN_SUBRESOURCE_DATA_JP] [D3D11_SUBRESOURCE_DATA(英語)][MSDN_SUBRESOURCE_DATA_EN]

作成については以上です。
これで自由な値をシェーダ側で使えるようになりました。
が、このままでは作成時に設定した値しか使えません。
DX11ではID3D11Bufferの値を更新する方法をもちろん提供していますので、次はそれについて見ていきます。

<h3>更新処理</h3>
ID3D11Bufferの値を更新する方法は2通りあります。
<h4>ID3D11DeviceContext::UpdateSubresource関数</h4>
ドキュメント: [ID3D11DeviceContext::UpdateSubresource(日本語)][MSDN_UPDATE_SUBRES_JP] [ID3D11DeviceContext::UpdateSubresource(英語)][MSDN_UPDATE_SUBRES_EN]
<br>
※ID3D11DeviceContext::UpdateSubresourceの日本語訳の内容は翻訳ミスにより一部誤ったものになっていますので注意してください<br>
[参考サイト][ATTENSION_MISS]
{% highlight c++ %}
//Scene::onUpdate関数一部
//ID3D11DeviceCOntext::UpdateSubresource関数を使った定数バッファの更新
Param param;
param.clearColor = DirectX::SimpleMath::Vector4(abs(sin(t)), (cos(t)), (sin(t)), 1.f);
param.screenSize.x = static_cast<float>(this->width());
param.screenSize.y = static_cast<float>(this->height());
//定数バッファの内容更新
this->mpImmediateContext->UpdateSubresource(this->mpCB.Get(), 0, nullptr, &param, 0, 0);
{% endhighlight %}

ID3D11DeviceContext::UpdateSubresource関数はD3D11_BUFFER_DESCのUsageにD3D11_USAGE_DEFAULTかD3D11_USAGE_STAGINGを指定したID3D11Bufferの内容を変更することができます。
引数がいくつかありますが、定数バッファの内容を更新したいときは第1引数に変更したい定数バッファと第4引数に変更内容だけを指定してください。

細かな話になりますが、UpdateSubresource関数がID3D11Bufferの内容を更新する際、一度第4引数に渡したデータをコマンドバッファと呼ばれる一時的なストレージ空間へコピーしています。
シェーダの設定やDispatch関数などの実行命令、UpdateSubresource関数などID3D11DeviceContextの関数を使うと全てコマンドバッファに一度格納され、
コマンドバッファに積まれた内容は積まれた順にGPUの都合がいいときに実行されます。
言い換えると、関数を呼び出したからと言って直ちにGPU上で命令が処理されるわけではありません。
DX11を使うことはCPUとGPU間でのマルチスレッドプログラミングを暗黙の上で行っていることになります。
なので、UpdateSubresource関数を呼び出したら、すぐにID3D11Bufferの内容(GPU上のメモリ)が変更されているとは考えない方がいいでしょう。

またこういった事情でコピーが2回起きる重たい処理となりますので、パフォーマンスが必要な場合は注意してください。

<h4>ID3D11DeviceContext::Map関数</h4>
ドキュメント: [ID3D11DeviceContext::Map(日本語)][MSDN_MAP_JP] [ID3D11DeviceContext::Map(英語)][MSDN_MAP_EN]
{% highlight c++ %}
//Sccene::onUpdate関数の一部
//Map関数を使うときはD3D11_BUFFER_DESCのUsageにD3D11_USAGE_DYNAMICを
// CPUAccessFlagsにD3D11_CPU_ACCESS_WRITEを指定してください。
UINT subresourceIndex = 0;
D3D11_MAPPED_SUBRESOURCE mapped;
auto hr = this->mpImmediateContext->Map(this->mpCBMappable.Get(), subresourceIndex, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
if (SUCCEEDED(hr)) {
  Param* p = static_cast<Param*>(mapped.pData);
  p->clearColor = DirectX::SimpleMath::Vector4(abs(sin(t*2.f)), (cos(t)), (sin(t)), 1.f);
  p->screenSize.x = static_cast<float>(this->width());
  p->screenSize.y = static_cast<float>(this->height());
  this->mpImmediateContext->Unmap(this->mpCBMappable.Get(), subresourceIndex);
}
{% endhighlight %}
ID3D11DeviceContext::Map関数はD3D11_BUFFER_DESCのUsageにD3D11_USAGE_DYNAMICを、CPUAccessFlagsにD3D11_CPU_ACCESS_WRITEを指定したID3D11Bufferの内容を変更することができます。
データの読み書きはD3D11_MAPPED_SUBRESOURCEを介して行います。
Map関数を使った後は、必ずID3D11DeviceContext::Unmap関数を呼び出してください。
これはGPUで使用中の定数バッファを内容を書き換えてしまうことを防ぐために必要になります。

第1引数にマップするID3D11Bufferを、第5引数にD3D11_MAPPED_SUBRESOURCEを渡してください。

定数バッファの場合、第2引数には0を必ず指定してください。
また第3引数にはD3D11_MAP_WRITE_DISCARDを指定する必要があります。
D3D11_MAP_WRITE_DISCARDはマップしたものの以前の内容を無効にしてデータを書き込むことを表していますので、定数バッファのすべての内容を設定する必要があります。

第4引数は第1引数に渡したID3D11BufferがGPUで使用中だった場合のCPU側の対応を指定します。
D3D11_MAP_FLAG_DO_NOT_WAITを渡すと使用中だった場合は戻り値にDXGI_ERROR_WAS_STILL_DRAWINGを返すようになります。
ですが、このフラグは第3引数にMAP_READかMAP_WRITE、MAP_READ_WRITEを指定したときにしか使うことが出来ません。

マップに成功したら、第5引数に渡したD3D11_MAPPED_SUBRESOURCEを使ってデータを書き込みます。
D3D11_MAPPED_SUBRESOURCEにはマップしたデータの先頭ポインタとデータの長さ、アライメント情報があります。
<br>ドキュメント: [D3D11_MAPPED_SUBRESOURCE(日本語)][MSDN_MAPPED_JP] [D3D11_MAPPED_SUBRESOURCE(英語)][MSDN_MAPPED_EN]

<h1 class="under-bar">まとめ</h1>
この記事ではシェーダから自由に使うことが出来る定数バッファについて見ていきました。
読み込みしかできませんが、これで汎用的にシェーダを作成することが出来ます。

ID3D11Bufferに関しては定数バッファ以外の目的でも使います。
DX11ではGPUメモリをID3D11Bufferと次のパートで説明するテクスチャを使って表現しています。
この2つは共通している部分が多く、今回見た更新処理はテクスチャでも同じく使うことが出来ます。

また、更新処理はCPUとGPUで異なるメモリを使っていることから、メモリ転送によるボトルネックが起きやすい処理になっています。
パフォーマンスの観点から見ればGPUとCPUでメモリを共有している特殊な環境でない限り、必要最低限しかCPU/GPU間のメモリのやり取りが起きないようする必要がありますので注意してください。

<h1 class="under-bar">3.参考サイト</h1>
<a name="Reference"></a>

[シェーダー定数(DirectX HLSL)(日本語)][MSDN_CB_JP]<br>
[シェーダー定数(DirectX HLSL)(英語)][MSDN_CB_EN]

<h1 class="under-bar">補足</h1>
<h4 class="under-bar">定数バッファのパッキング</h4>
シェーダ内で定数バッファを宣言するとき、変数の並びによってサイズが変わったりします。
{% highlight hlsl %}
//この並びだとサイズが60byteになるので、ID3D11Bufferのサイズは64byte必要になる
//c++側のデータの並びも16byteアライメントになるようすること
cbuffer Param1 {
  float2 v1;
  float4 v2;
  float2 v3;
  float  v4;
  float3 v5;
}
//こちらは48byteになり、ID3D11Bufferのサイズは48byte必要になる
cbuffer Param2 {
  float4 v2;
  float2 v1;
  float2 v3;
  float3 v5;
  float  v4;
}
{% endhighlight %}

定数バッファではアライメントが16byte単位になっており、上のParam1のv1のように
float2の後にfloat4 v2;と宣言すると(sizeof(float2)+sizeof(float4))=24byteとなり、v2が16byteの境界をまたいでしまいます。
この場合はv2は自動的に先頭アドレスが16byteの倍数になるよう配置され、v1の後ろにできる8byteは未使用領域となります。
これはC++での構造体などで起きることと同じことです。

Param2のようにv2やv4の後ろにfloat2やfloat等16byteの境界をまたがないように宣言すれば
未使用領域がない効率的なメモリ配置にすることができます。

以上から定数バッファを宣言するときはParam2のように変数の並びに注意する必要がありますが、
パッキング指定をすることでParam1の並びでもParam2と同じメモリ配置にすることが出来ます。
{% highlight hlsl %}
//パッキング指定をしたParam1
//これでParam2と同じメモリ配置になる
cbuffer Param1 {
  float2 v1 : packoffset(c0.x);
  float4 v2 : packoffset(c1.x);
  float2 v3 : packoffset(c0.z);
  float  v4 : packoffset(c2.w);
  float3 v5 : packoffset(c2.x);
}
{% endhighlight %}
パッキング指定した場合はすべての変数にパッキングを設定しなければいけません。
パッキングの詳細については3．参考サイトのMSDNドキュメントを参照してください。

アライメントについては[こちら][ALIGNMENT]を参考にしてください。

<h4 class="under-bar">GPUメモリのコピー</h4>
ID3D11DeviceContextにはGPU内でのメモリコピーを行うための関数が用意されています。

ID3D11DeviceContext::CopyResource関数 [MSDN(日本語)][MSDN_COPY_RESOURCE_JP] [MSDN(英語)][MSDN_COPY_RESOURCE_EN]<br>
ID3D11DeviceContext::CopySubresourceRegion関数　[MSDN(日本語)][MSDN_COPY_SUBRESOURCE_JP] [MSDN(英語)][MSDN_COPY_SUBRESOURCE_EN]

サンプルではScene::onRender関数の最後でシェーダの実行結果をバックバッファへコピーするために使用しています。
バックバッファについては別パートで説明しますが、DX11での最終出力先でバックバッファの内容が画面に表示されます。

[MSDN_CREATE_BUFFER_JP]: https://msdn.microsoft.com/ja-jp/library/ee419781(v=vs.85).aspx
[MSDN_CREATE_BUFFER_EN]: https://msdn.microsoft.com/en-us/library/ff476501(v=vs.85).aspx
[MSDN_BUFFER_DESC_JP]: https://msdn.microsoft.com/ja-jp/library/ee416048(v=vs.85).aspx
[MSDN_BUFFER_DESC_EN]: https://msdn.microsoft.com/en-us/library/ff476092(v=vs.85).aspx
[MSDN_BIND_FLAG_JP]: https://msdn.microsoft.com/ja-jp/library/ee416041(v=vs.85).aspx
[MSDN_BIND_FLAG_EN]: https://msdn.microsoft.com/en-us/library/ff476085(v=vs.85).aspx
[MSDN_USAGE_JP]: https://msdn.microsoft.com/ja-jp/library/ee416352(v=vs.85).aspx
[MSDN_USAGE_EN]: https://msdn.microsoft.com/en-us/library/ff476259(v=vs.85).aspx
[MSDN_CPU_ACCESS_JP]: https://msdn.microsoft.com/ja-jp/library/ee416074(v=vs.85).aspx
[MSDN_CPU_ACCESS_EN]: https://msdn.microsoft.com/en-us/library/ff476106(v=vs.85).aspx
[MSDN_SUBRESOURCE_DATA_JP]: https://msdn.microsoft.com/ja-jp/library/ee416284(v=vs.85).aspx
[MSDN_SUBRESOURCE_DATA_EN]: https://msdn.microsoft.com/en-us/library/ff476220(v=vs.85).aspx
[MSDN_UPDATE_SUBRES_JP]: https://msdn.microsoft.com/ja-jp/library/ee419755(v=vs.85).aspx
[MSDN_UPDATE_SUBRES_EN]: https://msdn.microsoft.com/en-us/library/ff476486(v=vs.85).aspx
[ATTENSION_MISS]: http://sygh.hatenadiary.jp/entry/2014/07/26/234223
[MSDN_MAPPED_JP]: https://msdn.microsoft.com/ja-jp/library/ee416246(v=vs.85).aspx
[MSDN_MAPPED_EN]: https://msdn.microsoft.com/en-us/library/ff476182(v=vs.85).aspx
[MSDN_MAP_JP]: https://msdn.microsoft.com/ja-jp/library/ee419694(v=vs.85).aspx
[MSDN_MAP_EN]: https://msdn.microsoft.com/en-us/library/ff476457(v=vs.85).aspx
[MSDN_CB_JP]: https://msdn.microsoft.com/ja-jp/library/ee418283(v=vs.85).aspx
[MSDN_CB_EN]: https://msdn.microsoft.com/en-us/library/bb509581(v=vs.85).aspx
[ALIGNMENT]: http://www5d.biglobe.ne.jp/~noocyte/Programming/Alignment.html
[MSDN_COPY_RESOURCE_JP]: https://msdn.microsoft.com/ja-jp/library/ee419574(v=vs.85).aspx
[MSDN_COPY_RESOURCE_EN]: https://msdn.microsoft.com/en-us/library/ff476392(v=vs.85).aspx
[MSDN_COPY_SUBRESOURCE_JP]: https://msdn.microsoft.com/ja-jp/library/ee419576(v=vs.85).aspx
[MSDN_COPY_SUBRESOURCE_EN]: https://msdn.microsoft.com/en-us/library/ff476394(v=vs.85).aspx
