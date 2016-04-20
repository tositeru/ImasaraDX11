---
layout: default
title: "シェーダを使った画面クリア"
categories: part
description: "まず初めに画面全体をGPUを使って単色で塗りつぶす単純なプログラムを作ることを題材に、DX11を使う上で最も重要なシェーダとその使い方について説明していきます。"
---
<section>
  <h3 class="under-bar">前書き</h3>
  <p>まず初めにDirect3D11(以下、DX11と省略)とはなにか？と聞かれたら、私はGPUを扱うためのAPIですと答えます。</p>
  <P>
    GPUについてはご存知でしょうか？
    GPUとは並列処理に特化したプロセッサです。
    登場した当初は画像処理目的にのみに使われていましたが、
    近年ではCPUをはるかに超える演算能力に着目して、
    物理演算や画像認識、Deep Learningなど幅広い用途でも使われるようになっています。(それらをまとめてGPGPUと呼ばれています。)
  </p>
  <p>
    なので、DX11を使用する際はCPUとGPUの2つのプロセッサが存在していることを認識して、
    今、どちらを触っているかをしっかり把握してプログラミングしていかなければなりません。
  </p>
  <p>
    と書くと難しく感じられますが、
    実際には、<b>シェーダ(英訳:Shader)</b>というGPU上で実行される言語を使用しますのでそこまで注意を払う必要はなく、
    CPUはDX11のAPIを、GPUはシェーダを使ってコーディングしていく形になります。
  </p>
  <p>
    DX11では<b>HLSL</b>と呼ばれるシェーダ言語を使います。
    HLSLもc言語の文法をベースにしていますので、すぐに慣れることでしょう。
    ただ、様々なお約束を覚える必要はありますが。
  </p>
</section>
<section>
  <h1 class="under-bar">{{page.title}}</h1>
  <p>
    まず初めに画面全体をGPUを使って単色で塗りつぶす単純なプログラムを作ることを題材に、
    DX11を使う上で最も重要なシェーダとその使い方について説明していきます。
    <br>今パートに対応しているサンプルプロジェクトは<b>Part01_ClearScreen</b>になります。
  </p>
  <div class="summary">
    <h4>概要</h4>
    <ol>
      <li>
        シェーダの作成
        <ul>
          <li>コンピュートシェーダ</li>
        </ul>
      </li>
      <li>
        シェーダの実行
        <ul>
          <li>
            実行命令<br>
            <b>ID3D11DeviceContext::Dispatch関数</b>
          </li>
          <li>
            実行させるのに必要な設定<br>
            <b>ID3D11DeviceContext::CSSetShader関数</b>と<b>ID3D11DeviceContext::CSSetUnorederAccessViews関数</b>
          </li>
        </ul>
      </li>
      <li>
        シェーダのコンパイル
        <ul>
          <li>
            実行時でのコンパイル<br>
            <b>D3DCompileFromFile関数</b>と<b>D3DCompile</b>
          </li>
          <li>
            オフラインでのコンパイル<br>
            <b>fxc.exe</b>
          </li>
        </ul>
      </li>
    </ol>
  </div>
</section>
<section>
  <h1 class="under-bar">1.シェーダの作成</h1>
  {% highlight hlsl %}
//ClearScreen.hlsl
RWTexture2D<float4> screen : register(u0);
[numthreads(1, 1, 1)]
void main( uint2 DTid : SV_DispatchThreadID )
{
  screen[DTid] = float4(1, 1, 0, 1);
}
  {% endhighlight %}
  <p>
    今回のサンプルでは<b>コンピュートシェーダ(英訳:Compute Shader)</b>を使用して画面を単色で塗りつぶしています。
  </p>
  <p>
    一口にシェーダといってもいくつかの種類があり、
    大きく分けて<b>グラフィックスパイプライン用のシェーダ</b>と<b>GPGPU用のシェーダ</b>があります。
    (グラフィックスパイプラインについては後のパートで説明します。)
  </p>
  <p>
    コンピュートシェーダはGPGPU目的でGPUを動作させるためのシェーダになり、Direct3D11から登場しました。
    コンピュートシェーダは現在のハイエンドゲームの制作において欠かすことのできないシェーダであり、シェーダの中でコーディングする制限が一番少ないものになります。
  </p>
  <p>
  話をソースコードに戻しまして、
  ClearScreen.hlslの以下の部分で画面を塗りつぶしています。
  {% highlight hlsl %}
//ClearScreen.hlsl main関数内
screen[DTid] = float4(1, 1, 0, 1);
  {% endhighlight %}
  </p>
  <p>
    screenが画面を表す<b>リソース(英訳:Resource)</b>になり、サンプルでは全画面を黄色を表す"float4(1, 1, 0, 1)"で塗りつぶしています。
    シェーダ上では<b>基本的に色を光の3原色である赤緑青とアルファ(不透明度)の4色で表現しており、0～1の範囲で調節します。</b>
    float4(赤, 緑, 青, アルファ)となっています。
  </p>
  <p>
    screenは<b>RWTexture2D&lt;float4&gt;オブジェクト</b>になります。
    RWTexture2Dは<b>読み書き可能<a href="#A1" class="attension">[補足]</a>な2次元テクスチャ</b>で、C++のテンプレートみたいに"<...>"で要素の型を指定しています。
    サンプルではfloat4を指定しているので、screenはシェーダ内でfloat4型を読み書き可能な2次元テクスチャとなります。
    2次元テクスチャの詳細はあとのパートで詳しく説明しますが、C++でいうところの2次元配列のようなもので、実際screenの各要素には配列のようにアクセスできます。
  </p>
  <p>
    例えば、screenの(100, 200)にアクセスしたい場合は、
    {% highlight hlsl %}
uint2 pos;
pos.x = 100;
pos.y = 200;
screen[pos] = ...;
    {% endhighlight %}
    としてください。
    C++のように a[100][200]と角括弧を2つ使わないので注意してください。
  </p>
  <p>
    さて、ここまで当たり前のようにfloat4やuint2といった型を使っていましたが、どのようなものか想像できるでしょうか？
    <b>float4はfloatを4つもつ型</b>で、<b>uint2はuintを2つもつ型</b>になります。
    float3だとfloatを3つ、uint4だとuintを4つもち、型名そのままの意味になります。
    当然、<b>float型とuint型も単体</b>であり、ほかには<b>int型</b>と<b>bool型</b>も同じように使えます。
  </p>
  <p>
    これらの型は一見すると配列のように見えますが、各要素へのアクセスは配列とは異なり、
    float4やuint4だと<b>x,y,z,w</b>か<b>r,b,g,a</b>を使ってアクセスします。
    float2やuint2だと<b>x,y</b>か<b>r,b</b>となり、
    <b>配列での0番目がx(r)に、1番目がy(b)に、2番目がz(b)、3番目がw(a)となっています。</b>
    xyzwとrgbaはそれぞれ同じ場所をさしています。c++言語でいうところの共用体みたいなものです。
  </p>
  <p>
    また、以下のようにすると、xとyに１が入り、
    {% highlight hlsl %}
float4 a;
a.xy = 1;
    {% endhighlight %}
    次のようにすると、wとy,xに２が入ります。
    {% highlight hlsl %}
a.wyx = 2;
    {% endhighlight %}
    これは<b>スウィズル(英訳:Swizzle)</b>と呼ばれる機能でシェーダ言語独特の文法になります。
  </p>
  <p>
  またこのような書き方も可能です。
  {% highlight hlsl %}
float3 a = {1, 2, 3};
float4 b = float4(a, 4);//bはfloat4(1, 2, 3, 4)になる。
  {% endhighlight %}
  </p>
  <p>
    より詳しい説明は以下のドキュメントを参照してください。ここでは触れていない<b>行列</b>を表す変数についても説明されています。
    <br>ドキュメント：
    <a href="https://msdn.microsoft.com/ja-jp/library/bb509634(v=vs.85).aspx">成分ごとの演算(日本語)</a>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/bb509634(v=vs.85).aspx">Per-Component Math Operations(英語)</a>
  </p>
  <p>
    ここまで、シェーダ内の画面を塗りつぶす部分について説明しました。
    次はそのシェーダを実行する手順について見ていきます。
    ClearScreen.hlslのいくつか部分には触れていませんが、それらはCPU側で使ったり、指定したりするものなので
    関連する所が出てきましたら合わせて説明していきます。
  </p>
</section>

<section>
  <h1 class="under-bar">2.シェーダの実行</h1>
  {% highlight c++ %}
//Scene::onRender()の一部
//実行するシェーダをGPUに設定する
this->mpImmediateContext->CSSetShader(this->mpCSClearScreen.Get(), nullptr, 0);
//シェーダのscreenとして扱うリソースを設定する。
std::array<ID3D11UnorderedAccessView*, 1> ppUAVs = { {
  this->mpScreenUAV.Get()
} };
std::array<UINT, 1> initCounts = { { 0u, } };
this->mpImmediateContext->CSSetUnorderedAccessViews(
  0,
  static_cast<UINT>(ppUAVs.size()),
  ppUAVs.data(),
  initCounts.data());
//ClearScreen.hlslの実行
this->mpImmediateContext->Dispatch(this->mWidth, this->mHeight, 1);
  {% endhighlight %}
  <h3 class="under-bar">実行命令</h3>
  <p>
    DX11を使ったコードは長くなりがちですが、行っていることは難しくありません。<br>
    上のコードの
    {% highlight c++ %}
//Scene::onRender()の一部
this->mpImmediateContext->Dispatch(this->mWidth, this->mHeight, 1);
    {% endhighlight %}
    でシェーダを実行しています。
  </p>
  <p>
    <b>Dispatch関数</b>の引数は、上にある<b>ClearScreen.hlslのDTid引数とmain関数の呼び出し回数に影響を与えます。</b>
    <br>ドキュメント
    <a href="https://msdn.microsoft.com/ja-jp/library/ee419587(v=vs.85).aspx">ID3D11DeviceContext::Dispatch(日本語)</a>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476405(v=vs.85).aspx">ID3D11DeviceContext::Dispatch(英語)</a>
    {% highlight c++ %}
//ClearScreen.hlsl
void main( uint2 DTid : SV_DispatchThreadID )
    {% endhighlight %}
  </p>
  <p>
    <br>
    Dispatch(2,2,1)にすると、ClearScreen.hlslのmain関数が2x2x1=4回呼び出され、
    DTidにはそれぞれuint2(0, 0), uint2(0, 1), uint2(1, 0), uint2(1, 1)の値が渡されます。
  </p>
  <p>
    Dispatch(100, 200, 1)にすると、main関数は100x200x1=20000回呼び出され、
    DTidには<br>
    uint2(0, 0)～uint2(99, 0)<br>
    uint2(0, 1)～uint2(99, 1)<br>
    ...<br>
    uint2(0, 199)～uint2(99, 199)<br>
    の値が渡されます。
  </p>
  <p>
    今回は画面を塗りつぶすので、画面の横幅と縦幅を指定しています。
    なので、サンプルの画面サイズはデフォルトでは1280x720なので、main関数は1280x720x1=921600回呼び出され、
    各々のDTidはuint2(0, 0)～uint2(1279, 719)の値が渡されます。
  </p>
  <p>
    実のところ、ClearScreen.hlslの次の部分もDTidの値に影響があります。
    {% highlight hlsl %}
//ClearScreen.hlsl
[numthreads(1,1,1)]
    {% endhighlight %}
    <b>numthreads属性</b>はコンピュートシェーダを使う上で非常に重要な意味を持つのですが、今回は説明しません。
    詳しく知りたいという方は、MSDNのドキュメントをご覧ください。<br>
    <a href="https://msdn.microsoft.com/ja-jp/library/ee422317(v=vs.85).aspx">numthreads(日本語)</a>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff471442(v=vs.85).aspx">numthreads(英語)</a>
  </p>
  <p>
    ここまでシェーダの実行の仕方について見てきました。<b>コンピュートシェーダではDispatch関数を使うことでシェーダを実行します。</b>
    C++の関数呼び出しと比べるとかなり特殊な性質をもっていますが、これは<b>GPUの大量のスレッドを同時に実行可能</b>という特徴を生かすためこうなってます。
    サンプルではClearScreen.hlslにはmain関数とは別にclearByOneThread関数というfor文を使ったC++で画面クリアをする場合と同じコードになるよう書いたものも用意していますので、一度目を通してみてください。
    このシェーダとClearScreen.hlslは同じことをしていますが、処理速度はGPUの性質を生かしているClearScreen.hlslの方が速くなります。
    {% highlight hlsl %}
//ClearScreen.hlslのclearByOneThread関数を簡略したもの。
[numthreads(1, 1, 1)]
void clearByOneThread(uint2 DTid : SV_DispatchThreadID)
{
  //screenのサイズを取得している
  uint2 size;
  screen.GetDimensions(size.x, size.y);
  //全ピクセルクリアー
  for (uint y = 0; y < size.y; ++y) {
    for (uint x = 0; x < size.x; ++x) {
      screen[uint2(x, y)] = float4(0.3f, 1, 0.3f, 1);
    }
  }
}
    {% endhighlight %}
  </p>
  <p>
    あと、単にDispatch関数だけを呼び出すだけではGPUから見ると実行するには情報不足です。
    次は上のコードの残りの部分である、GPUがどのシェーダをどのようなリソースを使って実行するかその設定方法を説明していきます。
  </p>
  <h3 class="under-bar">実行させるのに必要な設定</h3>
  <p>
    シェーダの実行の仕方がわかりましたので、次はGPUがどのシェーダをどのようなリソースを使って実行するかその設定方法を説明していきます。
  </p>
  <p>
    GPUが実行するシェーダの設定は次のコードで行います。
    {% highlight c++ %}
//Scene::onRender()の一部
this->mpImmediateContext->CSSetShader(this->mpCSClearScreen.Get(), nullptr, 0);
    {% endhighlight %}
  </p>
  <p>
    <b>CSSetShader関数</b>はコンピュートシェーダの設定を行う関数で、
    第一引数にコンピュートシェーダを表す<b>ID3D11ComputeShader</b>を渡します。
    ID3D11ComputeShaderの生成は次のシェーダのコンパイルの項目で説明します。
    残りの引数は<b>動的シェーダリンク(英訳:Dynamic Shader Linkage)</b>というシェーダでオブジェクト指向言語でのインターフェイスとクラスを扱うときに使いますが、
    Direct3D12では廃止になるようなので、無視します。
  </p>
  <p>
    次に使用するリソースの設定は以下の<b>CSSetUnorderedAccessViews関数</b>で行ってます。
    {% highlight c++ %}
//Scene::onRender()の一部
//シェーダのscreenとして扱うリソースを設定する。
std::array<ID3D11UnorderedAccessView*, 1> ppUAVs = { {
  this->mpScreenUAV.Get()
} };
std::array<UINT, 1> initCounts = { { 0u, } };
this->mpImmediateContext->CSSetUnorderedAccessViews(
  0,
  static_cast<UINT>(ppUAVs.size()),
  ppUAVs.data(),
  initCounts.data());
    {% endhighlight %}
  </p>
  <p>
    リソースにはいくつか種類があり、
    <ul>
      <li><b>定数バッファ (英訳:ConstantBuffer)</b></li>
      <li><b>シェーダリソースビュー (英訳:ShaderResourceView)</b></li>
      <li><b>サンプラステート (英訳:SamplerState)</b></li>
      <li><b>アンオーダードアクセスビュー (英訳:UnorderedAccessView, 以後、UAVと省略)</b></li>
    </ul>
    の4種類用意されています。
  </p>
  <p>
    今回のサンプルでは画面を塗りつぶすことを目的としているので、
    <b>リソースの中で唯一書き込みが可能なUAVを使用しています。</b>
  </p>
  <p>
    話をサンプルに戻しまして、<b>CSSetUnorderedAccessViews関数</b>の引数について見ていきます。
    <br>ドキュメント：
    <a href="https://msdn.microsoft.com/ja-jp/library/ee419586(v=vs.85).aspx">ID3D11DeviceContext::CSSetUnorderedAccessViews(日本語)</a>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476404(v=vs.85).aspx">ID3D11DeviceContext::CSSetUnorderedAccessViews(英語)</a>
    <ul>
      <li>第1引数：GPUに設定する開始スロット番号
        <p>
          スロットとは識別番号のようなもので、C++でいうところの配列の添え字みたいなものです。
          {% highlight hlsl %}
//ClearScreen.hlsl
RWTexture2D<float4> screen : register(u0);
          {% endhighlight %}
          上のコードの<b>register(u0)</b>がスロット番号にあたります。
          CPUからGPUにリソースを設定するときは各リソースに関連付けられているスロット番号を使って設定します。
        </p>
        <p>
          <b>"u0"はUAVの0番目のスロットを表しています</b>。
          リソースの後ろに": register(u0)"と書くと、そのリソースはUAVの0番目のスロットと関連付けられます。
          "u4"とするとUAVの4番目のスロット、"u10"とすると10番目のスロットになります。
        </p>
        <p>
          <b>1度リソースに設定したスロットは当然ながら他のリソースには使用できません。</b>
          また、スロットの設定を省略することもできます。
          その際は、コンパイラがほかのリソースで<b>使われているスロットと被らないよう自動的に割り当ててくれます。</b>
        </p>
      </li>
      <li>第2引数：一度に設定するUAVの個数</li>
      <li>第3引数：<b>ID3D11UnorderedAccessView</b>* の配列
        <p>
          <b>ID3D11UnorderedAccessViewはUAVを表すものになります。</b>
          この引数に渡す配列の要素数は必ず、第2引数に渡した値より大きくしてください。
        </p>
      </li>
    </ul>
  </p>
  <p>
    第4引数は今回は意味を持たないので、省略します。
  </p>
  <div class="topic">
    <h4>ID3D11DeviceContext</h4>
    <p>
      ところで、ここまで度々出てきたmpImmediateContextについて触れていませんでした。
      mpImmediateContextは<b>ID3D11DeviceContext</b>* になります。
      ID3D11DeviceContextは<b>GPUにシェーダやリソースの設定を行ったり、シェーダの実行、リソースのコピーなどを行うときに使用</b>するものです。
      DX11ではこれと後々出てくる<b>ID3D11Device</b>の2つを使って処理を行っていきますので、
      <b>この2つを理解できればDX11を理解したといってもいいくらい重要なもの</b>になります。
    </p>
  </div>
  <p>
    シェーダの実行の仕方については以上になります。
    最後に、シェーダのコンパイルの方法について見ていきましょう。
  </p>
</section>

<section>
  <h1 class="under-bar">3.シェーダのコンパイル</h1>
  <p>
    シェーダを実行するためには一度コンパイルする必要があります。
    コンパイルは実行時とオフラインの両方可能です。
  </p>
  <h3 class="under-bar">実行時のコンパイル</h3>
  {% highlight c++ %}
//Scene::onInit()の一部
//実行中にシェーダをコンパイルし、ID3D11ComputeShaderを作成する
  std::array<D3D_SHADER_MACRO, 2> macros = { {
    {"DEFINE_MACRO", "float4(0, 1, 1, 1)"},
    {nullptr, nullptr},
  } };
  UINT compileFlag = 0;
  compileFlag |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
  const char* entryPoint = "main";
  const char* shaderTarget = "cs_5_0";
  Microsoft::WRL::ComPtr<ID3DBlob> pShaderBlob, pErrorMsg;
  //シェーダのコンパイル
  hr = D3DCompileFromFile(
    L"ClearScreen.hlsl",
    macros.data(),
    nullptr,
    entryPoint,
    shaderTarget,
    compileFlag,
    0,
    pShaderBlob.GetAddressOf(),
    pErrorMsg.GetAddressOf());
  //ID3D11ComputeShaderの作成
  hr = this->mpDevice->CreateComputeShader(
    pShaderBlob->GetBufferPointer(),
    pShaderBlob->GetBufferSize(),
    nullptr,
    &this->mpCSClearScreenWithConstantBuffer);
  {% endhighlight %}
  <p>
    上のコードのD3DCompileFromFile関数でシェーダのコンパイルを行っています。
    引数がたくさんありますが、必ず必要となるものは以下のものになります。
    <br>ドキュメント：
    <a href="https://msdn.microsoft.com/ja-jp/library/windows/desktop/hh446872(v=vs.85).aspx">D3DCompileFromFile(英語)</a><br>
    <ul>
    	<li>
        第1引数 : ファイルパス
        <p>コンパイルしたいシェーダの<b>ファイルパス</b>になります。</p>
      </li>
    	<li>
        第4引数 : エントリポイント
        <p>
          GPUでシェーダを実行する際、<b>一番初めに呼び出される関数の名前</b>を指定します。
          ClearScreen.hlslなら"main"を指定します。
          シェーダファイルには複数のエントリポイントとなる関数を定義することができますが、
          それらの関数を使うためにはエントリポイントに<b>各々の関数名を指定して個別にコンパイルする必要</b>があります。
        </p>
      </li>
    	<li>
        第5引数 : シェーダターゲット
        <p>
          シェーダの種類とその<b>シェーダモデル(英訳:Shader Model)</b>を表す文字列を指定します。
          例えば、コンピュートシェーダのシェーダモデル5.0でコンパイルしたい場合は以下の文字列を渡してください。
        </p>
        <figure class="highlight">
          <pre><code class="language-c--" data-lang="hlsl">
const char* shaderTarget = "cs_5_0";
          </code></pre>
        </figure>
        <p>
          上のshaderTargetの"cs"がコンピュートシェーダを表し、"5_0"がシェーダモデル5.0を表しています。
          シェーダモデルは<b>シェーダのバージョン</b>のようなもので、
          現在シェーダモデル5.1が一番新しいものになります。(2016年度ぐらいには6.0が出てきそうですが)
          シェーダモデル5.1はDirect3D11.3とDirect3D12に対応したGPU上で実行できるシェーダのバージョンになります。
          シェーダモデルの詳細はMSDNの方を参照してください。
          ただ、シェーダターゲットの文字列を確認したい程度なら日本語翻訳されたもので十分なのですが、
          翻訳されたサイトは情報が古いのと、リニューアル後のMSDN(英語)の方が内容が充実していますので、出来るならリニューアル後の方を参考にした方がいいです。<br>
          ドキュメント：
          <a href="https://msdn.microsoft.com/ja-jp/library/ee418332(v=vs.85).aspx">シェーダー モデルとシェーダー プロファイル(日本語)</a>
          <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/jj215820(v=vs.85).aspx">Specifying Compiler Targets(英語)</a>
        </p>
      </li>
    	<li>
        第8引数 : コンパイルした結果を受け取るID3DBlob*
        <p>
          コンパイルされたシェーダバイナリを受け取る<b>ID3DBlob</b>を指定してください。
          <b>ここで指定した変数を使って、ID3D11ComputeShaderを作成します。</b>
          ID3DBlobはシェーダのコンパイルの結果を受け取るときに使われるもので、メンバに受け取ったデータへのポインタとそのサイズを取得する関数が用意されています。
        </p>
      </li>
    </ul>
  </p>
  <div class="topic">
    <h4>機能レベル</h4>
    <p>
      第5引数のシェーダターゲットと関連するものとして<b>機能レベル(英訳:Feature Levels)</b>というものがあります。
      GPUは世代によってハードウェアの構造が異なるため、
      DX11以降ではそれに対応するために機能レベルを使って使用できる機能を区別しています。
      シェーダモデル5.1は機能レベル12_0以降が対応しており、部分的に11_1と11_0が対応しています。
      サンプルでは以降、機能レベル11_0が対応しているシェーダモデル5.0を使ってシェーダをコンパイルしていきます。
      下のリンクは機能レベルのドキュメントになります。日本語訳は少し情報が古いので、英語の方も参考にしてください。<br>
      ドキュメント:
      <a href="https://msdn.microsoft.com/ja-jp/library/ee422086(v=vs.85).aspx">ダウンレベルのハードウェア上の Direct3D 11(日本語)</a>
      <a href="https://msdn.microsoft.com/en-us/library/ff476876(v=vs.85).aspx">Direct3D feature levels(英語)</a>
    </p>
  </div>
  <p>
  <br>
    さて先に、GPUにシェーダを設定するときに使用したCSSetShader関数に渡す<b>ID3D11ComputeShaderの作り方</b>を見ていきます。
    といっても簡単で、ID3D11Deviceとコンパイルしたシェーダバイナリを用意するだけでできます。
  </p>
  {% highlight c++ %}
//Scene::onInit()の一部
hr = this->mpDevice->CreateComputeShader(
  pShaderBlob->GetBufferPointer(),
  pShaderBlob->GetBufferSize(),
  nullptr,
  &this->mpCSClearScreenWithConstantBuffer);
  {% endhighlight %}
  <p>
    <b>ID3D11Device::CreateComputeShader関数</b>には単純にコンパイルされたシェーダと作成したいID3D11ComputeShaderを渡すだけです。
    第3引数のnullptrは動的シェーダリンクに関係するものなので無視します。
  </p>
  <div class="topic">
    <h4>ID3D11Device</h4>
    <p>
      ID3D11Deviceはシェーダやリソース、そのほかGPUに関係するものの作成や使っているGPUの機能レベルや対応している機能の取得などができます。
      DX11では<b>ID3D11Deviceが作成したものを使ってGPUを操作する</b>ので最も重要なものと言えるでしょう。 当然、DX11を使う際は一番最初に作成しなければいけませんが、そのやり方は別パートで説明します。
    </p>
  </div>
  <p>
    <br>
    実行時のシェーダのコンパイルで最低限必要となる引数とGPUに設定するために必要になるID3D11ComputeShaderの生成方法についてはこれで以上になります。
    この後はD3DCompileFromFileの残りの引数について説明していきます。
  </p>
  <p>
    <ul>
      <li>
        第9引数:エラーメッセージを受け取るID3DBlob
        <p>
          これはコンパイルエラーが発生した時のエラーメッセージを受け取るID3DBlobになります。
          コンパイルエラーが発生したときは第9引数に渡したID3DBlobには、文字列が設定されます。
          使い方は、以下のコードになります。
        {% highlight c++ %}
HRESULT hr = D3DCompileFromFile(
  L"ClearScreen.hlsl",
  macros.data(),
  nullptr,
  entryPoint,
  shaderTarget,
  compileFlag,
  0,
  pShaderBlob.GetAddressOf(),
  pErrorMsg.GetAddressOf());
if (FAILED(hr)) {//コンパイルエラー起きたかチェック
  if (pErrorMsg) {//エラーメッセージがあるかチェック
    OutputDebugStringA(static_cast<char*>(pErrorMsg->GetBufferPointer()));
  }
  throw std::runtime_error("ClearScreen.hlslのコンパイルに失敗");
}
        {% endhighlight %}
        </p>
      </li>
      <li>
        第6、7引数：コンパイルフラグ
        <p>
          第6引数はシェーダのコンパイルフラグになり、
          コンパイルしたシェーダに<b>デバッグ情報</b>をつけるか、<b>最適化レベルの設定</b>などのフラグがあります。
          詳細はMSDNを参考にしてください。<br>
          ドキュメント:
          <a href="https://msdn.microsoft.com/ja-jp/library/ee415892(v=vs.85).aspx">D3D10_SHADER 定数(日本語)</a>
          <a href="https://msdn.microsoft.com/ja-jp/library/windows/desktop/gg615083(v=vs.85).aspx">D3DCOMPILE Constants(英語)</a>
        </p>
        <p>
          第7引数はエフェクト(Effect)というグラフィックスパイプラインのシェーダをまとめたものをコンパイルするとき使うものですが、DX11以降、<b>エフェクトは廃止予定</b>になっていますので、説明を省きます。
        </p>
      </li>
      <li>
        第2引数：マクロの定義
        <p>
          C言語と同じようにシェーダも<b>マクロに対応</b>しており、使い方も同じです。
          D3DCompileFromFile関数に渡す際は<b>D3D_SHADER_MACROの配列として渡し</b>、
          <b>末尾にはメンバをnullptrで埋めたものを設定します。</b>
          {% highlight c++ %}
std::array<D3D_SHADER_MACRO, 2> macros = { {
    {"DEFINE_MACRO", "float4(0, 1, 1, 1)"},
    {nullptr, nullptr},//末尾はnullptrで埋める
  } };
hr = D3DCompileFromFile(
  L"ClearScreenWithConstantBuffer.hlsl"
   macros.data(),
   nullptr,
   entryPoint,
   shaderTarget,
   compileFlag,
   0,
   pShaderBlob.GetAddressOf(),
   pErrorMsg.GetAddressOf());
          {% endhighlight %}
        </p>
      </li>
      <li>
        第3引数：#includeキーワードが行う処理を定義した<b>ID3DIncludeの派生クラス</b>
        <p>
          これはシェーダ内に#includeキーワードがあったときのファイルを検索する方法や読み込みを制御するためのものです。
          D3DCompileFromFileを使ってHLSLをコンパイルする際は<b>自前でファイルを読み込む処理を作る必要があります。</b>
          引数にはID3DIncludeを継承したクラスを渡します。
          <b>この引数がnullptrだと、シェーダ内で#includeを使うとエラー</b>になってしまいますので注意してください。
          詳細は以下のサイトを参照してください。使っているのはID3D10Includeですが使い方は同じです。<br>
          <a href="http://wlog.flatlib.jp/?blogid=1&query=preprocess">http://wlog.flatlib.jp/?blogid=1&query=preprocess</a><br>
        </p>
      </li>
    </ul>
  </p>
  <p>
    D3DCompileFromFile関数については以上になります。
    この関数に似たものとして<b>D3DCompile関数</b>がありますがそちらはシェーダを表す文字列からコンパイルする関数になります。
    コンパイルエラーが起きたときの識別用の文字列を指定する以外は引数は同じになります。
    <br>ドキュメント:<br>
    <a href="https://msdn.microsoft.com/ja-jp/library/ee416450(v=vs.85).aspx">D3DCompile(日本語)</a>
    <a href="https://msdn.microsoft.com/ja-jp/library/windows/desktop/dd607324(v=vs.85).aspx">D3DCompile(英語)</a>
  </p>
  <h3 class="under-bar">オフラインでのコンパイル</h3>
  <p>
    DX11のシェーダ言語であるHLSLではMicrosoftが用意している<b>fxc.exeで事前にコンパイルすることが可能</b>です。
    fxc.exeの場所は<b>"Program Files (x86)\Windows Kits\10\bin\"以下のフォルダー</b>にあります。
    <b>VisualStudioの開発者用コマンドプロンプト</b>を使用すれば環境変数などの設定をしなくとも使用できますし、
    サンプルプロジェクトにsetupFXCPath.batを用意しているので、コマンドプロンプトからそれを起動していただければ使えるようになります。
  </p>
  <p>
    fxc.exe自体はD3DCompileFromFile関数と同じように使え、#includeキーワードにも対応しています。
    コンパイルフラグはオプションで指定しますが、他のコマンドとは異なり"-"ではなく"/"を前につけて指定します。(Windows的にはこちらがデフォルト？)
    ヘルプを見てもらえれば使い方はわかりますが、ヘルプの見方は"fxc /?"または"fxc /help"になっていますので、混乱しないよう注意してください。
    {% highlight bat %}
@rem 例
fxc /T cs_5_0 /Fo binary.cso ClearScreen.hlsl
    {% endhighlight %}
    後は実行時に出力ファイルを読み込んで、CreateComputeShader関数に渡せばID3D11ComputeShaderが作成できます。
  </p>
  <p>
    ちなみにサンプルのClearScreen.hlslは<b>プロジェクトのビルド時にコンパイルを行うようプロパティから設定</b>しています。
    その際、出力されるファイルはClearScreen.csoと拡張子がデフォルトで".cso"になりますので、この機能を使うときは覚えておいてください。
  </p>
</section>

<section>
  <h1 class="under-bar">まとめ</h1>
  <p>かなり長くなりましたが、DX11でのシェーダの使い方の説明は以上になります。</p>
  <p>
    今回の内容を踏まえたシェーダを実行したいときの流れは
    <div class="summary">
      <ol>
      	<li>実行したいシェーダを作る</li>
      	<li>
          シェーダを実行するのに必要なものID3D11Deviceを使って作る<br>
          <ul>
        		<li>シェーダをコンパイルして(またはオフラインコンパイルしたものを読み込んで)ID3D11ComputeShaderを作成する</li>
        		<li>シェーダで使うリソースを作る</li>
          </ul>
        </li>
      	<li>ID3D11DeviceContextを使ってGPUにシェーダとリソースを設定する</li>
        <li>シェーダの実行</li>
      </ol>
    </div>
    になります。
    今回説明した内容は<b>以降のパートで共通して使う</b>ものなので、一度に覚えようとせずDX11を使ったいろいろなシェーダやソースを見て慣れていけばいいでしょう。
  </p>
</section>

<section>
  <h2 class="under-bar">補足</h2>
  <div class="supplemental">
    <h4>画面クリアの便利関数</h4>
    <p>
      今回、画面を塗りつぶすシェーダを書きましたが
    	<b>ID3D11DeviceContext::ClearUnorderedAccessViewFloat関数</b>を使えばシェーダを使わずとも画面を塗りつぶすことができます。
      {% highlight c++ %}
// 例
float value[4] = {1, 0.7f, 1, 1};//左から赤、緑、青、アルファ
this->mpImmediateContext->ClearUnorderedAccessViewFloat(this->mpScreenUAV.Get(), value);
    {% endhighlight %}
    UAVには他にもUINT型で値を設定する<b>ID3D11DeviceContext::ClearUnorderedAccessViewUint関数</b>も用意されています。
    ID3D11DeviceContextにはこういった関数がいくつか用意されていますので、随時紹介していきます。
    </p>
  </div>
  <div class="supplemental">
    <h4>RWTexture2Dのデータ読み込みの制限</h4>
    <a name="A1"></a>
    <p>
      1.シェーダの作成にて、RWTexture2Dはデータの読み書きができると書きましたが、
    	<b>読み込みに関してはuint型かfloat型のみに制限されています。</b>
    	また、RWと名の付くものはすべてこの制限を持ちます。
    	この制限は<b>DX12とDX11.3以降、Typed Unordered Access View Loads</b>として幾分か緩和されています。
    	<br><a href="https://msdn.microsoft.com/ja-jp/library/windows/desktop/dn903947(v=vs.85).aspx">Typed Unordered Access View (UAV) Loads(英語)</a><br>
    	データをuint型に変換するなど回避策はありますが、直接リソースから読み込むことはできないので、DX11.2以前のGPUを使うときは注意してください。
    </p>
  </div>
</section>
