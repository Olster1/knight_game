
#define AUDIO_MONO 1
#define AUDIO_STEREO 2

#include "../libs/stb_vorbis.c"

struct GameSoundAsset {
    char *fileName; //NOTE: Ogg Vorbis filename
};

struct PlayingSound {
    stb_vorbis *stream;
    stb_vorbis_info info;

    bool active;
    float volume; //percent of original volume
    float fadeOutTimer;
    float maxFadeOutTimer;
    
    //NOTE(ollie): This is the next sound to play if it's looped or continues to next segment of song etc. 
    PlayingSound *nextSound;
    
    //NOTE(ollie): The playing sounds is a big linked list, so this is pointing to the sound that's next into the list
    struct PlayingSound *next;

    bool shouldEnd;
    
    //NOTE: For 3d sounds
    bool attenuate; //for if we lerp from the listner to the sound position
    float3 location;

    //NOTE(ollie): Radius for sound. Inside inner radius, volume at 1, then lerp between inner & outer. 
    float innerRadius;
    float outerRadius;
    //

};

typedef struct {
    //NOTE: Playing sounds lists
    PlayingSound *playingSoundsFreeList;
    PlayingSound *playingSounds;
    //
    
    //For 3d sounds
    // float3 listenerLocation;
    
} EasySound_SoundState;

static EasySound_SoundState *globalSoundState = {0};

void updateSoundsFade(float dt) {
    PlayingSound *sound = globalSoundState->playingSounds;
    while(sound) {
        if(sound->fadeOutTimer > 0) {
            sound->fadeOutTimer -= dt;

            if(sound->fadeOutTimer <= 0) {
                sound->shouldEnd = true;
                sound->fadeOutTimer = 0;
            }

            sound->volume = sound->fadeOutTimer / sound->maxFadeOutTimer;
            assert(sound->volume >= 0 && sound->volume <= 1);
            
        }

        sound = sound->next;
    }
}

PlayingSound *getPlaySound(GameSoundAsset *gameSoundAsset) {
    PlayingSound *result = globalSoundState->playingSoundsFreeList;
    if(result) {
        globalSoundState->playingSoundsFreeList = result->next;
    } else {
        result = pushStruct(&global_long_term_arena, PlayingSound);
    }
    assert(result);
    
    result->active = true;
    result->volume = 1.0f;

    int error = 0;
    result->stream = stb_vorbis_open_filename(gameSoundAsset->fileName, &error, NULL);
    if (!result->stream) { assert(false); }
    result->info = stb_vorbis_get_info(result->stream);
    
    result->nextSound = 0;
    result->shouldEnd = false;
    
    return result;
}

PlayingSound *playSound(GameSoundAsset *wavFile) {
    PlayingSound *result = getPlaySound(wavFile);
    
    //add to playing sounds. 
    result->next = globalSoundState->playingSounds;
    globalSoundState->playingSounds = result;
    
    return result;
}
