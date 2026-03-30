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

// ─── Densidade ────────────────────────────────────────────────────────────────
uniform float cloudMinCoverage;
uniform float weatherScale;
uniform float maxCloudHeight;
uniform float maxCloudAltitude;
uniform float erosionThreshold;
uniform vec4  shapeNoiseWeights;
uniform int   cloudMaxSteps;

// ─── Iluminação ───────────────────────────────────────────────────────────────
uniform vec3  lightDirection;
uniform vec3  lightColor;
uniform int   lightSteps;
uniform float absorptionCoefficient;
uniform float scatteringCoefficient;

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
         - vec2(windDirection.x, windDirection.z) * time * windSpeed;
}

// ═════════════════════════════════════════════════════════════════════════════
//  §5 Densidade
// ═════════════════════════════════════════════════════════════════════════════

float getCloudShape(vec3 p)
{
    vec4 noise             = texture(shapeNoise, p);
    vec4 normalizedWeights = shapeNoiseWeights / dot(shapeNoiseWeights, vec4(1.0));
    return 1.0 - dot(noise, normalizedWeights);
}

float getCloudDetail(vec3 p)
{
    vec3 s = texture(detailNoise, p).rgb;
    return 1.0 - (s.r * 0.625 + s.g * 0.25 + s.b * 0.125);
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
    // Guard: fora da casca esférica
    float dist = length(p - planetCenter());
    if (dist < innerCloudRadius || dist > outerCloudRadius) return 0.0;

    // Weather map
    vec3  weather  = texture(weatherMap, weatherUV(p)).rgb;
    float coverage = weather.r;
    if (coverage < cloudMinCoverage) return 0.0;

    float height   = weather.g * maxCloudHeight;
    float altitude = weather.b * maxCloudAltitude;

    // Fade de cobertura por distância à câmera no plano XZ
    float distXZ   = length(p.xz - vec2(0,0));
    float fadeEdge = outerCloudRadius * 0.8;
    coverage      *= smoothstep(0.0, fadeEdge, distXZ);
    if (coverage < 0.001) return 0.0;

    float hSignal = HeightSignal(p, height, altitude);
    if (hSignal < 0.001) return 0.0;

    // Shape noise
    float shapeFBM        = getCloudShape(p);
    float baseShapeDensity = shapeFBM * hSignal;
    if (baseShapeDensity <= 0.0) return 0.0;

    // Erosão por detail noise (só em densidades baixas)
    float finalDensity;
    if (baseShapeDensity < erosionThreshold) {
        float detailFBM   = getCloudDetail(p);
        float erodeWeight = clamp(1.0 - (baseShapeDensity / erosionThreshold), 0.0, 1.0);
        finalDensity      = baseShapeDensity - detailFBM * erodeWeight;
    } else {
        finalDensity = baseShapeDensity;
    }

    finalDensity *= HeightGradient(p, height, altitude);
    return clamp(finalDensity, 0.0, 1.0);
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
        stepSize     *= 1.5;
    }

    return exp(-absorptionCoefficient * totalDensity);
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
    vec2 inner = raySphereIntersect(ro, rd, innerCloudRadius);
    vec2 outer = raySphereIntersect(ro, rd, outerCloudRadius);

    if (outer.y < 0.0) return vec2(1e9, -1e9);

    float camDist = length(ro - planetCenter());
    float tStart, tEnd;

    if (camDist < innerCloudRadius) {
        // Câmera abaixo das nuvens
        tStart = inner.y;
        tEnd   = outer.y;
    } else if (camDist < outerCloudRadius) {
        // Câmera dentro da casca
        tStart = 0.0;
        tEnd   = (inner.x > 0.0) ? inner.x : outer.y;
    } else {
        // Câmera acima das nuvens
        if (outer.x < 0.0) return vec2(1e9, -1e9);
        tStart = outer.x;
        tEnd   = (inner.x > 0.0) ? inner.x : outer.y;
    }

    return vec2(max(tStart, 0.0), tEnd);
}

vec3 rayMarch(vec3 ro, vec3 rd)
{
    vec3 skyColor = texture(skybox, rd).rgb;

    if (rd.y < 0.0) return skyColor;

    vec2  range  = getAtmosphereRange(ro, rd);
    float tStart = range.x;
    float tEnd   = range.y;

    if (tStart >= tEnd) return skyColor;

    float stepSize      = (tEnd - tStart) / float(cloudMaxSteps);
    float t             = tStart;
    float transmittance = 1.0;
    vec3  lightEnergy   = vec3(0.0);
    float cosTheta      = dot(normalize(lightDirection), normalize(rd));
    float phaseVal      = HG(cosTheta, 0.8);

    for (int i = 0; i < cloudMaxSteps; i++)
    {
        if (t >= tEnd || transmittance < 0.01) break;

        vec3  p         = ro + rd * t;
        float cloudDens = cloudDensity(p);

        if (cloudDens > 0.0)
        {
            // Integração analítica de Hillaire (§5.2)
            float sigmaE  = max(absorptionCoefficient * cloudDens, 1e-7);
            float Tr      = exp(-sigmaE * stepSize);

            float lightT  = lightMarching(p);
            vec3  direct  = lightColor * lightT * phaseVal;
            vec3  ambient = lightColor * 0.5;
            vec3  S       = (direct + ambient) * cloudDens * absorptionCoefficient;

            lightEnergy   += transmittance * (S - S * Tr) / sigmaE;
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
