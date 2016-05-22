---
layout: default
title: "タイルリソース"
categories: part
description: "タイルリソースはGPU版仮想メモリみたいなもので、GPUのメモリに収まらないほど巨大なデータを扱うときなどに使われます。"
---
<h1 class="under-bar">タイルリソース</h1>

今パートではDX11.2から導入されたタイルリソース(英訳：Tiled Resource)についてみていきます。

タイルリソースはGPU版仮想メモリみたいなもので、GPUのメモリに収まらないほど巨大なデータを扱うときなどに使われます。
もとはメガテクスチャと呼ばれていたりとか、今流行のオープンワールド系のゲームや自動地形生成などで使われているそうです。
が、書いている人自身あまり詳しくないので上の内容が間違っているかもしれません。
ここでは使用法だけについて見ていきます。

<span class="important">タイルリソースはバッファとテクスチャ両方に対応していますので目的にあった方を使用してください。</span>

<h1 class="under-bar">概要</h1>
対応しているプロジェクトは<span class="important">Part17_TiledResource</span>になります。
タイルリソースはDX11.2に対応しているGPU上でしか利用できません。
そのため一部のGPUではサンプルを実行できない場合がありますがご容赦ください。

<div class="summary">
  <ol>
    <li><a href="#CREATE_RESOURCE">タイルリソースの作成</a></li>
    <li><a href="#CREATE_POOL">タイルプールの作成</a></li>
    <li><a href="#BIND_POOL">タイルリソースへタイルプールを紐付け</a></li>
    <li><a href="#UPDATE">内容の更新</a></li>
    <li><a href="#SUMMARY">まとめ</a></li>
    <li><a href="#SUPPLEMENTAL">補足</a>
      <ul>
        <li>タイルリソースに対応しているかの確認の仕方</li>
      </ul>
    </li>
  </ol>
</div>

<a name="CREATE_RESOURCE"></a>
<h1 class="under-bar">タイルリソースの作成</h1>

タイルリソースを使う場合に必要となるものはタイルリソースとタイププールの2つになります。
<span class="important">タイルプールがメモリの実態を表し、タイルリソースを介してタイルプールの内容にアクセスします。</span>

まず、タイルリソースの作成について見ていきましょう。
<span class="important">といっても他のリソースとほとんど変わりません。</span>
<span class="important">作成時に<span class="keyward">MiscFlags</span>へ<span class="keyward">D3D11_RESOURCE_MISC_TILED</span>を設定するだけです。</span>
<span class="important">リソースを作成した後は必要に応じてビューを作成することも変わりません。</span>

{% highlight c++ %}
// Scene::onInit関数の一部
//タイルリソースの作成
auto desc = makeTex2DDesc(this->width(), this->height(), DXGI_FORMAT_R8G8B8A8_UNORM);
//タイルリソースとして扱うときは必ずD3D11_RESOURCE_MISC_TILEDを指定すること
desc.MiscFlags = D3D11_RESOURCE_MISC_TILED;
desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
//desc.MipLevels = calMaxMipLevel(desc.Width, desc.Height);
//複数のタイルリソースを作成するので、作成処理を1つにまとめた
auto create = [&](TileTexture* pOut) {
  auto hr = this->mpDevice->CreateTexture2D(&desc, nullptr, pOut->mpResource.GetAddressOf());
  if (FAILED(hr)) {
    throw std::runtime_error("タイルリソースの作成に失敗");
  }
  //必要となるビューの作成
  hr = this->mpDevice->CreateShaderResourceView(pOut->mpResource.Get(), nullptr, pOut->mpSRV.GetAddressOf());
  if (FAILED(hr)) {
    throw std::runtime_error("タイルリソースのシェーダリソースビューの作成に失敗");
  }
  hr = this->mpDevice->CreateUnorderedAccessView(pOut->mpResource.Get(), nullptr, pOut->mpUAV.GetAddressOf());
  if (FAILED(hr)) {
    throw std::runtime_error("タイルリソースのアンオーダードアクセスビューの作成に失敗");
  }
};
this->mTextures.resize(6);
for (auto& tex : this->mTextures) {
  create(&tex);
}
create(&this->mTargetTex);
{% endhighlight %}

<a name="CREATE_POOL"></a>
<h1 class="under-bar">タイルプールの作成</h1>
次にタイルプールの作成について見ていきます。

<span class="important">タイルプールは<span class="keyward">ID3D11Buffer</span>として作成します。</span>
<span class="important">作成の際は<span class="keyward">MiscFlags</span>に<span class="keyward">D3D11_RESOURCE_MISC_TILE_POOL</span>を指定した上で、サイズが必ず64KBの倍数になるようにしなければなりません。</span>

<span class="important">タイルプールはタイルと呼ばれるものの集合になり、1タイル当たり64KBのサイズとなります。</span>
タイルリソースを扱う上で、タイルの集合体を操作している事を念頭に置くことが大事です。

{% highlight c++ %}
// Scene::onInitの一部
//タイルプールの作成
D3D11_BUFFER_DESC desc = {};
desc.ByteWidth = 64 * 1024;//64KBの倍数でないといけない
desc.Usage = D3D11_USAGE_DEFAULT;
desc.MiscFlags = D3D11_RESOURCE_MISC_TILE_POOL;
hr = this->mpDevice->CreateBuffer(&desc, nullptr, this->mpTilePool.GetAddressOf());
if (FAILED(hr)) {
  throw std::runtime_error("タイルプールの作成に失敗");
}

//タイルリソースの情報を取得する
//タイルリソースに関係する操作はID3D11Device2やID3D11DeviceContext2を使う必要がある
Microsoft::WRL::ComPtr<ID3D11Device2> pDevice2;
auto hr = this->mpDevice.Get()->QueryInterface(IID_PPV_ARGS(&pDevice2));
if (FAILED(hr)) {
  throw std::runtime_error("ID3D11Device2が使えません。");
}
TileInfo tileInfo;
tileInfo.subresourceTileCount = 1;
tileInfo.firstSubresourceTile = 0;
pDevice2->GetResourceTiling(this->mTargetTex.mpResource.Get(), &tileInfo.tileCount, &tileInfo.packedMipDesc, &tileInfo.tileShape, &tileInfo.subresourceTileCount, tileInfo.firstSubresourceTile, &tileInfo.subresourceTiling);
for (auto& tex : this->mTextures) {
  tex.mInfo = tileInfo;
}
this->mTargetTex.mInfo = tileInfo;
//タイルプールのサイズを必要となるサイズに変更している
UINT64 unitSize = tileInfo.tileCount * (64 * 1024);//タイルリソース１枚あたりのサイズ
hr = this->mpContext2->ResizeTilePool(this->mpTilePool.Get(), unitSize * this->mTextures.size());
if (FAILED(hr)) {
  throw std::runtime_error("タイルプールのリサイズに失敗");
}
{% endhighlight %}

<h3>タイルプールのリサイズ</h3>
<span class="important">タイルプールは作成後<span class="keyward">ID3D11DeviceContext2::ResizeTilePool</span>でそのサイズを変更することが出来ます。</span>
<span class="important">変更する場合もそのサイズは64KBの倍数か0でなければなりませんの注意してください。</span>

ちなみに<span class="keyward">ID3D11DeviceContext2</span>は<span class="keyward">ID3D11DeviceContext::QueryInterface</span>で取得できます。
詳しい使い方は下のコードを参考にしてください。

ドキュメント：<span class="keyward">ID3D11DeviceContext2::ResizeTilePool</span>
[(英語)][ResizeTilePool_EN]

[ResizeTilePool_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/dn280505(v=vs.85).aspx

{% highlight c++ %}
//ID3D11DeviceContext2の取得
auto hr = this->mpImmediateContext.Get()->QueryInterface(IID_PPV_ARGS(&mpContext2));
if (FAILED(hr)) {
  throw std::runtime_error("ID3D11DeviceContext2が使えません");
}
//タイルプールのサイズを必要となるサイズに変更している
UINT64 unitSize = tileInfo.tileCount * (64 * 1024);//タイルリソース１枚あたりのサイズ
hr = this->mpContext2->ResizeTilePool(this->mpTilePool.Get(), unitSize * this->mTextures.size());
if (FAILED(hr)) {
  throw std::runtime_error("タイルプールのリサイズに失敗");
}
{% endhighlight %}

<h3>タイルリソースが使用するタイル数の調べ方</h3>
タイルプールのサイズは自由に変更できますが、実際に必要となるサイズがどの程度か把握できればそれに越したことはありません。
<span class="important">そのような場合はタイルリソースの情報を調べる<span class="keyward">ID3D11Device2::GetResourceTiling</span>を使うと便利でしょう。</span>

<span class="keyward">ID3D11Device2::GetResourceTiling</span>を使うとタイルリソースに必要となるタイルの数や横と縦と奥行きのタイル数、ミップマップに関係する情報を取得できます。
サンプルでは1つのタイルリソースが必要となるタイル数を調べ、使用するタイルリソース分のタイルを確保しています。

<span class="keyward">ID3D11Device2</span>も<span class="keyward">ID3D11DeviceContext2</span>と同じく<span class="keyward">ID3D11Device::QueryInterface</span>で取得できます。

ドキュメント：<span class="keyward">ID3D11Device2::GetResourceTiling</span>
[(英語)][GetResourceTiling_EN]

[GetResourceTiling_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/dn280497(v=vs.85).aspx

{% highlight c++ %}
//タイルリソースの情報を取得する
//タイルリソースに関係する操作はID3D11Device2やID3D11DeviceContext2を使う必要がある
Microsoft::WRL::ComPtr<ID3D11Device2> pDevice2;
auto hr = this->mpDevice.Get()->QueryInterface(IID_PPV_ARGS(&pDevice2));
if (FAILED(hr)) {
  throw std::runtime_error("ID3D11Device2が使えません。");
}
TileInfo tileInfo;
tileInfo.subresourceTileCount = 1;
tileInfo.firstSubresourceTile = 0;
pDevice2->GetResourceTiling(this->mTargetTex.mpResource.Get(), &tileInfo.tileCount, &tileInfo.packedMipDesc, &tileInfo.tileShape, &tileInfo.subresourceTileCount, tileInfo.firstSubresourceTile, &tileInfo.subresourceTiling);
{% endhighlight %}

<a name="BIND_POOL"></a>
<h1 class="under-bar">タイルリソースへタイルプールを紐付け</h1>

タイルリソースとタイルプールの両方作成した後はタイルリソースへタイルプールを紐付けます。
<span class="important">紐付け方には直接タイルプールの場所を指定する方法と他のタイルリソースに設定されているものをコピーする方法があります。</span>

<h3>直接タイルプールの場所を指定する方法</h3>
直接、タイルリソースへタイルプールの場所を指定するには<span class="keyward">ID3D11DeviceContext2::UpdateTileMappings</span>を使用します。

ドキュメント:<span class="keyward">ID3D11DeviceContext2::UpdateTileMappings</span>
[(英語)][UpdateTileMappings_EN]

[UpdateTileMappings_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/dn280508(v=vs.85).aspx

{% highlight c++ %}
// Scene::onInit関数の一部
D3D11_TEXTURE2D_DESC desc;
this->mTargetTex.mpResource->GetDesc(&desc);
auto& tileInfo = this->mTargetTex.mInfo;
//設定するタイルリソース内の範囲
std::array<D3D11_TILE_REGION_SIZE, 1 > regionSizesInResource;
regionSizesInResource[0].Width = tileInfo.subresourceTiling.WidthInTiles;
regionSizesInResource[0].Height = tileInfo.subresourceTiling.HeightInTiles;
regionSizesInResource[0].Depth = tileInfo.subresourceTiling.DepthInTiles;
regionSizesInResource[0].bUseBox = true;
regionSizesInResource[0].NumTiles = regionSizesInResource[0].Width * regionSizesInResource[0].Height * regionSizesInResource[0].Depth;
//設定するタイルリソース内の開始座標
std::array<D3D11_TILED_RESOURCE_COORDINATE, 1> coordinatesInResource;
coordinatesInResource[0].Subresource = 0;
coordinatesInResource[0].X = 0;
coordinatesInResource[0].Y = 0;
coordinatesInResource[0].Z = 0;

//各タイルリソースにタイププールを割り当てる
for (UINT i = 0; i < this->mTextures.size(); ++i) {
  auto& tex = this->mTextures[i];
  //割り当てるタイルプールの場所の指定
  std::array<UINT, 1> rangeFlags = { { 0 } };
  std::array<UINT, 1> offsets = { { i * tileInfo.tileCount } };
  std::array<UINT, 1> rangeTileCounts = { { tileInfo.tileCount } };

  UINT flags = 0;
  //タイルリソースにタイルプールを設定する
  hr = this->mpContext2->UpdateTileMappings(
    tex.mpResource.Get(), static_cast<UINT>(coordinatesInResource.size()), coordinatesInResource.data(), regionSizesInResource.data(),
    this->mpTilePool.Get(), static_cast<UINT>(rangeFlags.size()), rangeFlags.data(), offsets.data(), rangeTileCounts.data(), flags);
  if (FAILED(hr)) {
    throw std::runtime_error("タイルリソースがさすタイルプールの場所の設定に失敗");
  }
}

//他のタイルリソースが設定されているタイルプールの場所もマップすることもできる
std::array<UINT, 1> rangeFlags = { { 0 } };
std::array<UINT, 1> offsets = { { 0 } };
std::array<UINT, 1> rangeTileCounts = { { tileInfo.tileCount } };

UINT flags = 0;
hr = this->mpContext2->UpdateTileMappings(
  this->mTargetTex.mpResource.Get(), static_cast<UINT>(coordinatesInResource.size()), coordinatesInResource.data(), regionSizesInResource.data(),
  this->mpTilePool.Get(), static_cast<UINT>(rangeFlags.size()), rangeFlags.data(), offsets.data(), rangeTileCounts.data(), flags);
if (FAILED(hr)) {
  throw std::runtime_error("タイルリソースがさすタイルプールの場所の設定に失敗");
}
{% endhighlight %}

<div class="argument">
  <h3>ID3D11DeviceContext2::UpdateTileMappings</h3>
  <p>
    <span class="keyward">ID3D11DeviceContext2::UpdateTileMappings</span>の引数は大まかに分けてタイルリソース側とタイルプール側の情報に別れます。
    また引数の大半は各々の使用する範囲を指定するものになり、1回の呼び出しで複数の範囲を指定することが可能となっています。
    <span class="important">ドキュメントにはより詳細な解説がありますので参考にしてください。</span>
  </p>
  <h4>タイルリソース側</h4>
  <ul>
    <li><span class="keyward">第1引数:pTiledResource</span>
      <p>設定するタイルリソース</p>
    </li>
    <li><span class="keyward">第2引数:NumTiledResourceRegions</span>
      <p>
        <span class="important">タイルプールのタイルをタイルリソース内のどの範囲に紐付けるかを表すパラメータの個数。</span>
        範囲は第3,4引数で指定し、それぞれ開始座標とサイズを指定します。
        第3,4引数はここで指定した数の要素を持つ配列を指定する必要があります。
      </p>
    </li>
    <li><span class="keyward">第3引数:pTiledResourceRegionStartCoordinates</span>
      <p>
        タイルリソース内の紐付ける場所の開始座標の配列。
        <span class="keyward">D3D11_TILED_RESOURCE_COORDINATE</span>で指定します。
        タイルリソースがテクスチャの場合は紐付ける対象となるミップレベルまたは配列の添字も指定する必要があります。
      </p>
      <p>
        ドキュメント:D3D11_TILED_RESOURCE_COORDINATE:<a href="https://msdn.microsoft.com/en-us/library/windows/desktop/dn280437(v=vs.85).aspx">(英語)</a>
      </p>
    </li>
    <li><span class="keyward">第4引数:pTiledResourceRegionSizes</span>
      <p>
        タイルリソース内の紐付ける場所の範囲の配列。
        <span class="keyward">D3D11_TILE_REGION_SIZE</span>で指定します。
        範囲はバッファなどではタイルの数だけを指定します。
        テクスチャの場合は<span class="keyward">bUseBox</span>を<span class="keyward">true</span>に指定した上で横、縦、奥行きを指定します。
      </p>
      <p>
        ドキュメント:D3D11_TILE_REGION_SIZE:<a href="https://msdn.microsoft.com/en-us/library/windows/desktop/dn280442(v=vs.85).aspx">(英語)</a>
      </p>
    </li>
  </ul>
  <h4>タイルプール側</h4>
  <ul>
    <li><span class="keyward">第5引数:pTilePool</span>
      <p>紐付け元となるタイルプールを指定します。</p>
    </li>
    <li><span class="keyward">第6引数:NumRanges</span>
      <p>
        <span class="important">紐付けを行うタイルプール内の範囲を表すパラメータの個数。</span>
        第7,8,9引数には個々で指定した数分の要素を持つ配列を指定する必要があります。
      </p>
    </li>
    <li><span class="keyward">第7引数:pRangeFlags</span>
      <p>
        紐付けを行う範囲のフラグを指定します。
        <span class="keyward">D3D11_TILE_RANGE_FLAG</span>で指定します。
      </p>
      <p>
        ドキュメント:D3D11_TILE_RANGE_FLAG:<a href="https://msdn.microsoft.com/en-us/library/windows/desktop/dn280441(v=vs.85).aspx">(英語)</a>
      </p>
      <ol>
        <li><span class="keyward">何も指定しない(0を指定した時)時:</span>
          <p>
            <span class="keyward">pRangeTileCounts</span>で指定した数分のタイルを使用します。
          </p>
        </li>
        <li><span class="keyward">D3D11_TILE_RANGE_REUSE_SINGLE_TILEの時:</span>
          <p>
            1タイルのみ紐付けられます。
            <span class="important">その際、<span class="keyward">pRangeTileCounts</span>には紐付け先となるタイルリソース側のタイルの個数を指定します。</span>
          </p>
        </li>
        <li><span class="keyward">D3D11_TILE_RANGE_NULLの時:</span>
          <p>
            紐付け先をNULLにします。
          </p>
        </li>
        <li><span class="keyward">D3D11_TILE_RANGE_SKIPの時:</span>
          <p>
            <span class="keyward">pRangeTileCounts</span>で指定した数分、紐付け先のタイルリソースのタイルを飛ばして、紐付け先の状態をそのままに保ちます。
            これを指定した時は<span class="keyward">pTilePoolStartOffsets</span>の値は無視されます。
          </p>
        </li>
      </ol>
    </li>
    <li><span class="keyward">第8引数:pTilePoolStartOffsets</span>
      <p>
        紐付けを行う範囲の開始場所を指定します。
        <span class="keyward">UINT</span>を使いタイル単位で指定します。
      </p>
    </li>
    <li><span class="keyward">第9引数:pRangeTileCounts</span>
      <p>
        紐付けを行う範囲のサイズを指定します。
        <span class="keyward">UINT</span>を使いタイル単位で指定します。
      </p>
      <p>
        <span class="keyward">pRangeFlags</span>が<span class="keyward">D3D11_TILE_RANGE_SKIP</span>以外でかつ<span class="keyward">NumRanges</span>が1の場合はnullptrを指定できます。
        その際はタイルリソース側で指定した個数分が紐付けられます。
      </p>
    </li>
  </ul>
  <h4>その他</h4>
  <ul>
    <li><span class="keyward">第10引数:Flags</span>
      <p>
        紐付けを行う際のフラグを指定します。
        指定できるのものは<span class="keyward">D3D11_TILE_MAPPING_FLAG</span>で定義されています。
      </p>
      <p>
        ドキュメント:D3D11_TILE_MAPPING_FLAG:<a href="https://msdn.microsoft.com/en-us/library/windows/desktop/dn280440(v=vs.85).aspx">(英語)</a>
      </p>
    </li>
  </ul>
</div>

<h3>他のタイルリソースに設定されているものをコピーする方法</h3>
他のタイルリソースに設定されているものをコピーするには<span class="keyward">ID3D11DeviceContext2::CopyTileMappings</span>を使用します。

ドキュメント：<span class="keyward">ID3D11DeviceContext2::CopyTileMappings</span>
[(英語)][CopyTileMappings_EN]

[CopyTileMappings_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/dn280500(v=vs.85).aspx

{% highlight c++ %}
// Scene::onKeyUp関数の一部
auto& tileInfo = this->mTargetTex.mInfo;
//設定するタイルリソース内の範囲
std::array<D3D11_TILED_RESOURCE_COORDINATE, 1> coordinates;
coordinates[0].Subresource = 0;
coordinates[0].X = 0;
coordinates[0].Y = 0;
coordinates[0].Z = 0;
//設定するタイルリソース内の開始座標
std::array<D3D11_TILE_REGION_SIZE, 1 > regionSizes;
regionSizes[0].Width = tileInfo.subresourceTiling.WidthInTiles;
regionSizes[0].Height = tileInfo.subresourceTiling.HeightInTiles;
regionSizes[0].Depth = tileInfo.subresourceTiling.DepthInTiles;
regionSizes[0].bUseBox = true;
regionSizes[0].NumTiles = regionSizes[0].Width * regionSizes[0].Height * regionSizes[0].Depth;

UINT flags = 0;
//マップ情報をコピーする
auto hr = this->mpContext2->CopyTileMappings(
  this->mTargetTex.mpResource.Get(), coordinates.data(),
  this->mTextures[this->mTargetIndex].mpResource.Get(), coordinates.data(), regionSizes.data(), flags);
if (FAILED(hr)) {
  throw std::runtime_error("タイルリソースのマッピング情報のコピーに失敗");
}
{% endhighlight %}

<div class="argument">
  <h3>ID3D11DeviceContext2::CopyTileMappings</h3>
  <ul>
    <li><span class="keyward">第1引数：pDestTiledResource</span>
      <p>
        コピー先となるタイルリソースを指定します。
      </p>
    </li>
    <li><span class="keyward">第2引数：pDestRegionStartCoordinate</span>
      <p>
        コピー先の開始座標を指定します。
      </p>
    </li>
    <li><span class="keyward">第3引数：pSourceTiledResource</span>
      <p>
        コピー元となるタイルリソースを指定します。
      </p>
    </li>
    <li><span class="keyward">第4引数：pSourceRegionStartCoordinate</span>
      <p>
        コピー元のタイルリソースからコピーする範囲の開始座標を指定します。
      </p>
    </li>
    <li><span class="keyward">第5引数：pTileRegionSize</span>
      <p>
        コピー元のタイルリソースからコピーする範囲の範囲を指定します。
      </p>
    </li>
    <li><span class="keyward">第6引数：Flags</span>
      <p>
        紐付けを行う際のフラグを指定します。
        指定できるのものは<span class="keyward">D3D11_TILE_MAPPING_FLAG</span>で定義されています。
      </p>
      <p>
        ドキュメント:D3D11_TILE_MAPPING_FLAG:<a href="https://msdn.microsoft.com/en-us/library/windows/desktop/dn280440(v=vs.85).aspx">(英語)</a>
      </p>
    </li>
  </ul>
</div>

このサンプルではタイルリソースの範囲や開始位置を指定する意味がほとんどありません。
なので、タイルリソースは面倒な設定が必要なものだと感じられるかもしれません。
が、サイズが縦横一万を超えるといった巨大なテクスチャを扱う際には必要不可欠となります。
そのような巨大なテクスチャだと全データがGPUのメモリに収まらないので、現在使用している部分のみをGPUのメモリに読み込ませることになります。
<span class="important">タイルリソースはまさにそういったケースに対応するために追加された機能で、範囲や開始位置を指定しているのは当然のことだと言えます。</span>

<a name="UPDATE"></a>
<h1 class="under-bar">内容の更新</h1>

タイルリソースにタイププールを紐付けが出来ましたら、今までのようにビューを通してシェーダでその内容へアクセスできます。
また<span class="keyward">ID3D11DeviceContext2::UpdateTiles</span>を使用することで、<span class="keyward">ID3D11DeviceContext::UpdateSubresource</span>のようにCPUからデータを転送することも出来ます。

ドキュメント:<span class="keyward">ID3D11DeviceContext2::UpdateTiles</span>
[(英語)][UpdateTiles_EN]

[UpdateTiles_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/dn280509(v=vs.85).aspx

<span class="keyward">ID3D11DeviceContext2::UpdateTiles</span>の引数は上で使った関数と似ています。
<span class="important">注意点として、転送するデータはタイル単位に生成することを忘れないで下さい。</span>

<span class="important">また、タイルプールの内容を異なるタイルリソース間で共有しているときは<span class="keyward">ID3D11DeviceContext2::TiledResourceBarrier</span>を使用しアクセスの順序を指定する必要があります。</span>

ドキュメント:<span class="keyward">ID3D11DeviceContext2::TiledResourceBarrier</span>
[(英語)][TiledResourceBarrier_EN]

[TiledResourceBarrier_EN]:https://msdn.microsoft.com/en-us/library/windows/desktop/dn280507(v=vs.85).aspx

{% highlight c++ %}
//Scene::onInitの一部
auto& last = * this->mTextures.rbegin();
auto& tileInfo = last.mInfo;
std::vector<uint32_t> src;
src.resize(tileInfo.tileCount * (64 * 1024 / sizeof(uint32_t)));//必ず64KBの倍数のデータ長になるようにする

// ... 転送するデータの生成

std::array<D3D11_TILED_RESOURCE_COORDINATE, 1> coordinates;
coordinates[0].Subresource = 0;
coordinates[0].X = 0;
coordinates[0].Y = 0;
coordinates[0].Z = 0;
std::array<D3D11_TILE_REGION_SIZE, 1 > regionSizes;
regionSizes[0].Width = tileInfo.subresourceTiling.WidthInTiles;
regionSizes[0].Height = tileInfo.subresourceTiling.HeightInTiles;
regionSizes[0].Depth = tileInfo.subresourceTiling.DepthInTiles;
regionSizes[0].bUseBox = true;
regionSizes[0].NumTiles = regionSizes[0].Width * regionSizes[0].Height * regionSizes[0].Depth;
this->mpContext2->UpdateTiles(last.mpResource.Get(), coordinates.data(), regionSizes.data(), src.data(), 0);
//CPUからデータを送り終えたことをGPUに伝える
this->mpContext2->TiledResourceBarrier(last.mpResource.Get(), last.mpResource.Get());
{% endhighlight %}

<a name="SUMMARY"></a>
<h1 class="under-bar">まとめ</h1>

このパートではタイルリソースについて見ていきました。
GPUメモリに乗りきらないほど膨大なデータを扱う際に効果を発揮するものになるでしょう。

<span class="important">サンプルコードで正しく使用できているか自信が無いのでMicrosoftが公開している[デモコード][DEMO_CODE]を見ることを強く推奨します。</span>
<span class="important">また、サンプルコード作成には[こちらのサイト][shikihuiku]も参考にしました。そちらも合わせてご覧ください。</span>

[DEMO_CODE]:https://code.msdn.microsoft.com/windowsapps/Direct3D-Tiled-Resources-80ee7a6e?tduid=(4c45dba9f25849592b3bc1186e4ff695)(256380)(2459594)(TnL5HPStwNw-J71tmQEktuebN0_wAX9FIQ)()
[shikihuiku]:https://shikihuiku.wordpress.com/2013/07/22/dx11-2%E3%81%AE%E7%9B%AE%E7%8E%89%E6%A9%9F%E8%83%BD%E3%80%82tiled-resouce/

<a name="SUPPLEMENTAL"></a>
<h1 class="under-bar">補足</h1>

<div class="supplemental">
  <h3>タイルリソースに対応しているかの確認の仕方</h3>
  <p>
    タイルリソースを使用できるかを確認するには<span class="keyward">ID3D11Device::CheckFeatureSupport</span>を使用します。
    この関数で取得できるものにはいくつかありますが、タイルリソースの場合は<span class="keyward">D3D11_FEATURE_DATA_D3D11_OPTIONS1</span>を使用します。
    <br>ドキュメント:<span class="keyward">ID3D11Device::CheckFeatureSupport</span>
    <a href="https://msdn.microsoft.com/ja-jp/library/ee419773(v=vs.85).aspx">(日本語)</a>
    <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ff476497(v=vs.85).aspx">(英語)</a>
    {% highlight c++ %}
// Scene::onInitの一部
//タイルリソースに対応しているかチェック
D3D11_FEATURE_DATA_D3D11_OPTIONS1 featureData;
this->mpDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS1, &featureData, sizeof(featureData));
if (D3D11_TILED_RESOURCES_NOT_SUPPORTED == featureData.TiledResourcesTier) {
  throw std::runtime_error("使用しているデバイスはタイルリソースに対応していません。");
}
    {% endhighlight %}
  </p>
</div>

<table class="table table-condensed">
  <tbody>
    <tr>
      <td class="left"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/texture2">＜前</a></td>
      <td class="center"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}">トップ</a></td>
      <td class="right">おわり</td>
      <!--
      <td class="right"><a href="{% if site.github.url %}{{ site.github.url }}{% else %}{{ "/" | prepend: site.url }}{% endif %}part/tile-resource">次＞</a></td>
      -->
    </tr>
  </tbody>
</table>
