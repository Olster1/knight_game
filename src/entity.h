static u32 global_entityId = 0;

enum EntityFlag {
    ENTITY_ACTIVE = 1 << 0, 
    LIGHT_COMPONENT = 1 << 1,
    ENTITY_SPRITE_FLIPPED = 1 << 3,
    ENTITY_ON_FIRE = 1 << 4, //NOTE: For buildings that catch fire
    ENTITY_CAN_WALK = 1 << 5, //NOTE: For entities that can walk around like the peasant, knight, etc.
    ENTITY_SELECTED = 1 << 6, //NOTE: Whether entity is selected
    ENTITY_SHOW_DAMAGE_SPLAT = 1 << 7,
};

#define MY_ENTITY_TYPE(FUNC) \
FUNC(ENTITY_PEASANT)\
FUNC(ENTITY_ARCHER)\
FUNC(ENTITY_KNIGHT)\
FUNC(ENTITY_MAN)\
FUNC(ENTITY_TREE)\
FUNC(ENTITY_TEMPLER_KNIGHT)\
FUNC(ENTITY_GOBLIN)\
FUNC(ENTITY_GOBLIN_TNT)\
FUNC(ENTITY_GOBLIN_BARREL)\
FUNC(ENTITY_HOUSE)\
FUNC(ENTITY_CASTLE)\
FUNC(ENTITY_TOWER)\
FUNC(ENTITY_GOBLIN_HOUSE)\
FUNC(ENTITY_GOBLIN_TOWER)\

typedef enum {
    MY_ENTITY_TYPE(ENUM)
} EntityType;

static char *MyEntity_TypeStrings[] = { MY_ENTITY_TYPE(STRING) };

enum ColliderIndex {
    PLATFORM_COLLIDER_INDEX = 0,
    ATTACK_COLLIDER_INDEX = 1,
    HIT_COLLIDER_INDEX = 2,
    INTERACT_COLLIDER_INDEX = 3,
    ENTITY_COLLIDER_INDEX_COUNT
};

struct CloudData {
    float2 pos;
    float scale;
    int cloudIndex;
    float fadePeriod;
    float darkness;
};

enum ColliderFlag {
    COLLIDER_ACTIVE = 1 << 0,
    COLLIDER_TRIGGER = 1 << 1,
};

enum CollideEventType {
    COLLIDE_ENTER,
    COLLIDE_STAY,
    COLLIDE_EXIT,
};

struct CollideEvent {
    u32 entityId;

    CollideEventType type;

    //NOTE: Cached values from the other entity we can use without having to look up the entity in the updateEntities loop
    EntityType entityType;
    float damage;
    float2 hitDir;

    //@private
    bool hitThisFrame; //NOTE: Only used by the internal collision code 
};

struct Collider {
    // ColliderType type; //NOTE: Assume everything is a rectangle

    int collideEventsCount;
    CollideEvent events[8];

    float3 offset;
    float3 scale; //NOTE: This is a percentage of the parent scale;

    u32 flags;
};

Collider make_collider(float3 offset, float3 scale, u32 flags) {
    Collider c = {};

    c.flags = flags;
    c.offset = offset;
    c.scale = scale;

    return c;
}

struct DamageSplat {
    int damage;
    float3 worldP;
    float timeAt;
    DamageSplat *next;
};

struct DefaultEntityAnimations {
    Animation idle;
	Animation run;
	
    //NOTE: ATTACK
	Animation attackUp;
    Animation attackDown;
    Animation attackSide;

    //NOTE: For the peasant
    Animation work;

    //NOTE: For the TNT barrel & Peasant
    Animation scared;

    //NOTE: For the tree
    Animation fallen;
    Animation dead;
};

struct EntityMove {
    float3 move;
    EntityMove *next;
};

struct Entity {
    u32 id; 

    Entity *parent; //TODO: Not sure if we need this
    EntityType type;
    u64 flags;

    //NOTE: TRANSFORM component
    float3 pos;
    float3 offsetP;
    float sortYOffset;
    float speed; //NOTE: How fast the entity moves - used to scale direction vectors
    float3 scale;
    float3 velocity;
    float rotation;
    float targetRotation;
    int health;

    int maxMoveDistance;
    EntityMove *moves;

    float perlinNoiseLight; //NOTE: Used for the lights

    float deltaTLeft; //NOTE: Used in collision loop
    float3 deltaPos; //NOTE: Used in collision loop

    //NOTE: Particle systems associated with this entity
    int particlerCount;
    ParticlerId particlerIds[2];
    Particler *particlers[2];
    float fireTimer;

    DefaultEntityAnimations *animations; //NOTE: This points to animations stored on the gameState
    EasyAnimation_Controller animationController;
};


bool hasEntityFlag(Entity *e, EntityFlag flag) {
    return (e->flags & (u64)flag);
}

void removeEntityFlag(Entity *e, EntityFlag flag) {
    e->flags &= ~((u64)flag);
}

void addEntityFlag(Entity *e, EntityFlag flag) {
    e->flags |= ((u64)flag);
}