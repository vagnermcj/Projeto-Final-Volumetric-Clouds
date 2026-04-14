#version 410 core
out vec4 FragColor;
in  vec2 TexCoord;

// ─── Câmera ───────────────────────────────────────────────────────────────────
uniform mat4 camMatrix;
uniform vec3 camPos;

// ─── Atmosfera / Planeta ──────────────────────────────────────────────────────
uniform float planetRadius;
uniform float innerCloudRadius;
uniform float outerCloudRadius;
uniform float atmosphereStart;
uniform float atmosphereHeight;
uniform float atmosphereMaxDepth;

// ─── Densidade ────────────────────────────────────────────────────────────────
uniform float weatherScale;
uniform float maxCloudHeight;
uniform float maxCloudAltitude;
uniform float detailNoiseWeight;
uniform vec4  shapeNoiseWeights;
uniform int   cloudMaxSteps;
uniform float shapeScale;
uniform float detailScale;

// ─── Iluminação ───────────────────────────────────────────────────────────────
uniform vec3  lightDirection;
uniform vec3  lightColor;
uniform int   lightSteps;
uniform float phase;
uniform float sigmaScattering;
uniform float sigmaExtinction;

// ─── Vento / Tempo ────────────────────────────────────────────────────────────
uniform vec3  windDirection;
uniform float windSpeed;
uniform float time;

// ─── Texturas ─────────────────────────────────────────────────────────────────
uniform sampler3D  shapeNoise;
uniform sampler3D  detailNoise;
uniform sampler2D  weatherMap;
uniform samplerCube skybox;

// ═════════════════════════════════════════════════════════════════════════════
//  Utilitários
// ═════════════════════════════════════════════════════════════════════════════

float remap(float v, float s1, float e1, float s2, float e2) {
    return s2 + (e2 - s2) * clamp((v - s1) / (e1 - s1), 0.0, 1.0);
}

float HG(float cosTheta, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * 3.14159265 * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 planetCenter() {
    return vec3(0.0, -planetRadius, 0.0);
}

// Altura real do ponto p acima da superfície do planeta
float altitudeOf(vec3 p) {
    return length(p - planetCenter()) - planetRadius;
}

// UV do weather map para o ponto p (com animação de vento)
vec2 weatherUV(vec3 p) {
    return vec2(p.x, p.z) / weatherScale
         + vec2(windDirection.x, windDirection.z) * time * windSpeed;
}

// ═════════════════════════════════════════════════════════════════════════════
//  §5 Densidade
// ═════════════════════════════════════════════════════════════════════════════

float getCloudShape(vec3 p)
{
    // UVW escalado + offset de vento para o shape
    vec3 shapeUVW = (p / shapeScale) + windDirection * time * windSpeed * 0.95;

    vec4 noise = texture(shapeNoise, shapeUVW);

    float perlin    = noise.r * shapeNoiseWeights.r;
    float worleyFBM = noise.g * shapeNoiseWeights.g + noise.b * shapeNoiseWeights.b 
                              + noise.a * shapeNoiseWeights.a;

    return remap(perlin, worleyFBM, 1.0, 0.0, 1.0);
}

float getCloudDetail(vec3 p)
{
    vec3 detailUVW = (p / detailScale) + windDirection * time * windSpeed * 1.2;

    vec3 s = texture(detailNoise, detailUVW).rgb;
    return s.r * 0.625 + s.g * 0.25 + s.b * 0.125;
}

// §5.1 — Height Signal: parábola que define onde a nuvem existe verticalmente
float HeightSignal(vec3 p, float height, float altitude)
{
    float x             = altitudeOf(p) - atmosphereStart;
    float oneOverHeight = 1.0 / (height * height);
    float signal        = (x - altitude) * (x - altitude - height) * (oneOverHeight * -4.0);
    return clamp(signal, 0.0, 1.0);
}

// Height Gradient: nuvem mais densa no topo, mais fina na base
float HeightGradient(vec3 p, float height, float altitude)
{
    float x              = altitudeOf(p) - atmosphereStart;
    float heightPercent  = clamp((x - altitude) / height, 0.0, 1.0);
    return remap(heightPercent, 0.0, 1.0, 0.1, 1.0);
}

float cloudDensity(vec3 p)
{
    float dist = length(p - planetCenter());
    if (dist < innerCloudRadius || dist > outerCloudRadius) return 0.0;

    vec3  weather  = texture(weatherMap, weatherUV(p)).rgb;
    float coverage = weather.r;

    float height   = weather.g * maxCloudHeight;
    float altitude = weather.b * maxCloudAltitude;

    // §4.1 — sequência exata do paper
    float density = coverage;
    density      *= HeightSignal(p, height, altitude);
    if (density < 0.001) return 0.0;

    density      *= getCloudShape(p); 
    if (density <= 0.0) return 0.0;

    // Erosão
    // Erosão com peso cúbico nas bordas
    float oneMinusShape = 1.0 - getCloudShape(p); // bordas = valor alto
    float erodeWeight   = oneMinusShape * oneMinusShape * oneMinusShape;
    float detailFBM     = getCloudDetail(p);
    density            -= detailFBM * erodeWeight * detailNoiseWeight;
    

    density *= HeightGradient(p, height, altitude);
    return clamp(density, 0.0, 1.0);
}

// ═════════════════════════════════════════════════════════════════════════════
//  §5.2 Iluminação — Light Marching
// ═════════════════════════════════════════════════════════════════════════════

float lightMarching(vec3 pos)
{
    float totalDensity = 0.0;
    float stepSize     = 1.0;

    for (int i = 0; i < lightSteps; i++)
    {
        vec3  p    = pos + normalize(lightDirection) * stepSize * float(i + 1);
        float dist = length(p - planetCenter());

        if (dist > outerCloudRadius || dist < innerCloudRadius) break;

        totalDensity += max(0.0, cloudDensity(p)) * stepSize;
        stepSize     *= 1.1;
    }

    return exp(-sigmaExtinction * totalDensity);
}

// ═════════════════════════════════════════════════════════════════════════════
//  §5.3 Ray Marching — Casca esférica
// ═════════════════════════════════════════════════════════════════════════════

vec2 raySphereIntersect(vec3 ro, vec3 rd, float radius)
{
    vec3  oc   = ro - planetCenter();
    float b    = dot(oc, rd);
    float c    = dot(oc, oc) - radius * radius;
    float disc = b * b - c;

    if (disc < 0.0) return vec2(1e9, -1e9);

    float sq = sqrt(disc);
    return vec2(-b - sq, -b + sq);
}

vec2 getAtmosphereRange(vec3 ro, vec3 rd)
{
    vec2 outer = raySphereIntersect(ro, rd, outerCloudRadius);
    if (outer.y < 0.0) return vec2(1e9, -1e9);

    vec2  inner   = raySphereIntersect(ro, rd, innerCloudRadius);
    float camDist = length(ro - planetCenter());

    float tStart, tEnd;

    if (camDist < innerCloudRadius) {
        // Câmera abaixo das nuvens — entra pela esfera interna, sai pela externa
        tStart = max(inner.y, 0.0);
        tEnd   = outer.y;
    } else if (camDist < outerCloudRadius) {
        // Câmera dentro da casca
        tStart = 0.0;
        // Se olhando para baixo, termina ao atingir a esfera interna
        // Se olhando para cima, termina ao sair pela esfera externa
        tEnd = (inner.x > 0.0) ? inner.x : outer.y;
    } else {
        // Câmera acima das nuvens — entra pela externa, sai pela interna
        if (outer.x < 0.0) return vec2(1e9, -1e9);
        tStart = outer.x;
        tEnd   = (inner.x > 0.0) ? inner.x : outer.y;
    }

    float maxDist = planetRadius;
    tEnd = min(tEnd, tStart + maxDist);

    return vec2(max(tStart, 0.0), tEnd);
}

vec3 rayMarch(vec3 ro, vec3 rd)
{
    vec3 skyColor = texture(skybox, rd).rgb;

    vec2  range  = getAtmosphereRange(ro, rd);
    float tStart = range.x;
    float tEnd   = range.y;

    if (tStart >= tEnd ) return skyColor;
    float minDepth = min((tEnd - tStart) , atmosphereMaxDepth);
    float stepSize      = minDepth / float(cloudMaxSteps);

    float jitter = rand(gl_FragCoord.xy) * stepSize;
    float t      = tStart + jitter; 


    float transmittance = 1.0;
    vec3  lightEnergy   = vec3(0.0);
    float cosTheta      = dot(normalize(lightDirection), normalize(rd));
    float phaseVal = HG(cosTheta, phase);


    for (int i = 0; i < cloudMaxSteps; i++)
    {
        if (t >= tEnd || transmittance < 0.01) break;

        vec3  p         = ro + rd * t;
        if(p.y < 0.0f) return skyColor;

        float cloudDens = cloudDensity(p);
        if (cloudDens < 0.01)
        {
            t += stepSize * 1.5;
            continue;
        }

        if (cloudDens > 0.0)
        {
            float sampleSigmaS = sigmaScattering * cloudDens;
            float sampleSigmaE = sigmaExtinction * cloudDens;

            float lightT       = lightMarching(p);
            float powderEffect = 1.0 - exp(-cloudDens * 2.0);

            vec3  ambient = lightColor * 0.9f;
            vec3  S       = (lightColor * lightT * phaseVal * powderEffect + ambient) * sampleSigmaS;

            float Tr      = exp(-sampleSigmaE * stepSize);
            vec3  Sint    = (S - S * Tr) / sampleSigmaE;

            lightEnergy   += transmittance * Sint;
            transmittance *= Tr;
        }

        t += stepSize;
    }

    return lightEnergy + skyColor * transmittance;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Main
// ═════════════════════════════════════════════════════════════════════════════

void main()
{
    vec2 uv        = TexCoord * 2.0 - 1.0;
    vec4 ray_clip  = vec4(uv, -1.0, 1.0);
    vec4 ray_world = inverse(camMatrix) * ray_clip;
    vec3 rd        = normalize(ray_world.xyz / ray_world.w - camPos);
    vec3 ro        = camPos;

    int debug = 0; // 0: Clouds | 1: Shape Noise | 2: Shape Noise (R)
    switch (debug) {
        case 0:
            FragColor = vec4(rayMarch(ro, rd), 1.0);
            return;
        case 1:
            FragColor = texture(shapeNoise, vec3(TexCoord, 0.5));
            return;
        case 2:
            FragColor = vec4(vec3(texture(shapeNoise, vec3(TexCoord, 0.5)).r), 1.0);
            return;
    }
}
