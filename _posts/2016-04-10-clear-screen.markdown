---
layout: default
title: "シェーダを使った画面クリア"
categories: part
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
    実際には、シェーダ(英訳:Shader)というGPU上で実行される言語を使用しますのでそこまで注意を払う必要はなく、
    CPUはDX11のAPIを、GPUはシェーダを使ってコーディングしていく形になります。
  </p>
  <p>
    DX11ではHLSLと呼ばれるシェーダ言語を使います。
    HLSLもc言語の文法をベースにしていますので、すぐに慣れることでしょう。
    ただ、様々なお約束を覚える必要はありますが。
  </p>
</section>
<section>
  <h1 class="under-bar">{{page.title}}</h1>
  <p>
    まず初めに画面全体をGPUを使って単色で塗りつぶす単純なプログラムを作ることを題材に、
    DX11を使う上で最も重要なシェーダとその使い方について説明していきます。
  </p>
  <div class="overview">
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
            ID3D11DeviceContext::Dispatch関数
          </li>
          <li>
            実行させるのに必要な設定<br>
            ID3D11DeviceContext::CSSetShader関数とID3D11DeviceContext::CSSetUnorederAccessViews関数
          </li>
        </ul>
      </li>
      <li>
        シェーダのコンパイル
        <ul>
          <li>
            実行時でのコンパイル<br>
            D3DCompileFromFileとD3DCompile
          </li>
          <li>
            オフラインでのコンパイル<br>
            fxc.exe
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
    今回のサンプルではコンピュートシェーダ(英訳:Compute Shader)を使用して画面を単色で塗りつぶしています。
  </p>
  <p>
    一口にシェーダといってもいくつかの種類があり、
    大きく分けてグラフィックスパイプライン用のシェーダとGPGPU用のシェーダがあります。
    (グラフィックスパイプラインについては後のページで説明します。)
  </p>
  <p>
    コンピュートシェーダはGPGPU目的でGPUを動作させるためのシェーダになり、Direct3D10から登場しました。
    Direct3D10では厳しい制限があったのですが、DX11ではその制限がかなり消えて使い勝手がよくなりました。
    コンピュートシェーダは現在のハイエンドゲームの制作において欠かすことのできないシェーダであり、シェーダの中でコーディングする制限が一番少ないものになります。
  </p>
  <p>
  話をソースコードに戻しまして、
  ClearScreen.hlslの
  {% highlight hlsl %}
screen[DTid] = float4(1, 1, 0, 1);
  {% endhighlight %}
  この部分で画面を塗りつぶしています。
  </p>
  <p>
    screenが画面を表すリソース(英訳:Resource)になり、サンプルでは全画面を黄色を表す"float4(1, 1, 0, 1)"で塗りつぶしています。
    シェーダ上では基本的に色を光の3原色である赤緑青とアルファ(不透明度)の4色で表現しており、0～1の範囲で調節します。
    float4(赤, 緑, 青, アルファ)となっています。
  </p>
  <p>
    screenはRWTexture2D<float4>オブジェクトになります。
    RWTexture2Dは読み書き可能<a href="#A1" class="attension">[補足]</a>な2次元テクスチャで、C++のテンプレートみたいに"<...>"で要素の型を指定しています。
    サンプルではfloat4を指定しているので、screenはシェーダ内でfloat4型を読み書き可能な2次元テクスチャとなります。
    2次元テクスチャはC++でいうところの2次元配列のようなもので、実際screenの各要素には配列のようにアクセスできます。
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
    さてさて、ここまで当たり前のようにfloat4やuint2といった型を使っていましたが、どのようなものか想像できるでしょうか？
    float4はfloatを4つもつ型で、uint2はuintを2つもつ型になります。
    float3だとfloatを3つ、uint4だとuintを4つもち、型名そのままの意味になります。
    当然、float型とuint型も単体であり、ほかにはint型とbool型も同じように使えます。
  </p>
  <p>
    これらの型は一見すると配列のように見えますが、各要素へのアクセスは配列とは異なり、
    float4やuint4だとx,y,z,wかr,b,g,aを使ってアクセスします。
    float2やuint2だとx,yかr,bとなり、
    配列での0番目がx(r)に、1番目がy(b)に、2番目がz(b)、3番目がw(a)となっています。
    xyzwとrgbaはそれぞれ同じ場所をさしています。c++言語でいうところの共用体みたいなものです。
  </p>
  <p>
    また、
    {% highlight hlsl %}
float4 a;
a.xy = 1;
    {% endhighlight %}
    とすると、xとyに１が入り、
    {% highlight hlsl %}
a.wyx = 2;
    {% endhighlight %}
    とすると、wとy,xに２が入ります。
    これはスウェルズ(英訳:Swizzle)と呼ばれる機能でシェーダ言語独特の文法になります。
  </p>
  <p>
  また
  {% highlight hlsl %}
float3 a = {1, 2, 3};
float4 b = float4(a, 4);//bはfloat4(1, 2, 3, 4)になる。
  {% endhighlight %}
  のような書き方も可能です。
  </p>
  <p>
  <br>
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
    DX11を使ったコードは長くなりがちですが、肝となる部分はそう長くはありません。<br>
    上のコードの
    {% highlight c++ %}
//Scene::onRender()の一部
this->mpImmediateContext->Dispatch(this->mWidth, this->mHeight, 1);
    {% endhighlight %}
    でシェーダを実行しています。
  </p>
  <p>
    Dispatch関数の引数は、上にあるClearScreen.hlslの
    {% highlight c++ %}
//ClearScreen.hlsl
void main( uint2 DTid : SV_DispatchThreadID )
    {% endhighlight %}
    DTid引数に影響を与えます。
  </p>
  <p>
    Dispatch関数の引数は、上にあるClearScreen.hlslのmain関数の
    {% highlight c++ %}
//ClearScreen.hlsl
void main( uint2 DTid : SV_DispatchThreadID )
    {% endhighlight %}
    DTid引数に影響を与えます。
  </p>
  <p>
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
    実のところ、ClearScreen.hlslの
    {% highlight hlsl %}
//ClearScreen.hlsl
[numthreads(1,1,1)]
    {% endhighlight %}
    も、DTidの値に影響があり、コンピュートシェーダを使う上で非常に重要な意味を持つのですが、今回は説明しません。
    詳しく知りたいという方は、MSDNのドキュメントをご覧ください。<br>
    <a href="https://msdn.microsoft.com/ja-jp/library/ee419587(v=vs.85).aspx">https://msdn.microsoft.com/ja-jp/library/ee419587(v=vs.85).aspx</a>
  </p>
  <p>
    ここまでシェーダの実行の仕方について見てきました。コンピュートシェーダではDispatch関数を使うことでシェーダを実行します。
    C++の関数呼び出しと比べるとかなり特殊な性質をもっていますが、これはGPUの大量のスレッドを同時に実行可能という特徴を生かすためこうなってます。
    サンプルではClearScreen.hlslにはmain関数とは別にclearByOneThread関数というfor文を使ったC++で画面クリアをする場合と同じコードになるよう書いたものも用意していますので、一度目を通してみてください。
    このシェーダとClearScreen.hlslは同じことをしていますが、処理速度はGPUの性質を生かしているClearScreen.hlslの方が速くなります。
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
    GPUが実行するシェーダの設定は
    {% highlight c++ %}
//Scene::onRender()の一部
this->mpImmediateContext->CSSetShader(this->mpCSClearScreen.Get(), nullptr, 0);
    {% endhighlight %}
    で行います。
  </p>
  <p>
    CSSetShader関数はコンピュートシェーダの設定を行う関数で、
    第一引数にコンピュートシェーダを表すID3D11ComputeShader*を渡します。
    ID3D11ComputeShaderの生成は次のシェーダのコンパイルの項目で説明します。
    残りの引数は動的シェーダリンク(英訳:Dynamic Shader Linkage)というシェーダでオブジェクト指向でいうところのインターフェイスを扱うときに使いますが、
    Direct3D12では廃止になるようなので、無視します。
  </p>
  <p>
    次に使用するリソースの設定は
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
    のmpImmediateContext->CSSetUnorderedAccessViews関数で行ってます。
  </p>
  <p>
    リソースにはいくつか種類があり、
    <ul>
      <li>定数バッファ (英訳:ConstantBuffer)</li>
      <li>シェーダリソースビュー (英訳:ShaderResourceView)</li>
      <li>サンプラステート (英訳:SamplerState)</li>
      <li>アンオーダードアクセスビュー (英訳:UnorderedAccessView, 以後、UAVと省略)</li>
    </ul>
    の4種類用意されています。
  </p>
  <p>
    今回のサンプルでは画面を塗りつぶすことを目的としているので、
    リソースの中で唯一書き込みが可能なアンオーダードアクセスビューを使用しています。
  </p>
  <p>
    話をサンプルに戻しまして、CSSetUnorderedAccessViews関数の第2引数で一度に設定するUAVの個数を指定し、
    第3引数でUAVを表すID3D11UnorderedAccessView*の配列を渡します。
    配列の要素数は必ず第2引数の値より大きいものにしてください。
  </p>
  <p>
    第1引数はGPUに設定する開始スロット番号になります。
    スロットとは識別番号のようなもので、ClearScreen.hlslでの
    {% highlight hlsl %}
//ClearScreen.hlsl
RWTexture2D<float4> screen : register(u0);
    {% endhighlight %}
    register(u0)にあたり、
    CPUからGPUにリソースを設定するときは各リソースに関連付けられているスロット番号を使って設定します。
    C++でいうところの配列の添え字みたいなものです。
  </p>
  <p>
    "u0"はアンオーダードアクセスビューの0番目のスロットを表しており、
    リソースの後ろに":"をつけてから"register(u0)"と書くと、
    そのリソースはUAVの0番目のスロットと関連付けられます。
    "u4"とするとUAVの4番目のスロット、"u10"とすると10番目のスロットになります。
  </p>
  <p>
    1度リソースに設定したスロットは当然ながら他のリソースには使用できません。
    またスロットの設定を省略することもできます。
    その際は、コンパイラがほかのリソースで使われているスロットと被らないよう自動的に割り当ててくれます。
  </p>
  <p>
    第4引数は今回は意味を持たないので、省略します。
  </p>
  <p>
    <br>
    ところで、ここまで度々出てきたmpImmediateContextについて触れていませんでした。
    mpImmediateContextはID3D11DeviceContext*になります。
    ID3D11DeviceContextはGPUにシェーダやリソースの設定を行ったり、シェーダの実行、リソースのコピーなどを行うときに使用するものです。
    DX11ではID3D11DeviceContextと後々出てくるID3D11Deviceの2つを使って処理を行っていきますので、
    この2つを理解できればDX11を理解したといってもいいくらい重要なものになります。
    <br>
    <br>
  </p>
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
    ドキュメントは<a href="https://msdn.microsoft.com/en-us/library/windows/desktop/hh446872(v=vs.85).aspx">こちら</a>になります。
    引数がたくさんありますが、必ず必要となるものは
    <ul>
    	<li>第1引数 ; ファイルパス</li>
    	<li>第4引数 : エントリポイント</li>
    	<li>第5引数 : シェーダターゲット</li>
    	<li>第8引数 : コンパイルした結果を受け取るID3DBlob*</li>
    </ul>
    になります。
  </p>
  <P>
    第1引数はコンパイルしたいシェーダのファイルパスになります。
  </p>
  <p>
    第4引数にはGPUでシェーダを実行する際、一番初めに呼び出される関数の名前を指定します。
    ClearScreen.hlslなら"main"を指定します。
    シェーダファイルには複数のエントリポイントとなる関数を定義することができますが、
    それらの関数を使うためにはエントリポイントに各々の関数名を指定して個別にコンパイルする必要があります。
  </p>
  <p>
    第5引数にはシェーダの種類とそのシェーダモデル(英訳:Shader Model)を指定します。
    シェーダモデルはシェーダのバージョンのようなもので、
    現在シェーダモデル5.1が一番新しいものになります。(2016年度ぐらいには6.0が出てきそうですが)
    シェーダモデル5.1はDirect3D11.3とDirect3D12に対応したGPU上で実行できるシェーダのバージョンになります。
  </p>
  <p>
    <br>
    GPUは世代によってハードウェアの構造が異なるため、
    DX11以降ではそれに対応するために機能レベル(英訳:Feature Levels)を使って使用できる機能を区別しています。
    シェーダモデル5.1は機能レベル12_0以降が対応しており、部分的に11_1と11_0が対応しています。
    サンプルでは以降、機能レベル11_0が対応しているシェーダモデル5.0を使ってシェーダをコンパイルしていきます。
    下のリンクは機能レベルのドキュメントになります。日本語訳は少し情報が古いので、英語の方も参考にしてください。<br>
    <a href="https://msdn.microsoft.com/ja-jp/library/ee422086(v=vs.85).aspx">https://msdn.microsoft.com/ja-jp/library/ee422086(v=vs.85).aspx(日本語)</a><br>
    <a href="https://msdn.microsoft.com/en-us/library/ff476876(v=vs.85).aspx">https://msdn.microsoft.com/en-us/library/ff476876(v=vs.85).aspx(英語)</a>
    <br>
    <br>
  </p>
  <p>
    話がそれましたが、D3DCompileFromFile関数の第5引数にはシェーダターゲットを表す文字列を渡します。
    例えば、ClearScreen.hlslをコンピュートシェーダのシェーダモデル5.0でコンパイルしたい場合は
    {% highlight c++ %}
const char* shaderTarget = "cs_5_0";
    {% endhighlight %}
    とします。
    上のshaderTargetの"cs"がコンピュートシェーダを表し、"5_0"がシェーダモデル5.0を表しています。
  </p>
  <p>
    シェーダモデルの詳細はMSDNの方を参照してください。
    ただ、シェーダターゲットの文字列を確認したい程度なら日本語翻訳されたもので十分なのですが、
    翻訳されたサイトは情報が古いのと、リニューアル後のMSDN(英語)の方が内容が充実していますので、出来るならリニューアル後の方を参考にした方がいいです。<br>
    <a href="https://msdn.microsoft.com/ja-jp/library/ee418332(v=vs.85).aspx">https://msdn.microsoft.com/ja-jp/library/ee418332(v=vs.85).aspx(日本語)</a><br>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/jj215820(v=vs.85).aspx">https://msdn.microsoft.com/en-us/library/windows/desktop/jj215820(v=vs.85).aspx(英語)</a>
  </p>
  <p>
    D3DCompileFromFile関数の第8引数にはコンパイルされたシェーダバイナリを受け取るID3DBlobを指定してください。
    ここで指定した変数を使って、ID3D11ComputeShaderを作成します。
  </p>
  <p>
    ID3DBlobはシェーダのコンパイルの結果を受け取るときに使われるもので、メンバに受け取ったデータへのポインタとそのサイズを取得する関数が用意されています。
  </p>
  <p>
    さて残りの引数については置いといて先に、GPUにシェーダを設定するときに使用したCSSetShader関数に渡すID3D11ComputeShaderの作り方を見ていきます。
    といっても簡単で、ID3D11Deviceとコンパイルしたシェーダバイナリを用意するだけでできます。
    {% highlight c++ %}
//Scene::onInit()の一部
hr = this->mpDevice->CreateComputeShader(
  pShaderBlob->GetBufferPointer(),
  pShaderBlob->GetBufferSize(),
  nullptr,
  &this->mpCSClearScreenWithConstantBuffer);
    {% endhighlight %}
    CreateComputeShader関数には単純にコンパイルされたシェーダと作成したいID3D11ComputeShaderを渡すだけです。
    第3引数のnullptrは動的シェーダリンクに関係するものなので無視します。
  </p>
  <p>
    実行時のシェーダのコンパイルで最低限必要となる引数とGPUに設定するために必要になるID3D11ComputeShaderの生成方法についてはこれで以上になります。
    この後はD3DCompileFromFileの残りの引数について説明していきます。
  </p>
  <p>
    まず、D3DCompileFromFile関数の第9引数ですが、コンパイルエラーが発生した時のエラーメッセージを受け取るID3DBlobになります。
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
  <p>次にD3DCompileFromFile関数の第6、7引数はコンパイルフラグになります。</p>
  <p>
    第6引数はシェーダのコンパイルフラグになり、
    デバッグ情報をつけるか、最適化レベルの設定などのフラグがあります。
    詳細はMSDNを参考にしてください。<br>
    <a href="https://msdn.microsoft.com/ja-jp/library/ee415892(v=vs.85).aspx">https://msdn.microsoft.com/ja-jp/library/ee415892(v=vs.85).aspx(日本語)</a><br>
    <a href="https://msdn.microsoft.com/ja-jp/library/windows/desktop/gg615083(v=vs.85).aspx">https://msdn.microsoft.com/ja-jp/library/windows/desktop/gg615083(v=vs.85).aspx(英語)</a>
  </p>
  <p>第7引数はエフェクト(Effect)というグラフィックスパイプラインのシェーダをまとめたものをコンパイルするとき使うものですが、DX11以降、エフェクトは廃止予定になっていますので、説明を省きます。</p>
  <p>
    次に移って、D3DCompileFromFile関数の第2引数はマクロを定義するときに使います。
    {% highlight c++ %}
std::array<D3D_SHADER_MACRO, 2> macros = { {
  {"DEFINE_MACRO", "float4(0, 1, 1, 1)"},
  {nullptr, nullptr},
} };
hr = D3DCompileFromFile(L"ClearScreenWithConstantBuffer.hlsl", macros.data(), nullptr, entryPoint, shaderTarget, compileFlag, 0, pShaderBlob.GetAddressOf(), pErrorMsg.GetAddressOf());
    {% endhighlight %}
    C言語と同じようにシェーダもマクロに対応しており、使い方も同じです。
    D3DCompileFromFile関数に渡す際はD3D_SHADER_MACROの配列として渡し、
    末尾にはメンバをnullptrで埋めたものを設定します。
  </p>
  <p>
    D3DCompileFromFile関数の第3引数はシェーダ内に#includeキーワードがあったときのファイルを検索する方法や読み込みを制御するためのものです。
    引数にはID3DIncludeを継承したクラスを渡します。
    詳細は以下のサイトを参照してください。使っているのはID3D10Includeですが使い方は同じです。<br>
    <a href="http://wlog.flatlib.jp/?blogid=1&query=preprocess">http://wlog.flatlib.jp/?blogid=1&query=preprocess</a><br>
    この引数がnullptrだと、シェーダ内で#includeを使うとエラーになってしまいますので注意してください。
  </p>
  <p>
    D3DCompileFromFile関数については以上になります。
    この関数に似たものとしてD3DCompile関数がありますがそちらはシェーダを表す文字列からコンパイルする関数になります。
    コンパイルエラーが起きたときの識別用の文字列を指定する以外は引数は同じになります。
  </p>
  <h3 class="under-bar">オフラインでのコンパイル</h3>
  <p>
    DX11のシェーダ言語であるHLSLではMicrosoftが用意しているfxc.exeで事前にコンパイルすることが可能です。
    fxc.exeの場所は"Program Files (x86)\Windows Kits\10\bin\"以下のフォルダーにあります。
    VisualStudioの開発者用コマンドプロンプトを使用すれば環境変数などの設定をしなくとも使用できますし、
    サンプルプロジェクトにsetupFXCPath.batを用意しているので、コマンドプロンプトからそれを起動いただければ使えるようになります。
  </p>
  <p>
    fxc.exe自体はD3DCompileFromFile関数と同じように使え、#includeキーワードにも対応しています。
    コンパイルフラグはオプションで指定しますが、他のコマンドとは異なり"-"ではなく"/"を前につけて指定します。(Windows的にはこちらがデフォルト？)
    ヘルプを見てもらえれば使い方はわかりますが、ヘルプの見方は"fxc /?"または"fxc /help"になっていますので、混乱しないよう注意してください。
    {% highlight bat %}
例)
fxc /T cs_5_0 /Fo binary.cso ClearScreen.hlsl
    {% endhighlight %}
    後は実行時に出力ファイルを読み込んで、CreateComputeShader関数に渡せばID3D11ComputeShaderが作成できます。
  </p>
  <p>
    ちなみにサンプルのClearScreen.hlslはプロジェクトのビルド時にコンパイルするようプロパティから設定しています。
    その際、出力されるファイルはClearScreen.csoと拡張子が".cso"になりますので、この機能を使うときは覚えておいてください。
  </p>
</section>

<section>
  <h1 class="under-bar">まとめ</h1>
  <p>かなり長くなりましたが、DX11でのシェーダの使い方の説明は以上になります。</p>
  <p>
    今回の内容を踏まえたシェーダを実行したいときの流れは
    <div class="overview">
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
    今回説明した内容は以降のパートで共通して使うものなので、一度に覚えようとせずDX11を使ったいろいろなシェーダやソースを見て慣れていけばいいでしょう。
  </p>
</section>

<section>
  <h2 class="under-bar">補足</h2>
  <h4 class="under-bar">画面クリアの便利関数</h4>
  <p>
    今回、画面を塗りつぶすシェーダを書きましたが
  	ID3D11DeviceContextのClearUnorderedAccessViewFloat関数を使えばシェーダを使わずとも画面を塗りつぶすことができます。
    {% highlight c++ %}
例)
float value[4] = {1, 0.7f, 1, 1};//左から赤、緑、青、アルファ
this->mpImmediateContext->ClearUnorderedAccessViewFloat(this->mpScreenUAV.Get(), value);
  {% endhighlight %}
  UAVには他にもUINT型で値を設定するID3D11DeviceContext::ClearUnorderedAccessViewUint関数も用意されています。
  ID3D11DeviceContextにはこういった関数がいくつか用意されていますので、随時紹介していきます。
  </p>
  <h4 class="under-bar">RWTexture2Dのデータ読み込みの制限</h4>
  <a name="A1"></a>
  <p>
    1.シェーダの作成にて、RWTexture2Dはデータの読み書きができると書きましたが、
  	読み込みに関してはuint型かfloat型のみに制限されています。
  	また、RWと名の付くものはすべてこの制限を持ちます。
  	この制限はTyped Unordered Access View (UAV) LoadsとしてDX12とDX11.3以降ではなくなっています。
  	DX11.2以前のGPUを使うときは注意してください。<br>
  	<a href="https://msdn.microsoft.com/ja-jp/library/windows/desktop/dn903947(v=vs.85).aspx">https://msdn.microsoft.com/ja-jp/library/windows/desktop/dn903947(v=vs.85).aspx</a><br>

  	データをuint型に変換するなど回避策はありますが、
  	直接リソースから読み込むことはできないので、DX11.2以前のGPUを使うときは注意してください。
  </p>
</section>
