#include "afuue_common.h"
#include "afuue_midi.h"

#if ENABLE_MIDI

#if ENABLE_BLE_MIDI
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"  // Apple BLE MIDI とするので固定
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"
#endif // ENABLE_BLE_MIDI


Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

//---------------------------------
bool AfuueMIDI::Initialize() {
#if ENABLE_BLE_MIDI
    BLEDevice::init("AFUUE2");
      
    // Create the BLE Server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
  
    // Create the BLE Service
    BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));
  
    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
      BLEUUID(CHARACTERISTIC_UUID),
      BLECharacteristic::PROPERTY_READ   |
      BLECharacteristic::PROPERTY_WRITE  |
      BLECharacteristic::PROPERTY_NOTIFY |
      BLECharacteristic::PROPERTY_WRITE_NR
    );
  
    // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
    // Create a BLE Descriptor
    pCharacteristic->addDescriptor(new BLE2902());
  
    // Start the service
    pService->start();
  
    // Start advertising
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->start();
  
    SerialPrintLn("BLE MIDI begin");
#else
    usb_midi.setStringDescriptor("AFUUE2R");
    MIDI.begin(MIDI_CHANNEL_OMNI); //すべてのチャンネルを受信
    delay(1000);
    isUSBMounted = TinyUSBDevice.mounted();
    if (isUSBMounted) {
      // USB MIDI
    } else {
      // MIDI TRS out
      Serial2.begin(31250, SERIAL_8N1, MIDI_OUT_PIN, MIDI_IN_PIN);
    }
    deviceConnected = true;
#endif
  return isUSBMounted;
}

//---------------------------------
void AfuueMIDI::Update(int note, float requestedVolume, bool isLipSensorEnabled, float bendNoteShift) {
  int v = (int)(requestedVolume * 127);
  if (playing == false) {
    if (v > 0) {
      SetLedColor(100, 100, 100);
      NoteOn(note, v);
    }
  } else {
    if (v <= 0) {
        NoteOff();
    } else {
      if (prevNoteNumber != note) {
        NoteOn(note, v);
      }
      BreathControl(v);
      int16_t b = 0;
      if (isLipSensorEnabled) {
        b = static_cast<int>(bendNoteShift * 8000);
      }
      PicthBendControl(b);
    }
  }
  unsigned long t = millis();
  if ((t < activeNotifyTime) || (activeNotifyTime + 200 < t)) {
    activeNotifyTime = t;
    ActiveNotify();
  }
}

//---------------------------------
void AfuueMIDI::SerialSend(uint8_t* midiPacket, size_t size) {
  if (isUSBMounted) {
    return;
  }
  Serial2.write(midiPacket, size);
}

//---------------------------------
void AfuueMIDI::NoteOn(int note, int vol) {
    if (playing) {
      NoteOff();
    }
    //if (playing != false) return;
#if ENABLE_BLE_MIDI
    midiPacket[0] = 0x80; // header
    midiPacket[1] = 0x80; // timestamp, not implemented
    midiPacket[2] = 0x90 + channelNo; // note down, channel 0
    midiPacket[3] = note;
    midiPacket[4] = 127;  // velocity
    midiPacket[5] = 0xB0 + channelNo; // control change, channel 0
    //midiPacket[6] = 0x0B; // expression
    midiPacket[6] = 0x02; // breath control
    midiPacket[7] = vol;
    pCharacteristic->setValue(midiPacket, 8); // packet, length in bytes
    pCharacteristic->notify();
#else
    midiPacket[0] = 0x90 + channelNo;
    midiPacket[1] = note;
    midiPacket[2] = 120;
    if (midiMode == MIDIMODE::AFTERTOUCH) { // after touch (channel pressure)
      midiPacket[3] = 0xD0 + channelNo;
      midiPacket[4] = vol;
      SerialSend(midiPacket, 5);
    } else {
      midiPacket[3] = 0xB0 + channelNo;
      midiPacket[4] = 0x0B; // expression
      if (midiMode == MIDIMODE::BREATHCONTROL) {
        midiPacket[4] = 0x02; // breath control
      }
      else if (midiMode == MIDIMODE::MAINVOLUME) {
        midiPacket[4] = 0x07; // main volume
      }
      else if (midiMode == MIDIMODE::CUTOFF) {
        midiPacket[5] = 43; // CUT OFF (for KORG NTS-1)
      }
      midiPacket[5] = vol;
      SerialSend(midiPacket, 6);
    }

    if (isUSBMounted) {
      MIDI.sendNoteOn(note, 120, channelNo);
    }
#endif
    playing = true;
    prevNoteNumber = note;
}

//---------------------------------
void AfuueMIDI::NoteOff() {
  // vol=0 で NoteOn というのもアリらしい (PSS-A50調べ)
    if ((playing == false) || (prevNoteNumber < 0)) return;
#if ENABLE_BLE_MIDI
    midiPacket[0] = 0x80; // header
    midiPacket[1] = 0x80; // timestamp, not implemented
    midiPacket[2] = 0x80 + channelNo; // note up, channel 0
    midiPacket[3] = prevNoteNumber;
    midiPacket[4] = 0;    // velocity
    pCharacteristic->setValue(midiPacket, 5); // packet, length in bytes)
    pCharacteristic->notify();
#else
    midiPacket[0] = 0x80 + channelNo;
    midiPacket[1] = prevNoteNumber;
    midiPacket[2] = 0;
      
    SerialSend(midiPacket, 3);

    if (isUSBMounted) {
      MIDI.sendNoteOff(prevNoteNumber, 0, channelNo);
    }
#endif
    playing = false;
    prevNoteNumber = -1;
}

//---------------------------------
void AfuueMIDI::PicthBendControl(int bend) {
    if (playing == false) return;

    uint16_t value = static_cast<uint16_t>(bend + 0x8000) >> 2; // 14bit
    uint8_t d0 = value & 0x7F;
    uint8_t d1 = (value >> 7) & 0x7F;
#if ENABLE_BLE_MIDI
    midiPacket[0] = 0x80; // header
    midiPacket[1] = 0x80; // timestamp, not implemented
    midiPacket[2] = 0xE0 + channelNo; 
    midiPacket[3] = d0;
    midiPacket[4] = d1;
    pCharacteristic->setValue(midiPacket, 5); // packet, length in bytes)
    pCharacteristic->notify();
#else
    midiPacket[0] = 0xE0 + channelNo;
    midiPacket[1] = d0;
    midiPacket[2] = d1;
    SerialSend(midiPacket, 3);

    if (isUSBMounted) {
      MIDI.sendPitchBend(bend, channelNo);
    }
#endif
}

//---------------------------------
void AfuueMIDI::BreathControl(int vol) {
#if ENABLE_BLE_MIDI
    midiPacket[0] = 0x80; // header
    midiPacket[1] = 0x80; // timestamp, not implemented
    midiPacket[2] = 0xB0 + channelNo; // control change, channel 0
    //midiPacket[3] = 0x0B; // expression
    midiPacket[3] = 0x02; // breath control
    midiPacket[4] = vol;
    pCharacteristic->setValue(midiPacket, 5); // packet, length in bytes
    pCharacteristic->notify();
#else
    if (midiMode == MIDIMODE::AFTERTOUCH) { // after touch (channel pressure)
      midiPacket[0] = 0xD0 + channelNo;
      midiPacket[1] = vol;
      SerialSend(midiPacket, 2);
    } else {
      midiPacket[0] = 0xB0 + channelNo;
      midiPacket[1] = 0x0B; // expression
      if (midiMode == MIDIMODE::BREATHCONTROL) {
        midiPacket[1] = 0x02; // breath control
      }
      else if (midiMode == MIDIMODE::MAINVOLUME) {
        midiPacket[1] = 0x07; // main volume
      }
      else if (midiMode == MIDIMODE::CUTOFF) {
        midiPacket[1] = 43; // CUTOFF (for KORG NTS-1)
      }
      midiPacket[2] = vol;
      SerialSend(midiPacket, 3);
    }

    if (isUSBMounted) {
      if (midiMode == MIDIMODE::AFTERTOUCH) {
          MIDI.sendAfterTouch(vol, channelNo);
      }
      else {
          MIDI.sendControlChange(midiPacket[1], vol, channelNo);
      }
    }
#endif
}

//---------------------------------
void AfuueMIDI::ProgramChange(int no) {
#if ENABLE_BLE_MIDI
    midiPacket[0] = 0x80; // header
    midiPacket[1] = 0x80; // timestamp, not implemented
    midiPacket[2] = 0xC0 + channelNo;
    midiPacket[3] = no;
    pCharacteristic->setValue(midiPacket, 4); // packet, length in bytes
    pCharacteristic->notify();
#else
    midiPacket[0] = 0xC0 + channelNo;
    midiPacket[1] = no;
      
    SerialSend(midiPacket, 2);

    if (isUSBMounted) {
      MIDI.sendProgramChange(no, channelNo);
    }
#endif
}

//---------------------------------
void AfuueMIDI::ChangeBreathControlMode() {
  BreathControl(127);
  midiMode = (MIDIMODE)(((int)midiMode + 1) % (int)MIDIMODE::MAX);
  NoteOn(60, 30);
  delay(100);
}

//---------------------------------
void AfuueMIDI::ActiveNotify() {
#if ENABLE_BLE_MIDI
    midiPacket[0] = 0x80; // header
    midiPacket[1] = 0x80; // timestamp, not implemented
    midiPacket[2] = 0xFE; // active sense
    pCharacteristic->setValue(midiPacket, 3); // packet, length in bytes
    pCharacteristic->notify();
#else
    midiPacket[0] = 0xFE;
      
    SerialSend(midiPacket, 1);

    if (isUSBMounted) {
      MIDI.sendActiveSensing();
    }
#endif
}

#endif // ENABLE_MIDI
