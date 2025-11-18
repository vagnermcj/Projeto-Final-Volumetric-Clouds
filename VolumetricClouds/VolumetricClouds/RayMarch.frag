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
            //Substituir depois pela densidade do perlin noise
            float boxDens = sceneSDF(p);

            color += lightColor * -boxDens * transmittance * cloudStepSize;
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
    return mix(backgroundColor, color, 0.5);
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
