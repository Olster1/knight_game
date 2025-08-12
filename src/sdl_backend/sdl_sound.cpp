#define SDL_AUDIO_CALLBACK(name) void name(void *udata, unsigned char *stream, int len)
typedef SDL_AUDIO_CALLBACK(sdl_audio_callback);

#include "../audio.h"

struct PlatformAudioSpec{
    SDL_AudioSpec audioSpec;
};

void loadOggVorbisFile(GameSoundAsset *result, char *fileName, PlatformAudioSpec *audioSpecWrapper) {
    result->fileName = fileName;
}

#define initAudioSpec(audioSpec, frequency) initAudioSpec_(audioSpec, frequency, audioCallback)

void initAudioSpec_(PlatformAudioSpec *audioSpecWrapper, int frequency, sdl_audio_callback *callback) {
    SDL_AudioSpec *audioSpec = &audioSpecWrapper->audioSpec;
    /* Set the audio format */
    audioSpec->freq = frequency;
    audioSpec->format = AUDIO_S16;
    audioSpec->channels = AUDIO_STEREO;
    audioSpec->samples = 4096; 
    audioSpec->callback = callback;
    audioSpec->userdata = NULL;
    assert(callback);
}

bool initAudio(PlatformAudioSpec *audioSpecWrapper) {
    SDL_AudioSpec *audioSpec = &audioSpecWrapper->audioSpec;
    bool successful = true;
    globalSoundState = pushStruct(&global_long_term_arena, EasySound_SoundState);

    SDL_AudioDeviceID id = SDL_OpenAudioDevice(NULL, 0, audioSpec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);
    /* Open the audio device, forcing the desired format */
    if (id == 0) {
        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        successful = false;
    } else {
        SDL_PauseAudioDevice(id, 0);
    }
    return successful;
}

SDL_AUDIO_CALLBACK(audioCallback) {
    
    SDL_memset(stream, 0, len);

    int bytes_per_sample = 2; // AUDIO_S16
    int totalSamples = len / bytes_per_sample; 

    //NOTE: This might be on a seperate thread so we can't use a temporary memory arena.
    //      Could have a seperate one for this thread
    float *mixerBuffer = (float *)malloc(totalSamples*sizeof(float));
    SDL_memset(mixerBuffer, 0, totalSamples*sizeof(float));
    s16 *oggBuffer = (s16 *)malloc(totalSamples*sizeof(s16));

    PlayingSound **soundPrt = &globalSoundState->playingSounds;
    while(*soundPrt) {
        bool advancePtr = true;
        PlayingSound *sound = *soundPrt;

        if(sound->active) {
            int samples = stb_vorbis_get_samples_short_interleaved(sound->stream, AUDIO_STEREO, oggBuffer, totalSamples);
            samples*=AUDIO_STEREO;

            //NOTE: This is the Audio Mixer - it sould do everything in float then sample down
            for(int i = 0; i < samples; i++) {
                float a = mixerBuffer[i];
                float value = (float)(((s16 *)oggBuffer)[i]);
                assert(sound->volume >= 0 && sound->volume <= 1);
                float b = sound->volume*value;
                mixerBuffer[i] = a + b;
            }

            free(oggBuffer);
          
            if(samples == 0) {
                if(sound->nextSound) {
                    //TODO: Allow the remaining bytes to loop back round and finish the full duration 
                    sound->active = false;
                    sound->nextSound->active = true;

                    *sound = *sound->nextSound;
                } else {
                    sound->active = false;
                    //remove from linked list
                    advancePtr = false;
                    *soundPrt = sound->next;
                    sound->next = globalSoundState->playingSoundsFreeList;
                    globalSoundState->playingSoundsFreeList = sound;
                }
            }
        }

        if(sound->shouldEnd) {
            advancePtr = false;
            stb_vorbis_close(sound->stream);
            *soundPrt = sound->next;
            sound->next = globalSoundState->playingSoundsFreeList;
            globalSoundState->playingSoundsFreeList = sound;
        }
        
        if(advancePtr) {
            soundPrt = &((*soundPrt)->next);
        }
    }

    s16 *s = (s16 *)stream;
    for(int i = 0; i < totalSamples; ++i) {
        int sample = (int)(mixerBuffer[i]);
        if (sample > 32767) sample = 32767;
        else if (sample < -32768) sample = -32768;
        s[i] = (s16)sample;
    }

    free(mixerBuffer);
}