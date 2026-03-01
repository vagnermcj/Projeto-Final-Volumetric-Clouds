#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
layout(rgba32f, binding = 0) uniform writeonly image3D outNoiseTex;

layout(std430, binding = 1) buffer AllPointsBuffer {
    vec4 allPoints[];
};

uniform ivec3 numCells;
uniform ivec3 offsets;
uniform bool  isShape;
uniform float perlinScale = 4.0;

// ─────────────────────────────────────────────
//  WORLEY NOISE  (já invertido: 1 = perto do feature point)
//  Isso gera formato de nuvem "billowing" diretamente.
// ─────────────────────────────────────────────
float worley(vec3 uvw, int cells, int bufferOffset)
{
    vec3  p        = uvw * float(cells);
    ivec3 baseCell = ivec3(floor(p));
    float minDist  = 1000.0;

    for (int z = -1; z <= 1; z++)
    for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++)
    {
        ivec3 neighbor = baseCell + ivec3(x, y, z);
        ivec3 wrapped  = ivec3(
            (neighbor.x % cells + cells) % cells,
            (neighbor.y % cells + cells) % cells,
            (neighbor.z % cells + cells) % cells
        );

        int   idx     = wrapped.x + cells * (wrapped.y + wrapped.z * cells);
        vec3  feature = allPoints[bufferOffset + idx].xyz;
        vec3  featurePos = vec3(neighbor) + feature;
        float d = length(p - featurePos);
        minDist = min(minDist, d);
    }

    float maxDist = sqrt(3.0);
    // Invertido: 1 perto do ponto, 0 longe → billowing
    return 1.0 - clamp(minDist / maxDist, 0.0, 1.0);
}

// ─────────────────────────────────────────────
//  PERLIN NOISE  (gradient noise, suave e natural)
//  Substitui o hash anterior que gerava ruído sem estrutura.
// ─────────────────────────────────────────────

// Vetores de gradiente pseudo-aleatórios
vec3 gradHash(vec3 p)
{
    p = vec3(dot(p, vec3(127.1, 311.7,  74.7)),
             dot(p, vec3(269.5, 183.3, 246.1)),
             dot(p, vec3(113.5, 271.9, 124.6)));
    return normalize(-1.0 + 2.0 * fract(sin(p) * 43758.5453123));
}

float perlin(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);

    vec3 u = f * f * (3.0 - 2.0 * f);

    float v000 = dot(gradHash(i + vec3(0,0,0)), f - vec3(0,0,0));
    float v100 = dot(gradHash(i + vec3(1,0,0)), f - vec3(1,0,0));
    float v010 = dot(gradHash(i + vec3(0,1,0)), f - vec3(0,1,0));
    float v110 = dot(gradHash(i + vec3(1,1,0)), f - vec3(1,1,0));
    float v001 = dot(gradHash(i + vec3(0,0,1)), f - vec3(0,0,1));
    float v101 = dot(gradHash(i + vec3(1,0,1)), f - vec3(1,0,1));
    float v011 = dot(gradHash(i + vec3(0,1,1)), f - vec3(0,1,1));
    float v111 = dot(gradHash(i + vec3(1,1,1)), f - vec3(1,1,1));

    float result = mix(
        mix(mix(v000, v100, u.x), mix(v010, v110, u.x), u.y),
        mix(mix(v001, v101, u.x), mix(v011, v111, u.x), u.y),
        u.z
    );

    return result * 0.5 + 0.5;
}

float perlinFBM(vec3 p)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int o = 0; o < 4; o++) {
        value     += perlin(p * frequency) * amplitude;
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// ─────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────
void main()
{
    ivec3 id  = ivec3(gl_GlobalInvocationID.xyz);
    ivec3 res = imageSize(outNoiseTex);
    if (any(greaterThanEqual(id, res))) return;

    vec3 uvw = vec3(id) / vec3(res);

    float w1 = worley(uvw, numCells.x, offsets.x);
    float w2 = worley(uvw, numCells.y, offsets.y);
    float w3 = worley(uvw, numCells.z, offsets.z);

    if (isShape) {
        float p = perlinFBM(uvw * perlinScale);
        imageStore(outNoiseTex, id, vec4(p, w1, w2, w3));
    } else {
        imageStore(outNoiseTex, id, vec4(w1, w2, w3, 1.0));
    }
}
