struct ItemInfo {
    char *title;
    char *description;
};

ItemInfo make_itemInfo(char *title, char *description) {
    ItemInfo result = {};

    result.title = title;
    result.description = description;
    
    return result;
}

ItemInfo getItemInfo(Entity *e) {
    ItemInfo result = {};

    if(e->type == ENTITY_BEAR) {

    } else if(e->type == ENTITY_TREE) {
        result.title = "Tree";
        result.description = "Good for cutting and making fires. You need an axe to cut it.";
    }

    return result;
}

ItemInfo getBlankItemInfo() {
    ItemInfo result = {};
    return result;
}