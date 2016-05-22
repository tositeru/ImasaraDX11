---
layout: default
title: "テッセレーション"
categories: part
description: "今パートではテッセレーションの使い方について見ていきます。"
---
<h1 class="under-bar">テッセレーション</h1>

今回はテッセレーションについて見ていきます。
テッセレーションはDX11から導入されたシェーダでシェーダモデル5.0以上に対応しているGPUで動作します。
これを利用することで１つの三角形や線分、四角形をより細かく分割することが出来ます。
この機能はディスプレイスメントマッピング(英訳：Displacement Mapping)やカメラからの距離に応じてポリゴンの数を調節するLevel of Detail(略称:LOD)などに利用されているようです。

<span class="important">テッセレーションは頂点シェーダの後に実行される3つのステージから構成されており、ハルシェーダとドメインシェーダの2つのシェーダとテッセレータステージのGPUの固定機能に分けることが出来ます。</span>
<span class="important">ハルシェーダでテッセレータステージがどのようにプリミティブを分割するかを決め、ドメインシェーダで分割した頂点の移動などの処理を行います。</span>
GPUの固定機能を使用しているためシステムセマンティクスの意味を理解することがテッセレーションの理解につながります。

参考サイト：
<br>ドキュメント テッセレーションの概要 [(日本語)][TESSELLATION_JP][(英語)][TESSELLATION_EN]
<br>[「DirectX 11」のテッセレーション－テッセレーションの意味と重要性][NVIDIA_JP]
<br>[西川善司の3DゲームファンのためのDirectX 11テッセレーション活用講座][GAME_WATCH_JP]

[TESSELLATION_JP]:https://msdn.microsoft.com/ja-jp/library/ee417841(v=vs.85).aspx
[TESSELLATION_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476340(v=vs.85).aspx
[NVIDIA_JP]:http://www.nvidia.co.jp/object/tessellation_jp.html
[GAME_WATCH_JP]:http://game.watch.impress.co.jp/docs/series/3dcg/20100310_353969.html

<h1 class="under-bar">概要</h1>
今パートではテッセレーションの使い方について見ていきます。
対応するプロジェクトは<span class="important">Part12_Tessellation</span>になります。

<div class="summary">
  <ol>
    <li><a href="#TRIANGLE">三角形の分割</a></li>
    <li><a href="#SQUARE">四角形の分割</a></li>
    <li><a href="#ISOLINE">線分の分割</a></li>
    <li><a href="#POINT_TO_TRIANGLE">点から三角形を生成</a></li>
    <li><a href="#SUMMARY">まとめ</a></li>
  </ol>
</div>

<a name="TRIANGLE"></a>
<h1 class="under-bar">三角形の分割</h1>
それでははじめに三角形の分割を仕方を見ながらハルシェーダとドメインシェーダについて見ていきましょう。

<h3>ハルシェーダ</h3>
<span class="important">ハルシェーダは与えられたプリミティブをどのように分割するかを決めるシェーダになります。</span>
<span class="important">ちなみに分割を行うプリミティブのことをパッチ(英訳:Patch)と呼ばれているようです。</span>

ドキュメント：ハル シェーダーの設計
[(日本語)][DESING_HULL_JP]
[(英語)][DESING_HULL_EN]

[DESING_HULL_JP]:https://msdn.microsoft.com/ja-jp/library/ee417840(v=vs.85).aspx
[DESING_HULL_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476339(v=vs.85).aspx

{% highlight c++ %}
// HSTriangle.hlsl

cbuffer Param : register(b0)
{
  float cbEdgeFactor;//三角形の辺の分割量の指定
  float cbInsideFactor;//三角形の内部の分割量の指定
};

//頂点シェーダからの入力
struct VS_CONTROL_POINT_OUTPUT
{
  float3 pos : POSITION;
};

//パッチ定数関数の入力値
struct HS_CONTROL_POINT_OUTPUT
{
  float3 pos : POSITION;
};
//パッチ定数関数の出力値
struct HS_CONSTANT_DATA_OUTPUT
{
  float EdgeTessFactor[3] : SV_TessFactor;
  float InsideTessFactor  : SV_InsideTessFactor;
};

// パッチ定数関数の定義
#define NUM_CONTROL_POINTS 3
HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
  InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
  uint PatchID : SV_PrimitiveID)
{
  HS_CONSTANT_DATA_OUTPUT Output;

  Output.EdgeTessFactor[0] = Output.EdgeTessFactor[1] = Output.EdgeTessFactor[2] = cbEdgeFactor;
  Output.InsideTessFactor = cbInsideFactor;

  return Output;
}

//ハルシェーダのエントリポイント定義
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("CalcHSPatchConstants")]
HS_CONTROL_POINT_OUTPUT main(
  InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> inputPatch,
  uint i : SV_OutputControlPointID,
  uint PatchID : SV_PrimitiveID )
{
  HS_CONTROL_POINT_OUTPUT Output;
  Output.pos = inputPatch[i].pos;
  return Output;
}
{% endhighlight %}

今まで見てきたシェーダと比べますとコード量や設定するものが多いですが順に見ていきます。

<h4>エントリポイント</h4>
まず、エントリポイントとなるmain関数についてみていきます。
{% highlight c++ %}
//ハルシェーダのエントリポイント定義
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("CalcHSPatchConstants")]
HS_CONTROL_POINT_OUTPUT main(
  InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> inputPatch,
  uint i : SV_OutputControlPointID,
  uint PatchID : SV_PrimitiveID )
{
  HS_CONTROL_POINT_OUTPUT Output;
  Output.pos = inputPatch[i].pos;
  return Output;
}
{% endhighlight %}

ハルシェーダのエントリポイントには属性が多数用意されています。

<div class="argument">
  <h3 class="under-bar">指定できる属性</h3>
  <ol>
    <li><span class="keyward">domain</span>
      <p>ハルシェーダで処理を行うパッチを指定します。三角形の<span class="keyward">tri</span>,四角形の<span class="keyward">quad</span>,線分の<span class="keyward">isoline</span>の3つ指定出来ます。</p>
    </li>
    <li><span class="keyward">partitioning</span>
      <p>分割方法を指定します。指定できるのは<span class="keyward">integer</span>,<span class="keyward">fractional_even</span>,<span class="keyward">fractional_odd</span>,<span class="keyward">pow2</span>になります。</p>
    </li>
    <li><span class="keyward">outputcontrolpoints</span>
      <p>ハルシェーダが作成する制御点の個数を指定します。</p>
    </li>
    <li><span class="keyward">outputtopology</span>
      <p>テッセレータの出力するプリミティブを指定します。<span class="keyward">line</span>,<span class="keyward">triangle_cw</span>,<span class="keyward">triangle_ccw</span>が指定できます。</p>
    </li>
    <li><span class="keyward">patchconstantfunc</span>
      <p>パッチ定数データを計算する関数を指定します。上のコードだと<span class="keyward">CalcHSPatchConstants関数</span>がパッチ定数データを計算する関数になります。</p>
    </li>
    <li><span class="keyward">maxtessfactor</span>
      <p>最大となる分割係数の値を指定します。</p>
    </li>
  </ol>
</div>

エントリポイントの引数にある<span class="keyward">InputPatch</span>はハルシェーダの入力値となる制御点の配列を表しています。
制御点の型と個数は<span class="keyward"><...></span>で指定します。

ドキュメント：InputPatch
[(日本語)][InputPatch_JP]
[(英語)][InputPatch_EN]

[InputPatch_JP]:https://msdn.microsoft.com/ja-jp/library/ee422338(v=vs.85).aspx
[InputPatch_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471462(v=vs.85).aspx

<span class="important">説明が後になってしまいましたが、エントリポイントは１つのパッチにつきその制御点の個数分呼びだされます。</span>

<h3>パッチ定数関数</h3>

パッチ定数関数がパッチ1つごとに1回実行され、分割数の決定やこちらで用意したパラメータを計算します。
<span class="important">言い換えるとパッチを分割するためのデータを計算する関数となります。</span>

{% highlight c++ %}
// HSTriangle.hlsl
//パッチ定数関数の出力値
struct HS_CONSTANT_DATA_OUTPUT
{
  float EdgeTessFactor[3] : SV_TessFactor;
  float InsideTessFactor  : SV_InsideTessFactor;
};
// パッチ定数関数の定義
HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
  InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
  uint PatchID : SV_PrimitiveID)
{
  HS_CONSTANT_DATA_OUTPUT Output;

  Output.EdgeTessFactor[0] = Output.EdgeTessFactor[1] = Output.EdgeTessFactor[2] = cbEdgeFactor;
  Output.InsideTessFactor = cbInsideFactor;

  return Output;
}
{% endhighlight %}

<span class="important">分割数を指定するにはシステムセマンティックの<span class="keyward">SV_TessFactor</span>と<span class="keyward">SV_InsideTessFactor</span>を利用します。</span>

ドキュメント：
<br>SV_TessFactor
[(日本語)][SV_TessFactor_JP]
[(英語)][SV_TessFactor_EN]
<br>SV_InsideTessFactor
[(日本語)][SV_InsideTessFactor_JP]
[(英語)][SV_InsideTessFactor_EN]

[SV_TessFactor_JP]:https://msdn.microsoft.com/ja-jp/library/ee422455(v=vs.85).aspx
[SV_TessFactor_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471574(v=vs.85).aspx
[SV_InsideTessFactor_JP]:https://msdn.microsoft.com/ja-jp/library/ee422453(v=vs.85).aspx
[SV_InsideTessFactor_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471572(v=vs.85).aspx

この2つは分割するプリミティブによって意味合いが変わります。
三角形の場合は以下の意味になります。
<div class="argument">
  <h4>三角形の場合の分割数の指定</h4>
  <ul>
    <li><span class="keyward">SV_TessFactor</span>
      <p>
        三角形の辺の分割数を指定します。
        <span class="important">三角形の場合、このシステムセマンティクスを指定する場合は必ず要素数が3つのfloat型の配列にする必要があります。</span>
        配列の0番目は三角形の0番目の頂点と向き合っている辺に対応しています。
        配列の1番目は三角形の1番目の頂点と向き合っている辺と、配列の2番目は三角形の2番目の頂点と向き合っている辺にそれぞれ対応しています。
      </p>
    </li>
    <li><span class="keyward">SV_InsideTessFactor</span>
      <p>
        三角形の内部の分割数を指定します。
        <span class="important">三角形の場合、このシステムセマンティクスを指定する場合は必ずfloat型の変数にする必要があります。</span>
      </p>
    </li>
  </ul>
</div>

手動で分割数を決める際は不均等な分割になってしまう可能性があります。
意図的にそうしている場合はいいですが、避けたい場合はHLSLの組み込み関数を利用するといいでしょう。
コード例はサンプルを御覧ください。

ドキュメント：三角形のテッセレーション係数の修正を行う組み込み関数
<br>ProcessTriTessFactorsAvg
[(日本語)][TRIANGLE_PROCESS_AVE_JP]
[(英語)][TRIANGLE_PROCESS_AVE_EN]
<br>ProcessTriTessFactorsMax
[(日本語)][TRIANGLE_PROCESS_MAX_JP]
[(英語)][TRIANGLE_PROCESS_MAX_EN]
<br>ProcessTriTessFactorsMin
[(日本語)][TRIANGLE_PROCESS_MIN_JP]
[(英語)][TRIANGLE_PROCESS_MIN_EN]

[TRIANGLE_PROCESS_AVE_JP]:https://msdn.microsoft.com/ja-jp/library/ee422158(v=vs.85).aspx
[TRIANGLE_PROCESS_AVE_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471433(v=vs.85).aspx
[TRIANGLE_PROCESS_MAX_JP]:https://msdn.microsoft.com/ja-jp/library/ee422159(v=vs.85).aspx
[TRIANGLE_PROCESS_MAX_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471434(v=vs.85).aspx
[TRIANGLE_PROCESS_MIN_JP]:https://msdn.microsoft.com/ja-jp/library/ee422160(v=vs.85).aspx
[TRIANGLE_PROCESS_MIN_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471435(v=vs.85).aspx

ハルシェーダは以上のことに注意して実装する必要があります。

<h3>ドメインシェーダ</h3>
<span class="important">ドメインシェーダはテッセレータステージで分割、生成した頂点を加工するシェーダになります。</span>

ドキュメント：ドメイン シェーダーの設計
[(日本語)][DESING_DOMAIN_JP]
[(英語)][DESING_DOMAIN_EN]

[DESING_DOMAIN_JP]:https://msdn.microsoft.com/ja-jp/library/ee417838(v=vs.85).aspx
[DESING_DOMAIN_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff476337(v=vs.85).aspx

{% highlight c++ %}
// DSTriangle.hlsl
uint2 random(uint stream, uint sequence){
  //実装は省略
}
float4 calColor(float3 domain){
  //実装は省略
}

//出力データ
struct DS_OUTPUT
{
  float4 pos  : SV_POSITION;
  float4 color : TEXCOORD0;
};
// テッセレータが出力した頂点
struct HS_CONTROL_POINT_OUTPUT
{
  float3 pos : POSITION;
};
// 出力パッチ定数データ。
struct HS_CONSTANT_DATA_OUTPUT
{
  float EdgeTessFactor[3] : SV_TessFactor;
  float InsideTessFactor : SV_InsideTessFactor;
};
#define NUM_CONTROL_POINTS 3
//エントリポイントの定義
[domain("tri")]
DS_OUTPUT main(
  HS_CONSTANT_DATA_OUTPUT input,
  float3 domain : SV_DomainLocation,
  const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
  DS_OUTPUT output;
  output.pos = float4(patch[0].pos * domain.x + patch[1].pos * domain.y + patch[2].pos * domain.z, 1);
  output.color = calColor(domain);
  return output;
}
{% endhighlight %}

使用している構造体が多いですが、エントリポイント自体はシンプルな処理になります。
{% highlight c++ %}
//エントリポイントの定義
[domain("tri")]
DS_OUTPUT main(
  //パッチ定数関数からの出力値
  HS_CONSTANT_DATA_OUTPUT input,
  //パッチ内での位置
  float3 domain : SV_DomainLocation,
  //ハルシェーダから出力された制御点
  const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
  DS_OUTPUT output;
  output.pos = float4(patch[0].pos * domain.x + patch[1].pos * domain.y + patch[2].pos * domain.z, 1);
  output.color = calColor(domain);
  return output;
}
{% endhighlight %}

<span class="important">エントリポイントにつける<span class="keyward">domain属性</span>はパッチのプリミティブを指定します。</span>
<span class="important">ハルシェーダと同じくtri、quad、isolineが指定できます。</span>

ドキュメント：<span class="keyward">domain属性</span>
[(日本語)][DOMAIN_ATTR_JP]
[(英語)][DOMAIN_ATTR_EN]

[DOMAIN_ATTR_JP]:https://msdn.microsoft.com/ja-jp/library/ee422313(v=vs.85).aspx
[DOMAIN_ATTR_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471438(v=vs.85).aspx


<span class="important">エントリポイントの引数にはハルシェーダからの出力値とパッチ内での位置（<span class="keyward">SV_DomainLocation</span>）が渡されます。</span>

<div class="argument">
  <h4>SV_DomainLocation</h4>
  <p>
    ドキュメント：
    <a href="https://msdn.microsoft.com/ja-jp/library/ee422448(v=vs.85).aspx">(日本語)</a>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff471567(v=vs.85).aspx">(英語)</a>
  </p>
  <p>
    生成された頂点のパッチ上の位置を求めるために使うものになります。
    <span class="important">ドメインシェーダのエントリポイントには必ずこのセマンティクスを付けた引数を用意する必要があります。</span>
    このセマンティクスを付けた変数の型は<span class="keyward">domain属性</span>で指定したプリミティブによって異なります。
    <span class="important">三角形の場合は<span class="keyward">float3</span>にする必要があります。</span>
    float3の各要素はxが0番目の頂点に、yは1番目の頂点、zが2番目の頂点の係数になります。
    位置を求めるには以下のように補間してください。
    {% highlight c++ %}
output.pos = float4(patch[0].pos * domain.x + patch[1].pos * domain.y + patch[2].pos * domain.z, 1);
    {% endhighlight %}
  </p>
</div>

三角形の分割は以上になります。
設定しないといけないものが多いですが使っていくうちに慣れるでしょう。
残りの四角形と線分は三角形と異なる部分について見てきます。

<a name="SQUARE"></a>
<h1 class="under-bar">四角形の分割</h1>
<span class="important">四角形を分割する際は各<span class="keyward">domain属性</span>に<span class="keyward">quad</span>を指定してください。</span>

<h3>ハルシェーダ</h3>
処理自体は三角形の分割とそう変わりはありません。
分割数の指定の仕方が少し変わったぐらいでしょうか。
{% highlight c++ %}
// HSQuad.hlsl一部
//パッチ定数関数の出力
struct HS_CONSTANT_DATA_OUTPUT
{
  float EdgeTessFactor[4] : SV_TessFactor;
  float InsideTessFactor[2] : SV_InsideTessFactor;
};
#define NUM_CONTROL_POINTS 4
// パッチ定数関数
HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
  InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
  uint PatchID : SV_PrimitiveID)
{
  HS_CONSTANT_DATA_OUTPUT Output;
  [unroll] for (uint i = 0; i < NUM_CONTROL_POINTS; ++i) {
    Output.EdgeTessFactor[i] = cbEdgeFactor;
  }
  Output.InsideTessFactor[0] = Output.InsideTessFactor[1] = cbInsideFactor;
  return Output;
}
//エントリポイント
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("CalcHSPatchConstants")]
[maxtessfactor(16.f)]
HS_CONTROL_POINT_OUTPUT main(
  InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> inputPatch,
  uint i : SV_OutputControlPointID,
  uint PatchID : SV_PrimitiveID)
{
  HS_CONTROL_POINT_OUTPUT Output;
  Output.pos = inputPatch[i].pos;
  return Output;
}
{% endhighlight %}

<div class="argument">
  <h4>四角形の場合の分割数の指定</h4>
  <ul>
    <li><span class="keyward">SV_TessFactor</span>
      <p>
        四角形の辺の分割数を指定します。
        <span class="important">四角形の場合、このシステムセマンティクスを指定する場合は必ず要素数が4つのfloat型の配列にする必要があります。</span>
        <br>配列の0番目は四角形の0番目のと2番目をつなぐ辺に対応しています。
        <br>配列の1番目は四角形の0番目のと1番目をつなぐ辺に対応しています。
        <br>配列の2番目は四角形の1番目のと3番目をつなぐ辺に対応しています。
        <br>配列の3番目は四角形の2番目のと3番目をつなぐ辺に対応しています。
      </p>
    </li>
    <li><span class="keyward">SV_InsideTessFactor</span>
      <p>
        四角形の内部の分割数を指定します。
        <span class="important">四角形の場合、このシステムセマンティクスを指定する場合は必ず要素数が2つのfloat型の配列にする必要があります。</span>
        <span class="important">内部の分割はいわゆる縦と横それぞれの分割になります。</span>
        <br>配列の0番目は四角形の0番目のと1番目をつなぐ辺の向きに分割する数になります。
        <br>配列の1番目は四角形の0番目のと2番目をつなぐ辺の向きに分割する数になります。
      </p>
    </li>
  </ul>
</div>

三角形の時と同じように分割数の調節を行う組み込み関数が用意されています。

ドキュメント：四角形のテッセレーション係数の修正を行う組み込み関数
<br>ProcessQuadTessFactorsAvg
[(日本語)][QUAD_PROCESS_AVE_JP]
[(英語)][QUAD_PROCESS_AVE_EN]
<br>ProcessQuadTessFactorsMax
[(日本語)][QUAD_PROCESS_MAX_JP]
[(英語)][QUAD_PROCESS_MAX_EN]
<br>ProcessQuadTessFactorsMin
[(日本語)][QUAD_PROCESS_MIN_JP]
[(英語)][QUAD_PROCESS_MIN_EN]
<br>Process2DQuadTessFactorsAvg
[(日本語)][QUAD2D_PROCESS_AVE_JP]
[(英語)][QUAD2D_PROCESS_AVE_EN]
<br>Process2DQuadTessFactorsMax
[(日本語)][QUAD2D_PROCESS_MAX_JP]
[(英語)][QUAD2D_PROCESS_MAX_EN]
<br>Process2DQuadTessFactorsMin
[(日本語)][QUAD2D_PROCESS_MIN_JP]
[(英語)][QUAD2D_PROCESS_MIN_EN]


[QUAD_PROCESS_AVE_JP]:https://msdn.microsoft.com/ja-jp/library/ee422155(v=vs.85).aspx
[QUAD_PROCESS_AVE_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471430(v=vs.85).aspx
[QUAD_PROCESS_MAX_JP]:https://msdn.microsoft.com/ja-jp/library/ee422156(v=vs.85).aspx
[QUAD_PROCESS_MAX_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471431(v=vs.85).aspx
[QUAD_PROCESS_MIN_JP]:https://msdn.microsoft.com/ja-jp/library/ee422157(v=vs.85).aspx
[QUAD_PROCESS_MIN_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471432(v=vs.85).aspx
[QUAD2D_PROCESS_AVE_JP]:https://msdn.microsoft.com/ja-jp/library/ee422150(v=vs.85).aspx
[QUAD2D_PROCESS_AVE_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471426(v=vs.85).aspx
[QUAD2D_PROCESS_MAX_JP]:https://msdn.microsoft.com/ja-jp/library/ee422151(v=vs.85).aspx
[QUAD2D_PROCESS_MAX_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471427(v=vs.85).aspx
[QUAD2D_PROCESS_MIN_JP]:https://msdn.microsoft.com/ja-jp/library/ee422152(v=vs.85).aspx
[QUAD2D_PROCESS_MIN_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471428(v=vs.85).aspx

<h3>ドメインシェーダ</h3>
ドメインシェーダも行う処理自体に変わりはありません。
<span class="important">パッチ内の位置を計算する方法が変わるぐらいでしょう。</span>

{% highlight c++ %}
// DSQuad.hlsl
uint2 random(uint stream, uint sequence){
  //実装省略
}
float4 calColor(float2 domain){
  //実装省略
}
//ドメインシェーダの出力
struct DS_OUTPUT
{
  float4 pos  : SV_POSITION;
  float4 color : TEXCOORD0;
};
// ハルシェーダからの出力制御点
struct HS_CONTROL_POINT_OUTPUT
{
  float3 pos : POSITION;
};
// 出力パッチ定数データ。
struct HS_CONSTANT_DATA_OUTPUT
{
  float EdgeTessFactor[4]: SV_TessFactor;
  float InsideTessFactor[2] : SV_InsideTessFactor;
};
#define NUM_CONTROL_POINTS 4
//エントリポイント
[domain("quad")]
DS_OUTPUT main(
  HS_CONSTANT_DATA_OUTPUT input,
  float2 domain : SV_DomainLocation,
  const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
  DS_OUTPUT output;
  //4点間の補間 bilinear interpolation
  output.pos = float4(
    patch[0].pos * (1 - domain.x) * (1 - domain.y)+
    patch[1].pos * domain.x * (1 - domain.y) +
    patch[2].pos * (1 - domain.x) * domain.y +
    patch[3].pos * domain.x * domain.y, 1);
  output.color = calColor(domain);
  return output;
}
{% endhighlight %}
<div class="argument">
  <h4>四角形の時のSV_DomainLocation</h4>
  <p>
    <span class="important">四角形の場合は<span class="keyward">float2</span>にする必要があります。</span>
    意味は0～1の範囲のUV座標系とおなじになります。
    <br>(0,0)で0番目の頂点の位置に、
    <br>(1,0)で1番目の頂点の位置に、
    <br>(0,1)で2番目の頂点の位置に、
    <br>(1,1)で3番目の頂点の位置になります。
    位置を求めるには以下のように補間してください。
    {% highlight c++ %}
//4点間の補間 bilinear interpolation
output.pos = float4(
  patch[0].pos * (1 - domain.x) * (1 - domain.y)+
  patch[1].pos * domain.x * (1 - domain.y) +
  patch[2].pos * (1 - domain.x) * domain.y +
  patch[3].pos * domain.x * domain.y, 1);
    {% endhighlight %}
  </p>
</div>

<a name="ISOLINE"></a>
<h1 class="under-bar">線分の分割</h1>
<span class="important">線分を分割する際は各<span class="keyward">domain属性</span>に<span class="keyward">isoline</span>を指定してください。</span>

<h3>ハルシェーダ</h3>
線分も前2つとほぼ同じです。
<span class="important">ただし、分割数を指定するための<span class="keyward">SV_TessFactor</span>の意味が少々異なり、SV_InsideTessFactorは必要ありません。</span>
{% highlight c++ %}
// HSIsoline.hlslの一部
//パッチ定数データ
struct HS_CONSTANT_DATA_OUTPUT
{
  float EdgeTessFactor[2] : SV_TessFactor;
};
#define NUM_CONTROL_POINTS 2
// パッチ定数関数
HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
  InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
  uint PatchID : SV_PrimitiveID)
{
  HS_CONSTANT_DATA_OUTPUT Output;

  Output.EdgeTessFactor[0] = cbDetailFactor;
  Output.EdgeTessFactor[1] = cbDensityFactor;
  return Output;
}
//エントリポイント
[domain("isoline")]
[partitioning("pow2")]
[outputtopology("line")]
[outputcontrolpoints(2)]
[patchconstantfunc("CalcHSPatchConstants")]
[maxtessfactor(64.f)]
HS_CONTROL_POINT_OUTPUT main(
  InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> inputPatch,
  uint i : SV_OutputControlPointID,
  uint PatchID : SV_PrimitiveID)
{
  HS_CONTROL_POINT_OUTPUT Output;
  Output.pos = inputPatch[i].pos;
  return Output;
}
{% endhighlight %}
<div class="argument">
  <h4>線分の場合の分割数の指定</h4>
  <ul>
    <li><span class="keyward">SV_TessFactor</span>
      <p>
        線分の辺の分割数を指定します。
        <span class="important">線分の場合、このシステムセマンティクスを指定する場合は必ず要素数が2つのfloat型の配列にする必要があります。</span>
        <br><span class="important">配列の0番目は線分の本数を調節するものになります。</span>
        <br><span class="important">配列の1番目は1線分の分割数になります。</span>
      </p>
    </li>
  </ul>
</div>

線分も同じように分割数の調節を行う組み込み関数が用意されています。

ドキュメント：線分のテッセレーション係数の修正を行う組み込み関数
<br>ProcessIsolineTessFactors
[(日本語)][ISOLINE_PROCESS_AVE_JP]
[(英語)][ISOLINE_PROCESS_AVE_EN]

[ISOLINE_PROCESS_AVE_JP]:https://msdn.microsoft.com/ja-jp/library/ee422154(v=vs.85).aspx
[ISOLINE_PROCESS_AVE_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/ff471429(v=vs.85).aspx

<h3>ドメインシェーダ</h3>
ドメインシェーダもあまり変わりはありません。
<span class="important">SV_DomainLocationの意味が変わったぐらいです。</span>

{% highlight c++ %}
// DSIsoline.hlsl
uint2 random(uint stream, uint sequence){
  //実装省略
}
float4 calColor(float2 domain){
  //実装省略
}
struct DS_OUTPUT
{
  float4 pos  : SV_POSITION;
  float4 color : TEXCOORD0;
};
// 出力制御点
struct HS_CONTROL_POINT_OUTPUT
{
  float3 pos : POSITION;
};
// 出力されたパッチ定数データ。
struct HS_CONSTANT_DATA_OUTPUT
{
  float EdgeTessFactor[2] : SV_TessFactor;
};
#define NUM_CONTROL_POINTS 2
[domain("isoline")]
DS_OUTPUT main(
  HS_CONSTANT_DATA_OUTPUT input,
  float2 domain : SV_DomainLocation,
  const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
  DS_OUTPUT output;
  //適当に位置をずらしている
  //実際にはパッチ定数関数の出力値を元に処理を行う形になると思う。
  output.pos = float4(lerp(patch[0].pos, patch[1].pos, domain.x), 1);
  output.pos.x += 0.5f * domain.y;
  //上に凸な曲線を描くためにyの位置をずらしている。
  const float RATE = abs(domain.x - 0.5f) * 2.f;
  const float PI = 3.14159265f;
  output.pos.y = cos(RATE * 0.5f * PI);

  output.color = calColor(domain);
  output.color.b = 0;
  return output;
}
{% endhighlight %}
<div class="argument">
  <h4>線分の時のSV_DomainLocation</h4>
  <p>
    <span class="important">線分の場合は<span class="keyward">float2</span>にする必要があります。</span>
    x成分が生成した線分を表すものでy成分が分割数となります。
    <span class="important">両方共0～1の範囲を取りますので注意してください。</span>
    線分の場合はパッチ定数関数の出力を使うのが必須となりそうです。
  </p>
</div>
<a name="POINT_TO_TRIANGLE"></a>
<h1 class="under-bar">点から三角形を生成</h1>
テッセレーションステージもジオメトリシェーダと同じく点から三角形を生成することが出来ます。
<span class="important">その際はパッチの制御点に1を指定する必要があります。</span>

{% highlight c++ %}
// HSPointToTriangle.hlslの一部
//もとは点なので制御点は1つになる
//InputPatch<>にて使用している
#define NUM_CONTROL_POINTS 1
//　パッチ定数関数
HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
  InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
  uint PatchID : SV_PrimitiveID)
{
  //...省略
}
[domain("tri")]//生成するプリミティブを指定する
//..省略
HS_CONTROL_POINT_OUTPUT main(
  InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> inputPatch,
  uint i : SV_OutputControlPointID,
  uint PatchID : SV_PrimitiveID)
{
  //..省略
}

// DSPointToTriangle.hlslの一部
// ドメインシェーダもハルシェーダと同じように設定する
// 制御点以外は三角形の分割と同じになる
//制御点は点なので当然1となる
//OutputPatch<>で使っている
#define NUM_CONTROL_POINTS 1
[domain("tri")]
DS_OUTPUT main(
  HS_CONSTANT_DATA_OUTPUT input,
  float3 domain : SV_DomainLocation,
  const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
  //...省略
}
{% endhighlight %}

<span class="important">InputPatchとOutputPatchの制御点数に1を設定している以外は三角形の分割と同じです。</span>

<a name="SUMMARY"></a>
<h1 class="under-bar">まとめ</h1>

今回はテッセレーションについて見てきました。
GPUの固定機能を使用しているため設定する部分が多く、使いこなすには知識も必要となります。
ですが行っていることは複雑ではないのでハルシェーダ、バッチ定数関数、ドメインシェーダの役割を把握すればうまくあつかえるようになるのではないでしょうか？

<span class="important">後、本文では説明しませんでしたがテッセレーションを使用する際、入力アセンブラステージのプリミティブトポロジにはパッチの制御点の個数を表すものを設定する必要がありますので忘れずに設定してください。</span>

このパートでグラフィックスパイプラインも一通り見終えました。
ここまでくればDX11の一山を登り終えたといえます。
シェーダについてはこれ以上新しい物はありません。
ここまでの内容があれば後は３DCG知識があれば実装に困ることはあまりないと思います。

<table class="table table-condensed">
  <tbody>
    <tr>
      <td class="left"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/stream-output">＜前</a></td>
      <td class="center"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}">トップ</a></td>
      <td class="right"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/deferred-context">次＞</a></td>
    </tr>
  </tbody>
</table>
