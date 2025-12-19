#pragma once

#include <SDL3/SDL.h>


namespace Audio
{
    // Holds data from a sound file
    struct SoundData
    {
        Uint8 *buffer;
        Uint32 length;
        SDL_AudioSpec spec;
    };


    // A single monophonic sound that can be played
    struct Sound
    {
        SoundData data;
        SDL_AudioStream *stream;
        void Play();
        bool doRepeat;
    };


    /*
    struct SoundPlayback
    {
        Sound *sound;
        bool isPlaying;
        bool doRepeat;
    };
    */

    void Init();
    void CleanUp();
    void Update();
    Sound* CreateSoundFromFile(const char* filename);
    void DeleteSound(Sound *sound);
    
}

