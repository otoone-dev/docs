#pragma once
#include <Arduino.h>
#include <driver/ledc.h>
#include <soc/ledc_struct.h>
#include "../SoundProcessor/WaveGenerator.h"
#include "../SoundProcessor/LowPassFilter.h"
#include "../SoundProcessor/Delay.h"
#include "../SoundProcessor/SoundProcessorBase.h"
#include "OutputDeviceBase.h"
#include "Parameters.h"

//---------------------------------
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

hw_timer_t* timer = NULL;
volatile uint32_t waveHigh = 0;
volatile uint32_t waveLow = 0;
//---------------------------------
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
class Speaker : public OutputDeviceBase {
public:
    Speaker(gpio_num_t low, gpio_num_t high, std::vector<SoundProcessorBase*>& soundProcessors)
    : m_lowPin(low)
    , m_highPin(high)
    , m_soundProcessors(soundProcessors) {
    }

    //--------------
    const char* GetName() const override {
        return "Speaker";
    }

    //--------------
    InitializeResult Initialize() override {
        InitializeResult result;
        pinMode(m_lowPin, OUTPUT);
        pinMode(m_highPin, OUTPUT);
        // LEDC set up
        ledcAttachChannel(m_highPin, 156250, 8, LEDC_CHANNEL_2);
        ledcAttachChannel(m_lowPin, 156250, 8, LEDC_CHANNEL_1);

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
    OutputResult Update(const Parameters& parameters, Message& msg) override {
        tickCount = CalcFrequency(parameters.fineTune, msg.note + msg.bend) / parameters.samplingRate;
        volume = msg.volume;

        for (auto& processor : m_soundProcessors) {
            processor->UpdateParameter(parameters, volume);
        }

        float load = cpuLoad / 1000000.0f;
        return OutputResult{ true, load / (1.0f / parameters.samplingRate) };
    }

    //--------------
    static float CalcFrequency(float fine, float note) {
        return fine * pow(2, (note - (69.0f-12.0f))/12.0f);
    }

    //--------------
    static void CreateWaveTask(void* parameter) {
        Speaker* pSystem = static_cast<Speaker*>(parameter);

        TickType_t xLastWakeTime;
        const TickType_t xFrequency = 1 / portTICK_PERIOD_MS; // 1ms

        float fmid = 30000.0f;
        const int32_t umid = 32768;
        SoundInfo info(60.0f, 0.0f, 0.0f, 0.0f, 0.0f);
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

                int32_t i = Clamp<int32_t>(umid + static_cast<int32_t>(info.wave * fmid), 0, 65535);
                uint16_t w = static_cast<uint16_t>(i);
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

private:
    std::vector<SoundProcessorBase*>& m_soundProcessors;
    gpio_num_t m_lowPin;
    gpio_num_t m_highPin;
};
