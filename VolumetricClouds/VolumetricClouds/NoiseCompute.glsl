#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
layout(rgba32f, binding = 0) uniform writeonly image3D outNoiseTex;

// Um único buffer contendo todas as oitavas sequencialmente
layout(std430, binding = 1) buffer AllPointsBuffer {
    vec3 allPoints[];
};

uniform ivec3 numCells;   // (oitava1, oitava2, oitava3)
uniform ivec3 offsets;    // (0, oitava1^3, oitava1^3 + oitava2^3)
uniform bool isShape;
uniform float perlinScale = 4.0;

// Função Worley acessando o buffer global diretamente
float worley(vec3 uvw, int cells, int bufferOffset) {
    vec3 p = uvw * float(cells);
    ivec3 cellID = ivec3(floor(p));
    float minDist = 1.0;

    for (int z = -1; z <= 1; z++) {
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                ivec3 adjID = cellID + ivec3(x, y, z);
                ivec3 wrappedID = (adjID % cells + cells) % cells;
                
                // Cálculo do índice com o offset da oitava
                int localIdx = wrappedID.x + cells * (wrappedID.y + wrappedID.z * cells);
                int finalIdx = bufferOffset + localIdx;
                
                vec3 pointInCell = allPoints[finalIdx];
                vec3 targetPos = vec3(adjID) + pointInCell;
                float d = distance(p, targetPos);
                minDist = min(minDist, d);
            }
        }
    }
    return clamp(1.0 - minDist, 0.0, 1.0);
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