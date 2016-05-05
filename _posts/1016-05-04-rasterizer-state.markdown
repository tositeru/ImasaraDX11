---
layout: default
title: "ラスタライザステート"
categories: part
description: ""
---
<h1 class="under-bar">ラスタライザステート</h1>

今パートではピクセルシェーダの前に実行されるラスタライザステージについて見ていきます。

<span class="important">ラスタライザステージでは前のステージで処理された点や三角形などを画面のピクセルに変換するステージになります。</span>
（ちなみにこの事をラスタライズと呼んだりします。）

ドキュメント:
<br>ラスタライザステージ
[(日本語)][Rasterizer_JP]
[(英語)][Rasterizer_EN]
<br>グラフィック パイプライン
[(日本語)][GRAPHICS_PIPELINE_JP]
[(英語)][GRAPHICS_PIPELINE_EN]

[GRAPHICS_PIPELINE_JP]:https://msdn.microsoft.com/ja-jp/library/ee422092(v=vs.85).aspx
[GRAPHICS_PIPELINE_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476882(v=vs.85).aspx
[Rasterizer_JP]:https://msdn.microsoft.com/ja-jp/library/ee415718(v=vs.85).aspx
[Rasterizer_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/bb205125(v=vs.85).aspx

ラスタライザステージはGPU側で処理を行うものになり、その制御はラスタライザステート(<span class="keyward">ID3D11RasterizerState</span>)で行います。
ラスタライザステートを使うことで三角形の裏面を描画しないようにしたり、ワイヤーフレームだけを描画したりすることができます。
また、ビューポートやシザー矩形などでレンダーターゲットに書き込む範囲も指定することができます。

それではラスタライザステージについて見ていきましょう。

<h1 class="under-bar">概要</h1>
今回はラスタライザステージで出来ることについて順に見ていきます。
対応しているプロジェクトは<span class="important">Part09_RasterizerState</span>になります。

<div class="summary">
  <ol>
    <li><a href="#STATE">ラスタライザステート</a></li>
    <li><a href="#VIEWPORT">ビューポート</a></li>
    <li><a href="#SCISSOR">シザー矩形</a></li>
    <li><a href="#SUMMARY">まとめ</a></li>
  </ol>
</div>

<a name="STATE"></a>
<h1 class="under-bar">ラスタライザステート</h1>

<span class="important">上でも書きましたが、ラスタライザステートは<span class="keyward">ID3D11RasterizerState</span>で表現します。</span>

<h3>設定</h3>
先にGPUへの設定は<span class="keyward">ID3D11DeviceContext::RSSetState</span>で行います。
<br>ドキュメント：<span class="keyward">ID3D11DeviceContext::RSSetState</span>
[(日本語)][RSSetState_JP]
[(英語)][RSSetState_EN]

[RSSetState_JP]:https://msdn.microsoft.com/ja-jp/library/ee419742(v=vs.85).aspx
[RSSetState_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476479(v=vs.85).aspx

{% highlight c++ %}
// Scene::onRender関数の一部
this->mpImmediateContext->RSSetState(this->mpRSScissor.Get());
{% endhighlight %}

<h3>作成</h3>

作成は<span class="keyward">ID3D11Device::CreateRasterizerState</span>で行います。
<br>ドキュメント：<span class="keyward">ID3D11DeviceContext::CreateRasterizerState</span>
[(日本語)][CreateRasterizerState_JP]
[(英語)][CreateRasterizerState_EN]

[CreateRasterizerState_JP]:https://msdn.microsoft.com/ja-jp/library/ee419799(v=vs.85).aspx
[CreateRasterizerState_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476516(v=vs.85).aspx

{% highlight c++ %}
// Scene::onInit関数の一部
//様々なラスタライザステートを作成している
//三角形の裏面をカリングするように指定したステート
D3D11_RASTERIZER_DESC desc = {};
desc.CullMode = D3D11_CULL_BACK;
desc.FillMode = D3D11_FILL_SOLID;
desc.FrontCounterClockwise = true;
desc.ScissorEnable = false;
desc.MultisampleEnable = false;
auto hr = this->mpDevice->CreateRasterizerState(&desc, this->mpRSCulling.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("カリング用のラスタライザーステートの作成に失敗");
}
//三角形をワイヤフレームで描画するよう指定したステート
desc.FillMode = D3D11_FILL_WIREFRAME;
hr = this->mpDevice->CreateRasterizerState(&desc, this->mpRSFill.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("カリング用のラスタライザーステートの作成に失敗");
}
//シザー矩形を使用するよう指定したステート
desc.CullMode = D3D11_CULL_NONE;
desc.FillMode = D3D11_FILL_SOLID;
desc.ScissorEnable = true;
hr = this->mpDevice->CreateRasterizerState(&desc, this->mpRSScissor.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("シザー矩形用のラスタライザーステートの作成に失敗");
}
{% endhighlight %}

<span class="keyward">D3D11_RASTERIZER_DESC</span>でラスタライザステートの内容決めます。各メンバの意味は以下のとおりです。
<br>ドキュメント：<span class="keyward">D3D11_RASTERIZER_DESC</span>
[(日本語)][D3D11_RASTERIZER_DESC_JP]
[(英語)][D3D11_RASTERIZER_DESC_EN]

[D3D11_RASTERIZER_DESC_JP]:https://msdn.microsoft.com/ja-jp/library/ee416262(v=vs.85).aspx
[D3D11_RASTERIZER_DESC_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476198(v=vs.85).aspx

<div class="argument">
  <h4>D3D11_RASTERIZER_DESC</h4>
  <ul>
    <li><span class="keyward">FillMode</span>
      <p>
        三角形描画時の描画モードを指定するものになります。
        指定には<span class="keyward">D3D11_FILL_MODE</span>を使用します。
        <br>ドキュメント：<span class="keyward">D3D11_FILL_MODE</span>
        <a href="https://msdn.microsoft.com/ja-jp/library/ee416127(v=vs.85).aspx">(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476131(v=vs.85).aspx">(英語)</a>
      </p>
    </li>
    <li><span class="keyward">CullMode</span>
      <p>
        描画しない三角形の面の向きを指定します。
        <span class="important">三角形の面向きにより描画するかしないかを決めることをカリング(英訳:Culling)と呼びます。</span>
        三角形の面向きは構成する頂点が時計回りか反時計回りかで決まります。
        <span class="important">それだけだと表裏は決まりませんが、下の<span class="keyward">FrontCounterClockwise</span>を使用することで決定します。</span>
        指定には<span class="keyward">D3D11_CULL_MODE</span>を使用します。
        <br>ドキュメント：<span class="keyward">D3D11_CULL_MODE</span>
        <a href="https://msdn.microsoft.com/ja-jp/library/ee416078(v=vs.85).aspx">(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476108(v=vs.85).aspx">(英語)</a>        
      </p>
    </li>
    <li><span class="keyward">FrontCounterClockwise</span>
      <p>
        三角形を構成する頂点が反時計回りの場合に表面とみなすかどうかを表すフラグになります。
        <span class="important"><span class="keyward">true</span>なら反時計回りを表面に、<span class="keyward">false</span>なら時計回りを表面にします。</span>
      </p>
    </li>
    <li><span class="keyward">DepthBias, DepthBiasClamp, SlopeScaledDepthBias</span>
      <p>
        深度バイアスを計算するときに使用する値になります。
        <span class="important">度バイアスは同じ深度値があった場合の優先順位をつけるためのものになります。</span>
        詳細はドキュメントを御覧ください。
        <br>ドキュメント：<span class="keyward">深度バイアス(英訳:Depth Bias)</span>
        <a href="https://msdn.microsoft.com/ja-jp/library/cc308048(v=vs.85).aspx">(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/cc308048(v=vs.85).aspx">(英語)</a>                
      </p>
    </li>
    <li><span class="keyward">DepthClipEnable</span>
      <p>
        距離によるクリッピングを行うかのフラグになります。
      </p>
    </li>
    <li><span class="keyward">ScissorEnable</span>
      <p>
        シザー矩形カリングを行うかのフラグになります。
        シザー矩形については下の方で説明します。
      </p>
    </li>
    <li><span class="keyward">MultisampleEnable</span>
      <p>
        マルチサンプリングアンチエイリアス(略称：MSAA)のレンダーターゲットを使用している時、四辺形ラインアンチエイリアス(英訳:quadrilateral line anti-aliasing algorithm)を行うか、アルファラインアンチエイリアス(英訳：alpha line anti-aliasing algorithm)を行うかを決めるフラグになります。
        <span class="keyward">ture</span>で四辺形ラインアンチエイリアスを<span class="keyward">false</span>でアルファラインアンチエイリアスを使用します。
        MSAAを使用するにはリソース生成時に<span class="keyward">DX11_SAMPLE_DESC::Count</span>を1より上の値を設定する必要があります。
      </p>
    </li>
    <li><span class="keyward">AntialiasedLineEnable</span>
      <p>
        MSAAのレンダーターゲットを使用している時、線分描画で<span class="keyward">MultisampleEnable</span>がfalseのとき、アンチエイリアスを有効にします。
      </p>
    </li>
  </ul>
</div>

<a name="VIEWPORT"></a>
<h1 class="under-bar">ビューポート</h1>
<span class="important">ビューポートはレンダーターゲットのどの範囲を使用するかを決めるものです。</span>

ビューポートを設定する際は特別なものを作成する必要はありません。
<span class="keyward">D3D11_VIEWPORT</span>を１つか複数用意し、<span class="keyward">ID3D11DeviceContext::RSSetViewports</span>で設定します。

<br>ドキュメント：
<br><span class="keyward">ID3D11DeviceContext::RSSetViewports</span>
[(日本語)][RSSetViewports_JP]
[(英語)][RSSetViewports_EN]
<br><span class="keyward">D3D11_VIEWPORT</span>
[(日本語)][D3D11_VIEWPORT_JP]
[(英語)][D3D11_VIEWPORT_EN]

[RSSetViewports_JP]:https://msdn.microsoft.com/ja-jp/library/ee419744(v=vs.85).aspx
[RSSetViewports_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476480(v=vs.85).aspx
[D3D11_VIEWPORT_JP]:https://msdn.microsoft.com/ja-jp/library/ee416354(v=vs.85).aspx
[D3D11_VIEWPORT_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476260(v=vs.85).aspx

{% highlight c++ %}
// Scene::onRender関数の一部
//ビューポートの設定
//この設定だとレンダーターゲットの左上から、縦横半分の領域を使用する
D3D11_VIEWPORT vp;
vp.Width = static_cast<float>(this->width() / 2);
vp.Height = static_cast<float>(this->height() / 2);
vp.TopLeftX = 0;
vp.TopLeftY = 0;
vp.MinDepth = 0.f;
vp.MaxDepth = 1.f;
this->mpImmediateContext->RSSetViewports(1, &vp);
{% endhighlight %}

<a name="SCISSOR"></a>
<h1 class="under-bar">シザー矩形</h1>

シザー矩形はビューポートと似ていますが、こちらは描画領域を指定するものになります。
<span class="important">シザー矩形の範囲外に描画されるピクセルはカリングされます。</span>
<span class="important">そのためゲームにおけるGUIの表示などで使われているみたいです。</span>
<span class="important">使用する際はラスタライザステートにて有効にする必要があるので注意してください。</span>

<span class="important">シザー矩形はレンダーターゲットの範囲内になるよう指定する必要があり、またビューポートとは独立しています。</span>

シザー矩形も設定する際は特別なものを作成する必要はありません。
<span class="keyward">D3D11_RECT</span>を１つか複数用意し、<span class="keyward">ID3D11DeviceContext::RSSetScissorRects</span>で設定します。
<br>ドキュメント：
<br><span class="keyward">ID3D11DeviceContext::RSSetScissorRects</span>
[(日本語)][RSSetScissorRects_JP]
[(英語)][RSSetScissorRects_EN]
<br><span class="keyward">D3D11_RECT</span>
[(日本語)][D3D11_RECT_JP]
[(英語)][D3D11_RECT_EN]

[RSSetScissorRects_JP]:https://msdn.microsoft.com/ja-jp/library/ee419739(v=vs.85).aspx
[RSSetScissorRects_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476478(v=vs.85).aspx
[D3D11_RECT_JP]:https://msdn.microsoft.com/ja-jp/library/ee416263(v=vs.85).aspx
[D3D11_RECT_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476199(v=vs.85).aspx

{% highlight c++ %}
// Scene::onRender関数の一部
//シザー矩形の設定
//この設定だとレンダーターゲットの左上から、縦横半分の領域を使用する
//レンダーターゲットの範囲から見た領域になり、ビューポートは関係ないので注意
UINT count = 1;
D3D11_RECT rect;
rect.left = static_cast<LONG>(this->width() / 4.f);
rect.right= static_cast<LONG>(this->width() * 3.f / 4.f);
rect.top = static_cast<LONG>(this->height() / 4.f);
rect.bottom = static_cast<LONG>(this->height() * 3.f / 4.f);
this->mpImmediateContext->RSSetScissorRects(count, &rect);
{% endhighlight %}

<a name="SUMMARY"></a>
<h1 class="under-bar">まとめ</h1>

今回はラスタライザステージについて見てきました。
このステージは三角形など形状データからピクセルデータへ変換を行うものになります。
またGPUの固定機能となります。

余分な三角形の描画を防ぐためのカリングや手軽にワイヤーフレームを描画できたりしますし、
Zバッファの値を調節することもできるなどちょっとした小技がこのステージでできますので頭の片隅にでも覚えておくといいかもしれません。

<table class="table table-condensed">
  <tbody>
    <tr>
      <td class="left"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/ZBuffer-and-depth-stencil">＜前</a></td>
      <td class="center"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}">トップ</a></td>
      <td class="right"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/geometry-shader">次＞</a></td>
    </tr>
  </tbody>
</table>
