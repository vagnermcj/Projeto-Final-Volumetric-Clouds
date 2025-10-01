#version 410 core
out vec4 FragColor;
in vec2 TexCoord;

uniform mat4 camMatrix;
uniform vec3 camPos;


// Função de distância: esfera
float sphereSDF(vec3 p, float r) {
    return length(p) - r;
}

// Função de distância: plano (y=0)
float planeSDF(vec3 p) {
    return p.y;
}

// Combinação: pega o menor valor
float sceneSDF(vec3 p) {
    float dSphere = sphereSDF(p - vec3(0.0, 1.0, -3.0), 1.0);
    float dPlane  = planeSDF(p);
    return min(dSphere, dPlane);
}

// Ray marching
float rayMarch(vec3 ro, vec3 rd) {
    float t = 0.0;
    for (int i = 0; i < 64; i++) {
        vec3 p = ro + rd * t;
        float d = sceneSDF(p);
        if (d < 0.001) return t; // bateu
        t += d;
        if (t > 100.0) break; // muito longe
    }
    return -1.0; // nada encontrado
}

void main()
{
    // coordenadas normalizadas (-1 a 1)
    vec2 uv = TexCoord * 2.0 - 1.0;
    uv.x *= 800.0/800.0; // aspect ratio (substituir pela largura/altura reais)

    // Constrói raio em espaço de clip
    vec4 ray_clip = vec4(uv, -1.0, 1.0);

    // Converte para espaço de mundo
    vec4 ray_world = inverse(camMatrix) * ray_clip;
    vec3 rd = normalize(ray_world.xyz / ray_world.w - camPos);

    vec3 ro = camPos; // origem = posição da câmera

    float t = rayMarch(ro, rd);

    if (t > 0.0)
        FragColor = vec4(0.6, 0.6, 0.6, 1.0); // cor do hit
    else
        FragColor = vec4(0.0, 0.0, 0.0, 1.0); // fundo preto
}
