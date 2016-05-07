---
layout: default
title: "ストリームアウトプット"
categories: part
description: "今回のパートではストリームアウトプットステージ(英訳:Stream-Output Stage)について見ていきます。"
---
<h1 class="under-bar">ストリームアウトプット</h1>

今回のパートではストリームアウトプットステージ(英訳:Stream-Output Stage)について見ていきます。
<b>ストリームアウトプットステージはジオメトリシェーダの次に実行されるステージになり、グラフィックスパイプラインで処理したプリミティブをバッファに出力することができます。</b>

ドキュメント：ストリームアウトプットステージ
[(日本語)][StreamOutput_JP]
[(英語)][StreamOutput_EN]

[StreamOutput_JP]:https://msdn.microsoft.com/ja-jp/library/ee415710(v=vs.85).aspx
[StreamOutput_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/bb205121(v=vs.85).aspx

<h1 class="under-bar">概要</h1>
それではストリームアウトプットステージの使い方、設定について見ていきましょう。
対応するプロジェクトは<b>Part11_StreamOutput</b>になります。
<div class="summary">
  <ol>
    <li><a href="#CONFIG">設定</a></li>
    <li><a href="#CREATE_SHADER">シェーダの生成</a></li>
    <li><a href="#DRAW_AUTO">ID3D11DeviceContext::DrawAuto</a></li>
    <li><a href="#SUMMARY">まとめ</a></li>
  </ol>
</div>

<a name="CONFIG"></a>
<h1 class="under-bar">シェーダの生成</h1>

ストリームアウトプットを利用するときに必要な設定について見ていきます。

<h4>ID3D11DeviceContext::SOSetTargets</h4>

{% highlight c++ %}
// Scene::onRender関数の一部
//三角形を生成する
this->mpImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
this->mpImmediateContext->VSSetShader(this->mpVSStreamOutput.Get(), nullptr, 0);
this->mpImmediateContext->GSSetShader(this->mpGeometryShader.Get(), nullptr, 0);
//ストリームアウトプットの出力先となるバッファを設定する
std::array<ID3D11Buffer*, 1> ppSOBufs = { {
  this->mpStreamOutputBuffer.Get(),
} };
std::array<UINT, 1> soOffsets = { { 0 } };
this->mpImmediateContext->SOSetTargets(static_cast<UINT>(ppSOBufs.size()), ppSOBufs.data(), soOffsets.data());

//プリミティブ生成を実行する
this->mpImmediateContext->Draw(this->M_STREAM_OUTPUT_COUNT, 0);

//生成したプリミティブ情報はストリームアウトプットから設定をはずさないと使用できないので注意
ppSOBufs[0] = nullptr;
this->mpImmediateContext->SOSetTargets(static_cast<UINT>(ppSOBufs.size()), ppSOBufs.data(), soOffsets.data());
ppSOBufs[0] = this->mpStreamOutputBuffer.Get();
{% endhighlight %}

<l>ID3D11DeviceContext::SOSetTargets</l>でストリームアウトプットの出力先バッファを設定します。
<b>出力先となるバッファは複数設定できます。</b>
<b>生成したバッファを別のところで使う際はストリームアウトプットステージから外さないと使えないので注意してください。</b>

ドキュメント：<l>ID3D11DeviceContext::SOSetTargets</l>
[(日本語)][SOSetTargets_JP]
[(英語)][SOSetTargets_EN]

[SOSetTargets_JP]:https://msdn.microsoft.com/ja-jp/library/ee419751(v=vs.85).aspx
[SOSetTargets_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476484(v=vs.85).aspx

<h4>出力先のバッファの作成</h4>
{% highlight c++ %}
// Scene::onInit関数の一部
//ストリームアウトプットの出力先となるバッファの作成
//生成できる最大数はバッファ生成時に決める必要があります。
D3D11_BUFFER_DESC desc = {};
desc.ByteWidth = sizeof(Vertex) * M_STREAM_OUTPUT_COUNT;
desc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;
auto hr = this->mpDevice->CreateBuffer(&desc, nullptr, this->mpStreamOutputBuffer.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("ストリームアウトプット用のバッファ作成に失敗");
}
{% endhighlight %}

<b>ストリームアウトプットの出力先のバッファを作る際はBindFlagsにD3D11_BIND_STREAM_OUTPUTを指定する必要があります。</b>
<b>また、生成するプリミティブの上限もバッファ生成時に決めないといけません。</b>

<a name="CREATE_SHADER"></a>
<h1 class="under-bar">シェーダの生成</h1>
さて、ストリームアウトプットステージは出力先を設定すると自動的に有効となり、ジオメトリシェーダの出力を設定したバッファに書き込んでくれます。
<b>サンプルではピクセルシェーダを使用していませんが、もちろんストリームアウトプットステージを使用しながら画面描画を行うこともできます。</b>

<b>出力にジオメトリシェーダを使用しますのでそれを用意する必要があるのですが、その時<l>ID3D11GeometryShader</l>を作成するには専用の関数を使用する必要があります。</b>
その関数は<l>ID3D11Device::CreateGeometryShaderWithStreamOutput </l>になります。

ドキュメント：<l>ID3D11Device::CreateGeometryShaderWithStreamOutput </l>
[(日本語)][CreateGeometryShader_JP]
[(英語)][CreateGeometryShader_EN]

[CreateGeometryShader_JP]:https://msdn.microsoft.com/ja-jp/library/ee419793(v=vs.85).aspx
[CreateGeometryShader_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476510(v=vs.85).aspx

{% highlight c++ %}
// Scene::onInit関数の一部
//ストリームアウトプットを使用するジオメトリシェーダの作成
//D3D11_SO_DECLARATION_ENTRYで出力する頂点のレイアウトを指定している
//入力レイアウトに似たもの
std::array<D3D11_SO_DECLARATION_ENTRY, 2> soEntrys = { {
  { 0, "POSITION", 0, 0, 3, 0 },
  { 0, "TEXCOORD", 0, 0, 4, 0 },
} };
std::array<UINT, 1> strides = { { sizeof(float) * 3 + sizeof(float) * 4 } };
//シェーダ作成
auto hr = this->mpDevice->CreateGeometryShaderWithStreamOutput(
  byteCode.data(),
  static_cast<SIZE_T>(byteCode.size()),
  soEntrys.data(),
  static_cast<UINT>(soEntrys.size()),
  strides.data(),
  static_cast<UINT>(strides.size()),
  D3D11_SO_NO_RASTERIZED_STREAM,
  nullptr,
  this->mpGeometryShader.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("GeometryShaderの作成に失敗");
}
{% endhighlight %}

<div class="argument">
  <h3>ID3D11Device::CreateGeometryShaderWithStreamOutputの引数一部</h3>
  <ol>
    <li><l>RasterizedStream</l>
      <p>
        画面描画を行う際、出力した頂点の中で使用するものの添字になります。
        画面描画を行わない場合は<l>D3D11_SO_NO_RASTERIZED_STREAM</l>を指定してください。
      </p>
    </li>
  </ol>
</div>

出力する頂点のレイアウトは<l>D3D11_SO_DECLARATION_ENTRY</l>で設定します。
その内容は頂点レイアウトに似たものとなります。

ドキュメント：<l>D3D11_SO_DECLARATION_ENTRY</l>
[(日本語)][D3D11_SO_DECLARATION_ENTRY_JP]
[(英語)][D3D11_SO_DECLARATION_ENTRY_EN]

[D3D11_SO_DECLARATION_ENTRY_JP]:https://msdn.microsoft.com/ja-jp/library/ee416280(v=vs.85).aspx
[D3D11_SO_DECLARATION_ENTRY_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476216(v=vs.85).aspx

<div class="argument">
  <h3>D3D11_SO_DECLARATION_ENTRY</h3>
  <ol>
    <li><l>Stream</l>
      <p>
        ０から始まるストリーム番号になります。
      </p>
    </li>
    <li><l>SemanticName</l>
      <p>
        要素のセマンティクスになります。
        指定できるものは<l>"POSITION"</l>、<l>"NORMAL"</l>、または <l>"TEXCOORD0"</l>になります。
        これにnullptrを設定した場合は、この要素はデータが書き込まれていない隙間とみなされ<l>ComponentCount</l>に５以上の値を設定できるようになります。
      </p>
    </li>
    <li><l>SemanticIndex</l>
      <p>
        セマンティクスの0から始まる番号になります。
      </p>
    </li>
    <li><l>StartComponent, ComponentCount</l>
      <p>
        使用する要素の範囲を指定します。
        <l>StartComponent</l>が2で<l>ComponentCount</l>が3なら、yzw成分を使用します。
      </p>
    </li>
    <li><l>OutputSlot</l>
      <p>
        出力するストリームアウトプットステージに設定したバッファへの添字になります。
      </p>
    </li>
  </ol>
</div>

以上でストリームアウトプットステージの設定およびシェーダの作成についての説明を終わります。

<a name="DRAW_AUTO"></a>
<h1 class="under-bar">ID3D11DeviceContext::DrawAuto</h1>

ストリームアウトプットで生成した頂点データは頂点バッファとして描画に使用できます。
が、実際に生成された頂点数はそのままだとわかりません。
<b>頂点数を調べる方法は存在しますが、この場合<l>ID3D11DeviceContext::DrawAuto</l>という特別なドローコールが用意されています。</b>

ドキュメント：<l>ID3D11DeviceContext::DrawAuto</l>
[(日本語)][DrawAuto_JP]
[(英語)][DrawAuto_EN]

[DrawAuto_JP]:https://msdn.microsoft.com/ja-jp/library/ee419590(v=vs.85).aspx
[DrawAuto_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476408(v=vs.85).aspx

{% highlight c++ %}
// Scene::onRender関数一部
//生成した三角形を描画する
//入力アセンブラステージ
std::array<UINT, 1> strides = { { sizeof(Vertex) } };
this->mpImmediateContext->IASetVertexBuffers(0, static_cast<UINT>(ppSOBufs.size()), ppSOBufs.data(), strides.data(), soOffsets.data());
this->mpImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
this->mpImmediateContext->IASetInputLayout(this->mpInputLayout.Get());
//頂点シェーダ
this->mpImmediateContext->VSSetShader(this->mpVertexShader.Get(), nullptr, 0);
//ジオメトリシェーダ
this->mpImmediateContext->GSSetShader(nullptr, nullptr, 0);
//ピクセルシェーダ
this->mpImmediateContext->PSSetShader(this->mpPixelShader.Get(), nullptr, 0);
//サイズ指定を省いて描画している
this->mpImmediateContext->DrawAuto();
{% endhighlight %}

使い方については特に説明はありません。
ストリームアウトプットによって生成された頂点データを入力アセンブラステージに設定し、<l>ID3D11DeviceContext::DrawAuto</l>を呼び出すだけで描画ができます。

<a name="SUMMARY"></a>
<h1 class="under-bar">まとめ</h1>

以上でストリームアウトプットの説明は終わります。
このステージを利用することでGPUの力を借りたプリミティブ生成が可能となりますので、活用できるときは活用しましょう。

<table class="table table-condensed">
  <tbody>
    <tr>
      <td class="left"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/geometry-shader">＜前</a></td>
      <td class="center"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}">トップ</a></td>
      <td class="right"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/tessellation">次＞</a></td>
    </tr>
  </tbody>
</table>
