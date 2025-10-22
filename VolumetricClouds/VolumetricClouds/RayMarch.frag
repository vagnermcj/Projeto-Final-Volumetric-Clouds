#version 410 core
out vec4 FragColor;
in vec2 TexCoord;

uniform mat4 camMatrix;
uniform vec3 camPos;

float sphereDensity(vec3 p) {
    float sphere = 1.0 - length(p - vec3(0.0, 1.0, -3.0));
    return clamp(sphere, 0.0, 1.0);
}


float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float boxDensity(vec3 p)
{
    vec3 boxCenter = vec3(2.0, 0.5, -4.0);
    vec3 boxSize   = vec3(0.5);
    float d = sdBox(p - boxCenter, boxSize);

    // transforma distância em densidade
    float dens = smoothstep(0.0, boxSize.x * 0.5, -d);
    dens = pow(dens, 2.0);
    return clamp(dens, 0.0, 1.0);
}


float sphereSDF(vec3 p, float r) {
    return length(p) - r;
}

float planeSDF(vec3 p) {
    return p.y;
}

float sceneSDF(vec3 p) {
    vec3 spherePos = vec3(0.0, 1.0, -3.0);
    vec3 boxPos = vec3(2.0, 0.5, -4.0);
    vec3 groundPos = vec3(0.0, -0.5, 0.0);
    float dBox = sdBox(p - boxPos, vec3(0.5, 0.5, 0.5)); //Caixa 1x1x1
    float dSphere = sphereSDF(p - spherePos, 1.0);
    float dPlane  = planeSDF(p - groundPos);
    return dBox;
}

// Ray marching
vec3 rayMarch(vec3 ro, vec3 rd) {
    float t = 0.0;
    vec3 color = vec3(0.0);
    vec3 bgColor = vec3(0.85, 0.9, 1.0); // cor do céu
    vec3 lightColor = vec3(1.0, 0.0, 0.0); //Cor da luz
    float transmittance = 1.0;
    float absorptionCoefficient = 0.05; //Coeficiente de absorção da luz
    vec3 lightDir = normalize(vec3(0.0,0.0,0.0)); // direção da luz
    float cloudStepSize = 0.05; // tamanho do passo para nuvens
    float extinction = 0.1; // taxa de extinção da luz
    float totalDensity = 0.0; // densidade total acumulada


    for (int i = 0; i < 124; i++) {
        vec3 p = ro + rd * t;
        float d = sceneSDF(p);
        if (d < 0.001) //Inside the cloud
        {
            //Substituir depois pela densidade do perlin noise
            float boxDens = boxDensity(p);
            //color += vec3(1.0) * boxDens * 0.05;
            color += lightColor * boxDens * transmittance * cloudStepSize;
            transmittance *= exp(absorptionCoefficient * -cloudStepSize);
            t += cloudStepSize;
        }
        else //Outside the cloud
        {
            t += d;
        }

        if(transmittance < 0.01) break;
        if (t > 100.0) break; // muito longe
    }
    return mix(bgColor, color, 0.5);
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

    vec3 color = rayMarch(ro, rd);

    FragColor = vec4(color, 1.0);
}
