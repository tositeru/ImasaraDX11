---
layout: default
title: "レンダーターゲットとブレンドステート"
categories: part
description: "今回のパートでは出力結合ステージのレンダーターゲットとブレンドステートについて見ていきます。"
---
<h1 class="under-bar">レンダーターゲットとブレンドステート</h1>

今回と次のパートでグラフィックスパイプラインの出力結合ステージ（英訳：Output-Merger Stage）について見ていきます。

<span class="important">出力結合ステージはピクセルシェーダの後に実行され、今までのステージの結果から画面に描画するか決めたり、描画先のピクセルとどのようにブレンドするかを決定するものになります。</span>

ドキュメント:
[グラフィック パイプライン(日本語)][GRAPHICS_PIPELINE_JP]
[Graphics Pipeline(英語)][GRAPHICS_PIPELINE_EN]

[GRAPHICS_PIPELINE_JP]:https://msdn.microsoft.com/ja-jp/library/ee422092(v=vs.85).aspx
[GRAPHICS_PIPELINE_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476882(v=vs.85).aspx

<h1 class="under-bar">概要</h1>
今回のパートでは出力結合ステージのレンダーターゲットとブレンドステートについて見ていきます。
対応するプロジェクトは<span class="important">Part07_RenderTargetAndBlendState</span>になります。

<div class="summary">
  <ol>
    <li><a href="#RT">レンダーターゲット</a></li>
    <li><a href="#BLEND_STATE">ブレンドステート</a></li>
    <li><a href="#SUMMARY">まとめ</a></li>
    <li><a href="#SUPPLEMENTAL">補足</a>
      <ul>
        <li>ID3D11RenderTargetViewのクリア関数</li>
      </ul>
    </li>
  </ol>
</div>

<h1 class="under-bar">レンダーターゲット</h1>
<a name="RT"></a>

レンダーターゲットはグラフィックスパイプラインの出力先となり、同時に複数設定できます。
基本的に画面を表すものなので２次元テクスチャを設定します。

<h3>シェーダ</h3>
まず、複数のレンダーターゲットに出力するシェーダを見ていきましょう。

{% highlight hlsl %}
// PixelShader.hlsl
void main(
  float4 pos : SV_POSITION,
  float4 color : COLOR0,
  out float4 outColor : SV_Target0,//<- 0番目のレンダーターゲットに出力する
  out float4 outColor2 : SV_Target1)//<- 1番目のレンダーターゲットに出力する
{
  outColor = color;
  outColor2 = color;
}
{% endhighlight %}

複数のレンダーターゲットに出力際はピクセルシェーダでその出力を制御します。
<span class="important"><span class="keyward">SV_Target0</span>と<span class="keyward">SV_Target1</span>のシステムセマンティクスを指定したものが各レンダーターゲットの出力先になります。</span>
<span class="important"><span class="keyward">SV_Target</span>の横の数字で出力先を指定し、0～7の数字が使用できます。</span>

<h3>設定</h3>
レンダーターゲットの設定は以下のように行います。
{% highlight c++ %}
// Sccene::onRender関数の一部
//レンダーターゲットの設定
std::array<ID3D11RenderTargetView*, 2> ppRTVs = { {
    this->mpRenderTarget1RTV.Get(),
    this->mpRenderTarget2RTV.Get(),
  } };
this->mpImmediateContext->OMSetRenderTargets(static_cast<UINT>(ppRTVs.size()), ppRTVs.data(), nullptr);
{% endhighlight %}

上の<span class="keyward">ID3D11DeviceContext::OMSetRenderTargets関数</span>でレンダーターゲットを行っています。
<br>ドキュメント：
[ID3D11DeviceContext::OMSetRenderTargets(日本語)][OMSetRenderTargets_JP]
[ID3D11DeviceContext::OMSetRenderTargets(英語)][OMSetRenderTargets_EN]

[OMSetRenderTargets_JP]:https://msdn.microsoft.com/ja-jp/library/ee419706(v=vs.85).aspx
[OMSetRenderTargets_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476464(v=vs.85).aspx

各引数の意味はシェーダリソースビュー等これまで出てきたものと似ており、設定するものが<span class="keyward">ID3D11RenderTargetView</span>になっただけです。
最後の引数は<span class="keyward">ID3D11DepthStencilView</span>に次にパートで詳しく見ていきます。

<h3>ID3D11RenderTargetViewの作成</h3>
ID3D11RenderTargetViewの作成方法ですが、これも他のビューと似ています。

{% highlight c++ %}
// Scene::onInit関数の一部
//レンダーターゲット2の作成
D3D11_TEXTURE2D_DESC desc = {};
desc.Width = this->width();
desc.Height = this->height();
desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
desc.BindFlags = D3D11_BIND_RENDER_TARGET;
desc.SampleDesc.Count = 1;
desc.MipLevels = 1;
desc.ArraySize = 1;
auto hr = this->mpDevice->CreateTexture2D(&desc, nullptr, this->mpRenderTarget2.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("レンダーターゲット2の作成に失敗");
}
//ID3D11RenderTargetViewの作成
D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
rtvDesc.Format = desc.Format;
rtvDesc.Texture2D.MipSlice = 0;
hr = this->mpDevice->CreateRenderTargetView(this->mpRenderTarget2.Get(), &rtvDesc, this->mpRenderTarget2RTV.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("レンダーターゲット2のレンダーターゲットビュー作成に失敗");
}
{% endhighlight %}

<span class="keyward">ID3D11Device::CreateRenderTargetView関数</span>で作成します。
引数に渡している<span class="keyward">D3D11_RENDER_TARGET_VIEW_DESC</span>の内容も他のビューの作成に使用した構造体と似ていますので説明を省略します。

ドキュメント：<br>
[ID3D11Device::CreateRenderTargetView(日本語)][CreateRenderTargetView_JP]
[ID3D11Device::CreateRenderTargetView(英語)][CreateRenderTargetView_EN]
[D3D11_RENDER_TARGET_VIEW_DESC(日本語)][D3D11_RENDER_TARGET_VIEW_DESC_JP]
[D3D11_RENDER_TARGET_VIEW_DESC(英語)][D3D11_RENDER_TARGET_VIEW_DESC_EN]

[CreateRenderTargetView_JP]:https://msdn.microsoft.com/ja-jp/library/ee419800(v=vs.85).aspx
[CreateRenderTargetView_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476517(v=vs.85).aspx
[D3D11_RENDER_TARGET_VIEW_DESC_JP]:https://msdn.microsoft.com/ja-jp/library/ee416265(v=vs.85).aspx
[D3D11_RENDER_TARGET_VIEW_DESC_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476201(v=vs.85).aspx

レンダーターゲットについては以上です。
<span class="important">説明はあっさりしていますが、現在ハイエンドゲームにおいてよく使われている遅延レンダリング(英訳: Deferred Rendering)やポストエフェクト(英訳: Post Effect)を使うときはこの機能を使っていますので、欠かすことのできないものとなっています。</span>


<h1 class="under-bar">ブレンドステート</h1>
<a name="BLEND_STATE"></a>

<span class="important">ブレンドステートはピクセルシェーダで出力した値をレンダーターゲットに書き込む際、もともとあった値とどのようにブレンドするかを指定するためのものです。</span>
<span class="keyward">ID3D11BlendState</span>をグラフィックスパイプラインに設定することでブレンドの方法を指定することができます。

ドキュメント：
[ブレンディング機能の構成(日本語)][BLENDING_JP]
[Configuring Blending Functionality(英語)][BLENDING_EN]

[BLENDING_JP]:https://msdn.microsoft.com/ja-jp/library/ee415665(v=vs.85).aspx
[BLENDING_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/bb205072(v=vs.85).aspx

ブレンドを行う式自体はある程度固定化されています。
{% highlight c++ %}
//　ブレンディングの擬似コード
//ブレンディングはRGBとAで別の式を指定できる
//RGBの計算
float3 rgbDest = ...; //ピクセルシェーダからの出力
float3 rgbSrc = ...;  //出力先のもともとあった値
//下３つはID3D11BlendStateで指定できるもの
D3D11_BLEND rgbDestBlend = ...;
D3D11_BLEND rgbSrcBlend = ...;
D3D11_BLEND_OP rgbOp = ...;
float3 rgbResult = rgbDest * rgbDestBlend rgbOp rgbSrc * rgbSrcBlend;

//Alphaの計算
float alphaDest = ...; //ピクセルシェーダからの出力
float alphaSrc = ...;  //出力先のもともとあった値
//下３つはID3D11BlendStateで指定できるもの
D3D11_BLEND alphaDestBlend = ...;
D3D11_BLEND alphaSrcBlend = ...;
D3D11_BLEND_OP alphaOp = ...;
float alphaResult = alphaDest * rgbDestBlend alphaOp alphaSrc * alphaSrcBlend;

return float4(rgbResult, alphaResult);
{% endhighlight %}

ブレンディングの式を変えることで透けた表現や明るさの表現など様々な画面効果を出すことができます。
[こちら][glblendfunc]のサイトでブレンドステートの設定を変えることができますので簡単に確認することができます。
OpenGLを使用していますが、DX11も同じパラメータを使用できますのでそこまで問題にならないでしょう。

[glblendfunc]:http://www.andersriggelsen.dk/glblendfunc.php

<h3>作成</h3>
それでは<span class="keyward">ID3D11BlendState</span>の作成の仕方について見ていきます。
{% highlight c++ %}
// Scene::onInit関数の一部
//ブレンドステートの作成
D3D11_BLEND_DESC desc = {};
desc.AlphaToCoverageEnable = false;
desc.IndependentBlendEnable = false;
desc.RenderTarget[0].BlendEnable = true;
desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//ID3D11BlendStateの作成
auto hr = this->mpDevice->CreateBlendState(&desc, this->mpBlendState.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("ブレンドステートの作成に失敗");
}
{% endhighlight %}

上の<span class="keyward">ID3D11Device::CreateBlendState関数</span>で作成します。
<span class="keyward">D3D11_BLEND_DESC</span>でブレンド方法を指定します。

ドキュメント：<br>
<span class="keyward">ID3D11Device::CreateBlendState関数</span>
[(日本語)][CreateBlendState_JP]
[(英語)][CreateBlendState_EN]<br>
<span class="keyward">D3D11_BLEND_DESC</span>
[(日本語)][D3D11_BLEND_DESC_JP]
[(英語)][D3D11_BLEND_DESC_EN]<br>

[CreateBlendState_JP]:https://msdn.microsoft.com/ja-jp/library/ee419779(v=vs.85).aspx
[CreateBlendState_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476500(v=vs.85).aspx
[D3D11_BLEND_DESC_JP]:https://msdn.microsoft.com/ja-jp/library/ee416043(v=vs.85).aspx
[D3D11_BLEND_DESC_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476087(v=vs.85).aspx

各レンダーターゲットに異なるブレンディングを設定することが可能です。
<div class="argument">
  <h4 class="under-bar">D3D11_BLEND_DESC</h4>
  <ol>
    <li><span class="keyward">AlphaToCoverageEnable</span>
      <p>
        アルファトゥカバレッジ機能を使用するときは<span class="keyward">true</span>を設定してください。
        アルファトゥカバレッジについては以下のリンク先を参考にしてください。
        <br>ドキュメント：
        <a href="https://msdn.microsoft.com/ja-jp/library/ee415665(v=vs.85).aspx">ブレンディング機能の構成(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/bb205072(v=vs.85).aspx">Configuring Blending Functionality(英語)</a>
      </p>
    </li>
    <li><span class="keyward">IndependentBlendEnable</span>
      <p>
        レンダーターゲットごとに異なるブレンディングを行うか決めるフラグになります。
        <span class="keyward">ture</span>で有効化されます。
        <span class="keyward">false</span>の場合は<span class="keyward">RenderTarget[0]</span>の内容がすべてのレンダーターゲットで使用されます。
      </p>
    </li>
    <li><span class="keyward">RenderTarget[8]</span>
      <p>
        各レンダーターゲットのブレンディングを設定するためのものです。
          <ul>
            <li><span class="keyward">BlendEnable</span>
              <p>ブレンディングを有効にするかのフラグ。</p>
            </li>
            <li><span class="keyward">SrcBlend, DestBlend</span>
              <p>
                ピクセルシェーダからの出力と出力先のもともとあった値のそれぞれのRGBに掛ける値を指定するためのものになります。
                Srcがもともとあった値に、Destがピクセルシェーダからの出力を表します。
                どのように使われるかは上の擬似コードを参考にしてください。
                指定できるものは以下のリンクを参考にしてください。
                <br>ドキュメント
                <a href="https://msdn.microsoft.com/ja-jp/library/ee416042(v=vs.85).aspx">D3D11_BLEND(日本語)</a>
                <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476086(v=vs.85).aspx">D3D11_BLEND(英語)</a>
              </p>
            </li>
            <li><span class="keyward">BlendOp</span>
              <p>
                ピクセルシェーダからの出力と出力先のもともとあった値のRGBをどのように組み合わせるかを指定するものになります。
                どのように使われるかは上の擬似コードを参考にしてください。
                指定できるものは以下のリンクを参考にしてください。
                <br>ドキュメント
                <a href="https://msdn.microsoft.com/ja-jp/library/ee416044(v=vs.85).aspx">D3D11_BLEND_OP(日本語)</a>
                <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476088(v=vs.85).aspx">D3D11_BLEND_OP(英語)</a>
              </p>
            </li>
            <li><span class="keyward">SrcBlendAlpha, DestBlendAlpha, BlendOpAlpha</span>
              <p>
                <span class="keyward">SrcBlend,DestBlend,BlendOp</span>のアルファ成分版になります。
              </p>
            </li>
            <li><span class="keyward">RenderTargetWriteMask</span>
              <p>
                書き込みマスクです。
                RGBA内、書き込みたい部分のビットを立てることで書き込む値を制御できます。
                値が0なら書き込みが無効になり、
                <span class="keyward">D3D11_COLOR_WRITE_ENABLE_ALL</span>を指定すればすべての成分が書き込まれます。
              </p>
            </li>
          </ul>
      </p>
    </li>
  </ol>
</div>


<h3>設定</h3>
ブレンドステートの設定は以下のようになります。

{% highlight c++ %}
// Scene::onRender関数の一部改変したもの
std::array<float, 4> factor = { {
  1, 1, 1, 1
} };
this->mpImmediateContext->OMSetBlendState(this->mpBlendState.Get(), factor.data(), 0xffffffff);
{% endhighlight %}

ドキュメント：
[ID3D11DeviceContext::OMSetBlendState(日本語)][OMSetBlendState_JP]
[ID3D11DeviceContext::OMSetBlendState(英語)][OMSetBlendState_EN]

[OMSetBlendState_JP]:https://msdn.microsoft.com/ja-jp/library/ee419702(v=vs.85).aspx
[OMSetBlendState_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476462(v=vs.85).aspx

<div class="argument">
  <h4 class="under-bar">D3D11_BLEND_DESC</h4>
  <ol>
    <li><span class="keyward">第２引数：BlendFactor</span>
      <p>
        <span class="keyward">SrcBlend, DestBlend</span>などに<span class="keyward">D3D11_BLEND_BLEND_FACTOR</span>を使用した時のブレンディング係数になります。
      </p>
    </li>
    <li><span class="keyward">第３引数：SampleMask</span>
      <p>
        マルチサンプリングを設定されているレンダーターゲットを使用している時の書くサンプリングのマスク値になります。
      </p>
    </li>
  </ol>
</div>

ブレンドステートについては以上になります。
ブレンドステートを使う場合はシェーダ側で特別な対応する必要はありません。
なので、同じシェーダに設定が異なるブレンドステートを設定することが可能です。

<h1 class="under-bar">まとめ</h1>
<a name="SUMMARY"></a>

今回、出力結合ステージのレンダーターゲットとブレンドステートについて見てきました。
レンダーターゲットは応用範囲が広い機能になり、ブレンドステートはパーティクルや半透明表現に使われるものになります。

グラフィックパイプラインにはブレンドステートのようにGPU側で処理を行うものがあります。
グラフィックスパイプラインはコンピュートシェーダのように全てこちらで制御することはできませんが、
ブレンドステートなどの機能がありますので活用していきましょう。

<h1 class="under-bar">補足</h1>
<a name="SUPPLEMENTAL"></a>

<div class="supplemental">
  <h4>ID3D11RenderTargetViewのクリア関数</h4>
  <p>
    レンダーターゲットを表す<span class="keyward">ID3D11RenderTargetView</span>には専用のクリア関数が用意されています。
    <br>ドキュメント：<span class="keyward">ID3D11DeviceContext::ClearRenderTargetView</span>
    <a href="https://msdn.microsoft.com/ja-jp/library/ee419570(v=vs.85).aspx">日本語</a>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476388(v=vs.85).aspx">英語</a>
    <br>
    クリアに使用する値はリソースのフォーマットにかかわらず<span class="keyward">float</span>になりますので注意してください。
  </p>
</div>

<table class="table table-condensed">
  <tbody>
    <tr>
      <td class="left"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/draw-call">＜前</a></td>
      <td class="center"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}">トップ</a></td>
      <td class="right"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/ZBuffer-and-depth-stencil">次＞</a></td>
    </tr>
  </tbody>
</table>
