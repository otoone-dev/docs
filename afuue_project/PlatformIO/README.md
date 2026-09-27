# AFUUE2R ソフトウェアバイナリ、ソースコードについて

## ファイル構成
このフォルダには以下のファイルがあります。
- [AFUUE2R_gen2.bin ダウンロード](AFUUE2R_gen2.bin)
- [AFUUE2R_gen2.zip ダウンロード](AFUUE2R_gen2.zip)

bin ファイルはソフトウェア書き込み用で、zip ファイルはソフトウェアを開発したい方向けです。

## zip ファイルについて
zip ファイルを展開すると、中に README.md がありますので、ソースコードの取り扱いについてはそちらを参照ください。

## ソフトウェア(bin ファイル)の書き込みについて
1. 上記リンクから AFUUE2_gen2.bin をダウンロードしてください。
2. AFUUE2R gen2 の AtomS3 lite (裏面の白い四角いマイコン) を Windows PC に USB で接続し、横にある小さなボタンを5秒ほど長押しします。ボタンの根本に緑のランプが点灯することで書き込みモードに入ったことが確認できます。AtomS3 lite は AFUUE2R gen2 につないだまま作業できます。

![AFUUE2R_program_mode](afuue2r_program_mode.png)

3. Windows PC の Chrome または Edge で下記ページを開きます。<br>
[WebSerial_ESPTool](https://jason2866.github.io/WebSerial_ESPTool/)

4. 右上の Connect を押します(ボタンが出てない場合はページの一番上にマウスカーソルを移動すると現れます)

![WebSerialESPTool screenshot_1](WebSerialESPTool_1.png)

5. USB_JTag / serial debug unit (COM5) などの文字の書かれたものを選択して接続を押します。

![WebSerialESPTool screenshot_2](WebSerialESPTool_2.png)

6. Choose a file ボタンを押し、ダウンロードした AFUUE2R_gen2.bin を選択します。<br>

![WebSerialESPTool screenshot_3](WebSerialESPTool_3.png)

7. Program ボタンを押します。
書き込みの緑のバーが最後まで進行したら終了です。Connect ボタンと同じ場所の Disconnect ボタンを押して接続を切って、USB ケーブルを抜いてください。
もし AFUUE2R に電源を入れて起動しない場合、再度書き込みをし、Disconnect してケーブルを抜いたら30秒ほど待ってから電源を入れてみてください。

![WebSerialESPTool screenshot_4](WebSerialESPTool_4.png)
