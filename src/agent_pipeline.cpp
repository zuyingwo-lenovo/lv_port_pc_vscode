#include "agent_pipeline.h"
#include "audio_sim.h"

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cctype>

#include "hailo/hailort.h"
#include "hailo/vdevice.hpp"
#include "hailo/device.hpp"
#include "hailo/genai/llm/llm.hpp"
#include "hailo/genai/speech2text/speech2text.hpp"

using namespace hailort;
using namespace hailort::genai;

static agent_state_t current_state = AGENT_STATE_WAITING;
static std::string last_user_text = "";
static std::string last_agent_text = "";
static std::mutex agent_mutex;

static std::shared_ptr<VDevice> vdevice;
static std::unique_ptr<Speech2Text> s2t;
static std::unique_ptr<LLM> llm;
static std::thread agent_thread;
static bool agent_running = false;

static std::string whisper_hef_path = "/usr/local/hailo/resources/models/hailo10h/Whisper-Base.hef";
static std::string llm_hef_path = "/usr/local/hailo/resources/models/hailo10h/Qwen2.5-1.5B-Instruct.hef";

// Helper to lowercase string
static std::string to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return result;
}

static void agent_thread_func() {
    while (agent_running) {
        float *audio_data = nullptr;
        size_t audio_size = 0;

        if (audio_sim_get_speech(&audio_data, &audio_size)) {
            // Speech chunk received!
            {
                std::lock_guard<std::mutex> lock(agent_mutex);
                current_state = AGENT_STATE_PROCESSING;
                last_user_text = "(Transcribing...)";
            }

            // 1. Transcribe with Whisper
            MemoryView audio_view(audio_data, audio_size * sizeof(float));
            auto transcribe_exp = s2t->generate_all_text(audio_view);
            std::string transcribed_text = "";
            if (transcribe_exp) {
                transcribed_text = transcribe_exp.release();
                std::cout << "[Agent] Whisper Transcription: '" << transcribed_text << "'" << std::endl;
            } else {
                std::cerr << "[Agent] Whisper error: " << transcribe_exp.status() << std::endl;
            }
            free(audio_data); // free memory from audio_sim

            std::string lower_text = to_lower(transcribed_text);
            
            // 2. Check wake word
            if (lower_text.find("hey") != std::string::npos || lower_text.find("hey thinkpad") != std::string::npos || lower_text.find("think pad") != std::string::npos || lower_text.find("pink") != std::string::npos) {
                std::cout << "[Agent] Wake word detected!" << std::endl;
                // Wake word detected! Extract the rest of the sentence.
                // For simplicity, we just pass the transcribed text to the LLM directly, 
                // the LLM can handle a prompt starting with "hey thinkpad"
                {
                    std::lock_guard<std::mutex> lock(agent_mutex);
                    last_user_text = transcribed_text;
                    last_agent_text = "(Thinking...)";
                }

                // 3. LLM Generation
                std::cout << "[Agent] Prompting LLM..." << std::endl;
                std::string prompt = "You are a helpful voice assistant named Thinkpad. Keep replies short. User says: " + transcribed_text;
                auto generator_exp = llm->create_generator();
                if (generator_exp) {
                    auto generator = generator_exp.release();
                    if (generator.write(prompt) == HAILO_SUCCESS) {
                        auto completion_exp = generator.generate();
                        if (completion_exp) {
                            auto completion = completion_exp.release();
                            std::string full_response = "";
                            int token_count = 0;
                            while (completion.generation_status() == LLMGeneratorCompletion::Status::GENERATING) {
                                auto token_exp = completion.read(std::chrono::milliseconds(2000));
                                if (token_exp) {
                                    std::string token = token_exp.release();
                                    full_response += token;
                                    {
                                        std::lock_guard<std::mutex> lock(agent_mutex);
                                        last_agent_text = full_response;
                                    }
                                    if (++token_count > 100) break; // Limit length for speed
                                }
                            }
                            
                            std::cout << "\n[Agent] LLM Generation complete. Sending to TTS..." << std::endl;
                            // 4. TTS Playback
                            {
                                std::lock_guard<std::mutex> lock(agent_mutex);
                                current_state = AGENT_STATE_SPEAKING;
                            }
                            // Escape quotes for espeak
                            std::string safe_response = full_response;
                            size_t pos = 0;
                            while ((pos = safe_response.find("\"", pos)) != std::string::npos) {
                                safe_response.replace(pos, 1, "\\\"");
                                pos += 2;
                            }
                            // Use espeak synchronously
                            std::string cmd = "espeak \"" + safe_response + "\"";
                            std::cout << "[Agent] Executing TTS: " << cmd << std::endl;
                            system(cmd.c_str());
                            std::cout << "[Agent] TTS finished." << std::endl;
                        } else {
                            std::cerr << "[Agent] Failed to generate: " << completion_exp.status() << std::endl;
                        }
                    } else {
                        std::cerr << "[Agent] Failed to write to generator" << std::endl;
                    }
                } else {
                    std::cerr << "[Agent] Failed to create generator: " << generator_exp.status() << std::endl;
                }
            } else {
                std::cout << "[Agent] Ignored audio (no wake word)." << std::endl;
            }
            
            // Return to waiting
            {
                std::lock_guard<std::mutex> lock(agent_mutex);
                current_state = AGENT_STATE_WAITING;
                last_user_text = "";
                last_agent_text = "";
            }
        } else {
            // Check if UI needs to update to LISTENING state when VAD is active
            if (audio_sim_is_listening()) {
                std::lock_guard<std::mutex> lock(agent_mutex);
                if (current_state == AGENT_STATE_WAITING) {
                    current_state = AGENT_STATE_LISTENING;
                    last_user_text = "";
                }
            } else {
                std::lock_guard<std::mutex> lock(agent_mutex);
                if (current_state == AGENT_STATE_LISTENING) {
                    current_state = AGENT_STATE_WAITING;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

static void agent_thread_wrapper() {
    std::cout << "[Agent Thread] Initializing Hailo VDevice..." << std::endl;
    hailo_vdevice_params_t params;
    auto status = hailo_init_vdevice_params(&params);
    if (status != HAILO_SUCCESS) {
        std::cerr << "[Agent Thread] Failed to init VDevice params" << std::endl;
        return;
    }
    params.group_id = "SHARED";

    auto vdevice_shared_exp = VDevice::create_shared(params);
    if (!vdevice_shared_exp) {
        std::cerr << "[Agent Thread] Failed to create shared VDevice!" << std::endl;
        return;
    }
    
    vdevice = vdevice_shared_exp.release();
    
    std::cout << "[Agent Thread] Loading Speech2Text model from: " << whisper_hef_path << std::endl;
    Speech2TextParams s2t_params(whisper_hef_path);
    auto s2t_exp = Speech2Text::create(vdevice, s2t_params);
    if (s2t_exp) {
        s2t = std::make_unique<Speech2Text>(s2t_exp.release());
        std::cout << "[Agent Thread] S2T model loaded successfully." << std::endl;
    } else {
        std::cerr << "[Agent Thread] Failed to load S2T: " << s2t_exp.status() << std::endl;
    }

    std::cout << "[Agent Thread] Loading LLM model from: " << llm_hef_path << std::endl;
    LLMParams llm_params(llm_hef_path);
    auto llm_exp = LLM::create(vdevice, llm_params);
    if (llm_exp) {
        llm = std::make_unique<LLM>(llm_exp.release());
        std::cout << "[Agent Thread] LLM model loaded successfully." << std::endl;
    } else {
        std::cerr << "[Agent Thread] Failed to load LLM: " << llm_exp.status() << std::endl;
    }

    std::cout << "[Agent Thread] Initialization complete! Entering main processing loop..." << std::endl;
    
    // Fall into the main logic loop now that everything is loaded.
    agent_thread_func();
}

extern "C" void agent_pipeline_init(void) {
    std::cout << "[Agent Init] Starting background loading thread..." << std::endl;
    agent_running = true;
    agent_thread = std::thread(agent_thread_wrapper);
}

extern "C" void agent_pipeline_step(void) {
    // Left empty, we use a thread instead of polling here.
}

extern "C" agent_state_t agent_get_state(void) {
    std::lock_guard<std::mutex> lock(agent_mutex);
    return current_state;
}

extern "C" const char* agent_get_last_user_text(void) {
    std::lock_guard<std::mutex> lock(agent_mutex);
    return last_user_text.c_str();
}

extern "C" const char* agent_get_last_agent_text(void) {
    std::lock_guard<std::mutex> lock(agent_mutex);
    return last_agent_text.c_str();
}
