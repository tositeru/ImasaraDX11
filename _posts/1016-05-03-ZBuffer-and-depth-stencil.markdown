---
layout: default
title: "Zバッファと深度ステンシルステート"
categories: part
description: "今回は残りのZバッファ(英訳：Z-Buffer)と深度ステンシルステート(英訳：DepthStencilState)について見ていきます。"
---
<h1 class="under-bar">Zバッファと深度ステンシルステート</h1>

前回のパートで出力結合ステージのレンダーターゲットとブレンドステートについて見てきました。
今回は残りのZバッファ(英訳：Z-Buffer)と深度ステンシルステート(英訳：DepthStencilState)について見ていきます。
この2つは隠線消去という3Dレンダリングおいて重要な要素の制御を行うものになります。
ピクセルシェーダの出力を使用するかしないかはこの2つで決定します。

ドキュメント:
[出力結合ステージ(日本語)][Output-Merger_JP]
[Output-Merger Stage(英語)][Output-Merger_EN]

[Output-Merger_JP]:https://msdn.microsoft.com/ja-jp/library/ee415707(v=vs.85).aspx
[Output-Merger_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/bb205120(v=vs.85).aspx

<h1 class="under-bar">概要</h1>
このパートではピクセルシェーダの後にその出力値を使うか使わないかをテストする深度テストとステンシルテストについて見ていきます。
この2つのテストは<l>ID3D11DepthStencilState</l>で制御します。
対応しているプロジェクトは<b>Part07_ZBufferAndDepthStencilState</b>になります。

<div class="summary">
  <ol>
    <li><a href="#VIEW">深度ステンシルビュー</a></li>
    <li><a href="#DEPTH_STENCIL_STATE">ID3D11DepthStencilState</a>
      <ul>
        <li>深度テストの設定</li>
        <li>ステンシルテストの設定</li>
      </ul>
    </li>
    <li><a href="#SUMMARY">まとめ</a></li>
    <li><a href="#SUPPLEMENTAL">補足</a>
      <ul>
        <li>Early Z</li>
        <li>ID3D11DepthStencilViewのクリア関数</li>
      </ul>
    </li>
  </ol>
</div>

<a name="VIEW"></a>
<h1 class="under-bar">深度ステンシルビュー</h1>

深度テストとステンシルテストではテスト結果をテクスチャに保存することができます。
<b>保存するときは深度ステンシルビューを作成する必要があります。</b>

<h3>作成</h3>

{% highlight c++ %}
// 深度ステンシルバッファの作成
D3D11_TEXTURE2D_DESC desc = {};
desc.Width = this->width();
desc.Height = this->height();
//深度値に24bitのfloat型をステンシル値に8ビットのuintを確保している
desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
desc.SampleDesc.Count = 1;
desc.MipLevels = 1;
desc.ArraySize = 1;
auto hr = this->mpDevice->CreateTexture2D(&desc, nullptr, this->mpDepthStencil.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("深度ステンシルバッファの作成に失敗");
}
D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
dsvDesc.Format = desc.Format;
dsvDesc.Texture2D.MipSlice = 0;
hr = this->mpDevice->CreateDepthStencilView(this->mpDepthStencil.Get(), &dsvDesc, this->mpDepthStencilDSV.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("深度ステンシルビュー作成に失敗");
}
{% endhighlight %}

上の<l>ID3D11Device::CreateDepthStencilView</l>で深度ステンシルビューを作成しています。
作り方はレンダーターゲットと同じく今まで出てきたビューとよく似ています。

ドキュメント:
<br><l>ID3D11Device::CreateDepthStencilView</l>
[(日本語)][CreateDepthStencilView_JP]
[(英語)][CreateDepthStencilView_EN]
<br><l>D3D11_DEPTH_STENCIL_VIEW_DESC</l>
[(日本語)][D3D11_DEPTH_STENCIL_VIEW_DESC_JP]
[(英語)][D3D11_DEPTH_STENCIL_VIEW_DESC_EN]

[CreateDepthStencilView_JP]:https://msdn.microsoft.com/ja-jp/library/ee419790(v=vs.85).aspx
[CreateDepthStencilView_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476507(v=vs.85).aspx
[D3D11_DEPTH_STENCIL_VIEW_DESC_JP]:https://msdn.microsoft.com/ja-jp/library/ee416084(v=vs.85).aspx
[D3D11_DEPTH_STENCIL_VIEW_DESC_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476112(v=vs.85).aspx

<b>深度ステンシルビューを作成するにはリソース作成時に<l>BindFlags</l>に<l>D3D11_BIND_DEPTH_STENCIL</l>を指定する必要があります。</b>

リソースの<l>Format</l>にはZバッファを表す<l>D</l>、ステンシル成分を表す<l>S</l>を持つものを使用してください。
<b>Zバッファとして使用したリソースをシェーダリソースビュー等ほかの用途で使用したい場合は、リソース作成時に<l>TYPELESS</l>を持つものを指定し、ビュー作成時フォーマットを改めて指定する必要があります。</b>

<h3>設定</h3>

設定は前回レンダーターゲットの設定した時に使った<l>ID3D11DeviceContext::OMSetRenderTargets</l>で行います。
<br>ドキュメント：
[ID3D11DeviceContext::OMSetRenderTargets(日本語)][OMSetRenderTargets_JP]
[ID3D11DeviceContext::OMSetRenderTargets(英語)][OMSetRenderTargets_EN]

[OMSetRenderTargets_JP]:https://msdn.microsoft.com/ja-jp/library/ee419706(v=vs.85).aspx
[OMSetRenderTargets_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476464(v=vs.85).aspx

{% highlight c++ %}
// Scene::onRender関数の一部改変したもの
//アウトプットマージャステージ
std::array<ID3D11RenderTargetView*, 1> ppRTVs = { {
  this->mpBackBufferRTV.Get(),
} };
this->mpImmediateContext->OMSetRenderTargets(static_cast<UINT>(ppRTVs.size()), ppRTVs.data(), this->mpDepthStencilDSV.Get());
{% endhighlight %}

<a name="DEPTH_STENCIL_STATE"></a>
<h1 class="under-bar">ID3D11DepthStencilState</h1>

深度テストとステンシルテストは<l>ID3D11DepthStencilState</l>で同時に設定します。
<b>この2つのテストの順序は深度テストから実行され、その後にステンシルテストが行われます。</b>

<h3>設定</h3>
まず、グラフィックスパイプラインへの設定は<l>ID3D11DeviceContext::OMSetDepthStencilState</l>で行います。

ドキュメント：<l>ID3D11DeviceContext::OMSetDepthStencilState</l>
[(日本語)][OMSetDepthStencilState_JP]
[(英語)][OMSetDepthStencilState_EN]

[OMSetDepthStencilState_JP]:https://msdn.microsoft.com/ja-jp/library/ee419704(v=vs.85).aspx
[OMSetDepthStencilState_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476463(v=vs.85).aspx

{% highlight c++ %}
// Scene::onRender関数の一部
this->mpImmediateContext->OMSetDepthStencilState(this->mpDSStencilTest.Get(), 0);
{% endhighlight %}

<h3>作成</h3>
次に<l>ID3D11DepthStencilState</l>の作成は<l>ID3D11Device::CreateDepthStencilState</l>で行います。
ドキュメント：<l>ID3D11DeviceContext::CreateDepthStencilState</l>
[(日本語)][CreateDepthStencilState_JP]
[(英語)][CreateDepthStencilState_EN]

[CreateDepthStencilState_JP]:https://msdn.microsoft.com/ja-jp/library/ee419789(v=vs.85).aspx
[CreateDepthStencilState_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476506(v=vs.85).aspx

{% highlight c++ %}
// Scene::onInit関数の一部
//深度テストとステンシルテストを別々で作っていますが、もちろん同時に行うこともできます。
D3D11_DEPTH_STENCIL_DESC desc = {};
//深度テストを行う深度ステンシルステートの作成
desc.DepthEnable = true;
desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
desc.StencilEnable = false;
auto hr = this->mpDevice->CreateDepthStencilState(&desc, this->mpDSDepthTest.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("深度テスト用のステート作成に失敗");
}

//ステンシルテストを行う深度ステンシルステートの作成
desc.DepthEnable = false;
desc.StencilEnable = true;
desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
desc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL;

desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
desc.BackFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL;

hr = this->mpDevice->CreateDepthStencilState(&desc, this->mpDSStencilTest.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("ステンシルテスト用のステート作成に失敗");
}
{% endhighlight %}

各テストの設定は<l>D3D11_DEPTH_STENCIL_DESC</l>で指定します。各テストの設定については分けてみていきます。
<br>ドキュメント：<l>D3D11_DEPTH_STENCIL_DESC</l>
[(日本語)][D3D11_DEPTH_STENCIL_DESC_JP]
[(英語)][D3D11_DEPTH_STENCIL_DESC_EN]

[D3D11_DEPTH_STENCIL_DESC_JP]:https://msdn.microsoft.com/ja-jp/library/ee416082(v=vs.85).aspx
[D3D11_DEPTH_STENCIL_DESC_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476110(v=vs.85).aspx

<h3>深度テストの設定</h3>

まず、深度テストの設定は以下のメンバで行います。
<div class="argument">
  <h4>D3D11_DEPTH_STENCIL_DESC</h4>
  <ul>
    <li><l>DepthEnable</l>
      <p>
        深度テストを行うかを指定するフラグになります。
        <l>true</l>でテストを行います。
      </p>
    </li>
    <li><l>DepthFunc</l>
      <p>
        <l>D3D11_COMPARISON_FUNC</l>を使い、深度テストを行う際の元データと上書きするデータ同士での比較方法を指定します。
        <br>ドキュメント:<l>D3D11_COMPARISON_FUNC</l>
        <a href="https://msdn.microsoft.com/ja-jp/library/ee416063(v=vs.85).aspx">D3D11_DEPTH_WRITE_MASK(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476101(v=vs.85).aspx">D3D11_DEPTH_WRITE_MASK(英語)</a>
      </p>
    </li>
    <li><l>DepthWriteMask</l>
      <p>
        <l>D3D11_DEPTH_WRITE_MASK</l>を使用して深度データを書き込む際のマスクを設定します。
        <l>D3D11_DEPTH_WRITE_MASK</l>は今のところ書き込むか書き込まないかの2種類用意されています。
      </p>
    </li>
  </ul>
</div>

<h3>ステンシルテストの設定</h3>
次に、ステンシルテストの設定は以下のメンバで行います。

<div class="argument">
  <h4>D3D11_DEPTH_STENCIL_DESC</h4>
  <ul>
    <li><l>StencilEnable </l>
      <p>
        ステンシルテストを行うかを指定するフラグになります。
        <l>true</l>でテストを行います。
      </p>
    </li>
    <li><l>StencilReadMask, StencilWriteMask</l>
      <p>
        読み込む部分または書き込む部分を指定するマスク値になります
      </p>
    </li>
    <li><l>FrontFace, BackFace</l>
      <p>
        深度テストとステンシルテストの結果でどのような処理を行うかを設定するものになります。
        <b>表面と裏面別で設定でき、<l>D3D11_DEPTH_STENCILOP_DESC</l>で指定します。</b>
        <br>ドキュメント:<l>D3D11_DEPTH_STENCILOP_DESC</l>
        <a href="https://msdn.microsoft.com/ja-jp/library/ee416080(v=vs.85).aspx">D3D11_DEPTH_WRITE_MASK(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476109(v=vs.85).aspx">D3D11_DEPTH_WRITE_MASK(英語)</a>
        <ul>
          <li><l>StencilFunc</l>
            <p>深度テストと同じく<l>D3D11_COMPARISON_FUNC</l>を使ってステンシルテストの元データと上書きするデータの比較方法を指定します。</p>
          </li>
          <li><l>StencilPassOp</l>
            <p>
              <l>D3D11_STENCIL_OP</l>を使って、ステンシルテストに成功した時に実行する処理を指定します。
              <br>ドキュメント:<l>D3D11_STENCIL_OP</l>
              <a href="https://msdn.microsoft.com/ja-jp/library/ee416283(v=vs.85).aspx">(日本語)</a>
              <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476219(v=vs.85).aspx">(英語)</a>              
            </p>
          </li>
          <li><l>StencilDepthFailOp</l>
            <p><l>StencilPassOp</l>と同じく、深度テストに失敗した時の処理を指定します。</p>
          </li>
          <li><l>StencilFailOp</l>
            <p><l>StencilPassOp</l>と同じく、ステンシルテストに失敗した時の処理を指定します。</p>
          </li>
        </ul>
      </p>
    </li>
  </ul>
</div>

<a name="SUMMARY"></a>
<h1 class="under-bar">まとめ</h1>
このパートでは出力結合ステージの深度テストとステンシルテストについて見てきました。
深度テストを使うことでカメラから近いものだけが描画できるようになったり、ステンシルテストで好きな部分だけを描画することができるようになります。

<b>重たいピクセルシェーダや上書きされるピクセルが多い場合はカメラから一番近いものだけに対してだけピクセルシェーダを実行できるよう、事前にZバッファだけに書き込むというZプリパス(英訳:Z Pre-Pass)という手法も存在します。</b>

<b>また、深度テストはその性質から前パートで説明したブレンディングと相性が悪いものになります。</b>
ガラスといった半透明なものや眩しさを表現するために加算合成を行ったパーティクルの描画を行う際は深度テストを切らないと正しい結果が得られません。

以上から、行いたい処理によって深度ステンシルステートとブレンドステートを切り替える必要がありますので注意してください。

<a name="SUPPLEMENTAL"></a>
<h1 class="under-bar">補足</h1>

<div class="supplemental">
  <h4>Early Z</h4>
  <p>
    最近のGPUにはピクセルシェーダを実行する前に深度テストを行う<l>Early Z</l>と呼ばれる機能が存在します。
    {% highlight hlsl %}
// PSEarlyZ.hlsl
[earlydepthstencil]
void main(float4 pos : SV_POSITION, float4 color : COLOR0, out float4 outColor : SV_Target0)
{
  outColor = color;
}
    {% endhighlight %}
    ピクセルシェーダのエントリポイント前に<l>earlydepthstencil属性</l>を指定することで<l>Early Z</l>の機能を使うことを宣言します。
    <br>ドキュメント:<l>earlydepthstencil</l>
    <a href="https://msdn.microsoft.com/ja-jp/library/ee422314(v=vs.85).aspx">(日本語)</a>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff471439(v=vs.85).aspx">(英語)</a>
    <br>
    この機能を使用することで不要なピクセルシェーダの実行を減らすことができ、実行速度が速くできる様になります。
    <b>制限事項として、この機能を使った場合はZバッファへの書き込みはできないため、深度バッファに書き込まない深度ステンシルステートを作成する必要がありますので注意してください。</b>
  </p>
  <p>
    また、効率的な深度テストのやり方については以下のリンクで詳しく説明されています。
    GPUによっては内部でZバッファの階層構造を生成し、深度テストの効率化を図るものも存在しているみたいです。
    そのようなこともリンク先にかかれているので、是非御覧ください。
    <br>
    <a href="http://amd-dev.wpengine.netdna-cdn.com/wordpress/media/2012/10/Depth_in-depth.pdf">Depth in-depth(英語)</a>
  </p>
</div>

<div class="supplemental">
  <h4>ID3D11DepthStencilViewのクリア関数</h4>
  <p>
    深度ステンシルビューを表す<span class="keyward">ID3D11DepthStencilView</span>にも専用のクリア関数が用意されています。
    <br>ドキュメント：
    <span class="keyward">ID3D11DeviceContext::ClearDepthStencilView</span>
    <a href="https://msdn.microsoft.com/ja-jp/library/ee419569(v=vs.85).aspx">日本語</a>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476387(v=vs.85).aspx">英語</a>

  </p>
</div>

<table class="table table-condensed">
  <tbody>
    <tr>
      <td class="left"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/RT-and-blend">＜前</a></td>
      <td class="center"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}">トップ</a></td>
      <td class="right"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/rasterizer-state">次＞</a></td>
    </tr>
  </tbody>
</table>
