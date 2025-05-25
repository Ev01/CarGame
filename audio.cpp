#include "audio.h"

#include <SDL3/SDL.h>

#include <vector>

struct SoundFile
{
    Audio::SoundData data;
    const char *filename;
};

static SDL_AudioDeviceID audioDevice;
//static std::vector<Audio::SoundData> existingSoundData;
static std::vector<SoundFile> loadedSounds;
static std::vector<Audio::Sound*> existingSounds;

static bool openAudioDevice()
{
    audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (audioDevice == 0) {
        SDL_Log("Could not open audio device: %s", SDL_GetError());
        return false;
    }
    SDL_Log("Opened audio device");
    return true;
}


static Audio::SoundData getSoundDataFromFile(const char* filename)
{
    // Search through existing sounds to see if this sound has already been
    // loaded
    for (SoundFile soundFile : loadedSounds) {
        // Can directly compare pointers because they are const char
        if (soundFile.filename == filename) {
            SDL_Log("File %s has already been loaded from disk.", filename);
            return soundFile.data;
        }
    }

    SDL_Log("Loading file %s", filename);
    // This file hasn't been loaded yet. Load it now
    Audio::SoundData soundData;
    if (!SDL_LoadWAV(filename, &soundData.spec, &soundData.buffer, &soundData.length)) {
        SDL_Log("Could not load audio file: %s", SDL_GetError());
    }

    // Add new entry to loaded sounds
    SoundFile soundFile;
    soundFile.data = soundData;
    soundFile.filename = filename;
    loadedSounds.push_back(soundFile);

    return soundData;
}



void Audio::Init()
{
    openAudioDevice();
}

Audio::Sound* Audio::CreateSoundFromFile(const char* filename)
{
    Audio::Sound *sound = (Audio::Sound*) SDL_malloc(sizeof(Audio::Sound));
    sound->data = getSoundDataFromFile(filename);
    sound->stream = SDL_CreateAudioStream(&sound->data.spec, NULL);
    if (sound->stream == NULL) {
        SDL_Log("Could not create audio stream: %s", SDL_GetError());
        SDL_free(sound);
        return NULL;
    }
    if (!SDL_BindAudioStream(audioDevice, sound->stream)) {
        SDL_Log("Could not bind audio stream: %s", SDL_GetError());
        SDL_free(sound);
        return NULL;
    }

    existingSounds.push_back(sound);

    return sound;
}


void Audio::Update()
{
    for (Audio::Sound *sound : existingSounds) {

        int queued = SDL_GetAudioStreamQueued(sound->stream); 
        if (queued < 0) {
            SDL_Log("SDL_GetAudioStreamQueued Failed: %s", SDL_GetError());
            return;
        }
        if (sound->doRepeat && (Uint32)queued < sound->data.length) {
            bool result = SDL_PutAudioStreamData(sound->stream,
                                                 sound->data.buffer,
                                                 sound->data.length);
            if (!result) {
                SDL_Log("Could not put audio stream data: %s", SDL_GetError());
            }
        }
    }
}




void Audio::Sound::Play()
{
    if (!SDL_PutAudioStreamData(stream, data.buffer, data.length)) {
        SDL_Log("Could not put audio stream data: %s", SDL_GetError());
    }
}
