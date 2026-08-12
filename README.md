# FoldVandal

FoldVandal is a YUP-based stereo asymmetric multi-fold wavefolder. Drive, bias, fold depth, and symmetry feed a 4x interpolated folding core with smoothing, DC blocking, tone filtering, dry/wet blend, and bounded output. It builds from this project directory, using the adjacent `../yup` checkout when present.

## Identity

- App ID: `jp.ehl.foldvandal`
- Plugin ID: `jp.ehl.foldvandal`
- AU subtype: `FdVn`
- Vendor: `ehl_`; AU manufacturer: `EHL1`
- Version: `0.1.0`
- Type: stereo input/output effect, no MIDI
- macOS formats: Standalone, VST3, AUv2
- Windows formats: Standalone, VST3

## Parameters

- `Drive`: pre-folder gain into the asymmetric folding rails.
- `Bias`: signed DC offset before folding; removed after shaping by the DC blocker.
- `Fold`: folding depth and rail tightness.
- `Symmetry`: skew between positive and negative fold rails.
- `Tone`: dark-to-open post-folder low-pass filter; maximum Tone bypasses filtering.
- `Mix`: dry/wet blend.
- `Output`: final linear gain before the bounded safety stage.

## Research basis

The implementation survey used Aalto University's work on [aliasing-aware nonlinear audio processing](https://aaltodoc.aalto.fi/items/470aab15-1702-4ccf-a148-24e6173079fb) and [antiderivative antialiasing for memoryless nonlinearities](https://aaltodoc.aalto.fi/items/cdd45f4e-38a0-49ec-8d2f-7ee0ca7576d5). FoldVandal uses a deliberately small fixed interpolation factor, asymmetric fold rails, smoothing, and a DC blocker as bounded product choices; it does not claim to reproduce a particular analog circuit.

## Standalone Audition

Standalone builds compile a small audition source behind `YUP_AUDIO_PLUGIN_ENABLE_STANDALONE`. The audition enable and type controls are runtime/UI state only: they are not host parameters and are not serialized. VST3/AU builds compile the no-generator branch, keep the signal path strictly input -> effect -> output, and preserve hosted silence.

The standalone editor shows input/output meters and audition controls. If the YUP standalone macro is unavailable, the editor fails closed as a plain parameter grid with no audition path.

## Build

Clone with `--recurse-submodules`, or initialize the shared [yup-ehl-design-module](https://github.com/EsionHsrahLatigid/yup-ehl-design-module) before configuring:

```sh
git submodule update --init
```

```sh
cmake --preset engine-debug
cmake --build --preset engine-debug
ctest --preset engine-debug
```

```sh
cmake --preset plugin-release
cmake --build --preset plugin-release
ctest --preset plugin-release
```

Release bundles are staged under the stable `artifacts/plugin-release/<platform-arch>/` tree. `build/` is CMake's internal workspace:

- `foldvandal_release_bundles`
- `foldvandal_standalone_plugin`
- `foldvandal_vst3_plugin`
- `foldvandal_au_plugin` on Apple platforms

On macOS, the local bundle paths are:

- `artifacts/plugin-release/macos-arm64/standalone/foldvandal_standalone_plugin.app`
- `artifacts/plugin-release/macos-arm64/vst3/foldvandal_vst3_plugin.vst3`
- `artifacts/plugin-release/macos-arm64/au/foldvandal_au_plugin.component`

Windows uses `artifacts/plugin-release/windows-x64/` with `standalone/` and `vst3/` directories.

## CI

`.github/workflows/ci.yml` is the required CI entrypoint for pushes to `main`, pull requests, and manual runs. A lightweight Linux classifier always runs. Changes limited to `README.md`, `DESIGN.md`, `LICENSE`, `docs/**`, or `.github/ISSUE_TEMPLATE/**` skip the heavy jobs; every other change runs Debug tests and Release bundle builds on macOS arm64 and Windows x64. Manual dispatches default to forcing both heavy jobs.

Successful heavy runs upload two immutable, 14-day artifacts:

- `FoldVandal-latest-macos-arm64`, containing `FoldVandal-latest-macos-arm64.zip` and `SHA256SUMS.txt`
- `FoldVandal-latest-windows-x64`, containing `FoldVandal-latest-windows-x64.zip` and `SHA256SUMS.txt`

`.github/workflows/release.yml` is the only `v*` tag workflow. It performs no compilation. The Ubuntu release job resolves lightweight or annotated tags to a commit, requires the tag version to match the CMake project version, requires one successful `CI` push run on `main` for that exact SHA, downloads exactly the two expected artifacts, verifies their SHA-256 manifests and ZIP integrity, then publishes versioned assets such as `FoldVandal-0.1.0-macos-arm64.zip` and `FoldVandal-0.1.0-windows-x64.zip`. Publication uses a draft release whose asset list is sanitized and rechecked to contain exactly those two ZIPs. A missing, expired, ambiguous, or mismatched provenance chain fails closed.

Release operator sequence: merge or push the version commit to `main`, wait for both platform jobs and `CI Summary` to pass, then create and push the version tag. GitHub CLI 2.x or newer is required by the release runner. Never move or reuse a published tag; correct the source and use the next patch version instead.

## Layout

- `include/foldvandal/` contains the realtime-safe DSP engine API and local DSP primitives.
- `source/` contains the engine implementation and YUP plugin/editor/state wrapper.
- `tests/` contains deterministic engine regression tests plus hosted and standalone plugin-wrapper bridge tests.
- `cmake/` contains the project-local macOS icon conversion workaround used by the standalone target.
