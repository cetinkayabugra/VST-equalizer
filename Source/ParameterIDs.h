#pragma once

#include <JuceHeader.h>

//==============================================================================
// Number of EQ bands
static constexpr int NUM_BANDS = 6;

//==============================================================================
/** Filter type for each EQ band. */
enum class FilterType : int
{
    HighPass  = 0,  ///< High-pass filter (no gain)
    LowShelf  = 1,  ///< Low shelf boost/cut
    Bell      = 2,  ///< Peak / bell filter
    HighShelf = 3,  ///< High shelf boost/cut
    LowPass   = 4   ///< Low-pass filter (no gain)
};

//==============================================================================
/** Default band configuration matching the spec's example parameter set. */
struct BandDefaults
{
    FilterType  type;
    float       freqHz;
    float       gainDb;
    float       q;
    bool        enabled;
};

static constexpr BandDefaults BAND_DEFAULTS[NUM_BANDS] =
{
    { FilterType::HighPass,  40.0f,     0.0f,  0.71f, true },
    { FilterType::LowShelf,  120.0f,    2.5f,  0.90f, true },
    { FilterType::Bell,      500.0f,   -3.0f,  1.40f, true },
    { FilterType::Bell,      2500.0f,   1.8f,  1.10f, true },
    { FilterType::HighShelf, 10000.0f,  3.5f,  0.80f, true },
    { FilterType::LowPass,   20000.0f,  0.0f,  0.71f, true }
};

static const juce::StringArray FILTER_TYPE_NAMES
{
    "High Pass",
    "Low Shelf",
    "Bell",
    "High Shelf",
    "Low Pass"
};

//==============================================================================
/** Centralised parameter ID strings. */
namespace ParamIDs
{
    static const juce::String bypass      = "bypass";
    static const juce::String outputGain  = "output_gain";

    static inline juce::String bandEnabled (int i) { return "band" + juce::String(i) + "_enabled"; }
    static inline juce::String bandType    (int i) { return "band" + juce::String(i) + "_type";    }
    static inline juce::String bandFreq    (int i) { return "band" + juce::String(i) + "_freq";    }
    static inline juce::String bandGain    (int i) { return "band" + juce::String(i) + "_gain";    }
    static inline juce::String bandQ       (int i) { return "band" + juce::String(i) + "_q";       }
}
