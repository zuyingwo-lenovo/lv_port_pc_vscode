# Voice Assistant Agent Integration Plan

This plan outlines the steps to integrate a wake word, VAD, Hailo AI pipeline (Whisper + LLM), and TTS into the `lv_port_pc_vscode` LVGL simulator project.

## User Review Required

> [!WARNING]
> Since the `lv_port_pc_vscode` project is primarily written in C, but the Hailo SDK is C++, I will create a C++ wrapper (`agent_pipeline.cpp`) exposing a C API to integrate seamlessly without renaming all your C files. Please confirm this is acceptable.

> [!IMPORTANT]
> **Wake Word Detection Strategy**: Since we don't have a dedicated low-power wake word model (like Porcupine), I propose using the RMS amplitude (already computed in [audio_sim.c](file:///home/andy/ws/voicewave-ui/lv_port_pc_vscode/src/audio_sim.c)) for basic Voice Activity Detection (VAD). Upon detecting speech, I will capture a short chunk and run it through **Hailo Whisper** to check if the transcript contains the wake word ("hey thinkpad"). Please confirm if this approach is fine, or if you prefer a different wake word mechanism.

> [!IMPORTANT]
> **Text-to-Speech (TTS)**: I plan to use a system command like `espeak` for TTS playback as a quick and reliable stub, blocking or running asynchronously until speech finishes. Do you have a specific TTS engine you want me to use instead?

## Proposed Changes

### Build Configuration

#### [MODIFY] [CMakeLists.txt](file:///home/andy/ws/voicewave-ui/lv_port_pc_vscode/CMakeLists.txt)
- Add `find_library` and `find_path` for `hailort`.
- Target link `hailort` to the [main](file:///home/andy/ws/voicewave-ui/lv_port_pc_vscode/src/freertos_main.c#159-189) executable.
- Add the new C++ source files (`agent_pipeline.cpp`) to `MAIN_SOURCES`.
- Ensure C++17 linking is enabled for the new Hailo objects.

---

### Audio and Agent Logic

#### [MODIFY] [audio_sim.c](file:///home/andy/ws/voicewave-ui/lv_port_pc_vscode/src/audio_sim.c)
- Implement a circular buffer to continuously store the last N seconds of audio.
- Enhance the RMS calculation to act as Voice Activity Detection (VAD).
  - Include logic to detect "end of speech" (e.g., RMS below a threshold for > 1 second).
- Expose functions to allow the agent to fetch audio chunks when speech is detected.

#### [NEW] [agent_pipeline.h](file:///home/andy/ws/voicewave-ui/lv_port_pc_vscode/src/agent_pipeline.h)
- Declare C-compatible interface for the agent (`extern "C"`).
- Define Agent States: `AGENT_STATE_WAITING`, `AGENT_STATE_WAKE_WORD_CHECK`, `AGENT_STATE_LISTENING`, `AGENT_STATE_PROCESSING`, `AGENT_STATE_SPEAKING`.
- Callbacks for UI updates (e.g., informing UI of state changes and transcribed text).

#### [NEW] [agent_pipeline.cpp](file:///home/andy/ws/voicewave-ui/lv_port_pc_vscode/src/agent_pipeline.cpp)
- **Initialization**: Create shared `VDevice`, load `Speech2Text` (Whisper), and load `LLM` models similar to `voice_assistant_cpp`.
- **Background Thread**: 
  1. Poll [audio_sim.c](file:///home/andy/ws/voicewave-ui/lv_port_pc_vscode/src/audio_sim.c) for new speech segments.
  2. If looking for wake word: Send to Whisper, check for "thinkpad".
  3. If wake word matched: transition to `LISTENING`.
  4. Once [audio_sim](file:///home/andy/ws/voicewave-ui/lv_port_pc_vscode/src/audio_sim.c#102-111) indicates 1s of silence: capture full audio, send to Whisper.
  5. Feed transcript to Qwen LLM. Get response.
  6. Send response to TTS.
- Dispatch state updates back to the UI safely via LVGL messaging or custom callbacks.

---

### User Interface

#### [MODIFY] [ui.c](file:///home/andy/ws/voicewave-ui/lv_port_pc_vscode/src/ui/ui.c) (and internal UI components)
- Add text labels to display the current state (e.g., "Waiting for wake word...", "Listening...", "User: [text]", "Agent: [text]").
- Export functions allowing `agent_pipeline.cpp` to update these text labels in a thread-safe manner (using `lv_async_call` or `lv_mutex` blocks if needed).
- Bind the wave component's activity to the Agent's state (e.g., only show big waves while listening).

## Verification Plan

### Automated Tests
- Running `cmake .. && make` to ensure the project builds with HailoRT and C++ libraries without linker errors.

### Manual Verification
1. Run [run.sh](file:///home/andy/ws/voicewave-ui/run.sh) or `./bin/main`.
2. Speak to the PC microphone: "Hey Thinkpad". The UI should change to "Listening".
3. Speak a prompt (e.g., "What is the capital of France?"). Then pause for > 1 second.
4. The UI should transition to "Processing", display the transcribed text, and then stream the LLM response text over the screen while using TTS to play back the audio.
