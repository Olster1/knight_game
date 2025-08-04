enum GameTurnType {
    GAME_TURN_PLAYER_KNIGHT,
    GAME_TURN_PLAYER_GOBLIN,
};

enum GameTurnPhase {
    GAME_TURN_PHASE_COMMAND, //Cast spells, activate hero abilities, use command abilities.
    GAME_TURN_PHASE_MOVE,
    GAME_TURN_PHASE_SHOOT,
    GAME_TURN_PHASE_CHARGE,
    GAME_TURN_PHASE_FIGHT,
    GAME_TURN_PHASE_BATTLESHOCK,
};

struct GamePlay {
    GameTurnType turnOn;
    float turnTime;
    float maxTurnTime;
    int turnCount;
    int maxTurnCount;
    GameTurnPhase phase;
    bool boardInited;
};

GamePlay init_gameplay() {
    GamePlay gamePlay = {};

    gamePlay.turnOn = GAME_TURN_PLAYER_KNIGHT;
    gamePlay.phase = GAME_TURN_PHASE_COMMAND;
    gamePlay.boardInited = false;

    gamePlay.turnTime = 0;
    gamePlay.maxTurnTime = 60*5; //NOTE: 5 mintues
    gamePlay.maxTurnCount = 10;
    gamePlay.turnCount = 0;

    return gamePlay;
}
