#pragma once

#include "BiQuadFilter.h"
#include <array>

namespace DSP
{
    /**
     * FilterBank - Manages multiple EQ bands
     * Implements a 5-band parametric equalizer
     */
    class FilterBank
    {
    public:
        static constexpr int NUM_BANDS = 5;

        FilterBank() = default;
        ~FilterBank() = default;

        /**
         * Initialize filter bank with sample rate
         */
        void prepare(double sampleRate)
        {
            this->sampleRate = sampleRate;
            for (auto& filter : filters)
            {
                filter.reset();
            }
        }

        /**
         * Set parameters for a specific band
         */
        void setBandParameters(int band, BiQuadFilter::FilterType type, 
                              double frequency, double gain, double qFactor)
        {
            if (band >= 0 && band < NUM_BANDS)
            {
                filters[band].setParameters(type, frequency, gain, qFactor, sampleRate);
            }
        }

        /**
         * Enable or disable a specific band
         */
        void setBandEnabled(int band, bool enabled)
        {
            if (band >= 0 && band < NUM_BANDS)
            {
                bandEnabled[band] = enabled;
            }
        }

        /**
         * Get combined frequency response at a specific frequency
         */
        [[nodiscard]] double getFrequencyResponse(double frequency) const noexcept
        {
            double response = 1.0;
            for (int i = 0; i < NUM_BANDS; ++i)
            {
                if (bandEnabled[i])
                {
                    response *= filters[i].getMagnitudeResponse(frequency);
                }
            }
            return response;
        }

        /**
         * Process audio through all filters in series
         */
        void processBlock(juce::AudioBuffer<float>& buffer)
        {
            const int numChannels = buffer.getNumChannels();
            
            for (int band = 0; band < NUM_BANDS; ++band)
            {
                if (!bandEnabled[band])
                    continue;

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    filters[band].processBlock(buffer, ch);
                }
            }
        }

        /**
         * Reset all filters
         */
        void reset()
        {
            for (auto& filter : filters)
            {
                filter.reset();
            }
        }

        /**
         * Get reference to a specific filter (for direct manipulation)
         */
        BiQuadFilter& getFilter(int band)
        {
            jassert(band >= 0 && band < NUM_BANDS);
            return filters[band];
        }

    private:
        std::array<BiQuadFilter, NUM_BANDS> filters;
        std::array<bool, NUM_BANDS> bandEnabled{{true, true, true, true, true}};
        double sampleRate = 44100.0;
    };
}
