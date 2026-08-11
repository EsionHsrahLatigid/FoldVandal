#include "foldvandal/FoldVandalEngine.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

using foldvandal::FoldVandalEngine;
using foldvandal::FoldVandalParameters;

namespace
{

std::vector<float> renderSine (FoldVandalParameters params, int samples, float amplitude = 0.6f)
{
    FoldVandalEngine engine;
    engine.prepare (48000.0);
    engine.setParameters (params);
    engine.reset();

    std::vector<float> output;
    output.reserve (static_cast<std::size_t> (samples));
    for (int i = 0; i < samples; ++i)
    {
        const auto phase = static_cast<float> (i) * 0.071f;
        output.push_back (engine.processSample (std::sin (phase) * amplitude,
                                                std::sin (phase * 0.93f) * amplitude).left);
    }
    return output;
}

float peak (const std::vector<float>& samples)
{
    float result = 0.0f;
    for (const auto sample : samples)
        result = std::max (result, std::fabs (sample));
    return result;
}

float averageStep (const std::vector<float>& samples)
{
    float total = 0.0f;
    for (std::size_t i = 1; i < samples.size(); ++i)
        total += std::fabs (samples[i] - samples[i - 1]);
    return total / static_cast<float> (std::max<std::size_t> (1, samples.size() - 1));
}

float averageDifference (const std::vector<float>& a, const std::vector<float>& b)
{
    assert (a.size() == b.size());
    float total = 0.0f;
    for (std::size_t i = 0; i < a.size(); ++i)
        total += std::fabs (a[i] - b[i]);
    return total / static_cast<float> (a.size());
}

float mean (const std::vector<float>& samples)
{
    float total = 0.0f;
    for (const auto sample : samples)
        total += sample;
    return total / static_cast<float> (samples.size());
}

void testSilenceStaysSilent()
{
    FoldVandalEngine engine;
    engine.prepare (48000.0);
    engine.reset();

    for (int i = 0; i < 8192; ++i)
    {
        const auto frame = engine.processSample (0.0f, 0.0f);
        assert (std::fabs (frame.left) <= 1.0e-7f);
        assert (std::fabs (frame.right) <= 1.0e-7f);
    }
}

void testFoldAndDriveIncreaseNonlinearMotion()
{
    FoldVandalParameters soft;
    soft.drive = 0.05f;
    soft.fold = 0.0f;
    soft.tone = 1.0f;
    soft.mix = 1.0f;

    auto hard = soft;
    hard.drive = 0.92f;
    hard.fold = 0.95f;

    const auto softOutput = renderSine (soft, 4096);
    const auto hardOutput = renderSine (hard, 4096);

    assert (averageDifference (softOutput, hardOutput) > 0.08f);
    assert (peak (hardOutput) <= 0.9801f);
}

void testBiasAndSymmetryChangeShape()
{
    FoldVandalParameters leftBias;
    leftBias.drive = 0.72f;
    leftBias.fold = 0.82f;
    leftBias.bias = -0.65f;
    leftBias.symmetry = 0.15f;
    leftBias.tone = 0.9f;

    auto rightBias = leftBias;
    rightBias.bias = 0.65f;
    rightBias.symmetry = 0.85f;

    const auto leftOutput = renderSine (leftBias, 4096);
    const auto rightOutput = renderSine (rightBias, 4096);

    assert (std::fabs (mean (leftOutput) - mean (rightOutput)) > 0.005f);
}

void testToneDarkensFoldEdges()
{
    FoldVandalParameters dark;
    dark.drive = 0.85f;
    dark.fold = 0.9f;
    dark.tone = 0.0f;

    auto bright = dark;
    bright.tone = 1.0f;

    const auto darkOutput = renderSine (dark, 4096, 0.9f);
    const auto brightOutput = renderSine (bright, 4096, 0.9f);

    assert (averageStep (brightOutput) > averageStep (darkOutput) * 1.18f);
}

void testDeterministic()
{
    FoldVandalParameters params;
    params.drive = 0.71f;
    params.bias = 0.23f;
    params.fold = 0.84f;
    params.symmetry = 0.29f;

    const auto a = renderSine (params, 4096);
    const auto b = renderSine (params, 4096);
    assert (a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i)
        assert (std::fabs (a[i] - b[i]) <= 1.0e-6f);
}

void testFiniteBoundedExtremeParameters()
{
    FoldVandalParameters params;
    params.drive = 1000.0f;
    params.bias = 1000.0f;
    params.fold = 1000.0f;
    params.symmetry = 1000.0f;
    params.tone = std::numeric_limits<float>::infinity();
    params.mix = 1000.0f;
    params.outputGain = 1000.0f;

    FoldVandalEngine engine;
    engine.prepare (0.0);
    engine.setParameters (params);
    engine.reset();

    for (int i = 0; i < 8192; ++i)
    {
        const auto frame = engine.processSample (1000.0f, -1000.0f);
        assert (std::isfinite (frame.left));
        assert (std::isfinite (frame.right));
        assert (frame.left >= -0.9801f && frame.left <= 0.9801f);
        assert (frame.right >= -0.9801f && frame.right <= 0.9801f);
    }
}

void testDenormalInputDoesNotLeak()
{
    FoldVandalEngine engine;
    engine.prepare (48000.0);
    engine.reset();

    for (int i = 0; i < 1024; ++i)
    {
        const auto frame = engine.processSample (1.0e-30f, -1.0e-30f);
        assert (std::fabs (frame.left) <= 1.0e-7f);
        assert (std::fabs (frame.right) <= 1.0e-7f);
    }
}

} // namespace

int main()
{
    testSilenceStaysSilent();
    testFoldAndDriveIncreaseNonlinearMotion();
    testBiasAndSymmetryChangeShape();
    testToneDarkensFoldEdges();
    testDeterministic();
    testFiniteBoundedExtremeParameters();
    testDenormalInputDoesNotLeak();

    std::cout << "FoldVandalEngineTests passed\n";
    return 0;
}
