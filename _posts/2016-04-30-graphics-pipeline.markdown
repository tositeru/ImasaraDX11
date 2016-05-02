---
layout: default
title: "グラフィックスパイプライン"
categories: part
description: "今パートではグラフィックスパイプラインの基本的なものになる頂点シェーダとピクセルシェーダの2つを使い、画面に点や線分、三角形を描画していきます。"
---
<h1 class="under-bar">グラフィックスパイプライン</h1>
今回からグラフィックスパイプラインについて見ていきます。
<span class="important">グラフィックスパイプラインはラスタライズ法で描画を行うために設計されたもので、点や線分、三角形を画面に描画する工程となります。</span>
グラフィックパイプラインの全体の流れはドキュメントを参考にしてください。

ドキュメント:
[グラフィック パイプライン(日本語)][GRAPHICS_PIPELINE_JP]
[Graphics Pipeline(英語)][GRAPHICS_PIPELINE_EN]

[GRAPHICS_PIPELINE_JP]:https://msdn.microsoft.com/ja-jp/library/ee422092(v=vs.85).aspx
[GRAPHICS_PIPELINE_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476882(v=vs.85).aspx

グラフィックスパイプラインのいくつかの工程にはこちらが用意したシェーダを実行することができます。
それらのシェーダを実装する際は設定する工程に適した処理内容にすることが重要です。
<span class="important">また、エントリポイントに渡される値や出力する値にはセマンティクスと呼ばれる値を識別する用の名前を付ける必要があります。</span>
<span class="important">特に予め用意されている<span class="keyward">システムセマンティクス</span>というものがどういった意味を持つのかを理解することはグラフィックスパイプラインを理解することにつながります。</span>
もちろんこちらで自由なセマンティクスをつけることも可能です。
<br>ドキュメント:
[セマンティクス(日本語)][SEMANTICS_JP]
[Semantics(英語)][SEMANTICS_EN]

[SEMANTICS_JP]:https://msdn.microsoft.com/ja-jp/library/bb509647(v=vs.85).aspx
[SEMANTICS_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/bb509647(v=vs.85).aspx

<span class="important">また、グラフィックパイプラインを理解する際はデータの流れを意識する必要がとても重要です。</span>
シェーダを実装する際はどのような値がエントリポイントに渡され、どういった値を出力すればいいのかがわかれば、グラフィックスパイプラインを自由に制御することが出来るでしょう。

<h1 class="under-bar">概要</h1>
今パートではグラフィックスパイプラインの基本的なものになる頂点シェーダとピクセルシェーダの2つを使い、画面に点や線分、三角形を描画していきます。
対応しているプロジェクトは<span class="important">Part05_GraphicsPipeline</span>になります。

<div class="summary">
  <ol>
    <li><a href="#VERTEX_BUFFER">頂点バッファ</a>
      <ul>
        <li>頂点バッファ</li>
        <li>入力レイアウト</li>
      </ul>
    </li>
    <li><a href="#CONFIG">GPUへの設定</a></li>
    <li><a href="#SHADER">頂点シェーダとピクセルシェーダ</a>
      <ul>
        <li>頂点シェーダ</li>
        <li>ピクセルシェーダ</li>
      </ul>
    </li>
    <li><a href="#PRIMITIVE_TOPOLOGY">プリミティブトポロジ</a></li>
    <li><a href="#SUMMARY">まとめ</a></li>
    <li><a href="#SUPPLEMENTAL">補足</a>
      <ul>
        <li>システムセマンティクス</li>
      </ul>
    </li>
  </ol>
</div>

<h1 class="under-bar">1.頂点バッファ</h1>
<a name="VERTEX_BUFFER"></a>

<h3>頂点バッファ</h3>
<span class="important">グラフィックパイプラインを使って画面に描画する際、頂点バッファと呼ばれるデータを入力値として設定する必要があります。</span>
<span class="important">頂点バッファは<span class="keyward">ID3D11Buffer</span>として扱い、生成する際は<span class="keyward">BindFlags</span>に<span class="keyward">D3D11_BIND_VERTEX_BUFFER</span>を設定する必要があり、ビューは必要ありません。</span>
その以外は他のバッファと同じです。

{% highlight c++ %}
//Scene::onInit関数の一部
//頂点バッファの作成 ３要素ある。
std::array<Vertex, 3> data = { {
  { {  0.0f,  0.5f, 0 } },//<- 要素１つあたりのデータ
  { {  0.5f, -0.5f, 0 } },
  { { -0.5f, -0.5f, 0 } },
} };
D3D11_BUFFER_DESC desc = {};
desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
desc.ByteWidth = sizeof(data);
D3D11_SUBRESOURCE_DATA initData = {};
initData.pSysMem = &data;
initData.SysMemPitch = sizeof(data);
auto hr = this->mpDevice->CreateBuffer(&desc, &initData, this->mpTriangleBuffer.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("三角形用の頂点バッファの作成に失敗");
}
{% endhighlight %}

<h3>入力レイアウト</h3>
<span class="important">頂点バッファの内容は自由に決めることが出来ますが、入力レイアウトと呼ばれるもので1要素当たりのデータの並びを指定する必要があります。</span>

{% highlight c++ %}
//Scene::onInit関数の一部
//入力レイアウトの作成
std::array<D3D11_INPUT_ELEMENT_DESC, 1> elements = { {
  {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
} };
hr = this->mpDevice->CreateInputLayout(elements.data(), static_cast<UINT>(elements.size()), byteCode.data(), byteCode.size(), this->mpInputLayout.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("入力レイアウトの作成に失敗");
}
{% endhighlight %}

入力レイアウトは<span class="keyward">ID3D11InputLayout</span>と表し、<span class="keyward">ID3D11InputLayout::CreateInputLayout関数</span>で作成します。
説明が前後しますが、<span class="important">作成するときは入力レイアウトに合う頂点シェーダのコンパイル済みバイナリを渡す必要があります。</span>
<br>ドキュメント:
[ID3D11Device::CreateInputLayout(日本語)][CREATE_INPUT_LAYOUT_JP]
[ID3D11Device::CreateInputLayout(英語)][CREATE_INPUT_LAYOUT_EN]

[CREATE_INPUT_LAYOUT_JP]:https://msdn.microsoft.com/ja-jp/library/ee419795(v=vs.85).aspx
[CREATE_INPUT_LAYOUT_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476512(v=vs.85).aspx

データの並びは<span class="keyward">D3D11_INPUT_ELEMENT_DESC</span>の配列を使って指定します。
<br>ドキュメント:
[D3D11_INPUT_ELEMENT_DESC(日本語)][D3D11_INPUT_ELEMENT_DESC_JP]
[D3D11_INPUT_ELEMENT_DESC(英語)][D3D11_INPUT_ELEMENT_DESC_EN]

[D3D11_INPUT_ELEMENT_DESC_JP]:https://msdn.microsoft.com/ja-jp/library/ee416244(v=vs.85).aspx
[D3D11_INPUT_ELEMENT_DESC_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476180(v=vs.85).aspx

<div class="argument">
  <h4 class="under-bar">D3D11_INPUT_ELEMENT_DESC</h4>
  <ol>
    <li><span class="keyward">SemanticName</span>
      <p>セマンティクス名</p>
    </li>
    <li><span class="keyward">SemanticIndex</span>
      <p>セマンティクス名の番号。同じ名前のものがあった時に識別するためのものになります。</p>
    </li>
    <li><span class="keyward">Format</span>
      <p>要素のフォーマット。どのようなフォーマットを設定するかは下のコードを参考にしてください。
        {% highlight c++ %}
  struct Vertex {
    float pos[3];   // -> DXGI_FORMAT_R32G32B32_FLOATを指定できる。  
    float uv[2];    // -> DXGI_FORMAT_R32G32_FLOATを指定できる。
    uint32_t color; // -> DXGI_FORMAT_R8G8B8A8_UNORMやDXGI_FORMAT_R32_UINT等を指定できる。
  };
  //上の構造体を要素にした時の頂点レイアウトの例
  std::array<D3D11_INPUT_ELEMENT_DESC, 1> elements = { {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  } };
        {% endhighlight %}
      </p>
    </li>
    <li><span class="keyward">InputSlot</span>
      <p>
        入力スロットの指定。
        頂点バッファはGPUに複数設定することができ、この要素がどのスロットのものかを指定するときに使用します。
      </p>
    </li>
    <li><span class="keyward">AlignedByteOffset</span>
      <p>
        要素内でのオフセット。
        <span class="keyward">D3D11_APPEND_ALIGNED_ELEMENT</span>を指定すると直前の要素の後ろに来るような値が設定されます。
        {% highlight c++ %}
  //D3D11_INPUT_PER_VERTEX_DATAを使わない時の例
  struct Vertex {
    float pos[3];   // -> DXGI_FORMAT_R32G32B32_FLOATを指定できる。  
    float uv[2];    // -> DXGI_FORMAT_R32G32_FLOATを指定できる。
    uint32_t color; // -> DXGI_FORMAT_R8G8B8A8_UNORMやDXGI_FORMAT_R32_UINT等を指定できる。
  };
  std::array<D3D11_INPUT_ELEMENT_DESC, 1> elements = { {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, sizeof(float) * 3 + sizeof(float) * 2, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  } };
        {% endhighlight %}
      </p>
    </li>
    <li><span class="keyward">InputSlotClass</span>
      <p>入力データの種類を設定。あとのパートで説明するインスタンス描画のときに使用します。</p>
    </li>
    <li><span class="keyward">InstanceDataStepRate</span>
      <p>
        あとのパートで説明するインスタンス描画のときに使用しますので、省略します。
      </p>
    </li>
  </ol>
</div>

<span class="important">ちなみに入力レイアウトは使用する頂点シェーダや頂点バッファの個数分生成する必要はなく、同じデータの並びだったり、互換性があるものなら１つの入力レイアウトを使い回すことが可能です。</span>

以上でグラフィックスパイプラインを使う際に必要となる頂点バッファと入力レイアウトについての説明は終わります。
あとはこの２つをグラフィックスパイプラインの始めのステージとなる<span class="keyward">入力アセンブラステージ</span>に設定すれば使うことができます。
<br>ドキュメント:
[入力アセンブラー ステージ(日本語)][IA_STAGE_JP]
[Input-Assembler Stage(英語)][IA_STAGE_EN]

[IA_STAGE_JP]:https://msdn.microsoft.com/ja-jp/library/ee415695(v=vs.85).aspx
[IA_STAGE_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/bb205116(v=vs.85).aspx

<h1 class="under-bar">2.GPUへの設定</h1>
<a name="CONFIG"></a>
次に頂点バッファと入力レイアウトをGPUへ設定する方法とそれを使ってグラフィックスパイプラインを実行する方法について見ていきましょう。
{% highlight c++ %}
// Scene::onRenderの実装を改変したもの
// 入力アセンブラステージへの設定
//入力レイアウトの設定
this->mpImmediateContext->IASetInputLayout(this->mpInputLayout.Get());
//頂点バッファの設定
std::array<ID3D11Buffer*, 1> ppVertexBuffers = {{
  this->mpPointBuffer.Get(),
}};
//頂点バッファの1要素のサイズを指定
std::array<UINT, 1> strides = { { sizeof(Vertex) } };
//グラフィックパイプラインで使う頂点バッファの開始オフセットの指定
std::array<UINT, 1> offsets = { { 0 } };
this->mpImmediateContext->IASetVertexBuffers(0, static_cast<UINT>(ppVertexBuffers.size()), ppVertexBuffers.data(), strides.data(), offsets.data());
//頂点バッファの各要素のペアを指定しているようなもの
this->mpImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
// 頂点シェーダの設定
this->mpImmediateContext->VSSetShader(this->mpVertexShader.Get(), nullptr, 0);
// ピクセルシェーダの設定
this->mpImmediateContext->PSSetShader(this->mpPixelShader.Get(), nullptr, 0);
// グラフィックスパイプラインの実行
UINT vertexCount = 3;
this->mpImmediateContext->Draw(vertexCount, 0);
{% endhighlight %}

上の<span class="keyward">ID3D11DeviceContext::IASetVertexBuffers関数</span>で頂点バッファを設定しています。
引数について詳しく見て行きませんが、<span class="important">設定するときは頂点バッファの配列以外に1要素のサイズと開始オフセットも一緒に指定する必要があります。</span>
<br>ドキュメント：
[ID3D11DeviceContext::IASetVertexBuffers(日本語)][IASetVertexBuffers_JP]
[ID3D11DeviceContext::IASetVertexBuffers(英語)][IASetVertexBuffers_EN]

[IASetVertexBuffers_JP]:https://msdn.microsoft.com/ja-jp/library/ee419692(v=vs.85).aspx
[IASetVertexBuffers_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476456(v=vs.85).aspx

{% highlight c++%}
//頂点バッファの設定
std::array<ID3D11Buffer*, 1> ppVertexBuffers = {{
  this->mpPointBuffer.Get(),
}};
//頂点バッファの1要素のサイズを指定
std::array<UINT, 1> strides = { { sizeof(Vertex) } };
//グラフィックパイプラインで使う頂点バッファの開始オフセットの指定
std::array<UINT, 1> offsets = { { 0 } };
this->mpImmediateContext->IASetVertexBuffers(0, static_cast<UINT>(ppVertexBuffers.size()), ppVertexBuffers.data(), strides.data(), offsets.data());
{% endhighlight %}

続いて入力レイアウトは<span class="keyward">ID3D11DeviceContext::IASetInputLayout関数</span>で設定します。
{% highlight c++%}
//入力レイアウトの設定
this->mpImmediateContext->IASetInputLayout(this->mpInputLayout.Get());
{% endhighlight %}

頂点バッファと入力レイアウトの設定の仕方は以上です。
<span class="important">あとは<span class="keyward">ID3D11DeviceContext::Draw関数</span>でグラフィックスパイプラインを実行すれば、設定した頂点バッファと入力レイアウトが使われます。</span>

<span class="keyward">ID3D11DeviceContext::Draw関数</span>は<span class="keyward">ドローコール(英訳:Draw Call)</span>と呼ばれ、これ以外にも幾つかの種類があります。
それらについては別パートで詳しく見ていきます。

あと説明が前後しますが、はじめのコードでは頂点バッファと入力レイアウト以外に<span class="keyward">プリミティブトポロジ</span>と頂点シェーダ、ピクセルシェーダの設定も行っています。
それらの説明は後で行いますが、グラフィックスパイプラインを実行する際はこれらも設定する必要があることを覚えておいてください。

<div class="topic">
  <h3>ID3D11DeviceContextの関数の名前</h3>
  <p>
    <span class="important">これまで<span class="keyward">ID3D11DeviceContext</span>を使ってGPUにいろいろなものを設定してきましたが、その関数には設定する対象に応じて一定のルールがあります。</span>
    例えばコンピュートシェーダに関連するものは名前の先頭に<span class="keyward">CS</span>がついており、今回出てきました入力アセンブラステージの場合は<span class="keyward">IA</span>がつきます。
    他にも頂点シェーダでは<span class="keyward">VS</span>が、ピクセルシェーダには<span class="keyward">PS</span>がつきます。
    今後グラフィックスパイプラインを見ていくうえで様々なステージが出てきますが、どういったものが設定可能なのかはこのルールを知っていればすぐに分かるようになっています。
  </p>
</div>

<h1 class="under-bar">3.頂点シェーダとピクセルシェーダ</h1>
<a name="SHADER"></a>
それでは頂点シェーダとピクセルシェーダについて見ていきましょう。

<h3>頂点シェーダ</h3>
<span class="important">頂点シェーダは入力アセンブラステージに設定された頂点バッファを入力値として取るシェーダになります。</span>

{% highlight hlsl %}
// VertexShader.hlsl
float4 main( float4 pos : POSITION ) : SV_POSITION
{
  return pos;
}
{% endhighlight %}
上のコードが頂点シェーダのとても簡単な実装になります。
行っていることは入力レイアウトで"POSITION"というセマンティクスを持つ頂点バッファの要素を"SV_POSITION"というセマンティクスに変えて次のステージに渡しているだけです。

main関数が呼ばれる回数は<span class="keyward">Draw Call</span>で指定した頂点数で決まります。
このため頂点シェーダで行う処理は1つの頂点を対象にしたものが適切となるでしょう。
例えば3Dから2Dへの変換行列をかけたり、スキンメッシュアニメーションを適応したりなどなどです。
{% highlight hlsl %}
// 頂点シェーダの例
//定数バッファやテクスチャなども使用できる
cbuffer Param :register(b0){
  float4x4 cbTransformMatrix;
  float3 cbPosOffset;
}
struct Input{
  float4 pos : POSITION;
};
struct Output {
  float4 pos : SV_POSITION;
};
//エントリポイントの引数や戻り値には構造体も指定できる
//その際は必ず、セマンティクスを指定すること
Output main( Input input )
{
  Output output;
  output.pos = input.pos;
  output.pos.xyz += cbPosOffset;
  output.pos = mul(output.pos, cbTransformMatrix);
  return output;
}
{% endhighlight %}

上のコードでは定数バッファを使用しています。シェーダモデル5.0ならテクスチャも使用可能ですが、それ以前だと使えないシェーダモデルもありますので注意してください。

ちなみにCPU上での頂点シェーダは<span class="keyward">ID3D11VertexShader</span>で表されます。
生成は<span class="keyward">ID3D11Device::CreateVertexShader関数</span>で行い、コンピュートシェーダと同じように作ります。
コンパイルの際は必ずシェーダモデルに頂点シェーダのものを指定することを忘れないでください。
<span class="important">また頂点シェーダのコンパイル済みバイナリは入力レイアウトの生成時に使用しますので、注意してください。</span>

<h3>ピクセルシェーダ</h3>
<span class="important">ピクセルシェーダはラスタライザーステージで処理されたデータを受け取り、画面に表示するピクセルの値を計算して返すシェーダになります。</span>
{% highlight hlsl %}
// PixelShader.hlsl
float4 main(float4 pos : SV_POSITION) : SV_TARGET
{
  //画面に表示される色を返す。
  return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
{% endhighlight %}

上のピクセルシェーダでは白色のピクセルを画面に出力するものになります。

<span class="important">ピクセルシェーダでは画面の好きな場所に値を出力することはできません。</span>
これはピクセルシェーダの1つ前のステージであるラスタライザーステージで処理するピクセルを決定しているためです。
そのためピクセルシェーダではピクセルの値を決めるための処理を行うのが適切となるでしょう。
<span class="keyward">SV_POSITIONセマンティクス</span>を指定した値で画面上の位置がわかるようになっていますのでそのような情報が欲しい場合は活用してください。

ラスタライザーステージは頂点シェーダとピクセルシェーダの間で実行されるステージになります。
<span class="important">詳しくは後のパートで説明しますが、頂点シェーダで出力されたデータはこのステージで補間されたり、SV_POSITIONを指定されたものはそれのw成分で割られ、画面の位置へと変換されます。</span>

main関数が呼ばれる回数は頂点バッファと後述するプリミティブトポロジなどで変わってきます。
<span class="important">これは描画する物によって描画範囲が異なるためです。</span>
例えば画面全体を覆う三角形と中央付近に小さく出る三角形では描画されるピクセルの個数は前者が多くなることは明らかです。
<span class="important">グラフィックスパイプラインでは頂点バッファの内容やピクセルシェーダ以前のステージの処理結果などによって自動的に処理を行うピクセル決めてくれます。</span>
もちろんコンピュートシェーダでもこれと同じ機能を実装することはできるでしょうが、GPU自体がこの処理に最適化されているためそのようなことをする意味はないと言ってもいいでしょう。

CPU上ではピクセルシェーダは<span class="keyward">ID3D11PixelShader</span>と表されます。
頂点シェーダと似たように<span class="keyward">ID3D11Device::CreatePixelShader関数</span>を使って生成されます。
また、コンパイルの際はシェーダモデルにピクセルシェーダのものを指定することを忘れないで下さい。

頂点シェーダとピクセルシェーダについては以上になります。

<h1 class="under-bar">4.プリミティブトポロジ</h1>
<a name="PRIMITIVE_TOPOLOGY"></a>
前2つで頂点バッファとシェーダについて見てきました。
ここで見ていくプリミティブトポロジは頂点バッファの各要素のペアを指定するものと言えます。
<br>ドキュメント：
[D3D11_PRIMITIVE_TOPOLOGY(日本語)][D3D11_PRIMITIVE_TOPOLOGY]
[D3D_PRIMITIVE_TOPOLOGY(英語)][D3D_PRIMITIVE_TOPOLOG]

[D3D11_PRIMITIVE_TOPOLOGY]:https://msdn.microsoft.com/ja-jp/library/ee416253(v=vs.85).aspx
[D3D_PRIMITIVE_TOPOLOG]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff728726(v=vs.85).aspx

サンプルで使用しているプリミティブトポロジは以下のものになります。
<ul>
  <li><span class="keyward">D3D11_PRIMITIVE_TOPOLOGY_POINTLIST</span>
    <p>頂点バッファの内容を点情報として扱います。</p>
  </li>
  <li><span class="keyward">D3D11_PRIMITIVE_TOPOLOGY_LINELIST</span>
    <p>
      頂点バッファの内容を線分情報として扱います。
      0と1番目の要素を線分のペアとし、2と3番目、4と5番目...も同じく線分のペアとしてみなします。
    </p>
  </li>
  <li><span class="keyward">D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST</span>
    <p>
      頂点バッファの内容を三角形情報として扱います。
      0,1,2番目を三角形の各頂点として、後の3,4,5番目、6,7,8番目...も同じく三角形の頂点としてみなします。
    </p>
  </li>
</ul>

それぞれのプリミティブを指定した時の違いは実際にサンプルを動かして確認してください。
指定できる要素のペアは点と線分と三角形しかありませんので注意してください。

<h1 class="under-bar">まとめ</h1>
<a name="SUMMARY"></a>
今回のパートではグラフィックスパイプラインについて見てきました。
ここで触った頂点バッファ、入力レイアウト、頂点シェーダ、ピクセルシェーダ、プリミティブトポロジがグラフィックスパイプラインの基本要素となりますので覚えておいてください。

<h1 class="under-bar">補足</h1>
<a name="SUPPLEMENTAL"></a>

<div class="supplemental">
  <h4>システムセマンティクス</h4>
  <p>
    頂点シェーダとピクセルシェーダで<span class="keyward">SV_POSITION</span>というセマンティクスを使用しました。
    <span class="keyward">SV_POSITION</span>の前にある<span class="keyward">SV_</span>はシステムセマンティクスを表し、グラフィックスパイプラインを実行する上で重要な意味を持ちます。
    頂点シェーダで<span class="keyward">SV_POSITION</span>を指定した値はラスタライザーステージで加工されピクセル位置に変換されます。
  </p>
  <figure class="highlight"><pre>
<code>//SV_POSITIONと画面上の位置の関係の例
float4 main( float4 pos : POSITION ) : SV_POSITION
{
  return float4(-1, 1, 0, 1);//<- 画面左上を表す
  return float4( 1, 1, 0, 1);//<- 画面右上を表す
  return float4(-1,-1, 0, 1);//<- 画面左上を表す
  return float4( 1, 1, 0, 1);//<- 画面左下を表す
  return float4( 0, 0, 0, 1);//<- 画面中央を表す
}
  </code></pre></figure>
  <p>
    GPUの設定よって上のコードで書いた場所に出るわけではありませんが、意味合いは同じです。
    ちなみに<span class="keyward">SV_POSITION</span>で指定した値は必ず<span class="keyward">float4</span>型を指定し、w成分は0にならないようにしてください。
    これはラスタライザーステージでwの値で割るためです。
    なぜ、自動でwの値で割るかと言われれば、変換行列について調べればわかるでしょう。
  </p>
  <p>
    その他のシステムセマンティクスについてはドキュメントを参照してください。
    シェーダステージよっては必須となるものもありますので覚えておいてください。
    <br>ドキュメント
    <a href="https://msdn.microsoft.com/ja-jp/library/ee418355(v=vs.85).aspx">セマンティクス(日本語)</a>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/bb509647(v=vs.85).aspx">Semantics(英語)</a>
  </p>
</div>

<table class="table table-condensed">
  <tbody>
    <tr>
      <td class="left"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/type-of-buffer">＜前</a></td>
      <td class="center"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}">トップ</a></td>
      <td class="right"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/graphics-pipeline">次＞</a></td>
    </tr>
  </tbody>
</table>
