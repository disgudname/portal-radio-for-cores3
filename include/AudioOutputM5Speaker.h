#pragma once

#include <AudioOutput.h>
#include <M5Unified.h>

/**
 * AudioOutput for M5.Speaker - based on official M5Unified example.
 *
 * Key design decisions for clean audio:
 * 1. Triple buffering (3 x 1536 samples) - prevents crackling
 * 2. No blocking in ConsumeSample() - always accept samples
 * 3. Stereo to mono mixing - M5 speaker is mono
 * 4. Fade in/out to eliminate clicks
 */
class AudioOutputM5Speaker : public AudioOutput {
public:
    static constexpr size_t TRI_BUF_SIZE = 1536;
    static constexpr size_t FADE_SAMPLES = 64;  // ~1.5ms at 44100Hz

    AudioOutputM5Speaker(m5::Speaker_Class* speaker, uint8_t channel = 0)
        : m_speaker(speaker)
        , m_channel(channel)
        , m_sampleRate(44100)
        , m_bufIdx(0)
        , m_triIdx(0)
        , m_sampleCount(0)
    {
        memset(m_triBuf, 0, sizeof(m_triBuf));
    }

    virtual ~AudioOutputM5Speaker() override {
        stop();
    }

    virtual bool begin() override {
        m_bufIdx = 0;
        m_triIdx = 0;
        m_sampleCount = 0;
        memset(m_triBuf, 0, sizeof(m_triBuf));
        return true;
    }

    virtual bool SetRate(int hz) override {
        m_sampleRate = hz;
        return true;
    }

    virtual bool SetBitsPerSample(int bits) override { return true; }
    virtual bool SetChannels(int ch) override { return true; }
    virtual bool SetGain(float f) override { return true; }

    virtual bool ConsumeSample(int16_t sample[2]) override {
        // Mix stereo to mono
        int32_t mixed = (sample[0] + sample[1]) / 2;

        // Apply fade-in for first FADE_SAMPLES to eliminate click at start
        if (m_sampleCount < FADE_SAMPLES) {
            mixed = (mixed * m_sampleCount) / FADE_SAMPLES;
        }
        m_sampleCount++;

        m_triBuf[m_triIdx][m_bufIdx] = static_cast<int16_t>(mixed);
        m_bufIdx++;

        if (m_bufIdx >= TRI_BUF_SIZE) {
            flush();
        }
        return true;
    }

    virtual bool stop() override {
        // Fade out to prevent click
        int16_t lastSample = (m_bufIdx > 0) ? m_triBuf[m_triIdx][m_bufIdx - 1] : 0;

        for (size_t i = 0; i < FADE_SAMPLES && m_bufIdx < TRI_BUF_SIZE; i++) {
            int32_t fade = (lastSample * (int32_t)(FADE_SAMPLES - i - 1)) / FADE_SAMPLES;
            m_triBuf[m_triIdx][m_bufIdx++] = static_cast<int16_t>(fade);
        }

        // Fill rest with silence
        while (m_bufIdx < FADE_SAMPLES * 2 && m_bufIdx < TRI_BUF_SIZE) {
            m_triBuf[m_triIdx][m_bufIdx++] = 0;
        }

        if (m_bufIdx > 0) {
            m_speaker->playRaw(m_triBuf[m_triIdx], m_bufIdx, m_sampleRate, false, 1, m_channel, false);
            m_bufIdx = 0;
        }
        return true;
    }

    virtual void flush() override {
        if (m_bufIdx == 0) return;

        m_speaker->playRaw(m_triBuf[m_triIdx], m_bufIdx, m_sampleRate, false, 1, m_channel, false);

        // Rotate to next buffer in triple-buffer
        m_triIdx = (m_triIdx + 1) % 3;
        m_bufIdx = 0;
    }

private:
    m5::Speaker_Class* m_speaker;
    uint8_t m_channel;
    uint32_t m_sampleRate;
    size_t m_bufIdx;
    size_t m_triIdx;
    int16_t m_triBuf[3][TRI_BUF_SIZE];
    size_t m_sampleCount;
};
