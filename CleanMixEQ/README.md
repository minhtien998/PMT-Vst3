# CleanMix EQ — VST3 starter project

A native JUCE VST3 parametric EQ starter for Windows 11 / FL Studio.

## Included
- 8 parametric peak bands (frequency, gain, Q, bypass)
- Output gain
- Real-time post-EQ spectrum display
- A heuristic low-mid resonance/mud candidate detector (180–500 Hz)
- Parameter state save/restore

## Important limitations
This is a functional starter implementation, not a finished commercial EQ and not a clone of FabFilter Pro-Q 4. The current detector is a simple spectral-peak heuristic. It cannot reliably decide that a frequency is "mud" in every mix: source material, arrangement, monitoring, masking and playback level matter. Always audition suggested cuts.
The current eight bands are peak filters. Dedicated HP/LP and shelf filter modes, pre/post overlay, draggable EQ nodes, dynamic EQ, mid/side processing, oversampling and a polished spectrum calibration are not implemented yet.

## Build on Windows 11
1. Install Visual Studio 2022 with **Desktop development with C++**.
2. Install CMake.
3. Download JUCE 8 from https://juce.com/ and build/install its CMake package.
4. Open a Developer PowerShell and run from this folder:

   `cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/JUCE"`

   Replace `C:/JUCE` with the folder containing JUCE's `lib/cmake/JUCE` package.
5. Build:

   `cmake --build build --config Release`

6. The VST3 will be generated under `build/CleanMixEQ_artefacts/Release/VST3/`.
7. In FL Studio, open **Options > Manage plugins**, add the VST3 folder if needed, then scan.

## Notes
- This source project requires JUCE; JUCE is not bundled in this ZIP.
- This environment has not compiled or tested the plugin. Treat it as source code, not a ready-to-install binary.
- Test first on a copy of a project and keep plugin output gain at a safe level.
