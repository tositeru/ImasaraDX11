---
layout: default
title: "ジオメトリシェーダ"
categories: part
description: ""
---
<h1 class="under-bar">ジオメトリシェーダ</h1>

今回はジオメトリシェーダについて見ていきます。

ジオメトリシェーダはDirect3D10から追加されたステージでシェーダモデル4.0以上に対応したGPUで使用できます。
グラフィックスパイプラインではラスタライザステージの前に実行されます。
<span class="important">特徴はGPU内で線分や三角形を生成することが可能な点でしょう。</span>
<span class="important">さらに頂点シェーダでは1つの頂点の情報しか見れませんでしたが、ジオメトリシェーダではペアとなる他の頂点の情報も見ることが出来ます。</span>
ジオメトリシェーダを利用することにより、グラフィックスパイプラインはより自由度の高い処理が行えるようになります。

ドキュメント：ジオメトリシェーダ
[(日本語)][Geometry-Shader_JP]
[(英語)][Geometry-Shader_EN]

[Geometry-Shader_JP]:https://msdn.microsoft.com/ja-jp/library/ee415747(v=vs.85).aspx#Geometry_Shader_Stage
[Geometry-Shader_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/bb205146(v=vs.85).aspx#Geometry_Shader_Stage

<h1 class="under-bar">概要</h1>
今パートではジオメトリシェーダの書き方について見ていきます。
対応するプロジェクトは<span class="important">Part10_GeometryShader</span>になります。
<div class="summary">
  <ol>
    <li><a href="#SHADER">ジオメトリシェーダ</a></li>
    <li><a href="#MAKE_PRIMITIVE">頂点バッファを使わない三角形を描画</a></li>
    <li><a href="#SUMMARY">まとめ</a></li>
  </ol>
</div>

<a name="SHADER"></a>
<h1 class="under-bar">ジオメトリシェーダ</h1>

<h3>シェーダ</h3>
ジオメトリシェーダは以下のように実装します。

{% highlight hlsl %}
//GeometryShader.hlsl
struct GSOutput
{
  float4 pos : SV_POSITION;
  float4 color : COLOR0;
};
static const float4 trianglePos[3] = {
  float4(0.f, 0.5f, 0.f, 0.f),
  float4(0.5f, -0.5f, 0.f, 0.f),
  float4(-0.5f, -0.5f, 0.f, 0.f),
};
//maxvertexcount属性：最大3つの頂点を生成することを宣言している
[maxvertexcount(3)]
void main(
  point float4 input[1] : POSITION,
  inout TriangleStream< GSOutput > output//三角形を生成するときに使うもの
){
  //点から三角形を生成するシェーダ
  //三角形を構成する頂点の生成するループ
  [unroll] for (uint i = 0; i < 3; i++)
  {
    GSOutput element;
    element.pos = input[0] + trianglePos[i];
    element.color = float4(1.0f, 1.0f, 0.3f, 1.f);
    output.Append(element);//ここで頂点を生成している
  }
  output.RestartStrip();//Append関数で生成した頂点を三角形として構成している
}
{% endhighlight %}

頂点シェーダやピクセルシェーダと違って、キーワードなどが多いですが1つずつ見ていきましょう。

<h4>maxvertexcount属性</h4>
<span class="keyward">maxvertexcount属性</span>はジオメトリシェーダが生成する頂点の最大数を指定する属性になります。
必ず記述してください。
カッコの中の数字が最大数になります。

<h4>プリミティブ型</h4>
まず、main関数の引数にある<span class="keyward">pointキーワード</span>です。
{% highlight hlsl %}
//頂点シェーダから点情報を受け取っている
//必ず配列にすること
void main(
  point float4 input[1] : POSITION,
  ...
//線分の場合
  line float4 input[2] : POSITION,
//三角形の場合
  triangle float4 input[3] : POSITION,
{% endhighlight %}

<span class="important"><span class="keyward">pointキーワード</span>はジオメトリシェーダの入力として点情報が渡されることを表しています。</span>
<span class="important">もちろん、線分や三角形のものもそれぞれ<span class="keyward">line</span>、<span class="keyward">triangle</span>として用意されています。</span>
<span class="important">また、線分と三角形には隣接する頂点も受け取る<span class="keyward">lineadj</span>と<span class="keyward">triangleadj</span>も用意されています。</span>
<span class="important">この2つを使用するときは頂点バッファ（またはインディクスバッファ）の並びを専用の並びにする必要がありますので注意してください。</span>
日本語の方はキーワードの説明までしか書かれていませんので、その詳細は英語ドキュメントを参考にしてください。
<br>ドキュメント：
[ジオメトリ シェーダー オブジェクト][Geometry-Shader_Object_JP]
[Geometry-Shader Object][Geometry-Shader_Object_EN]

[Geometry-Shader_Object_JP]:https://msdn.microsoft.com/ja-jp/library/ee418313(v=vs.85).aspx
[Geometry-Shader_Object_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/bb509609(v=vs.85).aspx

プリミティブ型一覧
<ul>
  <li>point:点情報</li>
  <li>line:線分情報</li>
  <li>triangle:三角形情報</li>
  <li>lineadj:隣接する頂点も含む線分情報</li>
  <li>triangleadj:隣接する頂点も含む三角形情報</li>
</ul>

<h4>ストリーム出力オブジェクト</h4>
次は新しいプリミティブを生成するとき使うオブジェクトです。
{% highlight hlsl %}
//TriangleStreamは三角形を生成するときに使うもの
void main(
  ...
  inout TriangleStream< GSOutput > output){
//線分の場合
  inout LineStream< GSOutput > output
//点の場合
  inout PointStream< GSOutput > output
{% endhighlight %}
<span class="keyward">TriangleStream</span>が三角形の生成を行います。
<span class="important">テンプレート引数のように<span class="keyward"><...></span>の中で頂点の型を指定します。</span>
<span class="important">必ず、inoutキーワードを付けてください。</span>

ストリーム出力オブジェクトには2つのメンバ関数が用意されています。
<ul>
  <li>Append:頂点データをストリーム追加する</li>
  <li>RestartStrip:追加した頂点でプリミティブを構成し、新しいプリミティブの生成を開始する</li>
</ul>

関数の詳しい説明はドキュメントを参考にしてください。
<br>ドキュメント：
[ストリーム出力オブジェクト][Stream-Output_Object_JP]
[Stream-Output Object][Stream-Output_Object_EN]

[Stream-Output_Object_JP]:https://msdn.microsoft.com/ja-jp/library/ee418375(v=vs.85).aspx
[Stream-Output_Object_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/bb509661(v=vs.85).aspx

ジオメトリシェーダでは以上の要素を使って実装します。
CPU側ではジオメトリシェーダは<span class="keyward">ID3D11GeometryShader</span>と表現されています。
シェーダのコンパイル時にはシェーダモデルに<span class="keyward">"gs_"</span>を指定してコンパイルしてください。
<span class="keyward">ID3D11GeometryShader</span>の作成とグラフィックスパイプラインへの設定は他のシェーダと似ていますので省略します。
サンプルを御覧ください。

<a name="MAKE_PRIMITIVE"></a>
<h1 class="under-bar">頂点バッファを使わない三角形を描画</h1>

<span class="important">ジオメトリシェーダを使用すると頂点バッファを使わずともプリミティブを描画することが可能です。</span>
その時は空の構造体をジオメトリシェーダの入力として受け取る必要があります。

{% highlight hlsl %}
// GeometryShader2.hlsl
//ダミーの入力値
//前のステージから何も受け取らない時はダミー構造体を受け取る必要がある
struct DummyInput {};
struct GSOutput
{
  float4 pos : SV_POSITION;
  float4 color : COLOR0;
};
static const float4 trianglePos[3] = {
  float4(0.f, 0.5f, 0.f, 1.f),
  float4(0.5f, -0.5f, 0.f, 1.f),
  float4(-0.5f, -0.5f, 0.f, 1.f),
};
[maxvertexcount(3)]
void main(
  point DummyInput input[1] : POSITION,
  in uint id : SV_PrimitiveID,
  inout TriangleStream< GSOutput > output
  )
{
  [unroll] for (uint i = 0; i < 3; i++) {
    GSOutput element;
    element.pos = trianglePos[i];
    element.color = float4(0.3f, 1.0f, 1.0f, 1.f);
    output.Append(element);
  }
}
{% endhighlight %}

上の<span class="keyward">SV_PrimitiveID</span>というシステムセマンティクスは描画中のプリミティブの識別子になります。
例えば、三角形を10個描画したとすると、<span class="keyward">SV_PrimitiveID</span>は0～9の値を各三角形に振り分けます。
頂点バッファを使用しない場合はシェーダリソースからデータを取得することになると思いますので、その時に使用するシステムセマンティクスになります。

<h4>何もしない頂点シェーダ</h4>
<span class="important">頂点バッファを使用しない時は何もしない頂点シェーダを用意する必要があります。</span>
<span class="important">このシェーダを使うときは入力レイアウトは必要ありません。</span>

{% highlight hlsl %}
// VSDummy.hlsl
//何もしない頂点シェーダ
void main()
{}
{% endhighlight %}

<a name="SUMMARY"></a>
<h1 class="under-bar">まとめ</h1>

以上でジオメトリシェーダの書き方についての説明は終わります。
<span class="important">ジオメトリシェーダはプリミティブを生成できる特徴からかなり自由度の高いシェーダになります。</span>
デバッグ用の簡易な箱をシェーダのみで生成できたりとちょっとしたこともできますので活用していきましょう。

また、グラフィックスパイプラインで処理したプリミティブをバッファに出力することができます。
これについては次のパートで見ていきます。

<table class="table table-condensed">
  <tbody>
    <tr>
      <td class="left"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/rasterizer-state">＜前</a></td>
      <td class="center"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}">トップ</a></td>
      <td class="right"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/stream-output">次＞</a></td>
    </tr>
  </tbody>
</table>
