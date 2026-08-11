#pragma once

#include "foldvandal/FoldVandalDspPrimitives.h"

namespace foldvandal
{

/** Realtime-safe parameter set for the FoldVandal stereo effect. */
struct FoldVandalParameters
{
    float drive = 0.48f;
    float bias = 0.0f;
    float fold = 0.62f;
    float symmetry = 0.38f;
    float tone = 0.56f;
    float mix = 1.0f;
    float outputGain = 0.82f;
};

/** Stereo asymmetric multi-fold wavefolder with bounded realtime-safe output. */
class FoldVandalEngine
{
public:
    FoldVandalEngine();

    /** Sets the sample rate and rebuilds coefficients; invalid rates fall back to 44.1 kHz. */
    void prepare (double sampleRate) noexcept;

    /** Clears hold, filter, and deterministic state. */
    void reset() noexcept;

    /** Clamps and applies all public parameters. */
    void setParameters (const FoldVandalParameters& parameters) noexcept;

    /** Processes one stereo input frame and returns finite output bounded to +/-0.98. */
    [[nodiscard]] StereoFrame processSample (float inputLeft, float inputRight) noexcept;

    /** Processes stereo buffers in-place. Null buffers and non-positive sizes are ignored. */
    void process (float* left, float* right, int numSamples) noexcept;

private:
    struct ClampedParameters
    {
        float drive = 0.48f;
        float bias = 0.0f;
        float fold = 0.62f;
        float symmetry = 0.38f;
        float tone = 0.56f;
        float mix = 1.0f;
        float outputGain = 0.82f;
    };

    void updateToneFilters() noexcept;
    [[nodiscard]] float processChannel (float input, float& previousInput, float& smoothState, Biquad& tone, DcBlocker& dc) noexcept;
    [[nodiscard]] float foldSample (float input) const noexcept;
    [[nodiscard]] StereoFrame sanitizeFrame (float left, float right) const noexcept;

    ClampedParameters params;
    double sampleRate = 44100.0;
    float previousLeft = 0.0f;
    float previousRight = 0.0f;
    float smoothLeft = 0.0f;
    float smoothRight = 0.0f;
    Biquad toneLeft;
    Biquad toneRight;
    DcBlocker dcLeft;
    DcBlocker dcRight;
};

} // namespace foldvandal
