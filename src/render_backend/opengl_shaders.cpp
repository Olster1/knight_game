static char *quadVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec2 texUV;	"

//per instanced variables
"in vec3 pos;"
"in vec4 uvAtlas;"
"in vec4 color;"
"in vec2 scale;"
"in float samplerIndex;"
"in uint aoMask;"

//uniform variables
"uniform mat4 projection;"

//outgoing variables
"out vec4 color_frag;"
"out vec2 uv_frag;"
"out float texture_array_index;"
"out vec3 viewP;"

"void main() {"
    "vec3 p = vertex;"

    "p.x *= scale.x;"
    "p.y *= scale.y;"

    "p += pos;"

    "viewP = p;"

    "gl_Position = projection * vec4(p, 1.0f);"
    "color_frag = color;"
    "texture_array_index = samplerIndex;"

    "uv_frag = vec2(mix(uvAtlas.x, uvAtlas.z, texUV.x), mix(uvAtlas.y, uvAtlas.w, texUV.y));"
"}";

static char* fragCloudShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec2 uv_frag; "
"in float AOValue;"
"in vec3 viewP;"
"uniform sampler2D diffuse;"
"uniform float totalTime;"
"out vec4 color;"

"vec2 fade(vec2 t) {"
    "return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);"
"}"

"float hash(vec2 p) {"
    "return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);"
"}"

"float grad(vec2 p, vec2 offset) {"
    "return hash(p + offset);"
"}"

"float perlin(vec2 p) {"
    "vec2 i = floor(p);"
    "vec2 f = fract(p);"

    "vec2 u = fade(f);"

    "float a = hash(i + vec2(0.0, 0.0));"
    "float b = hash(i + vec2(1.0, 0.0));"
    "float c = hash(i + vec2(0.0, 1.0));"
    "float d = hash(i + vec2(1.0, 1.0));"

    "float res = mix(mix(a, b, u.x), mix(c, d, u.x), u.y);"
    "return res;"
"}"
//
// Main Cloud Shader
//
"void main() {"
    // Scale and move the noise field
    "vec2 pos = uv_frag * 3.0 + vec2(10*totalTime * 0.05, 10*totalTime * 0.01) + viewP.xy;"

    // Stack multiple layers of Perlin noise for more richness
    "float noise = 0.0;"
    "float amplitude = 0.5;"
    "float frequency = 1.0;"
    "for (int i = 0; i < 5; i++) {"
        "noise += perlin(pos * frequency) * amplitude;"
        "frequency *= 2.0;"
        "amplitude *= 0.5;"
    "}"

    // Cloud threshold
    "float cloud = smoothstep(0.4, 0.7, noise);"

    // Color the clouds
    "vec3 skyColor = vec3(0.6, 0.8, 1.0);"
    "vec3 cloudColor = vec3(1.0);"

    "vec3 finalColor = mix(skyColor, cloudColor, cloud);"

    "color = color_frag*vec4(finalColor, 1.0);"
"}";

static char *quadAoMaskVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec2 texUV;	"

//per instanced variables
"in vec3 pos;"
"in vec4 uvAtlas;"
"in vec4 color;"
"in vec2 scale;"
"in float samplerIndex;"
"in uint aoMask;"

//uniform variables
"uniform mat4 projection;"

//outgoing variables
"out vec4 color_frag;"
"out vec2 uv_frag;"
"out float texture_array_index;"
"out float AOValue;"

"float aoFactors[4] = float[4](1, 0.6, 0.5, 0.3);"

"void main() {"
    "vec3 p = vertex;"

    "p.x *= scale.x;"
    "p.y *= scale.y;"

    "p += pos;"

    "gl_Position = projection * vec4(p, 1.0f);"
    "color_frag = color;"
    "texture_array_index = samplerIndex;"

    "uint aoIndex = uint(3);"
    "uint mask = aoMask >> uint(gl_VertexID*2);"
    "AOValue = aoFactors[mask & aoIndex];"

    "uv_frag = vec2(mix(uvAtlas.x, uvAtlas.z, texUV.x), mix(uvAtlas.y, uvAtlas.w, texUV.y));"
"}";

static char *pixelArtAoMaskFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec2 uv_frag; "
"in float AOValue;"
"uniform sampler2D diffuse;"
"out vec4 color;"
"void main() {"
    "vec2 size = textureSize(diffuse, 0);"
    "vec2 uv = uv_frag * size;"
    "vec2 duv = fwidth(uv);"
    "uv = floor(uv) + 0.5 + clamp(((fract(uv) - 0.5 + duv)/duv), 0.0, 1.0);"
    "uv /= size;"
    "vec4 sample = texture(diffuse, uv);"
    "sample = vec4(AOValue*sample.xyz, sample.w);"
   
    "color = sample*color_frag;"
"}";


static char *rectOutlineVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec2 texUV;	"

//per instanced variables
"in vec3 pos;"
"in vec4 uvAtlas;"
"in vec4 color;"
"in vec2 scale;"
"in float samplerIndex;"

//uniform variables
"uniform mat4 projection;"

//outgoing variables
"out vec4 color_frag;"
"out vec2 uv_frag;"
"out float texture_array_index;"
"out vec2 scale_world_space;"

"void main() {"
    "vec3 p = vertex;"

    "p.x *= scale.x;"
    "p.y *= scale.y;"

    "p += pos;"

    "gl_Position = projection * vec4(p, 1.0f);"
    "color_frag = color;"
    "texture_array_index = samplerIndex;"

    "uv_frag = texUV;"
    "scale_world_space = scale;"
"}";

static char *lineVertexShader = 
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec2 texUV;	"

//per instanced variables
"in vec3 pos1;"
"in vec3 pos2;"
"in vec4 color1;"

//uniform variables
"uniform mat4 projection;"

//outgoing variables
"out vec4 color_frag;"

"void main() {"
    "vec3 pos = vertex.x * pos1 + vertex.y * pos2;"
    "gl_Position = projection * vec4(pos, 1.0f);"
    "color_frag = color1;"
"}";

static char *rectOutlineFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec2 uv_frag;"
"in vec2 scale_world_space;"
"out vec4 colorOut;"
"void main() {"
    "float outline_width = 2;    "
    
    "float alpha_value = 0.0f;"

    "vec4 color = color_frag;"

    "float xAt = uv_frag.x*scale_world_space.x;"
    "float yAt = uv_frag.y*scale_world_space.y;"
    "if(xAt < outline_width || (scale_world_space.x - xAt) < outline_width || yAt < outline_width || (scale_world_space.y - yAt) < outline_width) {"
        "alpha_value = 1.0f;"
    "}"
    "color.a = alpha_value;"

    "colorOut = color;"
"}";


static char *quadTextureFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec2 uv_frag; "
"uniform sampler2D diffuse;"
"out vec4 color;"
"void main() {"
    "vec4 diffSample = texture(diffuse, uv_frag);"
    "color = diffSample*color_frag;"
"}";

static char *sdfFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec2 uv_frag; "
"uniform sampler2D diffuse;"
"out vec4 colorOut;"
"void main() {"
    "float smoothing = 0.2f;"
    "float boldness = 0.3f;"
    "vec4 sample = texture(diffuse, uv_frag); "

    "float distance = sample.a;"

    "float alpha = smoothstep(1.0f - boldness, (1.0f - boldness) + smoothing, distance);"

    "vec4 color = alpha * color_frag;"

    "color.xyz /= color.a;"

    "if(color.a == 0) {"
        "discard;"
    "}"

    "colorOut = color;"
"}";

static char *pixelArtLightsFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec2 uv_frag; "
"in vec3 viewP; "
"uniform sampler2D diffuse;"
"out vec4 color;"

"struct PointLight {"
    "vec3 position;"
    "vec3 color;"
"};"

"uniform PointLight lights[64];"
"uniform int lightCount;"
"uniform float dayNightValue;"

"void main() {"
    "vec2 size = textureSize(diffuse, 0);"
    "vec2 uv = uv_frag * size;"
    "vec2 duv = fwidth(uv);"
    "uv = floor(uv) + 0.5 + clamp(((fract(uv) - 0.5 + duv)/duv), 0.0, 1.0);"
    "uv /= size;"
    "vec4 sample = texture(diffuse, uv);"

    "if(sample.w == 0) {"
        "discard;"
    "}"

    "vec3 totalLight = vec3(4*dayNightValue);"

    "for (int i = 0; i < lightCount; i++) {"
        "vec2 toLight = lights[i].position.xy - viewP.xy;"
        "float dist = length(toLight);"

        "float attenuation = 1.0 / (dist * dist);"

        "totalLight +=  lights[i].color*vec3(attenuation);"
    "}"

    "vec3 mapped = totalLight / (totalLight + vec3(1.0));"

    "color = sample*color_frag*vec4(mapped, 1);"
"}";


static char *pixelArtFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"in vec2 uv_frag; "
"in vec3 viewP; "
"uniform sampler2D diffuse;"
"out vec4 color;"

"void main() {"
    "vec2 size = textureSize(diffuse, 0);"
    "vec2 uv = uv_frag * size;"
    "vec2 duv = fwidth(uv);"
    "uv = floor(uv) + 0.5 + clamp(((fract(uv) - 0.5 + duv)/duv), 0.0, 1.0);"
    "uv /= size;"
    "vec4 sample = texture(diffuse, uv);"

    "if(sample.w == 0) {"
        "discard;"
    "}"

    "color = sample*color_frag;"
"}";


static char *lineFragShader = 
"#version 330\n"
"in vec4 color_frag;" 
"out vec4 color;"
"void main() {"
    "color = color_frag;"
"}";