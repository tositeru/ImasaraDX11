---
layout: default
title: "定数バッファ"
categories: part
description: "DX11ではシェーダ実行時に自由に使うことができる値として、定数バッファ(英訳:ConstantBuffer)というものが用意されています。"
---
<h1 class="under-bar">定数バッファ</h1>

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

<h1>概要</h1>
DX11ではシェーダ実行時に自由に使うことができる値として、<span class="important">定数バッファ(英訳:ConstantBuffer)</span>というものが用意されています。
今パートに対応しているサンプルプロジェクトは<span class="important">Part02_ConstantBuffer</span>になります。

<div class="summary">
  <ol>
    <li>
      <a href="#USE_IN_SHADER">シェーダ内での使い方</a>
      <ul><li><span class="keyward">cbuffer</span>キーワード</li></ul>
    </li>
    <li>
      <a href="#DX11_Buffer">ID3D11Buffer</a>
      <ul>
        <li>
          設定と作成<br>
          <span class="keyward">ID3D11DeviceContext::CSSetConstantBuffers関数</span><br>
          <span class="keyward">ID3D11Device::CreateBuffer関数</span>
        </li>
        <li>
          更新処理<br>
          <span class="keyward">ID3D11DeviceContext::UpdateSubresource関数</span><br>
          <span class="keyward">ID3D11DeviceContext::Map関数</span>
        </li>
      </ul>
    </li>
    <li>まとめ</li>
    <li>補足
      <ul>
        <li>定数バッファのパッキング</li>
        <li>GPUメモリのコピー</li>
      </ul>
    </li>
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

定数バッファを使うときは<span class="keyward">cbuffer</span>キーワード使い、構造体のように宣言します。
あと、アンオーダードアクセスビュー(以下、UAV)と同じでスロット番号も指定可能です。
定数バッファの場合は"b0"のように接頭語にbをつけて指定してください。
詳細は参考サイトのMSDNをご覧ください

ドキュメント:
[シェーダー定数(DirectX HLSL)(日本語)][MSDN_CB_JP]
[Shader Constants(英語)][MSDN_CB_EN]

[MSDN_CB_JP]: https://msdn.microsoft.com/ja-jp/library/ee418283(v=vs.85).aspx
[MSDN_CB_EN]: https://msdn.microsoft.com/en-us/library/windows/desktop/bb509581(v=vs.85).aspx

上のコードでは"Param"と名付けた定数バッファを宣言し、クリアしたい色(clearColor)と画面サイズ(screenSize)の2つの変数を中で宣言しています。

定数バッファとして宣言された変数は<span class="important">シェーダ内のどこからでも使うことが出来ます。</span>
いわば、c++のグローバル変数のようなものです。
グローバル変数との違いは代入ができないというだけなので、<span class="important">const宣言されたグローバル変数</span>だという認識でいいでしょう。

使い方は以上です。
次はCPU側の説明になります。

<h1 class="under-bar">2.ID3D11Buffer</h1>
<a name="DX11_Buffer"></a>

<h3>設定</h3>
まず、定数バッファをGPUへ設定する方法について見ていきます。

{% highlight c++ %}
std::array<ID3D11Buffer*, 1> ppCBs = { {
  this->mpCB.Get(),
} };
this->mpImmediateContext->CSSetConstantBuffers(0, static_cast<UINT>(ppCBs.size()), ppCBs.data());
{% endhighlight %}

CPU側での定数バッファは<span class="keyward">ID3D11Buffer</span>として扱います。
<span class="keyward">ID3D11Buffer</span>とはGPUのメモリを表すものになります。
CPUからGPU上のメモリにアクセスするにはこれかまたは<span class="keyward">テクスチャ(英訳:Texture)</span>を介して行います。

<span class="keyward">ID3D11DeviceContext::CSSetConstantBuffers関数</span>で定数バッファの設定を行っています。
引数の数が異なるだけで、各引数は前パートで使った<span class="keyward">ID3D11DeviceContext::CSSetUnorderedAccessViews関数</span>と同じ意味合いになります。

設定した後は、前パートと同じく<span class="keyward">ID3D11DeviceContext::Dispatch関数</span>でシェーダを実行すれば、シェーダ内で定数バッファが使われます。

<h3>作成</h3>
それでは次に、<span class="keyward">ID3D11Buffer</span>の作成について見ていきましょう。
<span class="keyward">ID3D11Buffer</span>を作成することはGPUメモリを確保することであり、C++でいうnew演算子と同じ意味合いになります。

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

上のコードの<span class="keyward">this->mpDevice->CreateBuffer</span>で定数バッファを作成しています。

<span class="keyward">ID3D11Device::CreateBuffer関数</span>の引数は次のものになります。
<br>ドキュメント: [ID3D11Device::CreateBuffer(日本語)][MSDN_CREATE_BUFFER_JP] [ID3D11Device::CreateBuffer(英語)][MSDN_CREATE_BUFFER_EN]

[MSDN_CREATE_BUFFER_JP]: https://msdn.microsoft.com/ja-jp/library/ee419781(v=vs.85).aspx
[MSDN_CREATE_BUFFER_EN]: https://msdn.microsoft.com/en-us/library/ff476501(v=vs.85).aspx

<ol>
  <li>第1引数：<span class="keyward">D3D11_BUFFER_DESC</span>
    <p>
      作成する<span class="keyward">ID3D11Buffer</span>の情報を表す<span class="keyward">D3D11_BUFFER_DESC</span>を渡します。
      メンバ変数の数が多いですが、定数バッファを作成する上で必要となるものは上のコードで使っているものです。
      <br>ドキュメント:
      <a href="https://msdn.microsoft.com/ja-jp/library/ee416048(v=vs.85).aspx">D3D11_BUFFER_DESC(日本語)</a>
      <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476092(v=vs.85).aspx">D3D11_BUFFER_DESC(英語)</a>      
      <ul>
        <li><span class="keyward">ByteWidth</span>
          <p>
            <span class="keyward">ByteWidth</span>は<span class="keyward">ID3D11Buffer</span>が確保するGPU上のメモリサイズになります。
            <span class="important">単位はbyteになり、定数バッファとして扱う場合は必ず、値を16の倍数</span>でなければなりません。
            16の倍数でない場合は作成に失敗しますので注意してください。
          </p>
        </li>
        <li><span class="keyward">BindFlag</span>
          <p>
            <span class="keyward">BindFlag</span>はGPU上で<span class="important">どういった用途でID3D11Bufferを使うか指定する</span>ものです。
            <span class="keyward">ID3D11Buffer</span>は定数バッファ以外の目的でも使いますが、それについてはその都度説明していきます。
            今回は定数バッファとして使うので、<span class="keyward">D3D11_BIND_CONSTANT_BUFFER</span>を指定しています。<br>
            ドキュメント:
            <a href="https://msdn.microsoft.com/ja-jp/library/ee416041(v=vs.85).aspx">D3D11_BIND_FLAG(日本語)</a>
            <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476085(v=vs.85).aspx">D3D11_BIND_FLAG(英語)</a>
          </p>
        </li>
        <li><span class="keyward">Usage</span>
          <p>
            <span class="keyward">Usage</span>は<span class="important">どのようにメモリの読み書きを行うかを指定します。</span>
            今回は定数バッファはGPU上でしか読み書きしないので、<span class="keyward">D3D11_USAGE_DEFAULT</span>を指定しています。
            メモリ読み書きの種類は以下のリンクを参照してください<br>
            ドキュメント:
            <a href="https://msdn.microsoft.com/ja-jp/library/ee416352(v=vs.85).aspx">D3D11_USAGE(日本語)</a>
            <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476259(v=vs.85).aspx">D3D11_USAGE(英語)</a>
          </p>
        </li>
        <li><span class="keyward">CPUAccessFlags</span>
          <p>
            <span class="keyward">CPUAccessFlags</span>は<span class="keyward">CPUからアクセスするときどのようなアクセスを行うかを指定します。</span>
            <span class="keyward">Usage</span>に設定したものによって<span class="important">使えるフラグが変わりますので注意してください。</span><br>
            ドキュメント:
            <a href="https://msdn.microsoft.com/ja-jp/library/ee416074(v=vs.85).aspx">D3D11_CPU_ACCESS_FLAG(日本語)</a>
            <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476106(v=vs.85).aspx">D3D11_CPU_ACCESS_FLAG(英語)</a>
          </p>
        </li>
      </ul>
    </p>
  </li>
  <li>第2引数：<span class="keyward">D3D11_SUBRESOURCE_DATA</span>
    <p>
      <span class="keyward">D3D11_SUBRESOURCE_DATA</span>は初期データを表します。
      使い方はコードを見てもらえれば十分でしょう。
      初期データとして渡すCPU上のデータのアドレスとそのデータ長を設定するだけです。
      この構造体は<span class="keyward">テクスチャ</span>の作成時にも使用します。
      また後述する<span class="keyward">ID3D11DeviceContext::Map関数</span>でもこれと似た内容の<span class="keyward">D3D11_MAPPED_SUBRESOURCE</span>を使います。
      <br>ドキュメント:
      <a href="https://msdn.microsoft.com/ja-jp/library/ee416284(v=vs.85).aspx">D3D11_SUBRESOURCE_DATA(日本語)</a>
      <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476220(v=vs.85).aspx">D3D11_SUBRESOURCE_DATA(英語)</a>
    </p>
  </li>
  <li>第3引数：<span class="keyward">ID3D11Buffer**</span>
    <p>
      作成した<span class="keyward">ID3D11Buffer</span>を受け取る変数を渡します。
    </p>
  </li>
</ol>

作成については以上です。
これで自由な値をシェーダ側で使えるようになりました。
が、このままでは作成時に設定した値しか使えません。
DX11では<span class="keyward">ID3D11Buffer</span>の値を更新する方法をもちろん提供していますので、次はそれについて見ていきます。

<h3>更新処理</h3>
<span class="keyward">ID3D11Buffer</span>の値を更新する方法は2通りあります。
<h4>ID3D11DeviceContext::UpdateSubresource関数</h4>
ドキュメント: [ID3D11DeviceContext::UpdateSubresource(日本語)][MSDN_UPDATE_SUBRES_JP] [ID3D11DeviceContext::UpdateSubresource(英語)][MSDN_UPDATE_SUBRES_EN]
<br>
※<span class="keyward">ID3D11DeviceContext::UpdateSubresource</span>の日本語訳の内容は翻訳ミスにより一部誤ったものになっていますので注意してください<br>
[参考サイト][ATTENSION_MISS]

[MSDN_UPDATE_SUBRES_JP]: https://msdn.microsoft.com/ja-jp/library/ee419755(v=vs.85).aspx
[MSDN_UPDATE_SUBRES_EN]: https://msdn.microsoft.com/en-us/library/ff476486(v=vs.85).aspx
[ATTENSION_MISS]: http://sygh.hatenadiary.jp/entry/2014/07/26/234223

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

<span class="keyward">ID3D11DeviceContext::UpdateSubresource関数</span>は<span class="keyward">D3D11_BUFFER_DESC::Usage</span>に<span class="keyward">D3D11_USAGE_DEFAULT</span>か<span class="keyward">D3D11_USAGE_STAGING</span>を指定した<span class="keyward">ID3D11Buffer</span>の内容を変更することができます。
引数がいくつかありますが、定数バッファの内容を更新したいときは第1引数に変更したい定数バッファと第4引数に変更内容だけを指定してください。

<div class="topic">
  <h4>データが更新されるタイミング</h4>
  <p>
    細かな話になりますが、<span class="keyward">ID3D11DeviceContext::UpdateSubresource関数</span>を使った場合、データが更新されるタイミングは決まっていません。
    DX11ではシェーダの設定やDispatch関数などの実行命令、<span class="keyward">ID3D11DeviceContext::UpdateSubresource関数</span>など<span class="keyward">ID3D11DeviceContext</span>の関数を使うと全て
    <span class="important">コマンドバッファと呼ばれる一時的なストレージ空間</span>へ一度格納され、
    コマンドバッファに積まれた内容は積まれた順にGPUの都合がいいときに実行されます。
    言い換えると、関数を呼び出したからと言って<span class="important">直ちにGPU上で命令が処理されるわけではありません。</span>
    DX11を使うことは<span class="important">CPUとGPU間でのマルチスレッドプログラミングを暗黙の上で行っている</span>ことになります。
    なので、<span class="keyward">ID3D11DeviceContext::UpdateSubresource関数</span>を呼び出したら、
    直ちに<span class="keyward">ID3D11Buffer</span>の内容(GPU上のメモリ)が変更されているとは考えない方がいいでしょう。
  </p>
  <p>
    また非同期的にデータの更新を行っているため、<span class="keyward">ID3D11DeviceContext::UpdateSubresource関数</span>に渡したソースデータは一度、
    <span class="important">コマンドバッファへコピーされます。</span>
    <span class="keyward">ID3D11DeviceContext::UpdateSubresource関数</span>は<span class="important">コピーが2回起きる重たい処理</span>となりますので、パフォーマンスが必要な場合は注意してください。
  </p>
</div>

<h4>ID3D11DeviceContext::Map関数</h4>
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
<span class="keyward">ID3D11DeviceContext::Map関数</span>は<span class="keyward">D3D11_BUFFER_DESC::Usage</span>に
<span class="keyward">D3D11_USAGE_DYNAMIC</span>を、<span class="keyward">D3D11_BUFFER_DESC::CPUAccessFlags</span>に
<span class="keyward">D3D11_CPU_ACCESS_WRITE</span>を指定した<span class="keyward">ID3D11Buffer</span>の内容を変更することができます。
データの読み書きは<span class="keyward">D3D11_MAPPED_SUBRESOURCE</span>を介して行います。
<span class="important">Map関数を使った後は、必ずID3D11DeviceContext::Unmap関数を呼び出してください。</span>
これはGPUで使用中の定数バッファを内容を書き換えてしまうことを防ぐために必要になります。

ドキュメント: [ID3D11DeviceContext::Map(日本語)][MSDN_MAP_JP] [ID3D11DeviceContext::Map(英語)][MSDN_MAP_EN]

[MSDN_MAP_JP]: https://msdn.microsoft.com/ja-jp/library/ee419694(v=vs.85).aspx
[MSDN_MAP_EN]: https://msdn.microsoft.com/en-us/library/ff476457(v=vs.85).aspx

<ul>
  <li>第1引数：マップする<span class="keyward">ID3D11Buffer</span></li>
  <li>第2引数：サブリソースのインデックス
    <p>定数バッファの場合は必ず0を指定してください。</p>
  </li>
  <li>第3引数：行うマップの種類
    <p>
      定数バッファの場合は<span class="keyward">D3D11_MAP_WRITE_DISCARD</span>を指定する必要があります。
      <span class="keyward">D3D11_MAP_WRITE_DISCARD</span>はマップ対象の以前の内容を無効にしてデータを書き込むことを表していますので、
      定数バッファのすべての内容を設定する必要があります。
    </p>
  </li>
  <li>第4引数：GPUで使用中だった場合のCPU側の対応
    <p>
      第1引数に渡したリソースがGPUで使用中だった場合のCPU側の対応を指定します。
      <span class="keyward">D3D11_MAP_FLAG_DO_NOT_WAIT</span>を渡すと使用中だった場合は戻り値に<span class="keyward">DXGI_ERROR_WAS_STILL_DRAWING</span>を返すようになります。
      <span class="important">このフラグは第3引数にMAP_READかMAP_WRITE、MAP_READ_WRITEを指定したときにしか使うことが出来ません。</span>
    </p>
  </li>
  <li>第5引数：<span class="keyward">D3D11_MAPPED_SUBRESOURCE</span>
    <p>
      マップに成功した場合、渡した<span class="keyward">D3D11_MAPPED_SUBRESOURCE</span>にマップしたデータの先頭ポインタとデータの長さ等の情報が入ります。
      <br>ドキュメント:
      <a href="https://msdn.microsoft.com/ja-jp/library/ee416246(v=vs.85).aspx">D3D11_MAPPED_SUBRESOURCE(日本語)</a>
      <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476182(v=vs.85).aspx">D3D11_MAPPED_SUBRESOURCE(英語)</a>
    </p>
  </li>
</ul>

<h1 class="under-bar">まとめ</h1>
<a name="SUMMARY"></a>
この記事ではシェーダから自由に使うことが出来る定数バッファについて見ていきました。
読み込みしかできませんが、これで汎用的にシェーダを作成することが出来ます。

<span class="keyward">ID3D11Buffer</span>に関しては定数バッファ以外の目的でも使います。
DX11ではGPUメモリを<span class="keyward">ID3D11Buffer</span>と次のパートで説明するテクスチャを使って表現しています。
この2つは共通している部分が多く、今回見た更新処理はテクスチャでも同じく使うことが出来ます。

また、更新処理はCPUとGPUで異なるメモリを使っていることから、メモリ転送によるボトルネックが起きやすい処理になっています。
パフォーマンスの観点から見ればGPUとCPUでメモリを共有している特殊な環境でない限り、必要最低限しかCPU/GPU間のメモリのやり取りが起きないようする必要がありますので注意してください。

<h1 class="under-bar">補足</h1>
<div class="supplemental">
  <h4>定数バッファのパッキング</h4>
  <span class="important">シェーダ内で定数バッファを宣言するとき、変数の並びによってサイズが変わったりします。</span>
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

  <p>
    定数バッファでは<span class="important">アライメントが16byte単位</span>になっており、上のParam1のv1のように
    float2の後にfloat4 v2;と宣言すると(sizeof(float2)+sizeof(float4))=24byteとなり、v2が16byteの境界をまたいでしまいます。
    この場合はv2は自動的に先頭アドレスが16byteの倍数になるよう配置され、v1の後ろにできる8byteは未使用領域となります。
    <span class="important">これはC++での構造体などで起きることと同じことです。</span>
  </p>

  <p>
    Param2のようにv2やv4の後ろにfloat2やfloat等16byteの境界をまたがないように宣言すれば
    未使用領域がない効率的なメモリ配置にすることができます。
  </p>

  <p>
    以上から定数バッファを宣言するときはParam2のように変数の並びに注意する必要がありますが、
    <span class="important">パッキング指定をすることでParam1の並びでもParam2と同じメモリ配置にすることが出来ます。</span>
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
    なお、パッキング指定した場合はすべての変数にパッキングを設定しなければいけません。
  </p>

  アライメントについては<a href="http://www5d.biglobe.ne.jp/~noocyte/Programming/Alignment.html">こちら</a>を参考にしてください。
</div>

<div class="supplemental">
  <h4>GPUメモリのコピー</h4>
  <p><span class="keyward">ID3D11DeviceContext</span>にはGPU内でのメモリコピーを行うための関数が用意されています。</p>
  <p>
    ドキュメント<br>
    <span class="keyward">ID3D11DeviceContext::CopyResource関数</span>
    <a href="https://msdn.microsoft.com/ja-jp/library/ee419574(v=vs.85).aspx">日本語</a>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476392(v=vs.85).aspx">英語</a>
    <br>
    <span class="keyward">ID3D11DeviceContext::CopySubresourceRegion関数</span>
    <a href="https://msdn.microsoft.com/ja-jp/library/ee419576(v=vs.85).aspx">日本語</a>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476394(v=vs.85).aspx">英語</a>    
  </p>
  <p>
    サンプルではScene::onRender関数の最後でシェーダの実行結果をバックバッファへコピーするために使用しています。
    バックバッファについては別パートで説明しますが、DX11での最終出力先みたいなものでバックバッファの内容が画面に表示されます。
  </p>
</div>

<table class="table table-condensed">
  <tbody>
    <tr>
      <td class="left"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/clear-screen">＜前</a></td>
      <td class="center"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}">トップ</a></td>
      <td class="right"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/texture">次＞</a></td>
    </tr>
  </tbody>
</table>
