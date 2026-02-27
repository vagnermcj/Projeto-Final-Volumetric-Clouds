#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
layout(rgba32f, binding = 0) uniform writeonly image3D outNoiseTex;

layout(std430, binding = 1) buffer AllPointsBuffer {
    vec4 allPoints[];
};

uniform ivec3 numCells;   
uniform ivec3 offsets;    
uniform bool isShape;
uniform float perlinScale = 4.0;

float worley(vec3 uvw, int cells, int bufferOffset)
{
    vec3 p = uvw * float(cells);
    ivec3 baseCell = ivec3(floor(p));
    float minDist = 1000.0;

    for (int z = -1; z <= 1; z++)
    for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++)
    {
        ivec3 neighbor = baseCell + ivec3(x,y,z);

        ivec3 wrapped = ivec3(
            (neighbor.x % cells + cells) % cells,
            (neighbor.y % cells + cells) % cells,
            (neighbor.z % cells + cells) % cells
        );

        int idx = wrapped.x +
                  cells * (wrapped.y + wrapped.z * cells);

        vec3 feature = allPoints[bufferOffset + idx].xyz;

        vec3 featurePos = vec3(neighbor) + feature;

        float d = length(p - featurePos);
        minDist = min(minDist, d);
    }

    float maxDist = sqrt(3.0);
    return 1.0 - clamp(minDist / maxDist, 0.0, 1.0);
}

// Implementação básica de Perlin para o canal R
float perlin(vec3 p) {
    // Para fins de teste, você pode usar um ruído simples ou implementar o Grad/Fade
    return fract(sin(dot(p, vec3(12.9898, 78.233, 45.164))) * 43758.5453); 
}

void main() {
    ivec3 id = ivec3(gl_GlobalInvocationID.xyz);
    ivec3 res = imageSize(outNoiseTex);
    if (any(greaterThanEqual(id, res))) return;

    vec3 uvw = vec3(id) / vec3(res);
    
    // Calcula as 3 oitavas usando os offsets passados pela CPU
    float w1 = worley(uvw, numCells.x, offsets.x);
    float w2 = worley(uvw, numCells.y, offsets.y);
    float w3 = worley(uvw, numCells.z, offsets.z);

    if (isShape) {
        float p = perlin(uvw * perlinScale);
        imageStore(outNoiseTex, id, vec4(p, w1, w2, w3));
    } else {
        imageStore(outNoiseTex, id, vec4(w1, w2, w3, 1.0));
    }
}