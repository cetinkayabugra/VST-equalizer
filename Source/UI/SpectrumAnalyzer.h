#pragma once

#include <JuceHeader.h>
#include <vector>
#include <queue>

namespace DSP
{
    /**
     * SpectrumAnalyzer - Real-time FFT-based spectrum analysis
     * Provides input and output spectrum visualization
     */
    class SpectrumAnalyzer
    {
    public:
        static constexpr int FFT_SIZE = 2048;
        static constexpr int NUM_BINS = FFT_SIZE / 2;

        SpectrumAnalyzer()
            : fftEngine(FFT_SIZE),
              inputMagnitudes(NUM_BINS + 1, 0.0f),
              outputMagnitudes(NUM_BINS + 1, 0.0f),
              inputBuffer(2, FFT_SIZE),
              outputBuffer(2, FFT_SIZE),
              inputWriteIndex(0),
              outputWriteIndex(0)
        {
        }

        /**
         * Prepare analyzer with sample rate
         */
        void prepare(double sampleRate)
        {
            this->sampleRate = sampleRate;
        }

        /**
         * Push input sample for analysis
         */
        void pushInputSample(float sample) noexcept
        {
            if (inputWriteIndex < FFT_SIZE)
            {
                inputBuffer.setSample(0, inputWriteIndex, sample);
                if (inputWriteIndex == FFT_SIZE - 1)
                {
                    inputFFTReady = true;
                }
                inputWriteIndex = (inputWriteIndex + 1) % FFT_SIZE;
            }
        }

        /**
         * Push output sample for analysis
         */
        void pushOutputSample(float sample) noexcept
        {
            if (outputWriteIndex < FFT_SIZE)
            {
                outputBuffer.setSample(0, outputWriteIndex, sample);
                if (outputWriteIndex == FFT_SIZE - 1)
                {
                    outputFFTReady = true;
                }
                outputWriteIndex = (outputWriteIndex + 1) % FFT_SIZE;
            }
        }

        /**
         * Perform FFT analysis (call from UI thread)
         */
        void performAnalysis()
        {
            if (inputFFTReady.exchange(false))
            {
                performFFT(inputBuffer, inputMagnitudes);
            }

            if (outputFFTReady.exchange(false))
            {
                performFFT(outputBuffer, outputMagnitudes);
            }
        }

        /**
         * Get input spectrum magnitudes (dB)
         */
        [[nodiscard]] const std::vector<float>& getInputMagnitudes() const noexcept
        {
            return inputMagnitudes;
        }

        /**
         * Get output spectrum magnitudes (dB)
         */
        [[nodiscard]] const std::vector<float>& getOutputMagnitudes() const noexcept
        {
            return outputMagnitudes;
        }

        /**
         * Get frequency for a bin index
         */
        [[nodiscard]] float getFrequencyForBin(int binIndex) const noexcept
        {
            return static_cast<float>(sampleRate * binIndex / FFT_SIZE);
        }

    private:
        void performFFT(juce::AudioBuffer<float>& buffer, std::vector<float>& magnitudes)
        {
            // Copy and apply Hann window
            auto* fftInput = fftEngine.getBuffer().getWritePointer(0);
            auto* readPtr = buffer.getReadPointer(0);

            for (int i = 0; i < FFT_SIZE; ++i)
            {
                // Hann window
                float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (FFT_SIZE - 1)));
                fftInput[i] = readPtr[i] * window;
            }

            fftEngine.performRealOnlyForwardTransform(fftInput);

            // Convert to magnitude spectrum in dB
            for (int i = 0; i < NUM_BINS; ++i)
            {
                float real = fftInput[2 * i];
                float imag = fftInput[2 * i + 1];
                float magnitude = std::sqrt(real * real + imag * imag);
                
                // Normalize by FFT size
                magnitude /= FFT_SIZE;
                
                // Convert to dB (with floor to avoid log(0))
                magnitudes[i] = 20.0f * std::log10(std::max(magnitude, 1e-6f));
            }
        }

        juce::dsp::FFT fftEngine;
        std::vector<float> inputMagnitudes;
        std::vector<float> outputMagnitudes;
        juce::AudioBuffer<float> inputBuffer;
        juce::AudioBuffer<float> outputBuffer;

        std::atomic<bool> inputFFTReady{false};
        std::atomic<bool> outputFFTReady{false};
        
        int inputWriteIndex = 0;
        int outputWriteIndex = 0;
        double sampleRate = 44100.0;
    };
}
