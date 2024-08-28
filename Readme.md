# SlitScan_M
スリットスキャンスクリプト  
(息抜きとして作ったため、エラーハンドリングをちゃんとしていません。すみません。)

## 制約
「スリットスキャンの動画を出力するためだけのプロジェクト」用スクリプトとして作成したため、制約が多いです。

1. 時差は指定できません。1フレームごとになります。
2. カスタムオブジェクトではなく、アニメーション効果です。
3. オブジェクトの解像度の途中変更に対応していません。  
動画オブジェクトの下に「エフェクトを何もかけない・フレームバッファをクリアがオンのフレームバッファオブジェクト」を用意し、そこにかけることを想定しています。
4. 複数オブジェクトに同時にかけることを想定していません。
5. 一時保存系であるため、プレビューと出力が異なる場合があります。
6. 適用したオブジェクトの1フレーム目に一度シークしない限りは設定の変更などが適用されません。
7. オブジェクトの1フレーム目にシークした際、メモリを大量に確保します。
- 共有メモリを使用しているため、AviUtlの4GB制限には引っ掛かりにくいはずです。
- 具体的には (画像幅 \* 画像高さ \* 4 \* 分割数) byte 確保します。  
  画像サイズ 1920x1080 の 分割数100 にした場合のメモリの確保は以下の通りです。  
  1920\*1080\*4\*100 = 829440000 byte ≒ 791 MiB