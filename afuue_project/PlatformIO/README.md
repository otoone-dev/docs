# AFUUE2R ソフトウェアバイナリ、ソースコードについて

## ファイル構成
このフォルダには以下のファイルがあります。
- AFUUE2R_gen2.bin
- AFUUE2R_gen2.zip

bin ファイルはソフトウェア書き込み用になり、zip ファイルはソフトウェアを開発したい方向けです。

## zip ファイルについて
zip ファイルを展開すると、中に README.md がありますので、ソースコードの取り扱いについてはそちらを参照ください。

## bin ファイルの書き込み方について
1. AFUUE2R gen2 の StampS3 lite (裏面の白い四角いマイコン) を Windows PC に USB で接続し、横にある小さなボタンを長押します。ボタンの根本に緑のランプが点灯することで書き込みモードに入ったことが確認できます。AFUUE2R gen2 本体につないだまま作業できます。

2. Windows PC の Chrome (Edgeでもいけるかもしれません) で下記ページを開きます。<br>
[WebSerial ESPTool](https://jason2866.github.io/WebSerial_ESPTool/)

3. 右上の Connect を押します(ボタンが出てない場合はページの一番上にマウスカーソルを移動すると現れます)

4. USB_JTag / serial debug unit (COM5) などの文字の書かれたものを選択します。

5. Choose a file ボタンを押し、このフォルダの AFUUE2R_gen2.bin を選択します。<br>
（次回からはファイルは選択済になっています）

6. Program ボタンを押します。
書き込みが 100% まで進行したら終了です。Connect ボタンと同じ場所の Disconnect ボタンを押して接続を切って、USB ケーブルを抜いてください。

