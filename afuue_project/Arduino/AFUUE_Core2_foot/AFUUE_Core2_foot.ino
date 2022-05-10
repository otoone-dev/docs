#pragma mark - Depend ESP8266Audio and ESP8266_Spiram libraries
/* 
cd ~/Arduino/libraries
git clone https://github.com/earlephilhower/ESP8266Audio
git clone https://github.com/Gianbacchio/ESP8266_Spiram
*/

#include <vector>
#include <string>
#include <M5Core2.h>
//#include <WiFi.h>

#define BT_A2DP (0)
#define BT_DEVICENAME "BT earphone"
#if BT_A2DP
//------ A2DP ------
#include <BluetoothA2DPCommon.h>
//#include <BluetoothA2DPSink.h>
#include <BluetoothA2DPSource.h>
#include <SoundData.h>
BluetoothA2DPSource a2dp_source;
#else
//------ I2S ------
#include <driver/i2s.h>
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "driver/i2s.h"
AudioGeneratorMP3 *mp3;
AudioOutputI2S *out;
AudioFileSourceSD *file;
AudioFileSourceID3 *id3;
AudioFileSourceSD *fileB;
AudioFileSourceID3 *id3B;
#define CONFIG_I2S_BCK_PIN 12
#define CONFIG_I2S_LRCK_PIN 0
#define CONFIG_I2S_DATA_PIN 2
#define CONFIG_I2S_DATA_IN_PIN 34
#define Speak_I2S_NUMBER I2S_NUM_0
#define MODE_MIC 0
#define MODE_SPK 1
#endif

const double VOICE2_MAX = 1000;

const short wavEGuitar[256] = {
    -838, -1859, -2881, -3874, -4840, -5778, -6679, -7537, 
    -8354, -9109, -9814, -10456, -11029, -11543, -12013, -12435, 
    -12793, -13134, -13445, -13734, -13993, -14220, -14424, -14581, 
    -14719, -14807, -14825, -14810, -14752, -14639, -14485, -14286, 
    -14062, -13797, -13512, -13230, -12935, -12617, -12317, -12010, 
    -11706, -11380, -11065, -10747, -10422, -10107, -9810, -9498, 
    -9214, -8918, -8658, -8409, -8161, -7946, -7761, -7568, 
    -7402, -7247, -7087, -6908, -6716, -6540, -6351, -6171, 
    -5989, -5815, -5632, -5453, -5291, -5129, -4977, -4814, 
    -4630, -4465, -4287, -4093, -3919, -3779, -3632, -3511, 
    -3438, -3378, -3353, -3361, -3370, -3416, -3426, -3439, 
    -3440, -3429, -3410, -3379, -3333, -3305, -3311, -3314, 
    -3337, -3380, -3447, -3532, -3615, -3703, -3785, -3838, 
    -3882, -3924, -3949, -3958, -4001, -4078, -4195, -4362, 
    -4591, -4870, -5191, -5556, -5942, -6351, -6732, -7084, 
    -7407, -7678, -7893, -8078, -8210, -8278, -8341, -8394, 
    -8467, -8552, -8648, -8795, -8986, -9219, -9499, -9834, 
    -10205, -10602, -11026, -11465, -11906, -12336, -12727, -13046, 
    -13301, -13497, -13611, -13638, -13590, -13468, -13283, -13068, 
    -12824, -12551, -12269, -11973, -11673, -11361, -11045, -10723, 
    -10396, -10056, -9693, -9353, -9035, -8709, -8391, -8093, 
    -7808, -7520, -7217, -6910, -6566, -6149, -5698, -5187, 
    -4585, -3890, -3122, -2240, -1278, -230, 892, 2079, 
    3317, 4589, 5887, 7201, 8509, 9806, 11069, 12287, 
    13464, 14601, 15670, 16650, 17558, 18386, 19138, 19826, 
    20459, 21025, 21534, 22025, 22489, 22952, 23419, 23895, 
    24391, 24942, 25516, 26123, 26727, 27353, 27988, 28604, 
    29192, 29747, 30234, 30649, 30982, 31219, 31364, 31418, 
    31367, 31193, 30908, 30478, 29955, 29339, 28624, 27818, 
    26942, 25996, 24975, 23925, 22853, 21758, 20645, 19528, 
    18416, 17309, 16195, 15066, 13944, 12805, 11640, 10464, 
    9257, 8013, 6780, 5538, 4292, 3046, 1807, 597, 
};

const short waveWSynth[256] = {
    -11947,-14483,-17018,-19554,-22090,-24625,-26859,-27731,
    -28604,-29476,-30348,-31221,-31703,-31501,-31300,-31099,
    -30897,-30696,-30352,-29891,-29429,-28967,-28506,-28044,
    -27548,-27038,-26529,-26019,-25510,-25001,-24497,-23994,
    -23492,-22989,-22486,-21991,-21569,-21148,-20727,-20306,
    -19884,-19462,-19036,-18610,-18184,-17758,-17332,-16876,
    -16385,-15893,-15402,-14910,-14419,-13958,-13515,-13072,
    -12629,-12185,-11742,-11424,-11133,-10842,-10551,-10261,
    -9970,-9767,-9564,-9362,-9159,-8956,-8751,-8533,
    -8315,-8097,-7879,-7661,-7435,-7196,-6956,-6716,
    -6476,-6236,-5994,-5751,-5507,-5263,-5019,-4776,
    -4554,-4342,-4129,-3917,-3704,-3491,-3320,-3153,
    -2986,-2819,-2653,-2484,-2304,-2124,-1944,-1764,
    -1584,-1382,-1125,-868,-611,-354,-97,163,
    430,697,964,1231,1498,1715,1904,2093,
    2282,2470,2659,2829,2996,3162,3328,3495,
    3661,3848,4036,4223,4411,4598,4782,4947,
    5113,5278,5444,5610,5767,5909,6052,6194,
    6336,6479,6621,6763,6904,7046,7188,7330,
    7458,7582,7706,7830,7954,8078,8199,8319,
    8440,8560,8681,8806,8971,9137,9302,9468,
    9634,9809,10013,10216,10420,10623,10826,11038,
    11261,11484,11706,11929,12151,12343,12516,12690,
    12863,13037,13210,13281,13329,13376,13424,13472,
    13519,13562,13606,13649,13692,13735,13803,13986,
    14169,14352,14534,14717,14880,15006,15133,15259,
    15386,15512,15544,15497,15450,15403,15356,15309,
    15343,15408,15473,15538,15603,15668,15897,16143,
    16389,16634,16880,17115,17242,17368,17495,17621,
    17748,17872,17991,18109,18228,18346,18465,18599,
    18751,18904,19056,19208,19361,18903,18097,17291,
    16485,15678,14872,12654,10121,7589,5057,2524,
};


static int buttons = 0;
static int fileNo = 0;
static int mode = 0;
static int counter = 0;

#define CORE0 (0)
#define CORE1 (1)
#define DATA_SIZE 256

#define SAMPLING_RATE (44100) //44100
#define SAMPLING_RATE_DOUBLE (44100.0)
volatile double mainVolume = 1.0;
volatile bool isPressed = false;
static bool isVoiceStarted = false;

#define REVERB_BUFFER_SIZE (9801)
volatile double reverbBuffer[REVERB_BUFFER_SIZE];
volatile int reverbPos = 0;
static double reverbRate = 0.152;

volatile int16_t soundBuffer[DATA_SIZE];    // DMA転送バッファ
const int VOICE_MAX = 4;
volatile double fst[VOICE_MAX] = {0.0, 0.0, 0.0, 0.0};
volatile double phase[VOICE_MAX] = {0.0, 0.0, 0.0, 0.0};
volatile double volReq[VOICE_MAX] = {0.0, 0.0, 0.0, 0.0};
volatile double tm[VOICE_MAX] = {0.0, 0.0, 0.0, 0.0};
volatile double toff[VOICE_MAX] = {-1.0, -1.0, -1.0, -1.0};
const double adsr_attack = 0.01;
const double adsr_decay = 3;
const double adsr_sustain = 0.0;
const double adsr_release = 0.4;
volatile double currentPressure = 0.0;
volatile float accX = 0;
volatile float accY = 0;
volatile float accZ = 0;
volatile double dist = 0;

class Music {
public:
  std::string title;
  std::vector<std::string> codes;
  Music(std::string _title, std::vector<std::string> _codes) {
    title = _title;
    codes = _codes;
  }
  Music* prev = NULL;
  Music* next = NULL;
};

static Music* currentMusic = NULL;
static int codes_point = -1;
static int code_notes[] = {0, 0, 0, 0};
static std::string current_code = "";
static int codes_max = 1;
static int base_note = 60;

  std::vector<std::string> c1
  {
    "DM7", "C#m7", "Bm7", "AM7",
    "DM7", "C#m7", "Bm7", "AM7",
    "DM7", "C#m7", "Bm7", "AM7",
    "DM7", "C#m7", "Bm7", "AM7",
    "DM7", "C#m7", "Bm7", "AM7",
    "DM7", "C#m7", "Bm7", "AM7",
    "Bm7", "C#m7", "Bm7", "AM7",
    "Bm7", "C#m7", "Bm7", "C#m7", "DM7", "E7",
    "DM7", "C#m7", "Bm7", "AM7",
    "DM7", "C#m7", "Bm7", "AM7",
    "DM7", "C#m7", "Bm7", "AM7",
    "DM7", "C#m7", "Bm7", "AM7",
    "DM7", "C#m7", "Bm7", "AM7",
    "DM7", "C#m7", "Bm7", "AM7",
    "Bm7", "C#m7", "Bm7", "AM7",
    "Bm7", "C#m7", "Bm7", "C#m7", "DM7", "E7",
    "DM7", "C#m7", "Bm7", "AM7",
    "DM7", "C#m7", "Bm7", "AM7",
    "DM7", "C#m7", "Bm7", "AM7",
    "DM7", "C#m7", "Bm7", "AM7",
    "DM7", "C#m7", "Bm7", "AM7",
    "DM7", "C#m7", "Bm7", "AM7",
    "Bm7", "C#m7", "Bm7", "AM7",
    ""
  };
  std::vector<std::string> c2
  {
    "D", "F#m", "D7", "G",
    "Em", "A7", "D", "E9",
    "G", "A7", "D",
    "D", "F#m", "D7", "G",
    "Em", "A7", "D", "E9",
    "G", "A7", "D", "B7",
    "Em9", "A7", "F#m7", "Bm",
    "Em9", "A7", "F#m7", "Bm",
    "G", "A", "F#m7", "B7",
    "GM7", "A7", "D", "E9",
    "GM7", "A7", "D", "E9",
    "G", "A7", "D",
    "E", "G", "A7", "D",
    ""
  };
  std::vector<std::string> c3
  {
    "GM7", "F#m7", "Em7", "DM7",
    "GM7", "F#m", "Em7", "DM7",
    "Bm", "F#m", "Em7", "DM7",
    "Bm", "F#m7", "Em7", "F#7",
    "GM7", "F#m7", "Em7", "DM7",
    "GM7", "F#m", "Em7", "DM7",
    "Bm", "F#m", "Em7", "DM7",
    "Bm", "F#m7", "Em7", "F#7",
    "GM7", "F#m7", "Em7", "DM7",
    "GM7", "F#m", "Em7", "DM7",
    "Bm", "F#m", "Em7", "DM7",
    "Bm", "F#m7", "Em7", "F#7",
    "GM7", "GM7", "GM7", "GM7",
    ""
  };
  std::vector<std::string> c4
  {
    "C", "F", "Fm", "C",
    "Dm", "Em", "F", "G",
    "Am", "Em", "Am", "Em",
    "F", "G7", "F", "G7",
    "Am", "Dm", "Gsus4", "G",
    "C", "Em", "C7", "F",
    "Fm", "C", "Gsus4", "G",
    "C", "Em", "C7", "F",
    "Fm", "C", "G", "C",
    "C", "Em", "C7", "F",
    "Fm", "C", "Gsus4", "G",
    "C", "Em", "C7", "F",
    "Fm", "C", "G", "C",
    ""
  };
  std::vector<std::string> c5
  {
    "Bm", "G", "A", "D", "A",
    "Bm", "G", "A", "D", "A",
    "Bm", "G", "A", "D", "A",
    "Bm", "G", "A", "Bm", "A",
    "G", "D",
    "C#m7-5", "F#7", "Bm",
    "Gm", "Am", "A#", "Bm7-5", "C", "A",
    "C#m", "A", "B", "E", "D#dim",
    "C#m", "A", "B", "G#7",
    "C#m", "A", "B", "G#7",
    "A", "E", "D#m7-5", "G#7",
    "C#m", "B", "A", "C#m",
    "Bm", "A", "G", "D",
    
    "Bm", "G", "A", "D", "A",
    "Bm", "G", "A", "D", "A",
    "Bm", "G", "A", "D", "A",
    "Bm", "G", "A", "Bm", "A",
    "G", "D",
    "C#m7-5", "F#7", "Bm",
    "Gm", "Am", "A#", "Bm7-5", "C",
    "Em", "C", "D", "G",
    "Em", "C", "D", "G",
    "Em", "C", "D", "Em",
    "D#", "Dm", "Gsus4", "G",
    "C#m", "A", "B", "E", "D#dim",
    "C#m", "A", "B", "G#7",
    "C#m", "A", "B", "G#7",
    "A", "E", "D#m7-5", "G#7",
    "A", "E", "D#m7-5", "G#7",
    "A", "E", "G#7", "C#m",
    "A", "B", "C#m",
    "B", "A", "E",
    "Bm", "A", "G", "D",
    ""
  };
  std::vector<std::string> c6
  {
    "G", "Bm7", "Dm7", "E7", "Am",
    "Cm7", "F7", "BbM7", "A7", "D7",
    "G", "Bm7", "Dm7", "E7", "Am",
    "Cm7", "F7", "BbM7", "A7", "D7",
    "CM7", "D7", "Bm7", "Em", "Am7", "D7", "GM7", "G7",
    "CM7", "D7", "B7", "Em", "C#dim", "D7sus4", "D7",
    "Em", "D#aug", "G", "C#dim", "Am7", "D7", "G", "C", "GM7", "C",
    "G", "Bm7", "Dm7", "E7", "Am",
    "Cm7", "F7", "BbM7", "A7", "D7",
    "G", "Bm7", "Dm7", "E7", "Am",
    "Cm7", "F7", "BbM7", "A7", "D7",
    "CM7", "D7", "Bm7", "Em", "Am7", "D7", "GM7", "G7",
    "CM7", "D7", "B7", "Em", "C#dim", "D7sus4", "D7",
    "Em", "D#aug", "G", "C#dim", "Am7", "D7", "G", "G7",
    "CM7", "D7", "Bm7", "Em", "Am7", "D7", "GM7", "G7",
    "CM7", "D7", "B7", "Em", "C#dim", "D7sus4", "D7",
    "Em", "D#aug", "G", "C#dim", "Am7", "D7", "G", "C", "GM7", "C",
    ""
  };

//---------------------------------
void SetupMusics() {
  Music* music = new Music("Lovin' You", c1 );
  Music* music2 = new Music("Ellie, My Love", c2 );
  Music* music3 = new Music("MFTokyo2020", c3 );
  Music* music4 = new Music("Kyoukaisen", c4 );
  Music* music5 = new Music("Homura", c5 );
  Music* music6 = new Music("Listening to Olivia", c6 );
  currentMusic = music;

  const int MUSIC_MAX = 6;
  Music* musics[MUSIC_MAX] = { music3, music, music2, music4, music5, music6 };
  for (int i = 0; i < MUSIC_MAX; i++) {
    if (i == 0) {
      musics[0]->prev = musics[MUSIC_MAX-1];
    }
    else {
      musics[i]->prev = musics[i-1];
    }
    if (i == MUSIC_MAX-1) {
      musics[i]->next = musics[0];
    }
    else {
      musics[i]->next = musics[i+1];
    }
  }
}

//---------------------------------
#define TX1 (80)
#define TX2 (160)
#define TY1 (80)
#define TY2 (240)
#define TW (240)
#define TH (320)
void SetTextColor() {
    int len = current_code.length(); // CM7 = 3
    std::string last = current_code.substr(len-1);
    std::string last2 = "";
    std::string last3 = "";
    std::string last4 = "";
    if (len >= 3) last2 = current_code.substr(len-2);
    if (len >= 4) last3 = current_code.substr(len-3);
    if (len >= 5) last4 = current_code.substr(len-4);
    if (last == "7") {
      if (last2 == "M7") {
        M5.Lcd.setTextColor(M5.Lcd.color565(0x32, 0xCD, 0x32)); // limegreen
      }
      else if (last2 == "m7") {
        M5.Lcd.setTextColor(M5.Lcd.color565(0x00, 0xCE, 0xD1)); // darkturquoise
      }
      else {
        M5.Lcd.setTextColor(M5.Lcd.color565(0xFF, 0x00, 0xFF)); // magenta
      }
      return;
    }
    if (last == "9") {
        M5.Lcd.setTextColor(M5.Lcd.color565(0xAD, 0xFF, 0x2F)); // greenyellow
        return;
    }
    if (last4 == "sus4") {
      M5.Lcd.setTextColor(M5.Lcd.color565(0xFF, 0xFF, 0x00)); // yellow
    }
    else if (last3 == "aug") {
      M5.Lcd.setTextColor(M5.Lcd.color565(0x8A, 0x2B, 0xE2)); // blueviolet
    }
    else if (last3 == "dim") {
      M5.Lcd.setTextColor(M5.Lcd.color565(0xFF, 0x69, 0xB4)); // hotpink
    }
    else if (last3 == "7-5") {
      M5.Lcd.setTextColor(M5.Lcd.color565(0x99, 0x32, 0xCC)); // darkorchid
    }
    else if (last == "m") {
      M5.Lcd.setTextColor(M5.Lcd.color565(0x7B, 0x68, 0xEE)); // midiumslateblue
    }
    else {
      M5.Lcd.setTextColor(M5.Lcd.color565(0xFF, 0xA5, 0x00)); // orange
    }
}

void PrintInfo() {
#if 1
  M5.Lcd.fillRect(0, TY1+1, 240, (TY2-TY1)-1, BLACK);

  if (current_code != "") {
    M5.Lcd.setTextSize(2);
    SetTextColor();
    M5.Lcd.drawCentreString(current_code.c_str(), 120, 100, 4);
  }
  M5.Lcd.setTextColor(WHITE);
  if ((currentMusic != 0) && (currentMusic->title.c_str() != 0)) {  
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawCentreString(currentMusic->title.c_str(), 120, 195, 4);
  }
  int x = ((TW - 10)*codes_point) / codes_max;
  M5.Lcd.fillRect(x, TH/2-3, 10, 6, WHITE);
#endif
}

//---------------------------------
// D#mM7, C9, Ebsus4, Adim, Baug etc.
void SetNextCode(int step) {
  if (step == 0) {
    codes_point = 0;
  }
  else {
    codes_point += step;
    if (codes_point < 0) {
      codes_point = codes_max-1;
    }
    if (currentMusic->codes[codes_point].length() == 0) {
      codes_point = 0;
    }
  }
  current_code = currentMusic->codes[codes_point];
  const char* code = current_code.c_str();

  PrintInfo();
#if 1
  int len = strlen(code);
  int n[4] = {-1, -1, -1, -1};
  switch (code[0]) {
    default:
    case 'C': n[0] = 0; break;
    case 'D': n[0] = 2; break;
    case 'E': n[0] = 4; break;
    case 'F': n[0] = 5; break;
    case 'G': n[0] = 7; break;
    case 'A': n[0] = 9; break;
    case 'B': n[0] = 11; break;
  }
  int i = 1;
  if (len > i) {
    if (code[i] == '#') {
      n[0]++;
      i++;
    }
    if (code[i] == 'b') {
      n[0]--;
      i++;
    }
  }
  n[1] = (n[0] + 4) % 12;
  n[2] = (n[0] + 7) % 12;
  if (len > i) {
    if (code[i] == 'm') {
      n[1] = n[1] - 1; // minor
      i++;
    }
  }
  bool major = false;
  if (len > i) {
    if (code[i] == 'M') {
      major = true;
      i++;      
    }
    if (code[i] == '5') {
      n[1] = -1; // omit3
      i++;      
    }
  }
  if (len > i) {
    if (code[i] == '6') {
      n[3] = (n[0] + 9) % 12;  //6th
      i++;      
    }
    if (code[i] == '7') {
      if (major == false) {
        n[3] = (n[0] + 10) % 12;  // 7th
      }
      else {
        n[3] = (n[0] + 11) % 12;  // major 7th
      }
      i++;      
    }
  }
  if (len > i) {
    if (code[i] == '9') {
      n[3] = (n[0] + 14) % 12;  //9th
      i++;      
    }
  }
  if (len > i+1) {
    if (strcmp(code + i, "-5")==0) {
      n[2] = n[2] - 1; // flat5
      i += 2;
    }
    if (strcmp(code + i, "+5")==0) {
      n[2] = n[2] + 1; // aug5
      i += 2;
    }
  }
  if (len > i+2) {
    if (strcmp(code + i, "dim")==0) {
      n[1] = (n[0] + 3) % 12;
      n[2] = (n[0] + 6) % 12;
      i += 3;
    }
    if (strcmp(code + i, "aug")==0) {
      n[2] = n[2] + 1; // aug5
      i += 3;
    }
  }
  if (len > i+3) {
    if (strcmp(code + i, "sus2")==0) {
      n[1] = n[0] + 2; // sus2
      i += 4;
    }
    if (strcmp(code + i, "sus4")==0) {
      n[1] = n[1] + 1; // sus4
      i += 4;
    }
    if (strcmp(code + i, "add9")==0) {
      n[3] = (n[0] + 14) % 12; // add9
      i += 4;
    }
  }
  for (int j = 0; j < 4; j++) {
    if (n[j] >= 0) {
      code_notes[j] = base_note + n[j];
    }
    else {
      code_notes[j] = -1;
    }
  }
  for (int j = 0; j < 3; j++) {
    for (int k = j + 1; k < 4; k++) {
      if (code_notes[j] > code_notes[k]) {
        int c = code_notes[j];
        code_notes[j] = code_notes[k];
        code_notes[k] = c;
      }
    }
  }
#endif
}

//---------------------------------
void SetSong(int step) {
  if (step > 0) {
    currentMusic = currentMusic->next;
  }
  else if (step < 0) {
    currentMusic = currentMusic->prev;
  }
  
  codes_max = 0;
  while (1) {
    if (currentMusic->codes[codes_max].length() == 0) {
      break;
    }
    codes_max++;
    if (codes_max > 10000) break;
  }

  SetNextCode(0);

  PrintInfo();
}

#if !BT_A2DP
//---------------------------------
bool InitI2SSpeakOrMic(int mode)
{
    esp_err_t err = ESP_OK;

    i2s_driver_uninstall(Speak_I2S_NUMBER);
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER),
        .sample_rate = SAMPLING_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = 128,
    };
    if (mode == MODE_MIC)
    {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    }
    else
    {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        i2s_config.use_apll = false;
        i2s_config.tx_desc_auto_clear = true;
    }
    err += i2s_driver_install(Speak_I2S_NUMBER, &i2s_config, 0, NULL);
    i2s_pin_config_t tx_pin_config;

    tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;
    tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
    tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
    tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
    err += i2s_set_pin(Speak_I2S_NUMBER, &tx_pin_config);
    err += i2s_set_clk(Speak_I2S_NUMBER, SAMPLING_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
    i2s_zero_dma_buffer(Speak_I2S_NUMBER);

    delay(1000);
    return true;
}
#endif

//---------------------------------
double CalcAdsr(double t, double toff) {
  if (t < 0) {
    return 0.0;
  }
  if (t < adsr_attack) {
    return (t / adsr_attack);
  }
  else if ((toff >= 0) && (toff < t)) {
    double t0 = toff - adsr_attack;
    double v = 1 - (t0 / adsr_decay);
    if (v < adsr_sustain)
    {
        v = adsr_sustain;
    }
    if (v > 0) {
      double t1 = (1 - v) * adsr_release;
      double r = 1 - ((t - (toff - t1)) / adsr_release);
      if (r < 0)
      {
          r = 0;
      }
      return r * r;
    }
    return 0;
  }
  else {
    double t0 = t - adsr_attack;
    double v = 1 - (t0 / adsr_decay);
    if (v < adsr_sustain)
    {
        v = adsr_sustain;
    }
    return v * v;
  }
}

//---------------------------------
double Voice(double p) {
  double v = 255*p;
  int t = (int)v;
  double f = (v - t);
  
  double w0 = wavEGuitar[t];
  t = (t + 1) % 256;
  double w1 = wavEGuitar[t];
  double w = (w0 * (1-f)) + (w1 * f);
  return w;
}

//---------------------------------
double Voice2(double p) {
  if (p < 0.5) {
    return VOICE2_MAX;
  }
  else {
    return -VOICE2_MAX;
  }
}

#if BT_A2DP
//---------------------------------
int32_t bt_callback(Channels *data, int32_t len) {
      for (int i = 0; i < len; i++) {
        double e = 0;
        for (int j = 0; j < VOICE_MAX; j++) {
          phase[j] += fst[j];
          if (phase[j] >= 1.0) phase[j] -= 1.0;
          double g = Voice2(phase[j]);
          e += g * volReq[j];
        }
        e += reverbBuffer[reverbPos];
        if ( (-0.005 < e) && (e < 0.005) ) {
          e = 0.0;
        }
        if (e < -32000.0) e = -32000;
        else if (e > 32000.0) e = 32000;
        reverbBuffer[reverbPos] = e * reverbRate;
        reverbPos = (reverbPos + 1) % REVERB_BUFFER_SIZE;
        int16_t d = (int16_t)(e);

        data[i].channel1 = d;
        data[i].channel2 = d;
      }
  return len;
}

#else

//---------------------------------
void voiceThread(void *pvParameters)
{
    for(;;) {
      for (int i = 0; i < DATA_SIZE; i++) {
        double e = 0;
        for (int j = 0; j < VOICE_MAX; j++) {
          phase[j] += fst[j];
          if (phase[j] >= 1.0) phase[j] -= 1.0;
          double g = Voice2(phase[j]);
          e += g * volReq[j];
        }
        e += reverbBuffer[reverbPos];
        if ( (-0.005 < e) && (e < 0.005) ) {
          e = 0.0;
        }
        if (e < -32000.0) e = -32000;
        else if (e > 32000.0) e = 32000;
        reverbBuffer[reverbPos] = e * reverbRate;
        reverbPos = (reverbPos + 1) % REVERB_BUFFER_SIZE;
        int16_t d = (int16_t)(e);
        
        soundBuffer[i] = d;
      }
      size_t transBytes;
      i2s_write(Speak_I2S_NUMBER, (char*)soundBuffer, sizeof(soundBuffer), &transBytes, portMAX_DELAY);
    }
}
#endif

//--------------------------
double CalcInvFrequency(double note) {
  return 440.0 * pow(2, (note - (69-12))/12);
}

//----------------------------------
void setup()
{
  //Wire.begin(32, 33);
  M5.begin(true, true, true, false);

  M5.Axp.SetLed(1);
  delay(200);
  M5.Axp.SetLed(0);
  delay(200);
  M5.Axp.SetLed(1);
  delay(200);
  M5.Axp.SetLed(0);

  Serial.println("-----------");

  //M5.IMU.Init(); // accerelometer

  Serial.println("Hello CHOORD");
  //WiFi.mode(WIFI_OFF); 

  M5.Lcd.setBrightness(100);
  Serial.println("-----------");
  
  M5.Lcd.setRotation(0);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.drawFastHLine(0, TY1, TW, WHITE);
  M5.Lcd.drawFastHLine(0, TY2, TW, WHITE);
  M5.Lcd.drawFastVLine(TX1, 0, TH, WHITE);
  M5.Lcd.drawFastVLine(TX2, 0, TH, WHITE);
  M5.Lcd.setTextFont(4);

  const int keyPortList[7] = { 13, 14, 19, 27, 25, 26, 35 };
  for (int i = 0; i < 7; i++) {
    if (keyPortList[i] < 34) {
      pinMode(keyPortList[i], INPUT_PULLUP);
    }
    else {
      pinMode(keyPortList[i], INPUT); // 34-39 は外付け PULLUP が必要
    }
  }
  SetupMusics();
  SetSong(0);

#if BT_A2DP
  a2dp_source.start(BT_DEVICENAME, bt_callback);
  if (a2dp_source.isConnected() == false) {
    M5.Axp.SetLed(1);
    delay(100);
    M5.Axp.SetLed(0);
    delay(100);
  }
#else
  M5.Axp.SetSpkEnable(true);
  InitI2SSpeakOrMic(MODE_SPK);
  xTaskCreatePinnedToCore(voiceThread, "voiceThread", 8192, NULL, 3, NULL, CORE1);
#endif
}

//----------------------------------
static unsigned long sw0 = 0;
static unsigned long pd0 = -1;
static int swcnt = 0;
static int pdt = 0;
static double pdVol = 0.0;
static int pdmode = 0;
static uint8_t keyCurrent = 0;
static uint8_t keyStable = 0;
const uint8_t bitNext = (1 << 0);
const uint8_t bitLeft = (1 << 1);
const uint8_t bitPrev = (1 << 2);
const uint8_t bitReset = (1 << 3);
const uint8_t bitTouch = (1 << 4);
const uint8_t bitPedal = (1 << 5);
const uint8_t bitEdit = (1 << 6);

void PedalExec(int pedalA, int pedalB, double et) {
  switch (pdmode) {
    case 0: {
      if (pedalA == LOW) {
        pd0 = millis();
        pdmode = 1;
      }
      break;
    }
    case 1: {
      if (pedalB == LOW) {
        pdt = (int)(millis() - pd0); // 0ms - 300ms
        if (pdt > 500) pdt = 500;
        pdmode = 2;
        //Serial.println("2");
        int v = 0;
        for (int i = 0; i < VOICE_MAX; i++) {
          tm[i] = -i * (pdt/1000.0)/4;
          toff[i] = -1.0;
          if (code_notes[i] >= 0) {
            double ft = CalcInvFrequency(code_notes[i]);
            fst[i] = (ft / SAMPLING_RATE_DOUBLE);
            v++;
          }
          else {
            fst[i] = 0;
          }
        }
        pdVol = 0.5;
      }
      else {
        for (int i = 0; i < VOICE_MAX; i++) {
          if ((fst[i] > 0)&&(tm[i] < 10000)) {
            tm[i] += et;
            volReq[i] = pdVol * CalcAdsr(tm[i], toff[i]);
          }
        }
        if (pedalA != LOW) {
          pd0 = millis();
        }
      }
      break;
    }
    case 2: {
      for (int i = 0; i < VOICE_MAX; i++) {
        if (fst[i] > 0) {
          tm[i] += et;
          volReq[i] = pdVol * CalcAdsr(tm[i], toff[i]);
        }
      }
      if ((pedalA != LOW) && (pedalB != LOW)) {
        for (int i = 0; i < VOICE_MAX; i++) {
          toff[i] = tm[i];
        }
        pdmode = 3;
        //Serial.println("3");
      }
      break;
    }
    case 3: {
      for (int i = 0; i < VOICE_MAX; i++) {
        if (fst[i] > 0) {
          tm[i] += et;
          volReq[i] = pdVol * CalcAdsr(tm[i], toff[i]);
        }
      }
      if (pedalA == LOW) {
        pd0 = millis();
        pdmode = 1;
        //Serial.println("1");
      }
      break;
    }
  }
}

//----------------------------------
void loop()
{
  
  double et = 0.0;
  if (sw0 == 0) {
    sw0 = millis();
  }
  else {
    unsigned long sw = millis();
    et = ((int)(millis() - sw0)) / 1000.0;
    sw0 = sw;
  }
  int b = 0;
  TouchPoint_t pos= M5.Touch.getPressPoint();
  int touch_x = 240-pos.y;
  if (pos.y < 0) touch_x = -1;
  int touch_y = pos.x;

  bool keyReset = digitalRead(13);
  bool keyPrev = digitalRead(14);
  bool keyNext = digitalRead(19);
  bool keyLeft = digitalRead(27);
  bool keyPedal = digitalRead(35);
  bool pedalA = digitalRead(25);
  bool pedalB = digitalRead(26);

  uint8_t keyData = 0;
  if (keyNext == LOW) keyData |= bitNext;
  if (keyLeft == LOW) keyData |= bitLeft;
  if (keyPrev == LOW) keyData |= bitPrev;
  if (keyReset == LOW) keyData |= bitReset;
  if (keyPedal == LOW) keyData |= bitNext;
  if (M5.Touch.ispressed()) {
    keyData |= bitTouch;
    // タッチで物理ボタンの代わり
    if (touch_y > TY2) {
      if (touch_x < TX1) {
        keyData |= bitLeft;
      }
      else if (touch_x < TX2) {
        keyData |= bitPrev;
      }
      else {
        keyData |= bitNext;
      }
    }
    else if (touch_y < TY1) {
      if (touch_x < TX1) {
        keyData |= bitEdit;
      }
      else if (touch_x > TX2) {
        keyData |= bitReset;
      }
    }
  }

  uint8_t keyPush = 0;
  if (keyData == keyCurrent) {
    swcnt++;
    if (swcnt == 2) {
      keyPush = (keyStable ^ keyCurrent) & keyStable;
      keyStable = keyCurrent;
  
      if (keyPush & bitNext) {
        SetNextCode(1);
      }
      if (keyPush & bitReset) {
        SetNextCode(0);
      }
      if (keyPush & bitLeft) {
        SetSong(1);
      }
      if (keyPush & bitPrev) {
        SetNextCode(-1);
      }
    }
  } else {
    swcnt = 0;
  }
  keyCurrent = keyData;

  if (keyData & bitTouch) {
    if ((touch_y > TY1) && (touch_y < TY2)) {
      pedalA = LOW;
      pedalB = LOW;
    }
  }
  PedalExec(pedalA, pedalB, et);
  

#if 0
  //M5.Lcd.fillScreen();
  M5.Lcd.setTextSize(1);
  int y = 20;
  char s[64];
  sprintf(s, "POS(%d, %d, %d)    ", M5.Touch.ispressed(), touch_x, touch_y);
  M5.Lcd.drawString(s,0,y); y+= 28;  
  sprintf(s, "K %02d(%d, %d, %d, %d, %d, %d, %d)", swcnt%100, (int)keyLeft, (int)keyPrev, (int)keyNext, (int)keyReset, (int)keyPedal, (int)pedalA, (int)pedalB);
  M5.Lcd.drawString(s,0,y); y+= 28;  
  //sprintf(s, "ET(%d ms)    ", (int)(et*1000));
  //M5.Lcd.drawString(s,0,y); y+= 28;  
  sprintf(s, "PDT(%d ms)    ", pdt);
  M5.Lcd.drawString(s,0,y); y+= 28;  
  sprintf(s, "NT(%d, %d, %d, %d)", code_notes[0], code_notes[1], code_notes[2], code_notes[3] );
  M5.Lcd.drawString(s,0,y); y+= 28;

  if (current_code != "") {
    M5.Lcd.drawString(current_code.c_str(), 0, y); y+= 28;
  }
  
  //sprintf(s, "NOTE(%1.1f)    ", 59+(float)note);
  //M5.Lcd.drawString(s,0,y); y+= 28;  
#endif
  delay(5);
}
