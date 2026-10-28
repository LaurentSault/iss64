// ISS64: Recompiled. Sets up the window, renderer, audio and input, registers
// the game with librecomp, and hands control to the recompiled game code.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include "SDL.h"
#ifdef _WIN32
#include <SDL_syswm.h>
#endif

#include "librecomp/game.hpp"
#include "librecomp/overlays.hpp"
#include "librecomp/rsp.hpp"
#include "ultramodern/ultramodern.hpp"
#include "ultramodern/renderer_context.hpp"
#include "ultramodern/input.hpp"
#include "ultramodern/events.hpp"
#include "ultramodern/error_handling.hpp"
#include "ultramodern/threads.hpp"

#include "recompui/recompui.h"
#include "recompui/renderer.h"
#include "recompui/program_config.h"
#include "recompui/config.h"
#include "recompinput/recompinput.h"
#include "recompinput/input_state.h"
#include "recompinput/input_events.h"
#include "recompinput/players.h"
#include "recompinput/profiles.h"

#include <nfd.h>

extern "C" void recomp_entrypoint(uint8_t* rdram, recomp_context* ctx);
gpr get_entrypoint_address();
RspExitReason aspMain(uint8_t* rdram, uint32_t ucode_addr);

namespace iss64 { void register_overlays(); }

// recompui expects the app to define these.
SDL_Window* window = nullptr;
std::vector<recomp::GameEntry> supported_games;

namespace {

constexpr const char* program_name = "ISS64: Recompiled";
// Not const: select_rom and is_rom_valid take a non-const reference.
std::u8string game_id = u8"iss64.n64.us";

ultramodern::gfx_callbacks_t::gfx_data_t create_gfx() {
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    return nullptr;
}

ultramodern::renderer::WindowHandle create_window(ultramodern::gfx_callbacks_t::gfx_data_t) {
#if defined(_WIN32)
    window = SDL_CreateWindow(program_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 960, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_SysWMinfo wm_info;
    SDL_VERSION(&wm_info.version);
    SDL_GetWindowWMInfo(window, &wm_info);
    return ultramodern::renderer::WindowHandle{ wm_info.info.win.window, GetCurrentThreadId() };
#else
    window = SDL_CreateWindow(program_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 960, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    return window;
#endif
}

void update_gfx(ultramodern::gfx_callbacks_t::gfx_data_t) {
    // recompinput owns the SDL event pump and forwards UI events to recompui.
    recompinput::handle_events();
}

std::unique_ptr<ultramodern::renderer::RendererContext> create_render_context(
    uint8_t* rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode) {
    // ISS64 draws some frames with more than one display list (the main menu
    // labels come in a later one), so present on VI like the console does.
    // PresentEarly shows the frame after the first list and the labels flicker.
    return recompui::renderer::create_render_context(rdram, window_handle,
        ultramodern::renderer::PresentationMode::Console, developer_mode);
}

// Graphics tasks go straight to RT64; the only other RSP task the game sends is audio.
RspUcodeFunc* get_rsp_microcode(const OSTask*) {
    return aspMain;
}

ultramodern::input::connected_device_info_t get_connected_device_info(int controller_num) {
    return {
        controller_num == 0 ? ultramodern::input::Device::Controller : ultramodern::input::Device::None,
        ultramodern::input::Pak::None,
    };
}

SDL_AudioDeviceID audio_device = 0;

void set_frequency(uint32_t freq) {
    if (audio_device != 0) {
        SDL_CloseAudioDevice(audio_device);
    }
    SDL_AudioSpec want{};
    want.freq = static_cast<int>(freq);
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 256;
    audio_device = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);
    if (audio_device == 0) {
        fprintf(stderr, "Failed to open audio device: %s\n", SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(audio_device, 0);
}

void queue_samples(int16_t* samples, size_t count) {
    if (audio_device != 0 && count < (1u << 22)) {
        SDL_QueueAudio(audio_device, samples, static_cast<Uint32>(count * sizeof(int16_t)));
    }
}

size_t get_frames_remaining() {
    if (audio_device == 0) {
        return 0;
    }
    // The game sizes each audio frame as (target - osAiGetLength() / 4 + 0x100),
    // assuming the AI never holds more than about one buffer. Reporting the whole
    // SDL queue can make that negative, so cap it like the hardware would.
    constexpr size_t max_frames_remaining = 0x400;
    size_t frames = SDL_GetQueuedAudioSize(audio_device) / (2 * sizeof(int16_t));
    return frames < max_frames_remaining ? frames : max_frames_remaining;
}

std::string get_game_thread_name(const OSThread*) {
    return "iss64_game_thread";
}

void vi_callback() {}

} // namespace

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // recompui resolves assets/ relative to the working directory.
    std::error_code ec;
    std::filesystem::current_path(std::filesystem::path(SDL_GetBasePath()), ec);

    recompui::programconfig::set_program_name(program_name);
    SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "Audio init failed, continuing without sound: %s\n", SDL_GetError());
    }
    NFD_Init();

    recomp::register_config_path(std::filesystem::path(SDL_GetPrefPath("N64Recomp", "ISS64Recompiled")));
    iss64::register_overlays();

    recomp::GameEntry game{};
    game.rom_hash = 0x058D54E9CE94399FULL; // XXH3_64 of the USA [!] ROM
    game.internal_name = "I S S 64";
    game.display_name = "International Superstar Soccer 64";
    game.game_id = game_id;
    game.mod_game_id = "iss64";
    game.save_type = recomp::SaveType::None; // ISS64 saves to the Controller Pak
    game.is_enabled = true;
    game.has_compressed_code = false;
    game.entrypoint_address = get_entrypoint_address();
    game.entrypoint = recomp_entrypoint;
    supported_games.push_back(game);
    recomp::register_game(supported_games.back());

    recompinput::profiles::initialize_input_bindings();
    recompinput::players::set_single_player_mode(true);
    recompinput::playerassignment::add_keyboard_player();
    recompinput::playerassignment::commit_player_assignment();

    // The family must match the name stored inside the font file.
    recompui::register_primary_font("fonts/primary.ttf", "Inter Variable");
    recompui::config::create_general_tab(recompui::config::GeneralTabOptions{});
    recompui::config::create_graphics_tab();
    recompui::config::create_sound_tab();
    recompui::config::create_controls_tab();
    recompui::config::finalize();

    // ISS64_ROM=/path/to/rom stores the ROM and boots straight into the game,
    // skipping the launcher. Useful for development.
    if (const char* rom_path = std::getenv("ISS64_ROM")) {
        recomp::RomValidationError err = recomp::select_rom(std::filesystem::path(rom_path), game_id);
        if (err != recomp::RomValidationError::Good) {
            fprintf(stderr, "ISS64_ROM was rejected (error %d)\n", static_cast<int>(err));
        }
        recompui::register_launcher_update_callback([](recompui::LauncherMenu*) {
            static bool started = false;
            if (!started && recomp::is_rom_valid(game_id)) {
                started = true;
                recomp::start_game(game_id, {});
                recompui::hide_all_contexts();
            }
        });
    }

    recomp::rsp::callbacks_t rsp_callbacks{ .get_rsp_microcode = get_rsp_microcode };
    ultramodern::renderer::callbacks_t renderer_callbacks{ .create_render_context = create_render_context };
    ultramodern::audio_callbacks_t audio_callbacks{
        .queue_samples = queue_samples,
        .get_frames_remaining = get_frames_remaining,
        .set_frequency = set_frequency,
    };
    ultramodern::input::callbacks_t input_callbacks{
        .poll_input = recompinput::poll_inputs,
        .get_input = recompinput::profiles::get_n64_input,
        .set_rumble = recompinput::set_rumble,
        .get_connected_device_info = get_connected_device_info,
    };
    ultramodern::gfx_callbacks_t gfx_callbacks{
        .create_gfx = create_gfx,
        .create_window = create_window,
        .update_gfx = update_gfx,
    };
    ultramodern::events::callbacks_t events_callbacks{ .vi_callback = vi_callback };
    ultramodern::error_handling::callbacks_t error_callbacks{ .message_box = recompui::message_box };
    ultramodern::threads::callbacks_t threads_callbacks{ .get_game_thread_name = get_game_thread_name };

    recomp::start(recomp::Version{0, 1, 0}, {}, rsp_callbacks, renderer_callbacks, audio_callbacks,
        input_callbacks, gfx_callbacks, events_callbacks, error_callbacks, threads_callbacks);

    NFD_Quit();
    return 0;
}
