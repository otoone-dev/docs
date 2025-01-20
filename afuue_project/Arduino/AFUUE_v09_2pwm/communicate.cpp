#include "afuue_common.h"
#include "communicate.h"

const char* version = "AFUUE ver1.1.0.0";
const uint8_t commVer1 = 0x16;  // 1.6
const uint8_t commVer2 = 0x02;  // protocolVer = 2

//---------------------------------
int ChangeSigned(int data) {
    if (data < 64)
    {
        return data;
    }
    else
    {
        return data - 128;
    }
}

//---------------------------------
int ChangeUnsigned(int data)
{
    if (data >= 0)
    {
        return data;
    }
    else
    {
        return 128 + data;
    }
}

#if 0
//---------------------------------
void OnReceiveCommand() {
    /*
     * 0xA1 SET TONE
     * 0xA2 SET TRANSPOSE
     * 0xA3 SET FINE TUNE
     * 0xA4 SET REVERB LEVEL
     * 0xA5 SET PORTAMENTO LEVEL
     * 0xA6 SET KEY SENSITIVITY
     * 0xA7 SET BREATH SENSITIVITY
     * 0xA8 SET METRONOME
     * 0xA9 SET MIDI
     * 0xAA SET WAVE A DATA
     * 0xAB SET WAVE B DATA
     * 0xAC SET SHIFT TABLE
     * 
     * 0xB1 GET TONE
     * 0xB2 GET TRANSPOSE
     * 0xB3 GET FINE TUNE
     * 0xB4 GET REVERB LEVEL
     * 0xB5 GET PORTAMENTO LEVEL
     * 0xB6 GET KEY SENSITIVITY
     * 0xB7 GET BREATH SENSITIVITY
     * 0xB8 GET METRONOME
     * 0xB9 GET MIDI
     * 0xBA GET WAVE A DATA
     * 0xBB GET WAVE B DATA
     * 0xBC GET SHIFT TABLE
     * 
     * 0xC1 RESPONSE TONE
     * 0xC2 RESPONSE TRANSPOSE
     * 0xC3 RESPONSE FINE TUNE
     * 0xC4 RESPONSE REVERB LEVEL
     * 0xC5 RESPONSE PORTAMENTO LEVEL
     * 0xC6 RESPONSE KEY SENSITIVITY
     * 0xC7 RESPONSE BREATH SENSITIVITY
     * 0xC8 RESPONSE METRONOME
     * 0xC9 RESPONSE MIDI
     * 0xCA RESPONSE WAVE A DATA
     * 0xCB RESPONSE WAVE B DATA
     * 0xCC RESPONSE SHIFT TABLE
     * 
     * 0xE1 STORE CONFIGS
     * 
     * 0xEE SOFTWARE RESET
     * 
     * 0xF1 GET VERSION
     * 
     * 0xFE COMMAND ACK
     * 0xFF END MESSAGE
     */
    ToneSetting* ts = &toneSettings[toneNo];
    int command = receiveBuffer[0];
    switch (command)
    {
        case 0xA1: // SET TONE
        case 0xC1: // RESPONSE TONE
            {
                toneNo = receiveBuffer[1];
                channelVolume[0] = receiveBuffer[2] / 127.0;
                channelVolume[1] = receiveBuffer[3] / 127.0;
                channelVolume[2] = receiveBuffer[4] / 127.0;
                BeginPreferences(); {
                  pref.putInt("ToneNo", toneNo);
                  LoadToneSetting(toneNo);
                } EndPreferences();
                SendCommandAck();
            }
            break;
        case 0xB1: // GET TONE NUMBER
            {
              sendBuffer[0] = 0xC1;
              sendBuffer[1] = toneNo;
              sendBuffer[2] = (uint8_t)(channelVolume[0] * 127);
              sendBuffer[3] = (uint8_t)(channelVolume[1] * 127);
              sendBuffer[4] = (uint8_t)(channelVolume[2] * 127);
              sendBuffer[5] = 0xFF;
              Serial.write(sendBuffer, 4);
              Serial.flush();
            }
            break;
        case 0xA2: // SET TRANSPOSE
        case 0xC2: // RESPONSE TRANSPOSE
            {
                int tno = receiveBuffer[1];
                int data = ChangeSigned( receiveBuffer[2] );
                toneSettings[tno].transpose = data;
                baseNote = 60 + 1 + data; // 61  C (C#)
                SendCommandAck();
            }
            break;
        case 0xB2: // GET TRANSPOSE
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xC2;
              sendBuffer[2] = ChangeUnsigned(ts->transpose);
              sendBuffer[3] = 0xFF;
              Serial.write(sendBuffer, 4);
              Serial.flush();
            }
            break;
        case 0xA3: // SET FINE TUNE
        case 0xC3: // RESPONSE FINE TUNE
            {
                int tno = receiveBuffer[1];
                int data = ChangeSigned( receiveBuffer[2] );
                toneSettings[tno].fineTune = data;
                fineTune = 440.0 + data;
                SendCommandAck();
            }
            break;
        case 0xB3: // GET FINE TUNE
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xC3;
              sendBuffer[2] = ChangeUnsigned(ts->fineTune);
              sendBuffer[3] = 0xFF;
              Serial.write(sendBuffer, 4);
              Serial.flush();
            }
            break;
        case 0xA4: // SET REVERB LEVEL
        case 0xC4: // RESPONSE REVERB LEVEL
            {
                int tno = receiveBuffer[1];
                int data = ChangeSigned( receiveBuffer[2] ); // 0 - 20
                toneSettings[tno].reverb = data;
                reverbRate = data * 0.02; // (default 0.152)
                SendCommandAck();
            }
            break;
        case 0xB4: // GET REVERB LEVEL
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xC4;
              sendBuffer[2] = ChangeUnsigned(ts->reverb);
              sendBuffer[3] = 0xFF;
              Serial.write(sendBuffer, 4);
              Serial.flush();
            }
            break;
        case 0xA5: // SET PORTAMENTO LEVEL
        case 0xC5: // RESPONSE PORTAMENTO LEVEL
            {
                int tno = receiveBuffer[1];
                int data = ChangeSigned( receiveBuffer[2] ); // 0 - 20
                toneSettings[tno].portamento = data;
                portamentoRate = 1 - (data * 0.04); // (default 0.99)
                SendCommandAck();
            }
            break;
        case 0xB5: // GET PORTAMENTO LEVEL
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xC5;
              sendBuffer[2] = ChangeUnsigned(ts->portamento);
              sendBuffer[3] = 0xFF;
              Serial.write(sendBuffer, 4);
              Serial.flush();
            }
            break;
        case 0xA6: // SET KEY SENSITIVITY
        case 0xC6: // RESPONSE KEY SENSITIVITY
            {
                int tno = receiveBuffer[1];
                int data = ChangeSigned( receiveBuffer[2] ); // 0 - 20
                toneSettings[tno].keySense = data;
                noteDiv = 1.0 / (1 + (data * 0.3)); // (default 1.0 / 3.0)
                SendCommandAck();
            }
            break;
        case 0xB6: // GET KEY SENSITIVITY
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xC6;
              sendBuffer[2] = ChangeUnsigned(ts->keySense);
              sendBuffer[3] = 0xFF;
              Serial.write(sendBuffer, 4);
              Serial.flush();
            }
            break;
        case 0xA7: // SET BREATH SENSITIVITY
        case 0xC7: // RESPONSE BREATH SENSITIVITY
            {
                int tno = receiveBuffer[1];
                int data = ChangeSigned( receiveBuffer[2] ); // 0 - 20
                toneSettings[tno].breathSense = data;
                volumeRate = 0.9 - (data * 0.04); // (default 0.5)
                SendCommandAck();
            }
            break;
        case 0xB7: // GET BREATH SENSITIVITY
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xC7;
              sendBuffer[2] = ChangeUnsigned(ts->breathSense);
              sendBuffer[3] = 0xFF;
              Serial.write(sendBuffer, 4);
              Serial.flush();
            }
            break;
        case 0xA8: // SET METRONOME
        case 0xC8: // RESPONSE METRONOME
            {
              bool enabled = (receiveBuffer[1] == 1);
              uint8_t mode = receiveBuffer[2];
              int volume = receiveBuffer[3];
              int tempo = receiveBuffer[4] & 0x7F | ((receiveBuffer[5] & 0x7F) << 7);
              metronome = enabled;
              metronome_m = mode;
              metronome_t = tempo;
              metronome_v = volume;
              metronome_cnt = 0;
              BeginPreferences(); {
                pref.putInt("MetroMode", metronome_m);
                pref.putInt("MetroTempo", metronome_t);
                pref.putInt("MetroVolume", metronome_v);
              } EndPreferences();
              SendCommandAck();
              break;
            }
        case 0xB8: // GET METRONOME
            {
              sendBuffer[0] = 0xC8;
              sendBuffer[1] = 0;
              if (metronome) {
                sendBuffer[1] = 1;
              }
              sendBuffer[2] = metronome_m;
              sendBuffer[3] = metronome_v;
              sendBuffer[4] = (uint8_t)(metronome_t & 0x7F);
              sendBuffer[5] = (uint8_t)((metronome_t >> 7) & 0x7F);
              sendBuffer[6] = 0xFF;
              Serial.write(sendBuffer, 7);
              Serial.flush();
              break;
            }
        case 0xA9: // SET MIDI
            {
              midiMode = receiveBuffer[1];
              midiPgNo = receiveBuffer[2];
              BeginPreferences(); {
                pref.putInt("MidiMode", midiMode);
                pref.putInt("MidiPgNo", midiPgNo);
              } EndPreferences();
              SendCommandAck();
              break;
            }
        case 0xB9: // GET MIDI
            {
              sendBuffer[0] = 0xC9;
              sendBuffer[1] = midiMode;
              sendBuffer[2] = midiPgNo;
              sendBuffer[3] = 0xFF;
              Serial.write(sendBuffer, 4);
              Serial.flush();
              break;
            }
        case 0xAA: // SET WAVE A DATA
        case 0xCA: // SET WAVE A DATA
        case 0xAB: // SET WAVE B DATA
        case 0xCB: // SET WAVE B DATA
            if (receivePos >= 512) {
              int tno = receiveBuffer[1];
              for (int i = 0; i < 256; i++) {
                uint32_t d0 = receiveBuffer[2 + i*2];
                uint32_t d1 = receiveBuffer[2 + i*2 + 1];
                uint32_t rdata = ((d0 << 2) | (d1 << 9));
                int32_t data = *reinterpret_cast<int32_t*>(&rdata);
                if ((command & 0x0F) == 0x0A) {
                  toneSettings[tno].waveA[i] = data;
                  waveA[i] = data;
                } else {
                  toneSettings[tno].waveB[i] = data;
                  waveB[i] = data;
                }
              }
              SendCommandAck();
            }
            break;
        case 0xBA: // GET WAVE A DATA
        case 0xBB: // GET WAVE B DATA
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              if ((command & 0x0F) == 0x0A) {
                sendBuffer[0] = 0xCA;
                for (int i = 0; i < 256; i++) {
                  int d0 = (toneSettings[tno].waveA[i] >> 2) & 0x7F;
                  int d1 = (toneSettings[tno].waveA[i] >> 9) & 0x7F;
                  sendBuffer[2 + i*2] = (byte)d0; 
                  sendBuffer[2 + i*2 + 1] = (byte)d1; 
                }
              } else {
                sendBuffer[0] = 0xCB;
                for (int i = 0; i < 256; i++) {
                  int d0 = (toneSettings[tno].waveB[i] >> 2) & 0x7F;
                  int d1 = (toneSettings[tno].waveB[i] >> 9) & 0x7F;
                  sendBuffer[2 + i*2] = (byte)d0; 
                  sendBuffer[2 + i*2 + 1] = (byte)d1; 
                }
              }
              sendBuffer[514] = 0xFF;
              Serial.write(sendBuffer, 515);
              Serial.flush();
            }
            break;
        case 0xAC: // SET SHIFT TABLE
        case 0xCC:
            {
              int tno = receiveBuffer[1];
              for (int i = 0; i < 32; i++) {
                shiftTable[i] = receiveBuffer[2 + i];
                toneSettings[tno].shiftTable[i] = shiftTable[i];
              }
              SendCommandAck();
            }
            break;
        case 0xBC: // GET SHIFT TABLE
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xCC;
              for (int i = 0; i < 32; i++) {
                sendBuffer[2 + i] = toneSettings[tno].shiftTable[i];
              }
              sendBuffer[34] = 0xFF;
              Serial.write(sendBuffer, 35);
              Serial.flush();
            }
            break;
        case 0xAD: // SET NOISE TABLE
        case 0xCD:
            {
              int tno = receiveBuffer[1];
              for (int i = 0; i < 32; i++) {
                noiseTable[i] = receiveBuffer[2 + i];
                toneSettings[tno].noiseTable[i] = noiseTable[i];
              }
              SendCommandAck();
            }
            break;
        case 0xBD: // GET NOISE TABLE
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xCD;
              for (int i = 0; i < 32; i++) {
                sendBuffer[2 + i] = toneSettings[tno].noiseTable[i];
              }
              sendBuffer[34] = 0xFF;
              Serial.write(sendBuffer, 35);
              Serial.flush();
            }
            break;
        case 0xAE: // SET PITCH TABLE
        case 0xCE:
            {
              int tno = receiveBuffer[1];
              for (int i = 0; i < 32; i++) {
                pitchTable[i] = receiveBuffer[2 + i];
                toneSettings[tno].pitchTable[i] = pitchTable[i];
              }
              SendCommandAck();
            }
            break;
        case 0xBE: // GET PITCH TABLE
            {
              int tno = receiveBuffer[1];
              sendBuffer[1] = tno;
              sendBuffer[0] = 0xCE;
              for (int i = 0; i < 32; i++) {
                sendBuffer[2 + i] = toneSettings[tno].pitchTable[i];
              }
              sendBuffer[34] = 0xFF;
              Serial.write(sendBuffer, 35);
              Serial.flush();
            }
            break;
        case 0xE1: // WRITE TO FLASH
            if (receiveBuffer[1] == 0x7F) {
              SaveToneSetting(toneNo);
              SendCommandAck();
            }
            break;
        case 0xEE: // SOFTWARE RESET
            SendCommandAck();
            delay(1000);

            if (receiveBuffer[1] == 0x7F) {
              ESP.restart();
            } else if ((receiveBuffer[1] == 0x7A) && (receiveBuffer[2] == 0x3C)) {
              BeginPreferences(); {
                pref.clear(); // CLEAR ALL FLASH MEMORY
              } EndPreferences();
              ESP.restart();
            }
            break;
        case 0xF1: // GET AFUUE VERSION
          sendBuffer[0] = 0xF1;
          sendBuffer[1] = commVer1;
          sendBuffer[2] = commVer2;
          sendBuffer[3] = 0xFF;
          Serial.write(sendBuffer, 3);
          Serial.flush();
    }

    waitForCommand = true;
    receivePos = 0;
}

//---------------------------------
void SendCommandAck() {
  Serial.write(0xFE);
  Serial.write(0xFF);
  Serial.flush();
}


//---------------------------------
void SerialProc() {
  int dataSize = Serial.available();
  if (dataSize == 0) return;

  uint8_t buff[256];
  if (dataSize > 256) dataSize = 256;
  int readSize = Serial.readBytes(buff, dataSize);
  for (int i = 0; i < readSize; i++) {
    if (receivePos >= 1000) break;
    int d = buff[i];
    if (waitForCommand)
    {
        if ((d >= 0x80) && (d != 0xFF))
        {
            receivePos = 0;
            receiveBuffer[receivePos++] = d;
            waitForCommand = false;
        }
    }
    else
    {
        if (d < 0x80)
        {
            receiveBuffer[receivePos++] = d;
        }
        else
        {
            OnReceiveCommand();
            if (d < 0xFF)
            {
                receivePos = 0;
                receiveBuffer[receivePos++] = d;
                waitForCommand = false;
            }
        }
    }
  }
}
#endif
