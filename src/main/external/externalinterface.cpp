// Error reporting
#include <iostream>

// SDL Library
#include <SDL.h>
#ifndef SDL2
#pragma comment(lib, "SDLmain.lib") // Replace main with SDL_main
#endif
#pragma comment(lib, "SDL.lib")
#pragma comment(lib, "glu32.lib")

// SDL Specific Code
#if defined SDL2
#include "../sdl2/timer.hpp"
#else
#include "../sdl/timer.hpp"
#endif

#include "../sharedresources.hpp"
#include "externalinterface.hpp"
#include "../video.hpp"

#include "../romloader.hpp"
#include "../trackloader.hpp"
#include "../stdint.hpp"
#include "../setup.hpp"
#include "../engine/outrun.hpp"
#include "../frontend/config.hpp"

#include "../engine/oinputs.hpp"
#include "../engine/ooutputs.hpp"
#include "../engine/omusic.hpp"
#include "../engine/ostats.hpp"
#include "../engine/oinitengine.hpp"

// Initialize Shared Variables
using namespace cannonball;

static void quit_func(int code)
{
#ifdef COMPILE_SOUND_CODE
    audio.stop_audio();
#endif
    input.close();
    SDL_Quit();
    exit(code);
}


ExternalInterface external_interface;

ExternalInterface::ExternalInterface() {
}

ExternalInterface::~ExternalInterface() {

}

void ExternalInterface::init() {
    // Initialize timer and video systems
    if( SDL_Init( SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) == -1 )
    {
        std::cerr << "SDL Initialization Failed: " << SDL_GetError() << std::endl;
        return;
    }

    bool loaded = false;

    loaded = roms.load_revb_roms();

    if (loaded)
    {
        // Load XML Config
        config.load(FILENAME_CONFIG);

        // Load fixed PCM ROM based on config
        if (config.sound.fix_samples)
            roms.load_pcm_rom(true);

        // Load patched widescreen tilemaps
        if (!omusic.load_widescreen_map())
            std::cout << "Unable to load widescreen tilemaps" << std::endl;

#ifndef SDL2
        //Set the window caption
        SDL_WM_SetCaption( "Cannonball", NULL );
#endif

        // Initialize SDL Video
        if (!video.init(&roms, &config.video))
            quit_func(1);

#ifdef COMPILE_SOUND_CODE
        audio.init();
#endif

        oinputs.init();
        outrun.outputs->init();

        // Initalize controls
        input.init(config.controls.pad_id,
                   config.controls.keyconfig, config.controls.padconfig,
                   config.controls.analog,    config.controls.axis, config.controls.asettings);

        // FPS Counter (If Enabled)
        Timer fps_count;
        int frame = 0;
        fps_count.start();

        // General Frame Timing
        Timer frame_time;
        int t;
        double deltatime  = 0;
        int deltaintegral = 0;

        outrun.init();
        state = STATE_GAME;
    }
    else
    {
        quit_func(1);
    }
}

void ExternalInterface::reset() {
    return outrun.reset();
}

void ExternalInterface::tick(ExternalInput external_input) {
    // FPS Counter (If Enabled)
    Timer fps_count;
    int frame = 0;
    fps_count.start();

    // General Frame Timing
    Timer frame_time;
    int t;
    double deltatime  = 0;
    int deltaintegral = 0;

    frame_time.start();

    frame++;

    // Non standard FPS.
    // Determine whether to tick the current frame.
    if (config.fps != 30)
    {
        if (config.fps == 60)
            tick_frame = frame & 1;
        else if (config.fps == 120)
            tick_frame = (frame & 3) == 1;
    }

    input.handle_external_input(external_input);

    if (tick_frame)
        oinputs.tick(NULL); // Do Controls

#ifdef COMPILE_SOUND_CODE
    // Tick audio program code
    osoundint.tick();
        // Tick SDL Audio
        audio.tick();
    #endif
    outrun.tick(NULL, tick_frame);
    input.frame_done();

    // Draw SDL Video
    video.draw_frame();


#ifdef COMPILE_SOUND_CODE
    deltatime += (frame_ms * audio.adjust_speed());
#else
    deltatime += frame_ms;
#endif
    deltaintegral  = (int) deltatime;
    t = frame_time.get_ticks();

    // Cap Frame Rate: Sleep Remaining Frame Time
    if (!config.video.fps_cap_disable && t < deltatime)
    {
        SDL_Delay((Uint32) (deltatime - t));
    }

    deltatime -= deltaintegral;

    if (config.video.fps_count)
    {
        frame++;
        // One second has elapsed
        if (fps_count.get_ticks() >= 1000)
        {
            fps_counter = frame;
            frame       = 0;
            fps_count.start();
        }
    }
}

uint32_t* ExternalInterface::get_pixels_rgb()  {
    return video.get_pixels_rgb();
}

uint32_t* ExternalInterface::get_pixels_greyscale()  {
    return video.get_pixels_greyscale();
}

int ExternalInterface::get_screen_width() {
    return video.get_pixel_buffer_frame_width();
}

int ExternalInterface::get_screen_height() {
    return video.get_pixel_buffer_frame_height();
}

uint32_t ExternalInterface::get_score() {
    return ostats.get_score();
}

uint32_t ExternalInterface::get_speed() {
    return oinitengine.car_increment;
}

bool ExternalInterface::is_game_over() {
    return outrun.is_game_over();
}

void ExternalInterface::destroy() {
    quit_func(0);
}


