struct InventoryItem {
    int count;
    PickupItemType type;
};

struct Inventory {
    InventoryItem items[64];
};

bool addToInventory(Inventory *inventory, PickupItemType type) {
    bool found = false;
    int emptyIndex = -1;
    for(int i = 0; i < arrayCount(inventory->items) && !found; ++i) {
        if(inventory->items[i].type == type) {
            found = true;
            inventory->items[i].count++;
        } else if(emptyIndex < 0 && inventory->items[i].type == PICKUP_ITEM_NONE) {
            emptyIndex = i;
        }
    }

    if(!found && emptyIndex >= 0) {
        inventory->items[emptyIndex].count++;
        inventory->items[emptyIndex].type = type;
        found = true;
    }

    return found;
}