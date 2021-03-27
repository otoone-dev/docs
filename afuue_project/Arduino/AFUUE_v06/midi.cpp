#include "afuue_common.h"
#include "midi.h"

bool midiEnabled = false;
byte midiMode = 5; // 1:BreathControl 2:Expression 3:AfterTouch 4:MainVolume 5:CUTOFF(for KORG NTS-1)
byte midiPgNo = 51;
uint8_t midiPacket[32];
bool deviceConnected = false;
bool playing = false;
int channelNo = 0;
int prevNoteNumber = -1;
unsigned long activeNotifyTime = 0;


#if ENABLE_MIDI
#include <HardwareSerial.h>
HardwareSerial SerialHW(0);
#endif

#if ENABLE_BLE_MIDI
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"  // Apple BLE MIDI とするので固定
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

BLECharacteristic *pCharacteristic;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      SerialPrintLn("connected");

      MIDI_NoteOn(baseNote + 1, 50);
      delay(500);
      MIDI_NoteOff();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      SerialPrintLn("disconnected");
    }
};
#endif



//---------------------------------
void MIDI_Initialize() {
#if ENABLE_MIDI
    SerialPrintLn("hardware serial begin");
    pinMode(1, OUTPUT);
    pinMode(4, INPUT);
    SerialHW.begin(31250, SERIAL_8N1, 4, 1);
    deviceConnected = true;

    delay(100);
    MIDI_ProgramChange(midiPgNo);
    MIDI_NoteOn(baseNote + 1, 10);
    delay(500);
    MIDI_NoteOff();
#endif
#if ENABLE_BLE_MIDI
    BLEDevice::init(version);
      
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
#endif
}

//---------------------------------
void MIDI_NoteOn(int note, int vol) {
    if (playing != false) return;
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
#endif
#if ENABLE_MIDI
    midiPacket[0] = 0x90 + channelNo;
    midiPacket[1] = note;
    midiPacket[2] = 120;
    if (midiMode == 3) { // after touch (channel pressure)
      midiPacket[3] = 0xD0 + channelNo;
      midiPacket[4] = vol;
      SerialHW.write(midiPacket, 5);
    } else {
      midiPacket[3] = 0xB0 + channelNo;
      midiPacket[4] = 0x0B; // expression
      if (midiMode == 1) {
        midiPacket[4] = 0x02; // breath control
      }
      else if (midiMode == 4) {
        midiPacket[4] = 0x07; // main volume
      }
      else if (midiMode == 5) {
        midiPacket[5] = 43; // CUT OFF (for KORG NTS-1)
      }
      midiPacket[5] = vol;
      SerialHW.write(midiPacket, 6);
    }
#endif
    playing = true;
    prevNoteNumber = note;
    delay(8);
}

//---------------------------------
void MIDI_ChangeNote(int note, int vol) {
    if ((playing == false) || (prevNoteNumber < 0)) return;
    MIDI_NoteOff();
    
    MIDI_NoteOn(note, vol);
}

//---------------------------------
void MIDI_NoteOff() {
    if ((playing == false) || (prevNoteNumber < 0)) return;
#if ENABLE_BLE_MIDI
    midiPacket[0] = 0x80; // header
    midiPacket[1] = 0x80; // timestamp, not implemented
    midiPacket[2] = 0x80 + channelNo; // note up, channel 0
    midiPacket[3] = prevNoteNumber;
    midiPacket[4] = 0;    // velocity
    pCharacteristic->setValue(midiPacket, 5); // packet, length in bytes)
    pCharacteristic->notify();
#endif
#if ENABLE_MIDI
    midiPacket[0] = 0x80 + channelNo;
    midiPacket[1] = prevNoteNumber;
    midiPacket[2] = 0;
      
    SerialHW.write(midiPacket, 3);
#endif
    playing = false;
    prevNoteNumber = -1;
    delay(4);
}

//---------------------------------
void MIDI_BreathControl(int vol) {
#if ENABLE_BLE_MIDI
    midiPacket[0] = 0x80; // header
    midiPacket[1] = 0x80; // timestamp, not implemented
    midiPacket[2] = 0xB0 + channelNo; // control change, channel 0
    //midiPacket[3] = 0x0B; // expression
    midiPacket[3] = 0x02; // breath control
    midiPacket[4] = vol;
    pCharacteristic->setValue(midiPacket, 5); // packet, length in bytes
    pCharacteristic->notify();
#endif
#if ENABLE_MIDI
    if (midiMode == 3) { // after touch (channel pressure)
      midiPacket[0] = 0xD0 + channelNo;
      midiPacket[1] = vol;
      SerialHW.write(midiPacket, 2);
    } else {
      midiPacket[0] = 0xB0 + channelNo;
      midiPacket[1] = 0x0B; // expression
      if (midiMode == 1) {
        midiPacket[1] = 0x02; // breath control
      }
      else if (midiMode == 4) {
        midiPacket[1] = 0x07; // main volume
      }
      else if (midiMode == 5) {
        midiPacket[1] = 43; // CUTOFF (for KORG NTS-1)
      }
      midiPacket[2] = vol;
      SerialHW.write(midiPacket, 3);
    }
#endif
    delay(4);
}

//---------------------------------
void MIDI_ProgramChange(int no) {
#if ENABLE_BLE_MIDI
#endif
#if ENABLE_MIDI
    midiPacket[0] = 0xC0 + channelNo;
    midiPacket[1] = no;
      
    SerialHW.write(midiPacket, 2);
#endif
    delay(2);
}

//---------------------------------
void MIDI_ActiveNotify() {
#if ENABLE_BLE_MIDI
    midiPacket[0] = 0x80; // header
    midiPacket[1] = 0x80; // timestamp, not implemented
    midiPacket[2] = 0xFE; // active sense
    pCharacteristic->setValue(midiPacket, 3); // packet, length in bytes
    pCharacteristic->notify();
#endif
#if ENABLE_MIDI
    midiPacket[0] = 0xFE;
      
    SerialHW.write(midiPacket, 1);
#endif
    delay(1);
}
