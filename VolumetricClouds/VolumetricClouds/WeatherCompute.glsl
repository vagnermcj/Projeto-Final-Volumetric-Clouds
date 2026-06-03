#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform writeonly image2D outWeatherTex;

uniform int altitudePointCount;

uniform float coverageScale;
uniform float heightScale;
uniform float altitudeScale;
uniform vec2 noiseOffset;

layout(std430, binding = 1) buffer AltitudePointsBuffer {
    vec4 altitudePoints[]; // xy = posição, z = valor, w = padding
};

vec2 gradHash2D(vec2 p, float period)
{
    // mod garante wrap correto — p e p+period geram mesmo gradiente
    p = mod(p, period);  
    p = floor(p); 
    vec2 r = vec2(dot(p, vec2(127.1, 311.7)),
                  dot(p, vec2(269.5, 183.3)));
    return normalize(-1.0 + 2.0 * fract(sin(r) * 43758.5453123));
}

float perlin2D(vec2 p, float period)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    // Curva de Hermite C2
    vec2 u = f * f * (3.0 - 2.0 * f);

    float v00 = dot(gradHash2D(i + vec2(0, 0), period), f - vec2(0, 0));
    float v10 = dot(gradHash2D(i + vec2(1, 0), period), f - vec2(1, 0));
    float v01 = dot(gradHash2D(i + vec2(0, 1), period), f - vec2(0, 1));
    float v11 = dot(gradHash2D(i + vec2(1, 1), period), f - vec2(1, 1));

    float result = mix(mix(v00, v10, u.x), mix(v01, v11, u.x), u.y);
    return result * 0.5 + 0.5; // normaliza [-1,1] → [0,1]
}

float perlinFBM2D(vec2 p, float baseScale, float frequency)
{
    float value     = 0.0;
    float amplitude = 0.5;

    for (int o = 0; o < 5; o++) {
        float scale  = floor(baseScale * frequency);  // ← floor aqui
        float period = scale;                          // período = escala
        value     += perlin2D(p * scale, period) * amplitude;
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return clamp(value, 0.0, 1.0);
}

float remap(float v, float s1, float e1, float s2, float e2)
{
    return s2 + (e2 - s2) * clamp((v - s1) / (e1 - s1), 0.0, 1.0);
}


vec2 hash22(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

float worley2D(vec2 uv, float cells) {
    vec2  p        = uv * cells;
    ivec2 baseCell = ivec2(floor(p));
    float minDist  = 1000.0;

    for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++) {
        ivec2 neighbor = baseCell + ivec2(x, y);
        vec2  wrapped  = mod(vec2(neighbor), cells);
        vec2  feature  = hash22(wrapped);
        vec2  featurePos = vec2(neighbor) + feature;
        float d = length(p - featurePos);
        minDist = min(minDist, d);
    }

    return 1.0 - clamp(minDist / sqrt(2.0), 0.0, 1.0); // invertido
}

// ─────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────
void main()
{
    ivec2 id  = ivec2(gl_GlobalInvocationID.xy);
    ivec2 res = imageSize(outWeatherTex);
    if (any(greaterThanEqual(id, res))) return;

    vec2 uv = (vec2(id)) / vec2(res);   

    // ── Canal R: Coverage ──────────────────────────────
    float perlin = perlinFBM2D(uv, floor(coverageScale), 1.0);
    float coverage = smoothstep(0.4, 0.6, perlin);
    // ── Canal G: Height ────────────────────────────────
    float worley = worley2D(uv, heightScale);
    float height = remap(perlin, 1.0 - worley, 1.0, 0.0, 1.0);
    
    imageStore(outWeatherTex, id, vec4(coverage, height, 0.0, 1.0));
}
