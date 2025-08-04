#ifndef EASY_HEADERS_ASSERT
#define EASY_HEADERS_ASSERT(statement) if(!(statement)) { int *i_ptr = 0; *(i_ptr) = 0; }
#endif

#ifndef EASY_HEADERS_ALLOC
#include <stdlib.h>
#define EASY_HEADERS_ALLOC(size) malloc(size)
#endif

#ifndef EASY_HEADERS_FREE
#include <stdlib.h>
#define EASY_HEADERS_FREE(ptr) free(ptr)
#endif
	
#define EASY_MAX_FRAME_COUNT 32
typedef struct {
    char *frames[EASY_MAX_FRAME_COUNT];
    Texture *frameSprites[EASY_MAX_FRAME_COUNT];

    int frameCount;
    char *name;
}  Animation;

typedef struct EasyAnimation_ListItem EasyAnimation_ListItem;
typedef struct EasyAnimation_ListItem {
    float timerAt;
    float timerPeriod;

    int frameIndex;
    
    Animation *animation;

    EasyAnimation_ListItem *prev;
    EasyAnimation_ListItem *next;
} EasyAnimation_ListItem;

typedef struct {
    EasyAnimation_ListItem parent;

    Animation *lastAnimationOn;
    bool finishedAnimationLastUpdate; //goes to true if it finished an animation in the last update so you can change it if you need to
    unsigned int currentLoopCount; //NOTE: How many times the current animation has looped for
} EasyAnimation_Controller;

static char *easyAnimation2d_copyString(char *str) {

    int sizeOfString = 0;
    char *at = (char *)str;
    while(*at) {
        sizeOfString++;
        at++;
    }

    char *result = (char *)EASY_HEADERS_ALLOC(sizeOfString + 1);
    result[sizeOfString] = '\0';

    //Now copy the string
    char *src = (char *)str;
    char *dest = (char *)result;
    while(*src) {
        *dest = *src;
        src++;
        dest++;
    }

    return result;
}

static bool easyAnimation_isControllerValid(EasyAnimation_Controller *controller) {
    return (controller->parent.next != 0);
}

static void easyAnimation_initController(EasyAnimation_Controller *controller) {
    controller->parent.next = controller->parent.prev = &controller->parent;
    controller->finishedAnimationLastUpdate = false;
    controller->lastAnimationOn = 0;
    controller->currentLoopCount = 0;
}

static void easyAnimation_initAnimation_withFrames(Animation *animation, char **FileNames, int FileNameCount, char *name) {
    animation->name = name;
    animation->frameCount = 0;

    for(int i = 0; i < FileNameCount; ++i) {
        EASY_HEADERS_ASSERT(animation->frameCount < arrayCount(animation->frames));
        animation->frames[animation->frameCount++] = easyAnimation2d_copyString(FileNames[i]);
    }
}

static void easyAnimation_initAnimation(Animation *animation, char *name) {
    animation->name = name;
    animation->frameCount = 0;
}

static void easyAnimation_pushFrame(Animation *animation, Texture *t) {
    EASY_HEADERS_ASSERT(animation->frameCount < arrayCount(animation->frames));
    animation->frameSprites[animation->frameCount++] = t;
}

// static Animation *easyAnimation_findAnimationWithId(Animation *animations, int AnimationsCount, int id) {
//     Animation *Result = 0;
//     for(int i = 0; i < AnimationsCount; i++) {
//         Animation *Anim = animations + i;
//         if(Anim->animationId == id) {
//             Result = Anim;
//             break;
//         }
//     }
    
//     return Result;
// }

static Animation *easyAnimation_findAnimation(Animation *Animations, int AnimationsCount, char *name) {
    Animation *Result = 0;
    for(int i = 0; i < AnimationsCount; ++i) {
        Animation *Anim = Animations + i;
        if(easyString_stringsMatch_nullTerminated(Anim->name, name)) {
            Result = Anim;
            break;
        }
    }
    
    return Result;
}


static EasyAnimation_ListItem *easyAnimation_addAnimationToController(EasyAnimation_Controller *controller, EasyAnimation_ListItem **AnimationItemFreeListPtr, Animation *animation, float period) {
    EasyAnimation_ListItem *AnimationItemFreeList = *AnimationItemFreeListPtr;
    EasyAnimation_ListItem *Item = 0;
    if(AnimationItemFreeList) {
        Item = AnimationItemFreeList;
        *AnimationItemFreeListPtr = AnimationItemFreeList->next;
    } else {
        Item = (EasyAnimation_ListItem *)EASY_HEADERS_ALLOC(sizeof(EasyAnimation_ListItem));
    }
    
    EASY_HEADERS_ASSERT(Item);
    
    Item->timerAt = 0;
    Item->timerPeriod = period;
    
    Item->frameIndex = 0;
    
    Item->animation = animation;
        
    EasyAnimation_ListItem *AnimationListSentintel = &controller->parent;

    //Add animation to end of list;
    Item->next = AnimationListSentintel;
    Item->prev = AnimationListSentintel->prev;
    
    AnimationListSentintel->prev->next = Item;
    AnimationListSentintel->prev = Item;

    return Item;
    
}

static void easyAnimation_emptyAnimationContoller(EasyAnimation_Controller *controller, EasyAnimation_ListItem **AnimationItemFreeListPtr) {
    //NOTE(ollie): While still things on the list
    EasyAnimation_ListItem *AnimationListSentintel = &controller->parent;

    while(AnimationListSentintel->next != AnimationListSentintel) {
        EasyAnimation_ListItem *Item = AnimationListSentintel->next;
        //Remove from linked list
        AnimationListSentintel->next = Item->next;
        Item->next->prev = AnimationListSentintel;
        
        //Add to free list
        Item->next = *AnimationItemFreeListPtr;
        *AnimationItemFreeListPtr = Item;
    }

    controller->currentLoopCount = 0;
    
}

static void easyAnimation_randomStart(EasyAnimation_ListItem *item) {
    if(item) {
        item->timerAt = random_between_float(0, item->timerPeriod);
        item->frameIndex = random_between_int(0, item->animation->frameCount);
        assert(item->frameIndex < item->animation->frameCount);

    }
}

static char *easyAnimation_updateAnimation(EasyAnimation_Controller *controller, EasyAnimation_ListItem **AnimationItemFreeListPtr, float dt) {
    EasyAnimation_ListItem *AnimationListSentintel = &controller->parent;

    controller->finishedAnimationLastUpdate = false; //clear out the value

    EasyAnimation_ListItem *Item = AnimationListSentintel->next;
    EASY_HEADERS_ASSERT(Item != AnimationListSentintel);
    
    EASY_HEADERS_ASSERT(Item->timerAt >= 0);

    controller->lastAnimationOn = Item->animation;

    Item->timerAt += dt;    

    char *result = Item->animation->frames[Item->frameIndex];

    if(Item->timerAt >= Item->timerPeriod) {
        Item->frameIndex++;
        Item->timerAt = 0;
        
        if(Item->frameIndex >= Item->animation->frameCount)
        { 
            //finished animation
            Item->frameIndex = 0;

            if(Item->next != AnimationListSentintel) {
                //NOTE: There are more than one animation on the list, so go to the next one
                //Remove from linked list
                AnimationListSentintel->next = Item->next;
                Item->next->prev = AnimationListSentintel;
                
                //Add to free list
                Item->next = *AnimationItemFreeListPtr;
                *AnimationItemFreeListPtr = Item;

                controller->currentLoopCount = 0;
            } else {
                controller->currentLoopCount++;
            }

            controller->finishedAnimationLastUpdate = true;

        }
        
    }

    
    return result;
}

static Texture *easyAnimation_updateAnimation_getTexture(EasyAnimation_Controller *controller, EasyAnimation_ListItem **AnimationItemFreeListPtr, float dt) {
	EasyAnimation_ListItem *AnimationListSentintel = &controller->parent;

	easyAnimation_updateAnimation(controller, AnimationItemFreeListPtr, dt);

	EasyAnimation_ListItem *Item = AnimationListSentintel->next;
	assert(Item != AnimationListSentintel);
	assert(Item->timerAt >= 0);

	return Item->animation->frameSprites[Item->frameIndex];
}

inline static int easyAnimation_isControllerEmpty(EasyAnimation_Controller *c) {
    int Result = c->parent.next == &c->parent;
    return Result;
}

inline static float easyAnimation_getDirectionInRadians(float x, float y) {
    float DirectionValue = 0;
    if(x != 0 || y != 0) {
        //float2 EntityVelocity = normalizeV2(dp);
        DirectionValue = ATan2_0toTau(y, x);
    }
    return DirectionValue;
}

inline static char *easyAnimation_getFrameOn(EasyAnimation_ListItem *AnimationListSentintel) {
    char *currentFrame = AnimationListSentintel->next->animation->frames[AnimationListSentintel->next->frameIndex];
    return currentFrame;
}

inline static bool easyAnimation_getCurrentAnimation(EasyAnimation_Controller *c, Animation *a) {
    return (c->parent.next->animation == a);

}

struct AnimationState {
    Animation waterAnimation;
    Animation waterRocks[4];
    EasyAnimation_ListItem *animationItemFreeListPtr;
};
