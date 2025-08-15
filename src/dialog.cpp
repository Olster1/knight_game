
#define MY_DIALOG_TYPE(FUNC) \
FUNC(ENTITY_DIALOG_NULL)\
FUNC(ENTITY_DIALOG_ADVENTURER)\
FUNC(ENTITY_DIALOG_WEREWOLF_SIGN)\
FUNC(ENTITY_DIALOG_WELCOME_SIGN)\
FUNC(ENTITY_DIALOG_HOUSE)\
FUNC(ENTITY_DIALOG_FOREST_RIP)\
FUNC(ENTITY_DIALOG_LADY_CASTLE)\
FUNC(ENTITY_DIALOG_ASK_USER_TO_SAVE)\
FUNC(ENTITY_DIALOG_ENTER_SHOP)\
FUNC(ENTITY_DIALOG_SLEEP_IN_BED)\
FUNC(ENTITY_DIALOG_ENTER_CRAFTING)\

typedef enum {
    MY_DIALOG_TYPE(ENUM)
} DialogInfoType;

static char *MyDialog_DialogTypeStrings[] = { MY_DIALOG_TYPE(STRING) };


typedef enum {
	DIALOG_ACTION_NULL,
	DIALOG_ACTION_SAVE_PROGRESS,
	DIALOG_ACTION_EXIT_DIALOG, //TO play sound to make it more obvious you exited
	DIALOG_ACTION_GO_TO_SHOP,
	DIALOG_ACTION_SLEEP,
	DIALOG_ACTION_GO_TO_CRAFTING
} DialogActionType;

typedef struct EntityDialogNode EntityDialogNode;
typedef struct EntityDialogNode {
	int textCount;
	char *texts[4];

	int choiceCount;
	char *choices[4];

	//Add actions or items that the NPC might give the character here
	DialogActionType actions[4];

	//to match the choices
	EntityDialogNode *next[4];

	bool isEndNode; //NOTE: This is for choices so they can end while still sliding the choices back in
 } EntityDialogNode;



static void pushTextToNode(EntityDialogNode *n, char *text) {
	assert(n->textCount < arrayCount(n->texts));
	n->texts[n->textCount++] = text;
}

static void pushChoiceToNode(EntityDialogNode *n, char *text) {
	assert(n->choiceCount < arrayCount(n->choices));
	n->choices[n->choiceCount++] = text;
}

static void pushConnectionNode(EntityDialogNode *n, EntityDialogNode *n1, int index) {
	n->next[index] = n1;
}

static void dialog_pushActionForChoice(EntityDialogNode *n, int index, DialogActionType type) {
	assert(index < arrayCount(n->actions));
	n->actions[index] = type;
}

static EntityDialogNode *pushEmptyNode_longTerm() {
	EntityDialogNode *result = pushStruct(&global_long_term_arena, EntityDialogNode);
	easyMemory_zeroStruct(result, EntityDialogNode);
	return result;
}

typedef struct {
	//Fill out all the dialogs here
	EntityDialogNode *houseDialog;
	EntityDialogNode *philosophyDialog;
	EntityDialogNode *graveDialog;
	EntityDialogNode *ladyOutsideCastle;
	EntityDialogNode *errorDialogForDebugging;
	EntityDialogNode *promptUserSaveAtFire;
	EntityDialogNode *enterShopDialog;
	EntityDialogNode *promptSleepInBed;
	EntityDialogNode *promptEnterCratingArea;

	Memory_Arena perDialogArena;
	MemoryArenaMark perDialogArenaMark;
} GameDialogs;


static EntityDialogNode *constructDialogNode(GameDialogs *dialogs) {
	EntityDialogNode *result = pushStruct(&dialogs->perDialogArena, EntityDialogNode); 

	easyMemory_zeroStruct(result, EntityDialogNode);

	return result;
}


static void initDialogTrees(GameDialogs *gd) {

	gd->perDialogArena = initMemoryArena(Kilobytes(50));

	easyMemory_zeroStruct(&gd->perDialogArenaMark, MemoryArenaMark); 

	//man dialog
	{

		EntityDialogNode *n = pushEmptyNode_longTerm();	
		gd->houseDialog = n;

		pushTextToNode(n, "{s: 1}Hi, can you.{s: 1}.. {p: 1} can you do something for me?");
		pushChoiceToNode(n, "What is it?");
		pushChoiceToNode(n, "No way.");

		EntityDialogNode *n1 = pushEmptyNode_longTerm();
		pushTextToNode(n1, "{s: 2}You know what. There's some secrets up there.");
		pushTextToNode(n1, "{s: 2}Hidden deep in the caves of Moria you'll find something.");
		pushChoiceToNode(n1, "Wow, tell me more.");
		pushChoiceToNode(n1, "That doesn't sound great at all.");
		pushChoiceToNode(n1, "I need some dough. Can we do this together?");

		EntityDialogNode *n3 = pushEmptyNode_longTerm();
		pushTextToNode(n3, "{s: 2}Ok, forget I asked.");

		pushConnectionNode(n, n1, 0);
		pushConnectionNode(n, n3, 1);

		EntityDialogNode *n2 = pushEmptyNode_longTerm();
		pushTextToNode(n2, "{s: 2}I've got nothing else to say.");

		pushConnectionNode(n1, n2, 0);
		pushConnectionNode(n1, n2, 1);
		pushConnectionNode(n1, n2, 2);


		////TODO: HERE WE CAN ADD ACTIONS LIKE ADD TO QUEST 

	}

	//lady castle dialog
	{

		EntityDialogNode *n = pushEmptyNode_longTerm();
		gd->ladyOutsideCastle = n;

		pushTextToNode(n, "{s: 2}Are you going inside?");
		pushChoiceToNode(n, "If I can find a way to get in.");
		pushChoiceToNode(n, "What's it to you?");

		EntityDialogNode *n1 = pushEmptyNode_longTerm();
		pushTextToNode(n1, "{s: 2}Can you find my daughter inside? The Illoway army took her captive.");
		pushChoiceToNode(n1, "Consider it done.");
		pushChoiceToNode(n1, "No way.");

		EntityDialogNode *n3 = pushEmptyNode_longTerm();
		pushTextToNode(n3, "{s: 2}My daughter is inside. The Illoway army has taken her.");
		pushTextToNode(n3, "{s: 2}I am to weak to enter myself. Can you find her for me?");
		pushChoiceToNode(n3, "I can do this for you.");
		pushChoiceToNode(n3, "What's in it for me?");

		EntityDialogNode *n5 = pushEmptyNode_longTerm();
		pushTextToNode(n5, "{s: 2}I have gold I can offer.");
		pushChoiceToNode(n5, "Ok");
		pushChoiceToNode(n5, "Sorry, I need to keep moving.");

		EntityDialogNode *n2 = pushEmptyNode_longTerm();
		pushTextToNode(n2, "{s: 2}Thankyou, I will be forever be in your debt.");
		pushTextToNode(n2, "{s: 0.8}Please find her.");

		EntityDialogNode *n4 = pushEmptyNode_longTerm();
		pushTextToNode(n4, "{s: 2}Ok, I wish you well on your journey.");

		pushConnectionNode(n, n1, 0);
		pushConnectionNode(n, n3, 1);

		pushConnectionNode(n1, n2, 0);
		pushConnectionNode(n1, n4, 1);

		pushConnectionNode(n3, n2, 0);
		pushConnectionNode(n3, n5, 1);

		pushConnectionNode(n5, n2, 0);
		pushConnectionNode(n5, n4, 1);
		

		

	}


	//graveDialog
	{
		EntityDialogNode *n = pushEmptyNode_longTerm();
		gd->graveDialog = n;

		pushTextToNode(n, "{s: 1}R{p: 0.5}.I{p: 0.5}.P{p: 0.5}. Here lays the protector of this forest.");
	}

	//Error dialog for debugging when you haven't set the type to link up with the 
	{
		EntityDialogNode *n = pushEmptyNode_longTerm();
		gd->errorDialogForDebugging = n;

		pushTextToNode(n, "ERROR: You didn't link up the dialog type to the actual dialog.");
	}

	//Fire place prompt user to save
	{
		EntityDialogNode *n = pushEmptyNode_longTerm();
		gd->promptUserSaveAtFire = n;

		pushTextToNode(n, "{s: 3}Would you like to save your progress?");
		pushChoiceToNode(n, "Yes");
		pushChoiceToNode(n, "No");

		dialog_pushActionForChoice(n, 0, DIALOG_ACTION_SAVE_PROGRESS);
		dialog_pushActionForChoice(n, 1, DIALOG_ACTION_EXIT_DIALOG);
		
		EntityDialogNode *n1 = pushEmptyNode_longTerm();
		pushTextToNode(n1, "{s: 3}Progress Saved.");

		EntityDialogNode *n2 = pushEmptyNode_longTerm();
		n2->isEndNode = true;
		pushTextToNode(n2, "");

		pushConnectionNode(n, n1, 0);
		pushConnectionNode(n, n2, 1);
		
	}

	{
		EntityDialogNode *n = pushEmptyNode_longTerm();
		gd->enterShopDialog = n;

		pushTextToNode(n, "{s: 3}Hi, back again for some supplies?");
		pushChoiceToNode(n, "Yes");
		pushChoiceToNode(n, "No");

		dialog_pushActionForChoice(n, 0, DIALOG_ACTION_GO_TO_SHOP);
		dialog_pushActionForChoice(n, 1, DIALOG_ACTION_EXIT_DIALOG);
		
		EntityDialogNode *n2 = pushEmptyNode_longTerm();
		n2->isEndNode = true;
		pushTextToNode(n2, "");

		pushConnectionNode(n, n2, 1);
		

	}

	{
		EntityDialogNode *n = pushEmptyNode_longTerm();
		gd->promptEnterCratingArea = n;

		pushTextToNode(n, "{s: 3}Do you want to use the Cauldron?");
		pushChoiceToNode(n, "Yes");
		pushChoiceToNode(n, "No");

		dialog_pushActionForChoice(n, 0, DIALOG_ACTION_GO_TO_CRAFTING);
		dialog_pushActionForChoice(n, 1, DIALOG_ACTION_EXIT_DIALOG);
		
		EntityDialogNode *n2 = pushEmptyNode_longTerm();
		n2->isEndNode = true;
		pushTextToNode(n2, "");

		pushConnectionNode(n, n2, 1);

	}

	

	//NOTE: Sleep in bed
	{
		EntityDialogNode *n = pushEmptyNode_longTerm();
		gd->promptSleepInBed = n;

		pushTextToNode(n, "{s: 3}Would you like to sleep?");
		pushChoiceToNode(n, "Yes");
		pushChoiceToNode(n, "No");

		dialog_pushActionForChoice(n, 0, DIALOG_ACTION_SLEEP);
		dialog_pushActionForChoice(n, 1, DIALOG_ACTION_EXIT_DIALOG);
		
		EntityDialogNode *n2 = pushEmptyNode_longTerm();
		n2->isEndNode = true;
		pushTextToNode(n2, "");

		pushConnectionNode(n, n2, 0);
		pushConnectionNode(n, n2, 1);
		

	}

	//Philosophy dialog
	{

		// EntityDialogNode *n = pushStruct(&globalLongTermArena, EntityDialogNode);	
		// gd->houseDialog = n;

		// pushTextToNode(n, "{s: 2}How do you know what's real?.");
		// pushChoiceToNode(n, "I can see it, measure it and tomorrow I'll get the same result.");
		// pushChoiceToNode(n, "I don't know.");
		// pushChoiceToNode(n, "It just is");

		// EntityDialogNode *n1 = pushStruct(&globalLongTermArena, EntityDialogNode);	
		// pushTextToNode(n1, "{s: 2}It's.");
		// pushTextToNode(n1, "{s: 2}Hidden deep in the caves of Moria you'll find something.");
		// pushChoiceToNode(n1, "Wow, tell me more.");
		// pushChoiceToNode(n1, "That doesn't sound great at all.");
		// pushChoiceToNode(n1, "I need some dough. Can we do this together?");

		// EntityDialogNode *n3 = pushStruct(&globalLongTermArena, EntityDialogNode);	
		// pushTextToNode(n3, "{s: 2}Ok, forget I asked. Suit yourself.");

		// pushConnectionNode(n, n1, 0);
		// pushConnectionNode(n, n3, 1);
		// pushConnectionNode(n, n1, 2);

		// EntityDialogNode *n2 = pushStruct(&globalLongTermArena, EntityDialogNode);	
		// pushTextToNode(n2, "{s: 2}I've got nothing else to say.");

		// pushConnectionNode(n1, n2, 0);
		// pushConnectionNode(n1, n2, 1);
		// pushConnectionNode(n1, n2, 2);

	}


	
}


static EntityDialogNode *findDialogInfo(DialogInfoType type, GameDialogs *gd) {
	EntityDialogNode *result = gd->errorDialogForDebugging;

	if(false) {

	} else if(type == ENTITY_DIALOG_ADVENTURER) {
	} else if(type == ENTITY_DIALOG_WEREWOLF_SIGN) {
	} else if(type == ENTITY_DIALOG_WELCOME_SIGN) {
	} else if(type == ENTITY_DIALOG_FOREST_RIP) {
		result = gd->graveDialog;
	} else if(type == ENTITY_DIALOG_HOUSE) {
		result = gd->houseDialog;
	} else if(type == ENTITY_DIALOG_LADY_CASTLE) {
		result = gd->ladyOutsideCastle;
	} else if(type == ENTITY_DIALOG_ASK_USER_TO_SAVE) {
		result = gd->promptUserSaveAtFire;
	} else if(type == ENTITY_DIALOG_ENTER_SHOP) {
		result = gd->enterShopDialog;
	} else if(type == ENTITY_DIALOG_SLEEP_IN_BED) {
		result = gd->promptSleepInBed;
	} else if(type == ENTITY_DIALOG_ENTER_CRAFTING) {
		result = gd->promptEnterCratingArea;
	}

	return result;
}
//////////////////////////////////////////////////////////////////////////