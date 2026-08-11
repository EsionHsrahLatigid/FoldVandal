#include "foldvandal/FoldVandalEngine.h"

#include <algorithm>
#include <cmath>

namespace foldvandal
{
namespace
{
constexpr float ceiling = 0.98f;

[[nodiscard]] float sanitizeAudio (float value) noexcept
{
    return clampFinite (value, -8.0f, 8.0f, 0.0f);
}
} // namespace

FoldVandalEngine::FoldVandalEngine()
{
    prepare (44100.0);
    reset();
}

void FoldVandalEngine::prepare (double newSampleRate) noexcept
{
    sampleRate = std::isfinite (newSampleRate) && newSampleRate > 1.0 ? newSampleRate : 44100.0;
    updateToneFilters();
    dcLeft.prepare (sampleRate, 7.0f);
    dcRight.prepare (sampleRate, 7.0f);
    reset();
}

void FoldVandalEngine::reset() noexcept
{
    previousLeft = 0.0f;
    previousRight = 0.0f;
    smoothLeft = 0.0f;
    smoothRight = 0.0f;
    toneLeft.reset();
    toneRight.reset();
    dcLeft.reset();
    dcRight.reset();
}

void FoldVandalEngine::setParameters (const FoldVandalParameters& parameters) noexcept
{
    params.drive = clampFinite (parameters.drive, 0.0f, 1.0f, FoldVandalParameters {}.drive);
    params.bias = clampFinite (parameters.bias, -1.0f, 1.0f, FoldVandalParameters {}.bias);
    params.fold = clampFinite (parameters.fold, 0.0f, 1.0f, FoldVandalParameters {}.fold);
    params.symmetry = clampFinite (parameters.symmetry, 0.0f, 1.0f, FoldVandalParameters {}.symmetry);
    params.tone = clampFinite (parameters.tone, 0.0f, 1.0f, FoldVandalParameters {}.tone);
    params.mix = clampFinite (parameters.mix, 0.0f, 1.0f, FoldVandalParameters {}.mix);
    params.outputGain = clampFinite (parameters.outputGain, 0.0f, 2.0f, FoldVandalParameters {}.outputGain);

    updateToneFilters();
}

StereoFrame FoldVandalEngine::processSample (float inputLeft, float inputRight) noexcept
{
    const auto dryLeft = sanitizeAudio (inputLeft);
    const auto dryRight = sanitizeAudio (inputRight);
    const auto wetLeft = processChannel (dryLeft, previousLeft, smoothLeft, toneLeft, dcLeft);
    const auto wetRight = processChannel (dryRight, previousRight, smoothRight, toneRight, dcRight);

    const auto dry = 1.0f - params.mix;
    return sanitizeFrame ((dryLeft * dry + wetLeft * params.mix) * params.outputGain,
                          (dryRight * dry + wetRight * params.mix) * params.outputGain);
}

void FoldVandalEngine::process (float* left, float* right, int numSamples) noexcept
{
    if (left == nullptr || right == nullptr || numSamples <= 0)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto frame = processSample (left[i], right[i]);
        left[i] = frame.left;
        right[i] = frame.right;
    }
}

void FoldVandalEngine::updateToneFilters() noexcept
{
    const auto cutoff = 520.0f * std::pow (30.0f, params.tone);
    const auto harshness = 0.7f + params.fold * 0.45f;
    toneLeft.setLowPass (sampleRate, cutoff, harshness);
    toneRight.setLowPass (sampleRate, cutoff * (0.99f + 0.04f * params.symmetry), harshness);
}

float FoldVandalEngine::processChannel (float input, float& previousInput, float& smoothState, Biquad& tone, DcBlocker& dc) noexcept
{
    const auto gain = 1.0f + params.drive * params.drive * 15.0f;
    const auto bias = params.bias * (0.04f + params.fold * 0.44f);
    float folded = 0.0f;
    for (int i = 0; i < 4; ++i)
    {
        const auto alpha = static_cast<float> (i + 1) * 0.25f;
        const auto interpolated = previousInput + (input - previousInput) * alpha;
        folded += foldSample (interpolated * gain + bias);
    }
    previousInput = input;

    const auto target = folded * 0.25f;
    const auto smoothing = 0.08f + 0.18f * (1.0f - params.fold);
    smoothState += smoothing * (target - smoothState);
    const auto toned = params.tone >= 0.999f ? smoothState : tone.process (smoothState);
    return sanitizeAudio (dc.process (toned));
}

float FoldVandalEngine::foldSample (float input) const noexcept
{
    auto value = sanitizeAudio (input);
    const auto skew = (params.symmetry - 0.5f) * 0.72f;
    const auto positive = 0.22f + (1.0f - params.fold) * 0.72f + skew * 0.28f;
    const auto negative = 0.22f + (1.0f - params.fold) * 0.72f - skew * 0.28f;
    const auto positiveLimit = std::max (0.08f, positive);
    const auto negativeLimit = std::max (0.08f, negative);

    for (int i = 0; i < 12; ++i)
    {
        if (value > positiveLimit)
            value = positiveLimit - (value - positiveLimit) * (0.84f + params.symmetry * 0.22f);
        else if (value < -negativeLimit)
            value = -negativeLimit - (value + negativeLimit) * (1.06f - params.symmetry * 0.22f);
        else
            break;
    }

    const auto soft = std::tanh (value * (1.15f + params.fold * 1.4f));
    return sanitizeAudio (soft);
}

StereoFrame FoldVandalEngine::sanitizeFrame (float left, float right) const noexcept
{
    auto safeLeft = boundedDrive (left, 1.04f + params.drive * 0.7f);
    auto safeRight = boundedDrive (right, 1.04f + params.drive * 0.7f);
    if (std::fabs (safeLeft) < 1.0e-20f)
        safeLeft = 0.0f;
    if (std::fabs (safeRight) < 1.0e-20f)
        safeRight = 0.0f;
    return { std::clamp (safeLeft, -ceiling, ceiling),
             std::clamp (safeRight, -ceiling, ceiling) };
}

} // namespace foldvandal
