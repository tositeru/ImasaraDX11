---
layout: default
title: "シェーダを使った画面クリア"
categories: part
description: "まず初めに画面全体をGPUを使って単色で塗りつぶす単純なプログラムを作ることを題材に、DX11を使う上で最も重要なシェーダとその使い方について説明していきます。"
---
<section>
  <h1 class="under-bar">{{page.title}}</h1>
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
    実際には、<span class="important">シェーダ(英訳:Shader)</span>というGPU上で実行される言語を使用しますのでそこまで注意を払う必要はなく、
    CPUはDX11のAPIを、GPUはシェーダを使ってコーディングしていく形になります。
  </p>
  <p>
    DX11では<span class="important">HLSL</span>と呼ばれるシェーダ言語を使います。
    HLSLもc言語の文法をベースにしていますので、すぐに慣れることでしょう。
    ただ、様々なお約束を覚える必要はありますが。
  </p>
</section>
<section>
  <h1 class="under-bar">概要</h1>
  <p>
    まず初めに画面全体をGPUを使って単色で塗りつぶす単純なプログラムを作ることを題材に、
    DX11を使う上で最も重要なシェーダとその使い方について説明していきます。
    <br>今パートに対応しているサンプルプロジェクトは<span class="important">Part01_ClearScreen</span>になります。
  </p>
  <div class="summary">

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
            <span class="keyward">ID3D11DeviceContext::Dispatch関数</span>
          </li>
          <li>
            実行させるのに必要な設定<br>
            <span class="keyward">ID3D11DeviceContext::CSSetShader関数</span>と<span class="keyward">ID3D11DeviceContext::CSSetUnorederAccessViews関数</span>
          </li>
        </ul>
      </li>
      <li>
        シェーダのコンパイル
        <ul>
          <li>
            実行時でのコンパイル<br>
            <span class="keyward">D3DCompileFromFile関数</span>と<span class="keyward">D3DCompile関数</span>
          </li>
          <li>
            オフラインでのコンパイル<br>
            <span class="keyward">fxc.exe</span>
          </li>
        </ul>
      </li>
      <li>まとめ</li>
      <li>補足
        <ul>
          <li>画面クリアの便利関数</li>
          <li><span class="keyward">RWTexture2D</span>のデータ読み込みの制限</li>
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
    今回のサンプルでは<span class="important">コンピュートシェーダ(英訳:Compute Shader)</span>を使用して画面を単色で塗りつぶしています。
  </p>
  <p>
    一口にシェーダといってもいくつかの種類があり、
    大きく分けて<span class="important">グラフィックスパイプライン用のシェーダ</span>と<span class="important">GPGPU用のシェーダ</span>があります。
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
    screenが画面を表す<span class="important">リソース(英訳:Resource)</span>になり、サンプルでは全画面を黄色を表す"float4(1, 1, 0, 1)"で塗りつぶしています。
    シェーダ上では<span class="important">基本的に色を光の3原色である赤緑青とアルファ(不透明度)の4色で表現しており、0～1の範囲で調節します。</span>
    float4(赤, 緑, 青, アルファ)となっています。
  </p>
  <p>
    screenは<span class="keyward">RWTexture2D&lt;float4&gt;オブジェクト</span>になります。
    <span class="keyward">RWTexture2D</span>は<span class="important">読み書き可能<a href="#A1" class="attension">[補足]</a>な2次元テクスチャ</span>で、
    C++のテンプレートみたいに"<...>"で要素の型を指定しています。
    サンプルでは<span class="keyward">float4</span>を指定しているので、screenはシェーダ内で<span class="keyward">float4型</span>を読み書きできる2次元テクスチャとなります。
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
    さて、ここまで当たり前のように<span class="keyward">float4</span>や<span class="keyward">uint2</span>といった型を使っていましたが、どのようなものか想像できるでしょうか？
    <span class="important">float4はfloatを4つもつ型</span>で、<span class="important">uint2はuintを2つもつ型</span>になります。
    <span class="keyward">float3</span>だとfloatを3つ、<span class="keyward">uint4</span>だとuintを4つもち、型名そのままの意味になります。
    当然、<span class="keyward">float型とuint型も単体</span>であり、ほかには<span class="keyward">int型</span>と<span class="keyward">bool型</span>も同じように使えます。
  </p>
  <p>
    これらの型は一見すると配列のように見えますが、各要素へのアクセスは配列とは異なり、
    <span class="keyward">float4</span>や<span class="keyward">uint4</span>だと<span class="keyward">x,y,z,w</span>か<span class="keyward">r,b,g,a</span>を使ってアクセスします。
    <span class="keyward">float2</span>や<span class="keyward">uint2</span>だと<span class="keyward">x,y</span>か<span class="keyward">r,b</span>となり、
    <span class="important">配列での0番目がx(r)に、1番目がy(b)に、2番目がz(b)、3番目がw(a)となっています。</span>
    <span class="keyward">xyzw</span>とrgba</span>はそれぞれ同じ場所をさしています。c++言語でいうところの共用体みたいなものです。
  </p>
  <p>
    また、以下のようにすると、xとyに１が入り、
    {% highlight hlsl %}
float4 a;
a.xy = 1;
    {% endhighlight %}
    次のようにすると、w,y,xに２が入ります。
    {% highlight hlsl %}
a.wyx = 2;
    {% endhighlight %}
    これは<span class="important">スウィズル(英訳:Swizzle)</span>と呼ばれる機能でシェーダ言語独特の文法になります。
  </p>
  <p>
  またこのような書き方も可能です。
  {% highlight hlsl %}
float3 a = {1, 2, 3};
float4 b = float4(a, 4);//bはfloat4(1, 2, 3, 4)になる。
  {% endhighlight %}
  </p>
  <p>
    より詳しい説明は以下のドキュメントを参照してください。ここでは触れていない<span class="important">行列</span>を表す変数についても説明されています。
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
  <p>
    シェーダを実行するコードは以下になります。
  </p>
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
    <span class="important">Dispatch関数の引数は、ClearScreen.hlslのDTid引数とmain関数の呼び出し回数に影響を与えます。</span>
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
    <span class="keyward">numthreads属性</span>はコンピュートシェーダを使う上で非常に重要な意味を持つのですが、今回は説明しません。
    詳しく知りたいという方は、MSDNのドキュメントをご覧ください。<br>
    <a href="https://msdn.microsoft.com/ja-jp/library/ee422317(v=vs.85).aspx">numthreads(日本語)</a>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff471442(v=vs.85).aspx">numthreads(英語)</a>
  </p>
  <p>
    ここまでシェーダの実行の仕方について見てきました。<span class="important">コンピュートシェーダではDispatch関数を使うことでシェーダを実行します。</span>
    C++の関数呼び出しと比べるとかなり特殊な性質をもっていますが、これは<span class="important">GPUの大量のスレッドを同時に実行可能</span>という特徴を生かすためこうなってます。
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
    さて、単にDispatch関数だけを呼び出すだけではGPUから見ると実行するには情報不足です。
    次は上のコードの残りの部分である、<span class="important">GPUがどのシェーダをどのようなリソースを使って実行するかその設定方法を説明していきます。</span>
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
    <span class="keyward">CSSetShader関数</span>はコンピュートシェーダの設定を行う関数で、
    第一引数にコンピュートシェーダを表す<span class="keyward">ID3D11ComputeShader</span>を渡します。
    <span class="keyward">ID3D11ComputeShader</span>の生成は次のシェーダのコンパイルの項目で説明します。
    残りの引数は<span class="keyward">動的シェーダリンク(英訳:Dynamic Shader Linkage)</span>というシェーダでオブジェクト指向言語でのインターフェイスとクラスを扱うときに使いますが、
    Direct3D12では廃止になるようなので無視します。
  </p>
  <p>
    次に使用するリソースの設定は以下の<span class="keyward">CSSetUnorderedAccessViews関数</span>で行ってます。
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
      <li><span class="keyward">定数バッファ (英訳:ConstantBuffer)</span></li>
      <li><span class="keyward">シェーダリソースビュー (英訳:ShaderResourceView)</span></li>
      <li><span class="keyward">サンプラステート (英訳:SamplerState)</span></li>
      <li><span class="keyward">アンオーダードアクセスビュー (英訳:UnorderedAccessView, 以後、UAVと省略)</span></li>
    </ul>
    の4種類用意されています。
  </p>
  <p>
    今回のサンプルでは画面を塗りつぶすことを目的としているので、
    <span class="important">リソースの中で唯一書き込みが可能なUAVを使用しています。</span>
  </p>
  <p>
    話をサンプルに戻しまして、<span class="keyward">CSSetUnorderedAccessViews関数</span>の引数について見ていきます。
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
          上のコードの<span class="important">register(u0)</span>がスロット番号にあたります。
          CPUからGPUにリソースを設定するときは各リソースに関連付けられているスロット番号を使って設定します。
        </p>
        <p>
          <span class="important">"u0"はUAVの0番目のスロットを表しています</span>。
          リソースの後ろに": register(u0)"と書くと、そのリソースはUAVの0番目のスロットと関連付けられます。
          "u4"とするとUAVの4番目のスロット、"u10"とすると10番目のスロットになります。
        </p>
        <p>
          <span class="important">1度リソースに設定したスロットは当然ながら他のリソースには使用できません。</span>
          また、スロットの設定を省略することもできます。
          その際は、コンパイラがほかのリソースで<span class="important">使われているスロットと被らないよう自動的に割り当ててくれます。</span>
        </p>
      </li>
      <li>第2引数：一度に設定するUAVの個数</li>
      <li>第3引数：<span class="keyward">ID3D11UnorderedAccessView</span>* の配列
        <p>
          <span class="important">ID3D11UnorderedAccessViewはUAVを表すものになります。</span>
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
      mpImmediateContextは<span class="keyward">ID3D11DeviceContext</span>* になります。
      <span class="keyward">ID3D11DeviceContext</span>は<span class="important">GPUにシェーダやリソースの設定を行ったり、シェーダの実行、リソースのコピーなどを行うときに使用</span>するものです。
      DX11ではこれと後々出てくる<span class="keyward">ID3D11Device</span>の2つを使って処理を行っていきますので、
      <span class="important">この2つを理解できればDX11を理解したといってもいいくらい重要なもの</span>になります。
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
    上のコードの<span class="keyward">D3DCompileFromFile関数</span>でシェーダのコンパイルを行っています。
    引数がたくさんありますが、必ず必要となるものは以下のものになります。
    <br>ドキュメント：
    <a href="https://msdn.microsoft.com/ja-jp/library/windows/desktop/hh446872(v=vs.85).aspx">D3DCompileFromFile(英語)</a><br>
    <ul>
    	<li>
        第1引数 : ファイルパス
        <p>コンパイルしたいシェーダの<span class="keyward">ファイルパス</span>になります。</p>
      </li>
    	<li>
        第4引数 : エントリポイント
        <p>
          GPUでシェーダを実行する際、<span class="important">一番初めに呼び出される関数の名前</span>を指定します。
          ClearScreen.hlslなら"main"を指定します。
          シェーダファイルには複数のエントリポイントとなる関数を定義することができますが、
          それらの関数を使うためにはエントリポイントに<span class="keyward">各々の関数名を指定して個別にコンパイルする必要</span>があります。
        </p>
      </li>
    	<li>
        第5引数 : シェーダターゲット
        <p>
          シェーダの種類とその<span class="important">シェーダモデル(英訳:Shader Model)</span>を表す文字列を指定します。
          例えば、コンピュートシェーダのシェーダモデル5.0でコンパイルしたい場合は以下の文字列を渡してください。
        </p>
        {% highlight hlsl %}
const char* shaderTarget = "cs_5_0";
        {% endhighlight %}
        <p>
          上のshaderTargetの"cs"がコンピュートシェーダを表し、"5_0"がシェーダモデル5.0を表しています。
          シェーダモデルは<span class="important">シェーダのバージョン</span>のようなもので、
          現在シェーダモデル5.1が一番新しいものになります。(2016年度ぐらいには6.0が出てきそうですが)
          シェーダモデル5.1はDirect3D11.3とDirect3D12に対応したGPU上で実行できるシェーダのバージョンになります。
          シェーダモデルの詳細はMSDNの方を参照してください。
          <br>ドキュメント：
          <a href="https://msdn.microsoft.com/ja-jp/library/ee418332(v=vs.85).aspx">シェーダー モデルとシェーダー プロファイル(日本語)</a>
          <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/jj215820(v=vs.85).aspx">Specifying Compiler Targets(英語)</a>
        </p>
      </li>
    	<li>
        第8引数 : コンパイルした結果を受け取るID3DBlob*
        <p>
          コンパイルされたシェーダバイナリを受け取る<span class="keyward">ID3DBlob</span>を指定してください。
          <span class="important">ここで指定した変数を使って、ID3D11ComputeShaderを作成します。</span>
          ID3DBlobはシェーダのコンパイルの結果を受け取るときに使われるもので、メンバに受け取ったデータへのポインタとそのサイズを取得する関数が用意されています。
        </p>
      </li>
    </ul>
  </p>
  <div class="topic">
    <h4>機能レベル</h4>
    <p>
      第5引数のシェーダターゲットと関連するものとして<span class="important">機能レベル(英訳:Feature Levels)</span>というものがあります。
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
    さてまだ説明していない<span class="keyward">D3DCompileFromFile関数</span>の引数がありますが、先にGPUにシェーダを設定するときに使用したCSSetShader関数に渡す<span class="important">ID3D11ComputeShaderの作り方</span>を見ていきます。
    といっても簡単で、<span class="keyward">ID3D11Device</span>とコンパイルしたシェーダバイナリを用意するだけでできます。
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
    <span class="keyward">ID3D11Device::CreateComputeShader関数</span>には単純にコンパイルされたシェーダと作成したい<span class="keyward">ID3D11ComputeShader</span>を渡すだけです。
    第3引数のnullptrは動的シェーダリンクに関係するものなので無視します。
  </p>
  <div class="topic">
    <h4>ID3D11Device</h4>
    <p>
      ID3D11Deviceは<span class="important">シェーダやリソース、そのほかGPUに関係するものの作成</span>や<span class="important">使っているGPUの機能レベルや対応している機能の取得</span>などができます。
      DX11では<span class="important">ID3D11Deviceが作成したものを使ってGPUを操作する</span>ので最も重要なものと言えるでしょう。 当然、DX11を使う際は一番最初に作成しなければいけませんが、そのやり方は別パートで説明します。
    </p>
  </div>
  <p>
    <br>
    実行時のシェーダのコンパイルで最低限必要となる引数とGPUに設定する際に使う<span class="keyward">ID3D11ComputeShader</span>の生成方法についてはこれで以上になります。
    この後は<span class="keyward">D3DCompileFromFile</span>の残りの引数について説明していきます。
  </p>
  <p>
    <ul>
      <li>
        第9引数:エラーメッセージを受け取る<span class="keyward">ID3DBlob</span>
        <p>
          これは<span class="important">コンパイルエラーが発生した時のエラーメッセージを受け取るID3DBlob</span>になります。
          コンパイルエラーが発生したときは第9引数に渡した<span class="keyward">ID3DBlob</span>には、文字列が設定されます。
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
          コンパイルしたシェーダに<span class="important">デバッグ情報</span>をつけるか、<span class="important">最適化レベルの設定</span>などのフラグがあります。
          詳細はMSDNを参考にしてください。<br>
          ドキュメント:
          <a href="https://msdn.microsoft.com/ja-jp/library/ee415892(v=vs.85).aspx">D3D10_SHADER 定数(日本語)</a>
          <a href="https://msdn.microsoft.com/ja-jp/library/windows/desktop/gg615083(v=vs.85).aspx">D3DCOMPILE Constants(英語)</a>
        </p>
        <p>
          第7引数はエフェクト(Effect)というグラフィックスパイプラインのシェーダをまとめたものをコンパイルするとき使うものですが、DX11以降、<span class="important">エフェクトは廃止予定</span>になっていますので、説明を省きます。
        </p>
      </li>
      <li>
        第2引数：マクロの定義
        <p>
          C言語と同じようにシェーダも<span class="important">マクロに対応</span>しており、使い方も同じです。
          <span class="keyward">D3DCompileFromFile関数</span>に渡す際は<span class="keyward">D3D_SHADER_MACRO</span>の配列として渡し、
          <span class="important">末尾にはメンバをnullptrで埋めたものを設定します。</span>
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
        第3引数：#includeキーワードが行う処理を定義した<span class="important">ID3DIncludeの派生クラス</span>
        <p>
          これはシェーダ内に#includeキーワードがあったとき、ファイルを検索する方法や読み込みを制御するためのものです。
          <span class="keyward">D3DCompileFromFile</span>を使ってHLSLをコンパイルする際は<span class="important">自前でファイルを読み込む処理を作る必要があります。</span>
          引数には<span class="keyward">ID3DInclude</span>を継承したクラスを渡します。
          <span class="important">この引数がnullptrだと、シェーダ内で#includeを使うとエラー</span>になってしまいますので注意してください。
          詳細は以下のサイトを参照してください。使っているのは<span class="keyward">ID3D10Include</span>ですが使い方は同じです。<br>
          <a href="http://wlog.flatlib.jp/?blogid=1&query=preprocess">http://wlog.flatlib.jp/?blogid=1&query=preprocess</a><br>
        </p>
      </li>
    </ul>
  </p>
  <p>
    <span class="keyward">D3DCompileFromFile関数</span>については以上になります。
    この関数に似たものとして<span class="keyward">D3DCompile関数</span>がありますがそちらはシェーダを表す文字列からコンパイルする関数になります。
    コンパイルエラーが起きたときの識別用の文字列を指定する以外は引数は同じになります。
    <br>ドキュメント:<br>
    <a href="https://msdn.microsoft.com/ja-jp/library/ee416450(v=vs.85).aspx">D3DCompile(日本語)</a>
    <a href="https://msdn.microsoft.com/ja-jp/library/windows/desktop/dd607324(v=vs.85).aspx">D3DCompile(英語)</a>
  </p>
  <h3 class="under-bar">オフラインでのコンパイル</h3>
  <p>
    DX11のシェーダ言語であるHLSLではMicrosoftが用意している<span class="important">fxc.exeで事前にコンパイルすることが可能</span>です。
    <span class="keyward">fxc.exe</span>の場所は<span class="important">"Program Files (x86)\Windows Kits\10\bin\"以下のフォルダー</span>にあります。
    <span class="important">VisualStudioの開発者用コマンドプロンプト</span>を使用すれば環境変数などの設定をしなくとも使用できますし、
    サンプルプロジェクトにsetupFXCPath.batを用意しているので、コマンドプロンプトからそれを起動していただければ使えるようになります。
  </p>
  <p>
    <span class="keyward">fxc.exe</span>自体は<span class="keyward">D3DCompileFromFile関数</span>と同じように使え、#includeキーワードにも対応しています。
    コンパイルフラグはオプションで指定しますが、他のコマンドとは異なり"-"ではなく"/"を前につけて指定します。(Windows的にはこちらがデフォルト？)
    ヘルプを見てもらえれば使い方はわかりますが、ヘルプの見方は"fxc /?"または"fxc /help"になっていますので、混乱しないよう注意してください。
    {% highlight bat %}
@rem 例
fxc /T cs_5_0 /Fo binary.cso ClearScreen.hlsl
    {% endhighlight %}
    後は実行時に出力ファイルを読み込んで、<span class="keyward">ID3D11Device::CreateComputeShader関数</span>に渡せば<span class="keyward">ID3D11ComputeShader</span>が作成できます。
  </p>
  <p>
    ちなみにサンプルのClearScreen.hlslは<span class="important">プロジェクトのビルド時にコンパイルを行うようプロパティから設定</span>しています。
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
    <span class="important">今回説明した内容は以降のパートで共通して使う</span>ものなので、一度に覚えようとせずDX11を使ったいろいろなシェーダやソースを見て慣れていけばいいでしょう。
  </p>
</section>

<section>
  <h2 class="under-bar">補足</h2>
  <div class="supplemental">
    <h4>画面クリアの便利関数</h4>
    <p>
      今回、画面を塗りつぶすシェーダを書きましたが
    	<span class="keyward">ID3D11DeviceContext::ClearUnorderedAccessViewFloat関数</span>を使えばシェーダを使わずとも画面を塗りつぶすことができます。
      {% highlight c++ %}
// 例
float value[4] = {1, 0.7f, 1, 1};//左から赤、緑、青、アルファ
this->mpImmediateContext->ClearUnorderedAccessViewFloat(this->mpScreenUAV.Get(), value);
    {% endhighlight %}
    UAVには他にもUINT型で値を設定する<span class="keyward">ID3D11DeviceContext::ClearUnorderedAccessViewUint関数</span>も用意されています。
    ID3D11DeviceContextにはこういった関数がいくつか用意されていますので、随時紹介していきます。
    </p>
  </div>
  <div class="supplemental">
    <h4>RWTexture2Dのデータ読み込みの制限</h4>
    <a name="A1"></a>
    <p>
      1.シェーダの作成にて、RWTexture2Dはデータの読み書きができると書きましたが、
    	<span class="important">読み込みに関してはuint型かfloat型のみに制限されています。</span>
    	また、RWと名の付くものはすべてこの制限を持ちます。
    	この制限は<span class="important">DX12とDX11.3以降、Typed Unordered Access View Loads</span>として幾分か緩和されています。
    	<br><a href="https://msdn.microsoft.com/ja-jp/library/windows/desktop/dn903947(v=vs.85).aspx">Typed Unordered Access View (UAV) Loads(英語)</a><br>
    	データをuint型に変換するなど回避策はありますが、直接リソースから読み込むことはできないので、DX11.2以前のGPUを使うときは注意してください。
    </p>
  </div>
</section>

<table class="table table-condensed">
  <tbody>
    <tr>
      <td class="left"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}">＜前</a></td>
      <td class="center"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}">トップ</a></td>
      <td class="right"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/constant-buffer">次＞</a></td>
    </tr>
  </tbody>
</table>
