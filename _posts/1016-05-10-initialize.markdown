---
layout: default
title: "初期化"
categories: part
description: "このパートでは今まで使ってきたID3D11DeviceとID3D11DeviceContextとバックバッファの作成方法について見ていきます。"
---
<h1 class="under-bar">初期化</h1>

前回までの内容でDX11の機能はあらかた見終えました。
残すパートもあとわずかとなり、今パートではようやくDX11の初期化について見ていきます。

<h1 class="under-bar">概要</h1>
このパートでは今まで使ってきた<span class="keyward">ID3D11Device</span>と<span class="keyward">ID3D11DeviceContext</span>の作成方法とDXGIについて見ていきます。
対応するプロジェクトは<span class="important">Part15_Initialize</span>になります。

<div class="summary">
  <ol>
    <li><a href="#CREATE_DEVICE">ID3D11DeviceとID3D11DeviceContextの作成</a></li>
    <li><a href="#ADAPTER">アダプタ</a></li>
    <li><a href="#CREATE_SWAPCHAIN">IDXGISwapChainの作成</a></li>
    <li><a href="#RESIZE_BACKBUFFER">バックバッファのリサイズ</a></li>
    <li><a href="#CHANGE_MODE">全画面モードとウィンドウモードの切替</a></li>
    <li><a href="#SUMMARY">まとめ</a></li>
    <li><a href="#SUPPLEMENTAL">補足</a>
      <ul>
        <li>D3D11CreateDeviceAndSwapChain</li>
        <li>IDXGISwapChain::Present</li>
      </ul>
    </li>
  </ol>
</div>

<a name="CREATE_DEVICE"></a>
<h1 class="under-bar">ID3D11DeviceとID3D11DeviceContextの作成</h1>
<span class="keyward">ID3D11Device</span>と<span class="keyward">ID3D11DeviceContext</span>の作成には<span class="keyward">D3D11CreateDevice</span>を使用します。

ドキュメント：<span class="keyward">D3D11CreateDevice</span>
[(日本語)][D3D11CreateDevice_JP]
[(英語)][D3D11CreateDevice_EN]

[D3D11CreateDevice_JP]:https://msdn.microsoft.com/ja-jp/library/ee416031(v=vs.85).aspx
[D3D11CreateDevice_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476082(v=vs.85).aspx

{% highlight c++ %}
// Scene::onInit関数の一部
//ID3D11DeviceとID3D11DeviceContextの作成
std::array<D3D_FEATURE_LEVEL, 3> featureLevels = { {
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0
  } };
UINT flags = 0;
D3D_FEATURE_LEVEL usedLevel;
hr = D3D11CreateDevice(
  this->mpAdapter.Get(),
  D3D_DRIVER_TYPE_UNKNOWN,
  nullptr,
  flags,
  featureLevels.data(),
  static_cast<UINT>(featureLevels.size()),
  D3D11_SDK_VERSION,
  this->mpDevice.GetAddressOf(),
  &usedLevel,
  this->mpImmediateContext.GetAddressOf()
  );
if (FAILED(hr)) {
  throw std::runtime_error("ID3D11Deviceの作成に失敗。");
}
{% endhighlight %}
<div class="argument">
  <h4>D3D11CreateDevice</h4>
  <ul>
    <li><span class="keyward">第1引数：pAdapter</span>
      <p>
        使用するGPUを表すアダプタを渡します。型は<span class="keyward">IDXGIAdapter</span>になり、この後詳しく見ていきます。
        この引数にはnullptrを渡すことができ、その時は使用できる<span class="keyward">IDXGIAdapter</span>の中から１つ自動的に選択されます。
      </p>
    </li>
    <li><span class="keyward">第2引数：DriverType</span>
      <p>
        作成する<span class="keyward">ID3D11Device</span>のドライバタイプを指定します。
        GPUを使用するときは<span class="keyward">D3D_DRIVER_TYPE_HARDWARE</span>を指定することになります。
        <span class="important">注意点として<span class="keyward">pAdapter</span>を指定して作成するときは必ず<span class="keyward">D3D_DRIVER_TYPE_UNKNOWN</span>を指定する必要があります。</span>
        <br>ドキュメント：<span class="keyward">D3D_DRIVER_TYPE</span>
        <a href="https://msdn.microsoft.com/ja-jp/library/ee417659(v=vs.85).aspx">(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476328(v=vs.85).aspx">(英語)</a>
      </p>
    </li>
    <li><span class="keyward">第3引数：Software</span>
      <p>
        ソフトウェアによるラスタライザを実装したDLLを渡します。
        <span class="keyward">DriverType</span>に<span class="keyward">D3D_DRIVER_TYPE_SOFTWARE</span>を指定した際に使用します。
        詳細はドキュメントを参照してください。
      </p>
    </li>
    <li><span class="keyward">第4引数：Flags</span>
      <p>
        ランタイムレイヤーを表すフラグを渡します。
        フラグは<span class="keyward">D3D11_CREATE_DEVICE_FLAG</span>で定義されています。
        <br>ドキュメント：
        <br><span class="keyward">D3D11_CREATE_DEVICE_FLAG</span>
        <a href="https://msdn.microsoft.com/ja-jp/library/ee416076(v=vs.85).aspx">(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476107(v=vs.85).aspx">(英語)</a>
        <br><span class="keyward">ソフトウェア レイヤー</span>
        <a href="https://msdn.microsoft.com/ja-jp/library/ee422091(v=vs.85).aspx">(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476881(v=vs.85).aspx">(英語)</a>
      </p>
    </li>
    <li><span class="keyward">第4引数：pFeatureLevels</span>
      <p>
        使用したい機能レベルを配列で渡します。
        優先順位は要素の若いものとなります。
      </p>
    </li>
    <li><span class="keyward">第5引数：FeatureLevels</span>
      <p><span class="keyward">pFeatureLevels</span>の要素数を渡します。</p>
    </li>
    <li><span class="keyward">第6引数：SDKVersion</span>
      <p><span class="keyward">D3D11_SDK_VERSION</span>を渡してください。</p>
    </li>
    <li><span class="keyward">第7,8,9引数：ppDevice, pFeatureLevel, ppImmediateContext</span>
      <p>作成した<span class="keyward">ID3D11Device</span>と<span class="keyward">ID3D11DeviceContext</span>および、機能レベルを受け取る変数を渡します。</p>
    </li>
  </ul>
</div>

<a name="ADAPTER"></a>
<h1 class="under-bar">アダプタ</h1>
次にアダプタについて見ていきます。アダプタは<span class="keyward">IDXGIAdapter</span>と表されます。

<span class="keyward">IDXGIAdapter</span>は<span class="keyward">DirectX Graphics Infrastructure(略称：DXGI)</span>に属するクラスになります。
<span class="important">DXGIはDirect3D10から導入され、Direct3Dに依存しない部分を管理し、Direct3Dの各バージョンの共通したフレームワークを提供しています。</span>
<span class="important">全画面表示や現在の使用できるディスプレイを列挙したりするときに利用します。</span>

<span class="keyward">IDXGIAdapter</span>についての詳細はドキュメントを参照にしていただきたいのですが、簡単に言うとGPUのことになります。
DXGIを使用することで現在使用できるGPUを調べることが出来ます。

また、<span class="keyward">IDXGIAdapter</span>にはDXGIのバージョンに合わせていくつか種類があり、ここでは<span class="keyward">DXGI1.1</span>に対応した<span class="keyward">IDXGIAdapter1</span>を使っています。
<span class="keyward">DXGI1.1</span>はDX11で追加されたフォーマットに対応しているバージョンになっています。

ドキュメント：
<br><span class="keyward">DXGI の概要</span>
[(日本語)][DXGI_JP]
[(英語)][DXGI_EN]
<br>[DirectX Graphics Infrastructure (DXGI):ベスト プラクティス][DXGI_BEST_JP]
<br><span class="keyward">IDXGIAdapter1</span>
[(日本語)][IDXGIAdapter1_JP]
[(英語)][IDXGIAdapter1_EN]

[DXGI_JP]:https://msdn.microsoft.com/ja-jp/library/bb205075(v=vs.85).aspx
[DXGI_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx
[DXGI_BEST_JP]:https://msdn.microsoft.com/ja-jp/library/ee417025(v=vs.85).aspx
[IDXGIAdapter1_JP]:https://msdn.microsoft.com/ja-jp/library/ee421895(v=vs.85).aspx
[IDXGIAdapter1_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471329(v=vs.85).aspx

{% highlight c++ %}
// Scene::onInit関数の一部
//DXGIを使う上で必要となるIDXGIFactory1を作成
HRESULT hr;
Microsoft::WRL::ComPtr<IDXGIFactory1> pFactory;
hr = CreateDXGIFactory1(IID_PPV_ARGS(pFactory.GetAddressOf()));
if (FAILED(hr)) {
  throw std::runtime_error("IDXGIFactoryクラスの作成に失敗しました。");
}
//GPUアダプターを列挙して一番最初に見つかった使えるものを選ぶ
Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapterIt;
for (UINT adapterIndex = 0; S_OK == pFactory->EnumAdapters1(adapterIndex, pAdapterIt.GetAddressOf());  ++adapterIndex) {
  DXGI_ADAPTER_DESC1 desc;
  pAdapterIt->GetDesc1(&desc);

  // ...アダプタ情報のログ出力は省略

  if (nullptr == this->mpAdapter) {
    if (desc.Flags ^= DXGI_ADAPTER_FLAG_SOFTWARE) {
      this->mpAdapter = pAdapterIt;
      OutputDebugStringA(std::string("このアダプターを使用します。 adapterIndex = " + std::to_string(adapterIndex) + "\n").c_str());
    }
  }
  //使い終わったら必ずReleaseすること
  pAdapterIt.Reset();
}
{% endhighlight %}

<br>
<h4>IDXGIFactoryの作成</h4>
<span class="important">DXGIを利用するときははじめに<span class="keyward">IDXGIFactory</span>を生成します。</span>
DXGIでは<span class="keyward">IDXGIFactory</span>を使って必要なものを作成していきますので、<span class="keyward">ID3D11Device</span>と似た役割になります。

<span class="important"><span class="keyward">IDXGIFactory</span>の作成には<span class="keyward">CreateDXGIFactory</span>を使用します。</span>
また、<span class="keyward">IDXGIFactory</span>も<span class="keyward">IDXGIAdapter</span>と同じでいくつかの種類が存在しますので、DXGIのバージョンを確認して使用する種類を選択してください。

ドキュメント：
<br><span class="keyward">IDXGIFactory1</span>
[(日本語)][IDXGIFactory1_JP]
[(英語)][IDXGIFactory1_EN]
<br><span class="keyward">CreateDXGIFactory1</span>
[(日本語)][CreateDXGIFactory1_JP]
[(英語)][CreateDXGIFactory1_EN]

[IDXGIFactory1_JP]:https://msdn.microsoft.com/ja-jp/library/ee421912(v=vs.85).aspx
[IDXGIFactory1_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471335(v=vs.85).aspx
[CreateDXGIFactory1_JP]:https://msdn.microsoft.com/ja-jp/library/ee415212(v=vs.85).aspx
[CreateDXGIFactory1_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471318(v=vs.85).aspx

{% highlight c++ %}
// Scene::onInit関数の一部
//DXGIを使う上で必要となるIDXGIFactory1を作成
HRESULT hr;
Microsoft::WRL::ComPtr<IDXGIFactory1> pFactory;
hr = CreateDXGIFactory1(IID_PPV_ARGS(pFactory.GetAddressOf()));
if (FAILED(hr)) {
  throw std::runtime_error("IDXGIFactoryクラスの作成に失敗しました。");
}
{% endhighlight %}

<br>
<h4>使用するアダプタの選択</h4>
<span class="keyward">IDXGIFactory</span>を作成したら<span class="keyward">EnumAdapters</span>を利用して、使用する<span class="keyward">IDXGIAdaptor</span>を選択します。

<span class="keyward">EnumAdapters</span>の引数には0から始まるアダプタの添字とアダプタを受け取る変数を渡します。
<span class="important">使用できるアダプタ全てを見ていくにはサンプルコードのように<span class="keyward">EnumAdapters</span>が<span class="keyward">DXGI_ERROR_NOT_FOUND</span>を返すまでループします。</span>

アダプタの情報は<span class="keyward">IDXGIAdaptor::GetDesc</span>で取得できる<span class="keyward">DXGI_ADAPTER_DESC</span>にあります。
製品情報やメモリ量など見れますが、アダプタの選択の際には<span class="keyward">Flags</span>の内容を使用しています。
サンプルコードではGPUを使用したいので<span class="keyward">Flags</span>が<span class="keyward">DXGI_ADAPTER_FLAG_SOFTWARE</span>でないものを選択しています。
<span class="keyward">DXGI_ADAPTER_FLAG_SOFTWARE</span>はWindows 8から導入されたものなので注意してください。

ドキュメント:
<br><span class="keyward">IDXGIFactory1::EnumAdapters1</span>
[(日本語)][EnumAdapters1_JP]
[(英語)][EnumAdapters1_EN]
<br><span class="keyward">DXGI_ADAPTER_DESC1</span>
[(日本語)][DXGI_ADAPTER_DESC1_JP]
[(英語)][DXGI_ADAPTER_DESC1_EN]

[EnumAdapters1_JP]:https://msdn.microsoft.com/ja-jp/library/ee421913(v=vs.85).aspx
[EnumAdapters1_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471336(v=vs.85).aspx
[DXGI_ADAPTER_DESC1_JP]:https://msdn.microsoft.com/ja-jp/library/ee418112(v=vs.85).aspx
[DXGI_ADAPTER_DESC1_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471326(v=vs.85).aspx

{% highlight c++ %}
// Scene::onInit関数の一部
//GPUアダプターを列挙して一番最初に見つかった使えるものを選ぶ
Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapterIt;
for (UINT adapterIndex = 0; S_OK == pFactory->EnumAdapters1(adapterIndex, pAdapterIt.GetAddressOf());  ++adapterIndex) {
  DXGI_ADAPTER_DESC1 desc;
  pAdapterIt->GetDesc1(&desc);

  // ...アダプタ情報のログ出力は省略

  if (nullptr == this->mpAdapter) {
    if (desc.Flags ^= DXGI_ADAPTER_FLAG_SOFTWARE) {
      this->mpAdapter = pAdapterIt;
      OutputDebugStringA(std::string("このアダプターを使用します。 adapterIndex = " + std::to_string(adapterIndex) + "\n").c_str());
    }
  }
  //使い終わったら必ずReleaseすること
  pAdapterIt.Reset();
}
{% endhighlight %}

このように選択したアダプタを先に説明した<span class="keyward">D3D11CreateDevice</span>に渡すことで好きなGPUを使うことが出来ます。

<a name="CREATE_SWAPCHAIN"></a>
<h1 class="under-bar">IDXGISwapChainの作成</h1>

<span class="important">次は描画結果をディスプレイに表示するために必要な<span class="keyward">IDXGISwapChain</span>について見ていきます。</span>

{% highlight c++ %}
// Scene::onInitの一部
//IDXGISwapChainの作成
DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
swapChainDesc.OutputWindow = Win32Application::hwnd();
swapChainDesc.BufferCount = 2;
swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
//swapChainDesc.Flags = 0;
swapChainDesc.SampleDesc.Count = 1;
swapChainDesc.SampleDesc.Quality = 0;
//フルスクリーンとウィンドモードの切り替えがしたい場合は、まずウィンドウモードとして生成することを推奨しているみたい
//https://msdn.microsoft.com/en-us/library/bb174579(v=vs.85).aspx
swapChainDesc.Windowed = true;
//希望する画面設定
swapChainDesc.BufferDesc.Width = this->width();
swapChainDesc.BufferDesc.Height = this->height();
swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//上の画面設定に一番近いものを調べる
Microsoft::WRL::ComPtr<IDXGIOutput> pOutput;
if (DXGI_ERROR_NOT_FOUND == this->mpAdapter->EnumOutputs(0, pOutput.GetAddressOf())) {
  throw std::runtime_error("アダプターの出力先が見つかりません。");
}
DXGI_MODE_DESC modeDesc;
hr = pOutput->FindClosestMatchingMode(&swapChainDesc.BufferDesc, &modeDesc, this->mpDevice.Get());
if (FAILED(hr)) {
  throw std::runtime_error("表示モードの取得に失敗");
}
//IDXGISwapChainの作成
swapChainDesc.BufferDesc = modeDesc;
hr = pFactory->CreateSwapChain(this->mpDevice.Get(), &swapChainDesc, this->mpSwapChain.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("IDXGISwapChainの作成に失敗");
}
{% endhighlight %}

<span class="important"><span class="keyward">IDXGISwapChain</span>は画面に表示するバッファを管理するものになり、描画中に起きる画面のちらつきを防ぐための手法であるバックバッファを簡単に扱うことが出来ます。</span>

バックバッファを簡単に説明すると画面に表示するバッファと描画を行うバッファを用意して、描画が終わったらその２つを入れ替える手法になります。
画面に表示するイメージをフロントバッファと呼ばれています。
イメージの枚数は2枚でなくてもよく実装しているものに合わせて枚数を増やせます。

ドキュメント：<span class="keyward">IDXGISwapChain</span>
[(日本語)][IDXGISwapChain_JP]
[(英語)][IDXGISwapChain_EN]

[IDXGISwapChain_JP]:https://msdn.microsoft.com/ja-jp/library/bb174569(v=vs.85).aspx
[IDXGISwapChain_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/bb174569(v=vs.85).aspx

<span class="keyward">IDXGISwapChain</span>を作成している部分は下のものになります。

ドキュメント:<span class="keyward">IDXGIFactory::CreateSwapChain</span>
[(日本語)][CreateSwapChain_JP]
[(英語)][CreateSwapChain_EN]

[CreateSwapChain_JP]:https://msdn.microsoft.com/ja-jp/library/bb174537(v=vs.85).aspx
[CreateSwapChain_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/bb174537(v=vs.85).aspx

{% highlight c++ %}
// Scene::onInitの一部
//IDXGISwapChainの作成
hr = pFactory->CreateSwapChain(this->mpDevice.Get(), &swapChainDesc, this->mpSwapChain.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("IDXGISwapChainの作成に失敗");
}
{% endhighlight %}

<span class="keyward">IDXGIFactory::CreateSwapChain</span>の第1引数には<span class="keyward">ID3D11Device</span>を渡してください。
第2引数の<span class="keyward">DXGI_SWAP_CHAIN_DESC</span>で<span class="keyward">IDXGISwapChain</span>の設定を行います。

<div class="argument">
  <h4>DXGI_SWAP_CHAIN_DESC</h4>
  <ol>
    <li><span class="keyward">BufferDesc</span>
      <p>
        バッファの設定を行うためのものです。
        <span class="important">画面サイズやリフレッシュレートなど自由に設定できますが、ディスプレイによって最適な設定が存在しますのでそうなるように設定してください。</span>
        <span class="important">最適な設定にはサンプルコードのように<span class="keyward">IDXGIOutput::FindClosestMatchingMode</span>を利用することができますので、活用していきましょう。</span>
        <span class="important">また、全画面モードを使用する際は必ず最適な設定にする必要があります。</span>
        <span class="keyward">IDXGIOutput</span>はディスプレイなどを表すものになり、使い方はドキュメントとサンプルを参考にしてください。
        <br>ドキュメント：
        <br><span class="keyward">DXGI_SWAP_CHAIN_DESC</span>
        <a href="https://msdn.microsoft.com/ja-jp/library/bb173064(v=vs.85).aspx">(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/bb173064(v=vs.85).aspx">(英語)</a>
        <br><span class="keyward">IDXGIOutput</span>
        <a href="https://msdn.microsoft.com/ja-jp/library/bb174546(v=vs.85).aspx">(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/bb174546(v=vs.85).aspx">(英語)</a>
        <br><span class="keyward">IDXGIOutput::FindClosestMatchingMode</span>
        <a href="https://msdn.microsoft.com/ja-jp/library/bb174547(v=vs.85).aspx">(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/bb174547(v=vs.85).aspx">(英語)</a>
      </p>
    </li>
    <li><span class="keyward">SampleDesc</span>
      <p>
        マルチサンプリングの設定を行うためのものです。
        <br>ドキュメント:<span class="keyward">DXGI_SAMPLE_DESC</span>
        <a href="https://msdn.microsoft.com/ja-jp/library/bb173072(v=vs.85).aspx">(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/bb173072(v=vs.85).aspx">(英語)</a>
      </p>
    </li>
    <li><span class="keyward">BufferUsage</span>
      <p>
        バッファの使用法を指定します。
        バッファはシェーダリソースやレンダーターゲット、アンオーダードアクセスとして設定することができます。
        <br>ドキュメント:<span class="keyward">DXGI_USAGE</span>
        <a href="https://msdn.microsoft.com/ja-jp/library/bb173078(v=vs.85).aspx">(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/bb173078(v=vs.85).aspx">(英語)</a>
      </p>
    </li>
    <li><span class="keyward">BufferCount</span>
      <p>内部に持つバッファの数を指定します</p>
    </li>
    <li><span class="keyward">OutputWindow</span>
      <p>出力先のウィンドウをあわわす<span class="keyward">HWND</span>を渡します。<span class="keyward">HWND</span>についてはWINPAIについて調べてください。</p>
    </li>
    <li><span class="keyward">Windowed</span>
      <p>全画面モードかウィンドウモードかを表すフラグです。trueでウィンドウモードになります。</p>
    </li>
    <li><span class="keyward">SwapEffect</span>
      <p>
        フロントバッファの切り替えを行った時のバッファの内容をどうするかを指定するものになります。
        指定には<span class="keyward">DXGI_SWAP_EFFECT</span>を使用します。
        <br>ドキュメント:<span class="keyward">DXGI_SWAP_EFFECT</span>
        <a href="https://msdn.microsoft.com/ja-jp/library/bb173077(v=vs.85).aspx">(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/bb173077(v=vs.85).aspx">(英語)</a>
      </p>
    </li>
    <li><span class="keyward">Flags</span>
      <p>
        <span class="keyward">IDXGISwapChain</span>の動作を指定するフラグになります。
        指定できるものは<span class="keyward">DXGI_SWAP_CHAIN_FLAG</span>で定義されています。
        <br>ドキュメント:<span class="keyward">DXGI_SWAP_CHAIN_FLAG</span>
        <a href="https://msdn.microsoft.com/ja-jp/library/bb173076(v=vs.85).aspx">(日本語)</a>
        <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/bb173076(v=vs.85).aspx">(英語)</a>
      </p>
    </li>
  </ol>
</div>

<span class="keyward">IDXGISwapChain</span>の作成は以上になります。

<a name="RESIZE_BACKBUFFER"></a>
<h1 class="under-bar">バックバッファのリサイズ</h1>

<span class="keyward">IDXGISwapChain</span>を作成した後に画面サイズを変更するには<span class="keyward">IDXGISwapChain::ResizeBuffers</span>を使用します。
<span class="important">使用する際は<span class="keyward">IDXGISwapChain</span>が生成したバッファのビューなどを全て開放する必要がありますので、忘れないようにしてください。</span>

<span class="keyward">IDXGISwapChain::ResizeBuffers</span>には変更したい画面サイズとフォーマットやバッファの数、フラグも一緒に変更できます。
サンプルでは作成した時のものと同じものを設定しています。

ドキュメント：<span class="keyward">IDXGISwapChain::ResizeBuffers</span>
[(日本語)][ResizeBuffers_JP]
[(英語)][ResizeBuffers_EN]

[ResizeBuffers_JP]:https://msdn.microsoft.com/ja-jp/library/bb174577(v=vs.85).aspx
[ResizeBuffers_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/bb174577(v=vs.85).aspx

{% highlight c++ %}
// Scene::onResizeWindowの一部
DXGI_SWAP_CHAIN_DESC swapChainDesc;
this->mpSwapChain->GetDesc(&swapChainDesc);
if (!swapChainDesc.Windowed) {
  return;
}
this->mpBackBuffer.Reset();
this->mpBackBufferRTV.Reset();
auto hr = this->mpSwapChain->ResizeBuffers(swapChainDesc.BufferCount, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags);
if (FAILED(hr)) {
  throw std::runtime_error("バックバッファのサイズ変更に失敗");
}
this->initRenderTargetAndDepthStencil(width, height);
{% endhighlight %}

<a name="RESIZE_BACKBUFFER"></a>
<h1 class="under-bar">全画面モードとウィンドウモードの切替</h1>

続いて全画面モードとウィンドウモードの切り替えも行えます。
<span class="important">その際は<span class="keyward">DXGI_SWAP_CHAIN_DESC</span>の<span class="keyward">Flags</span>に<span class="keyward">DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH</span>を指定する必要があります。</span>

切り替えには<span class="keyward">IDXGISwapChain::SetFullscreenState</span>を使用します。
<span class="important">第1引数に全画面かウィンドウのどちらにするかを指定し、trueで全画面モードに、falseでウィンドウモードに切り替えます。</span>
<span class="important">また全画面モードに切り替える際は第2引数に渡す<span class="keyward">IDXGIOutput</span>でどのディスプレイを全画面にするかを決めることができます。</span>

<span class="important">全画面モードにするときはディスプレイが対応できるサイズを指定しないと、パフォーマンスが落ちますので注意してください。</span>

<span class="important">また、全画面モードとウィンドウモードの切り替えを行う際は、<span class="keyward">IDXGISwapChain</span>作成時には必ずウィンドウモードで作成してください。</span>

ドキュメント：<span class="keyward">IDXGISwapChain::SetFullscreenState</span>
[(日本語)][SetFullscreenState_JP]
[(英語)][SetFullscreenState_EN]

[SetFullscreenState_JP]:https://msdn.microsoft.com/ja-jp/library/bb174579(v=vs.85).aspx
[SetFullscreenState_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/bb174579(v=vs.85).aspx

{% highlight c++ %}
// Scene::onKeyUpの一部
DXGI_SWAP_CHAIN_DESC desc;
this->mpSwapChain->GetDesc(&desc);
if (desc.Windowed) {
  //全画面モードに切り替える際はどのディスプレイを対象にするか決めれる
  if (DXGI_ERROR_NOT_FOUND == this->mpAdapter->EnumOutputs(0, pOutput.GetAddressOf())) {
    throw std::runtime_error("アダプターの出力先が見つかりません。");
  }
}
auto hr = this->mpSwapChain->SetFullscreenState(desc.Windowed, nullptr);
if (FAILED(hr)) {
  throw std::runtime_error("フルスリーンモードとウィンドウモードの切り替えに失敗。");
}
{% endhighlight %}

<a name="SUMMARY"></a>
<h1 class="under-bar">まとめ</h1>

今回のパートではDX11の初期化について見てきました。
これでフルスクラッチでDX11のアプリケーションを作成することもできるようになると思います。
(WINAPIについての知識も必要となりますが、まぁ大丈夫でしょう。）

ここまでくればもうDX11について説明することはほとんどありません。
あとは興味のある分野について調べて<span class="important">DX11を利用して</span>実装していくだけになります。

<a name="SUPPLEMENTAL"></a>
<h1 class="under-bar">補足</h1>

<div class="supplemental">
  <h3>D3D11CreateDeviceAndSwapChain</h3>
  <p>
    DX11では<span class="keyward">ID3D11Device</span>と<span class="keyward">IDXGISwapChain</span>を一緒に作成できる関数が用意されています。
    使い方は今回説明した内容とほぼ一緒ですので、紹介だけにとどめておきます。
  </p>
  <p>
    ドキュメント：
    <span class="keyward">D3D11CreateDeviceAndSwapChain</span>
    <a href="https://msdn.microsoft.com/ja-jp/library/ee416033(v=vs.85).aspx">(日本語)</a>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476083(v=vs.85).aspx">(英語)</a>
  </p>
</div>

<div class="supplemental">
  <h3>IDXGISwapChain::Present</h3>
  <p>
    <span class="important">バックバッファに描画をし終えた後、<span class="keyward">IDXGISwapChain::Present</span>を使ってその内容をディスプレイにおくる必要があります。</span>
    呼び出すのを忘れると画面になにも表示されないのでかなり重要な事なのですが、説明するタイミングがなかったのでここで紹介しました。

    {% highlight c++ %}
//DXSample.cpp DXSample::render関数の一部

//     ...
// 何かを描画する
//     ...

//画面に表示したいものを描画し終えたらフロントバッファを切り替える。
this->mpSwapChain->Present(1, 0);
    {% endhighlight %}
  </p>
  <p>
    ドキュメント：
    <span class="keyward">IDXGISwapChain::Present</span>
    <a href="https://msdn.microsoft.com/ja-jp/library/bb174576(v=vs.85).aspx">(日本語)</a>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/bb174576(v=vs.85).aspx">(英語)</a>
  </p>
</div>

<table class="table table-condensed">
  <tbody>
    <tr>
      <td class="left"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/query">＜前</a></td>
      <td class="center"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}">トップ</a></td>
      <td class="right"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/initialize">次＞</a></td>
    </tr>
  </tbody>
</table>
