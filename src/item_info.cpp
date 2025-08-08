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

    } else if(e->type == ENTITY_PICKUP_ITEM) {
        if(e->pickupItemType == PICKUP_ITEM_SKINNING_KNIFE) {
            result.title = "Skinning Knife";
            result.description = "A knife with a curved blade. Could be good for skinning pelts.";
        } else if(e->pickupItemType == PICKUP_ITEM_BEAR_PELT) {
            result.title = "Bear Pelt";
            result.description = "A bear pelt. Looks like useful craft item.";
        }
        
    }

    return result;
}

ItemInfo getBlankItemInfo() {
    ItemInfo result = {};
    return result;
}