#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <array>

namespace DSP
{
    /**
     * BiQuadFilter - Implements a biquadratic IIR filter
     * A fundamental building block for EQ filters
     */
    class BiQuadFilter
    {
    public:
        enum class FilterType
        {
            HighPass,
            LowPass,
            LowShelf,
            HighShelf,
            Bell
        };

        BiQuadFilter() = default;
        ~BiQuadFilter() = default;

        /**
         * Set filter parameters and recalculate coefficients
         */
        void setParameters(FilterType type, double frequency, double gain, double qFactor, double sampleRate)
        {
            if (frequency <= 0.0 || sampleRate <= 0.0 || qFactor <= 0.0)
                return;

            this->frequency = frequency;
            this->gain = gain;
            this->qFactor = qFactor;
            this->sampleRate = sampleRate;

            calculateCoefficients(type);
        }

        /**
         * Process a single sample through the filter
         */
        [[nodiscard]] double processSample(double inputSample) noexcept
        {
            // Direct Form II (transposed)
            double output = b0 * inputSample + z1;
            z1 = b1 * inputSample + z2 - a1 * output;
            z2 = b2 * inputSample - a2 * output;

            return output;
        }

        /**
         * Process an audio buffer
         */
        void processBlock(juce::AudioBuffer<float>& buffer, int channel)
        {
            const auto* readPtr = buffer.getReadPointer(channel);
            auto* writePtr = buffer.getWritePointer(channel);
            const int numSamples = buffer.getNumSamples();

            for (int i = 0; i < numSamples; ++i)
            {
                writePtr[i] = static_cast<float>(processSample(readPtr[i]));
            }
        }

        /**
         * Reset internal state
         */
        void reset() noexcept
        {
            z1 = 0.0;
            z2 = 0.0;
        }

        /**
         * Get magnitude response at a specific frequency
         */
        [[nodiscard]] double getMagnitudeResponse(double testFrequency) const noexcept
        {
            const double w = 2.0 * juce::MathConstants<double>::pi * testFrequency / sampleRate;
            const double cosW = std::cos(w);
            const double sinW = std::sin(w);

            // H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)
            double numeratorReal = b0 + b1*cosW + b2*cosW*cosW;
            double numeratorImag = b1*sinW + 2.0*b2*cosW*sinW;

            double denominatorReal = 1.0 + a1*cosW + a2*cosW*cosW;
            double denominatorImag = a1*sinW + 2.0*a2*cosW*sinW;

            // Magnitude = sqrt(real^2 + imag^2)
            double numeratorMag = std::sqrt(numeratorReal*numeratorReal + numeratorImag*numeratorImag);
            double denominatorMag = std::sqrt(denominatorReal*denominatorReal + denominatorImag*denominatorImag);

            return denominatorMag > 0.0 ? numeratorMag / denominatorMag : 1.0;
        }

    private:
        void calculateCoefficients(FilterType type)
        {
            const double w0 = 2.0 * juce::MathConstants<double>::pi * frequency / sampleRate;
            const double sinW0 = std::sin(w0);
            const double cosW0 = std::cos(w0);
            const double alpha = sinW0 / (2.0 * qFactor);

            double a0 = 1.0, a1_temp = 0.0, a2 = 0.0;
            b0 = 1.0; b1 = 0.0; b2 = 0.0;

            const double gainLinear = std::pow(10.0, gain / 20.0);

            switch (type)
            {
                case FilterType::HighPass:
                    b0 = (1.0 + cosW0) / 2.0;
                    b1 = -(1.0 + cosW0);
                    b2 = (1.0 + cosW0) / 2.0;
                    a0 = 1.0 + alpha;
                    a1_temp = -2.0 * cosW0;
                    a2 = 1.0 - alpha;
                    break;

                case FilterType::LowPass:
                    b0 = (1.0 - cosW0) / 2.0;
                    b1 = 1.0 - cosW0;
                    b2 = (1.0 - cosW0) / 2.0;
                    a0 = 1.0 + alpha;
                    a1_temp = -2.0 * cosW0;
                    a2 = 1.0 - alpha;
                    break;

                case FilterType::LowShelf:
                {
                    const double sqrtGain = std::sqrt(gainLinear);
                    const double S = 2.0 * alpha * sqrtGain;
                    b0 = gainLinear * ((gainLinear + 1.0) - (gainLinear - 1.0) * cosW0 + S);
                    b1 = 2.0 * gainLinear * ((gainLinear - 1.0) - (gainLinear + 1.0) * cosW0);
                    b2 = gainLinear * ((gainLinear + 1.0) - (gainLinear - 1.0) * cosW0 - S);
                    a0 = (gainLinear + 1.0) + (gainLinear - 1.0) * cosW0 + S;
                    a1_temp = -2.0 * ((gainLinear - 1.0) + (gainLinear + 1.0) * cosW0);
                    a2 = (gainLinear + 1.0) + (gainLinear - 1.0) * cosW0 - S;
                    break;
                }

                case FilterType::HighShelf:
                {
                    const double sqrtGain = std::sqrt(gainLinear);
                    const double S = 2.0 * alpha * sqrtGain;
                    b0 = gainLinear * ((gainLinear + 1.0) + (gainLinear - 1.0) * cosW0 + S);
                    b1 = -2.0 * gainLinear * ((gainLinear - 1.0) + (gainLinear + 1.0) * cosW0);
                    b2 = gainLinear * ((gainLinear + 1.0) + (gainLinear - 1.0) * cosW0 - S);
                    a0 = (gainLinear + 1.0) - (gainLinear - 1.0) * cosW0 + S;
                    a1_temp = 2.0 * ((gainLinear - 1.0) - (gainLinear + 1.0) * cosW0);
                    a2 = (gainLinear + 1.0) - (gainLinear - 1.0) * cosW0 - S;
                    break;
                }

                case FilterType::Bell:
                {
                    const double A = std::sqrt(gainLinear);
                    const double S = 2.0 * alpha * A;
                    b0 = 1.0 + alpha * A;
                    b1 = -2.0 * cosW0;
                    b2 = 1.0 - alpha * A;
                    a0 = 1.0 + alpha / A;
                    a1_temp = -2.0 * cosW0;
                    a2 = 1.0 - alpha / A;
                    break;
                }
            }

            // Normalize
            b0 /= a0;
            b1 /= a0;
            b2 /= a0;
            a1 = a1_temp / a0;
            a2 = a2 / a0;
        }

        // Filter coefficients
        double b0 = 0.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;

        // State variables (Direct Form II)
        double z1 = 0.0, z2 = 0.0;

        // Parameters
        double frequency = 1000.0;
        double gain = 0.0;
        double qFactor = 1.0;
        double sampleRate = 44100.0;
    };
}
