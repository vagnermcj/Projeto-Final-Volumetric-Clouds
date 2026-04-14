#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform writeonly image2D outWeatherTex;

uniform int altitudePointCount;

// Escala de cada canal — frequências bem diferentes entre si
uniform float coverageScale;
uniform float heightScale;
uniform float altitudeScale;

// Controle do threshold de cobertura
uniform float coverageMin;
uniform float coverageMax;

layout(std430, binding = 1) buffer AltitudePointsBuffer {
    vec4 altitudePoints[]; // xy = posição, z = valor, w = padding
};

// ─────────────────────────────────────────────
//  PERLIN 2D TILEABLE
// ─────────────────────────────────────────────
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

float perlinFBM2D(vec2 p, float baseScale)
{
    float value     = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

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

float nearestAltitude(vec2 uv)
{
    for (int i = 0; i < altitudePointCount; i++) {
        vec2  pos    = altitudePoints[i].xy;
        float val    = altitudePoints[i].z;
        float radius = altitudePoints[i].w;

        // Distância com wrap toroidal
        vec2  diff = abs(uv - pos);
        diff = min(diff, 1.0 - diff);
        float d = length(diff);

        if (d < radius) {
            return val;
        }
    }

    return 0.0; // fora de todas as manchas → nuvem na base
}

// ─────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────
void main()
{
    ivec2 id  = ivec2(gl_GlobalInvocationID.xy);
    ivec2 res = imageSize(outWeatherTex);
    if (any(greaterThanEqual(id, res))) return;

    vec2 uv = vec2(id) / vec2(res); // [0, 1]

    // ── Canal R: Coverage ──────────────────────────────
    float coverage = perlinFBM2D(uv, coverageScale);
    coverage = smoothstep(coverageMin, coverageMax, coverage);

    // ── Canal G: Height ────────────────────────────────
    // Offset no UV garante que não correlaciona com coverage
    float height = perlinFBM2D(uv , heightScale);
    
    height = smoothstep(coverageMin, coverageMax, height);

    // ── Canal B: Altitude ──────────────────────────────
    float altitude = perlinFBM2D(uv , heightScale);

    imageStore(outWeatherTex, id, vec4(coverage, height, altitude, 1.0));
}
