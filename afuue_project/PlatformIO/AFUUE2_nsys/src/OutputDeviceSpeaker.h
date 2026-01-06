#pragma once
#include <Arduino.h>
#include <driver/ledc.h>
#include <soc/ledc_struct.h>
#include "SoundProcessorWaveGen.h"
#include "SoundProcessorDelay.h"
#include "SoundProcessorBase.h"
#include "OutputDeviceBase.h"

#define SOUND_TWOPWM
#define PWMPIN_LOW (GPIO_NUM_5)
#define PWMPIN_HIGH (GPIO_NUM_6)

//---------------------------------
#define CLOCK_DIVIDER (50)//(80)
#define TIMER_ALARM (50)//(40)
//#define SAMPLING_RATE (20000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*50) = 20kHz
//#define SAMPLING_RATE (25000.0f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (80*40) = 25kHz
#define SAMPLING_RATE (32000) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (50*50) = 32kHz
//#define SAMPLING_RATE (44077.13f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (55*33) = 44kHz
//#define SAMPLING_RATE (48019.2f) // = (80*1000*1000 / (CLOCK_DIVIDER * TIMER_ALARM)) // 80MHz / (49*34) = 48kHz

/*
    |   write     read   |   write
    |   -->|       -->|  |   -->|
                      |<-- d -->|

    |   read      write  |
    |   -->|       -->|  |
    |      |<-- d ^-->|
*/
#define WAVEOUT_BUFFERMAX (512)
#define WAVEOUT_BUFFERING_SIZE (400)
uint16_t waveOutBuffer[WAVEOUT_BUFFERMAX];
int32_t waveOutBufferWritePos = 0;
int32_t waveOutBufferReadPos = WAVEOUT_BUFFERMAX / 2;
volatile float tickCount = 0;
volatile float volume = 0;
volatile uint64_t cpuLoad = 0;

hw_timer_t *timer = NULL;
volatile uint32_t waveHigh = 0;
volatile uint32_t waveLow = 0;
void IRAM_ATTR OnTimer() {
#ifdef SOUND_TWOPWM
    ledcWriteChannel(LEDC_CHANNEL_1, waveLow);
    ledcWriteChannel(LEDC_CHANNEL_2, waveHigh);
#else
    //dacWrite(DACPIN, h);
#endif
    uint32_t w = waveOutBuffer[waveOutBufferReadPos];
    waveLow = (w & 0xFF);// << 4;
    waveHigh = ((w >> 8) & 0xFF);// << 4;
    waveOutBufferReadPos = (waveOutBufferReadPos + 1) % WAVEOUT_BUFFERMAX;
}

//---------------------------------
class OutputDeviceSpeaker : public OutputDeviceBase {
private:
    std::vector<SoundProcessorBase*> m_soundProcessors;
public:
    //--------------
    const char* GetName() const override {
        return "Speaker";
    }

    //--------------
    InitializeResult Initialize() override {
        InitializeResult result;

        m_soundProcessors.push_back(new SoundProcessorWaveGen());
        m_soundProcessors.push_back(new SoundProcessorDelay());

        pinMode(PWMPIN_LOW, OUTPUT);
        pinMode(PWMPIN_HIGH, OUTPUT);
        // LEDC set up
        ledcAttachChannel(PWMPIN_HIGH, 156250, 8, LEDC_CHANNEL_2);
        ledcAttachChannel(PWMPIN_LOW, 156250, 8, LEDC_CHANNEL_1);
        for (int i = 0; i < WAVEOUT_BUFFERMAX; i++) {
            waveOutBuffer[i] = 0x8000;
        }
        xTaskCreatePinnedToCore(CreateWaveTask, "CreateWaveTask", 16384, this, 3, NULL, CORE0); // 波形生成は Core0 が専念

        timer = timerBegin(80*1000*1000 / CLOCK_DIVIDER);
        timerAttachInterrupt(timer, &OnTimer);
        timerAlarm(timer, TIMER_ALARM, true, 0);
        return result;
    }

    //--------------
    OutputResult Update(float note, float vol) override {
        tickCount = CalcFrequency(note) / SAMPLING_RATE;
        volume = vol;
        float load = cpuLoad / 1000000.0f;
        return OutputResult{ true, load / (1.0f / SAMPLING_RATE) };
    }

    //--------------
    static float CalcFrequency(float note) {
        return 440.0f * pow(2, (note - (69.0f-12.0f))/12.0f);
    }

    //--------------
    static void CreateWaveTask(void *parameter) {
        OutputDeviceSpeaker *pSystem = static_cast<OutputDeviceSpeaker *>(parameter);

        TickType_t xLastWakeTime;
        const TickType_t xFrequency = 1 / portTICK_PERIOD_MS; // 1ms

        float fmid = 32768.0f;
        const int32_t umid = 32768;
        SoundInfo info(69.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        while (1) {
            int createCount = 0;
            uint64_t t0 = micros();
            while (1) {
                int d = (waveOutBufferWritePos > waveOutBufferReadPos)
                ? waveOutBufferWritePos - waveOutBufferReadPos
                : WAVEOUT_BUFFERMAX + waveOutBufferWritePos - waveOutBufferReadPos;
                if (d > WAVEOUT_BUFFERING_SIZE) {
                    break;
                }

                info.tickCount = tickCount;
                info.volume = volume;
                for (auto& processor : pSystem->m_soundProcessors) {
                    processor->ProcessAudio(info);
                }

                float f = info.wave * fmid;
                if ( (-0.000002f < f) && (f < 0.000002f) ) {
                    f = 0.0f;
                }
                uint16_t w = static_cast<uint16_t>(umid + static_cast<int32_t>(f));
                waveOutBuffer[waveOutBufferWritePos] = w;
                waveOutBufferWritePos = (waveOutBufferWritePos + 1) % WAVEOUT_BUFFERMAX;
                createCount++;
            }
            if (createCount > 0) {
                cpuLoad = (micros() - t0) / createCount;
            }
            xTaskDelayUntil( &xLastWakeTime, xFrequency );
        }
    }
};
