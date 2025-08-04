struct FloodFillEvent {
    int x; 
    int y;
	int z;
    FloodFillEvent *next;
    FloodFillEvent *prev;
};

struct NodeDirection {
	float3 p;
	NodeDirection *next;
};

struct FloodFillResult {
	FloodFillEvent *foundNode;
	NodeDirection *cameFrom;
};