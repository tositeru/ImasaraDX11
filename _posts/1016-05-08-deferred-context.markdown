---
layout: default
title: "ディファードコンテキスト"
categories: part
description: "今パートではID3D11DeviceContextの1種であるディファードコンテキスト(英訳：Deferred Context)について見ていきます。"
---
<h1 class="under-bar">ディファードコンテキスト</h1>

今パートでは<span class="keyward">ID3D11DeviceContext</span>の1種であるディファードコンテキスト(英訳：Deferred Context)について見ていきます。

今までシェーダやリソースの設定などで<span class="keyward">ID3D11DeviceContext</span>を使ってきましたが、それらはイミディエイトコンテキスト(英訳：Immediate Context)と呼ばれています。
<span class="important">イミディエイトを訳すと「即時」といった意味になり、イミディエイトコンテキストを通じて行う設定など(以後、コマンドと呼びます)は直ちにコマンドバッファという一時的なストレージ空間へ積まれGPUの都合がいい時に実行されます。</span>
<span class="important">またイミディエイトコンテキストはメインスレッド上からでしか使うことができません。</span>
なので、マルチスレッドを利用している場合は何かしらの方法でGPUを実行するために必要な情報をメインスレッドに移す必要があります。

<span class="important">それに対してディファードコンテキストはディファードの意味である「延期」が表すように、それを通じたコマンドはディファードコンテキストに記録され実行はされません。</span>
<span class="important">ディファードコンテキストが記録したコマンドは後でイミディエイトコンテキストを使って実行する必要がありますが、メインスレッド以外でもコマンドの記録が可能となっています。</span>
つまり、マルチスレッド上でGPUで実行させるコマンドを作成して、あとはメインスレッドにディファードコンテキストを渡して実行するだけで済みます。
<span class="important">更に発展してGPUが動作している裏で次に実行させるコマンドを記録し、GPUの動作が終わったら裏で記録していたコマンドをすぐに実行させることもでき、
GPUを絶え間なく動作させることが出来ます。</span>


<h1 class="under-bar">概要</h1>
それではディファードコンテキストについて見ていきましょう。
対応するパートは<span class="important">Part13_DeferredContext</span>になります。

<div class="summary">
  <ol>
    <li><a href="#CREATE">作成</a></li>
    <li><a href="#USED">コマンドの記録とその実行</a></li>
    <li><a href="#SUMMARY">まとめ</a></li>
  </ol>
</div>

<a name="CREATE"></a>
<h1 class="under-bar">作成</h1>
まず、ディファードコンテキストの作成について見ていきます。
といっても1行で作ることが出来ます。

{% highlight c++ %}
// Scene::onInit関数の一部
//ディファードコンテキストの作成
this->mpDevice->CreateDeferredContext(0, this->mpDeferedContext.GetAddressOf());
{% endhighlight %}

<span class="keyward">ID3D11Device::CreateDeferredContext</span>でディファードコンテキストを作成します。
<span class="important">第1引数には必ず0を渡してください。</span>

ドキュメント：<span class="keyward">ID3D11Device::CreateDeferredContext</span>
[(日本語)][CreateDeferredContext_JP]
[(英語)][CreateDeferredContext_EN]

[CreateDeferredContext_JP]:https://msdn.microsoft.com/ja-jp/library/ee419788(v=vs.85).aspx
[CreateDeferredContext_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476505(v=vs.85).aspx

<a name="USED"></a>
<h1 class="under-bar">コマンドの記録とその実行</h1>

<span class="important">コマンドを記録する方法はイミディエイトコンテキストと同じように関数を呼び出すだけです。</span>
コマンドの記録を終える場合は<span class="keyward">ID3D11DeviceContext::FinishCommandList</span>を呼び出してください。
<span class="important">また、ディファードコンテキストで記録したコマンドは他のディファードコンテキストの<span class="keyward">ID3D11DeviceContext::ExecuteCommandList</span>に渡すことでコピーすることができます。</span>

ドキュメント：
<br><span class="keyward">ID3D11DeviceContext::FinishCommandList</span>
[(日本語)][FinishCommandList_JP]
[(英語)][FinishCommandList_EN]
<br><span class="keyward">ID3D11DeviceContext::ExecuteCommandList</span>
[(日本語)][ExecuteCommandList_JP]
[(英語)][ExecuteCommandList_EN]

[FinishCommandList_JP]:https://msdn.microsoft.com/ja-jp/library/ee419627(v=vs.85).aspx
[FinishCommandList_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476424(v=vs.85).aspx
[ExecuteCommandList_JP]:https://msdn.microsoft.com/ja-jp/library/ee419626(v=vs.85).aspx
[ExecuteCommandList_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476423(v=vs.85).aspx

{% highlight c++ %}
std::thread record([&]() {
  while (this->mIsRunRecordingThread) {
    Sleep(2);
    std::unique_lock<std::mutex> lock(this->mMutex);
    //記録したコマンドをクリアしている
    this->mpCommandLists.Reset();
    //別のディファードコンテキストが記録したものをコピーすることも出来る
    //グラフィックスパイプラインに関係するものは複数のディファードコンテキストにまたがって記録することはできないので注意
    this->mpDeferedContext->ExecuteCommandList(this->mpCLRBindTAndVP.Get(), false);
    //ビューポートの設定
    D3D11_VIEWPORT vp;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = static_cast< float >(this->width());
    vp.Height = static_cast< float >(this->height());
    vp.MinDepth = 0.f;
    vp.MaxDepth = 1.f;
    this->mpDeferedContext->RSSetViewports(1, &vp);
    //レンダーターゲットの設定
    std::array<ID3D11RenderTargetView*, 1> RTs = { {
      this->mpBackBufferRTV.Get()
    } };
    this->mpDeferedContext->OMSetRenderTargets(static_cast<UINT>(RTs.size()), RTs.data(), this->mpZBufferDSV.Get());
    //ピクセルシェーダ
    this->mpDeferedContext->PSSetShader(this->mpPixelShader.Get(), nullptr, 0);
    //入力アセンブラステージ
    std::array<ID3D11Buffer*, 1> ppVertexBuffers = { {
      this->mpTriangleBuffer.Get(),
    } };
    std::array<UINT, 1> strides = { { sizeof(Vertex) } };
    std::array<UINT, 1> offsets = { { 0 } };
    this->mpDeferedContext->IASetVertexBuffers(
      0,
      static_cast<UINT>(ppVertexBuffers.size()),
      ppVertexBuffers.data(),
      strides.data(),
      offsets.data());
    this->mpDeferedContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    this->mpDeferedContext->IASetInputLayout(this->mpInputLayout.Get());
    //頂点シェーダ
    this->mpDeferedContext->VSSetShader(this->mpVertexShader.Get(), nullptr, 0);
    //実行
    this->mpDeferedContext->Draw(3, 0);
    //ここまでのコマンドを記録する
    this->mpDeferedContext->FinishCommandList(true, this->mpCommandLists.GetAddressOf());

    this->mEnableExecuting = true;
    this->mEnableExecuteFlag.notify_one();
  }
});
{% endhighlight %}

<span class="important">記録したコマンドの実行は<span class="keyward">ID3D11DeviceContext::ExecuteCommandList</span>をイミディエイトコンテキストから呼び出します。</span>
イミディエイトコンテキスト場合は第2引数で<span class="keyward">ID3D11DeviceContext::ExecuteCommandList</span>を呼び出す前の設定状況を復元するかを指定することが出来ます。

{% highlight c++ %}
// Scene::onRender関数
std::unique_lock<std::mutex> lock(this->mMutex);
while (!this->mEnableExecuting) {
  this->mEnableExecuteFlag.wait(lock);
}
//記録したコマンドの実行
this->mpImmediateContext->ExecuteCommandList(this->mpCommandLists.Get(), false);
this->mEnableExecuting = false;
{% endhighlight %}

ディファードコンテキストについては以上になります。

<a name="SUMMARY"></a>
<h1 class="under-bar">まとめ</h1>

今回のパートではディファードコンテキストについてみていきました。
作成や使い方は簡単ですが、問題はどのように使うのかが一番の悩みとなるでしょう。
マルチスレッドプログラミングを行う必要があるので実装はイミディエイトコンテキストだけを使っている時と比べて設計的な意味で難しくなってしまいます。

また、Direct3D12では<span class="keyward">ID3D11DeviceContext</span>の周りが大きく変わり、<span class="keyward">ID3D11DeviceContext</span>が裏で行っていたことをこちらで制御する必要がでてきます。
実装の自由度が高くなった反面、扱いが難しくなっています。
効率的な実装にはGPUやマルチスレッドプログラミングへの知識もかなり重要となり、ますますグラフィックスAPIの敷居が高くなってしまいました。

単にCGを触りたいとなるとゲームエンジンを利用するの方が細かい部分を気にせず実装できるので、それらを利用するのがいいと思います。

<table class="table table-condensed">
  <tbody>
    <tr>
      <td class="left"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/tessellation">＜前</a></td>
      <td class="center"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}">トップ</a></td>
      <td class="right"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/query">次＞</a></td>
    </tr>
  </tbody>
</table>
