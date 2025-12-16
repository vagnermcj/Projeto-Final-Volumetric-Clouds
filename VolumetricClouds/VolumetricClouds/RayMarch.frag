#version 410 core
out vec4 FragColor;
in vec2 TexCoord;

uniform mat4 camMatrix;
uniform vec3 camPos;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 backgroundColor;
uniform float absorptionCoefficient;
uniform float cloudStepSize;
uniform int cloudMaxSteps;
uniform vec3 cloudPosition;
uniform vec3 cloudScale;
uniform float time;
uniform float windSpeed;
uniform vec3 windDirection;
uniform float cloudCoverage;
uniform sampler3D perlinTex;
uniform sampler3D worleyTex;


float sphereDensity(vec3 p) {
    float sphere = 1.0 - length(p - vec3(0.0, 1.0, -3.0));
    return clamp(sphere, 0.0, 1.0);
}


float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float sdEllipsoid( vec3 p, vec3 r )
{
  float k0 = length(p/r);
  float k1 = length(p/(r*r));
  return k0*(k0-1.0)/k1;
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
    
    float dEllipsoid = sdEllipsoid(p - cloudPosition, cloudScale);
    
    return dEllipsoid;
}

float remap(float value, float start1, float stop1, float start2, float stop2)
{
    float outgoing = start2 + (stop2 - start2) * ((value - start1) / (stop1 - start1));
    return outgoing;
}

float cloudDensity(vec3 p, float sdf)
{
    // 1. Coordenadas com vento
    vec3 q = p - windDirection * time * windSpeed;

    // --- A BASE DA NUVEM (SHAPE) ---
    
    // Leitura do Perlin (Baixa frequência - forma geral)
    float perlin = texture(perlinTex, q * 0.01).r;
    
    // Leitura do Worley (Média frequência - os "gomos")
    // IMPORTANTE: Inverta o Worley para ter "bolhas" em vez de "células"
    float worley = 1.0 - texture(worleyTex, q * 0.05).r; // Ajuste a escala (0.02) conforme necessário
    
    // Combinar: O Perlin define onde "pode" ter nuvem, o Worley define o formato "arredondado"
    // Essa técnica é chamada de "Perlin-Worley"
    float baseCloud = remap(perlin, 0.0, 1.0, worley, 1.0); 
    // Ou uma mistura simples para testar: float baseCloud = mix(perlin, worley, 0.5);

    // --- DEFININDO A ESPARSIDADE (COVERAGE) ---
    
    // Aqui está a mágica para deixar "esparso".
    // Usamos o remap para cortar tudo que estiver abaixo do cloudCoverage.
    // cloudCoverage alto = nuvens menores e mais separadas.
    // cloudCoverage baixo = céu nublado (overcast).
    float density = remap(baseCloud, cloudCoverage, 1.0, 0.0, 1.0);
    
    // Se a densidade for 0 ou negativa após o remap, não tem nuvem aqui.
    if (density <= 0.0) return 0.0;

    // --- ENVELOPE DO ELIPSOIDE ---
    // O elipsoide agora serve apenas como "container" máximo, não como forma principal.
    // Suavizamos as bordas do elipsoide para a nuvem não ser cortada abruptamente.
    float envelope = smoothstep(0.0, -0.5, sdf); 
    
    density *= envelope;

    return clamp(density, 0.0, 1.0);
}


// Ray marching
vec3 rayMarch(vec3 ro, vec3 rd) {
    
    //Variaveis
    float t = 0.0;
    vec3 color = vec3(0.0);
    float transmittance = 1.0;
    float totalDensity = 0.0; // densidade total acumulada


    for (int i = 0; i < cloudMaxSteps; i++) {
        vec3 p = ro + rd * t;
        float d = sceneSDF(p);
        if (d < 0.001) //Inside the cloud
        {
            float cloudDens = cloudDensity(p, d);

            color += lightColor * cloudDens * transmittance * cloudStepSize * 0.5;
            transmittance *= exp(absorptionCoefficient * -cloudStepSize);
            t += cloudStepSize;
        }
        else //Outside the cloud
        {
            t += d;
        }

        if(transmittance < 0.1) break;
        if (t > 100.0) break; // muito longe
    }
    return mix(backgroundColor, color, 0.5);
}


void main()
{
    // Normaliza UV
    vec2 uv = TexCoord * 2.0 - 1.0;

    // Ray em espaço de clip
    vec4 ray_clip = vec4(uv, -1.0, 1.0);

    // Para world space
    vec4 ray_world = inverse(camMatrix) * ray_clip;
    vec3 rd = normalize(ray_world.xyz / ray_world.w - camPos);
    vec3 ro = camPos;

    vec3 color = rayMarch(ro, rd);

    FragColor = vec4(color, 1.0);
}
