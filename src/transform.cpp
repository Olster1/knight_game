struct TransformX {
	float3 pos;
    float3 scale;
    float rotation;
};

float3 getRenderWorldP(float3 p) {
    p.y += p.z;
    return p;
}