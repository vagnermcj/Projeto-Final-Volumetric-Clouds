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

//Terrain
uniform sampler2D depthTex;
uniform mat4      invProjView;

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
uniform vec3  ambientColor;
uniform float cloudTopType;
uniform float cloudBottomType;
uniform int   lightSteps;
uniform float extinctionCoef;
uniform float scatteringCoef;
uniform float phaseG;
uniform float ambientIntensity;
uniform float precipitation;
uniform float silver_intensity;
uniform float silver_spread;

// ─── Performance Benchmark Controls ───────────────────────────────────────────
uniform bool enableDetailErosion;
uniform bool enableLightMarching;
uniform bool enableBeersLaw;
uniform bool enablePowderEffect;
uniform bool enablePhaseFunction;
uniform bool enableSilverSheen;

// ─── Vento / Tempo ────────────────────────────────────────────────────────────
uniform vec3  windDirection;
uniform float windSpeed;
uniform float time;

// ─── Texturas ─────────────────────────────────────────────────────────────────
uniform sampler3D  shapeNoise;
uniform sampler3D  detailNoise;
uniform sampler2D  weatherMap;
uniform samplerCube skybox;

float dimensionalProfile = 1.0;

// ═════════════════════════════════════════════════════════════════════════════
//  Utilitários
// ═════════════════════════════════════════════════════════════════════════════

float remap(float v, float s1, float e1, float s2, float e2) {
    return s2 + (e2 - s2) * clamp((v - s1) / (e1 - s1), 0.0, 1.0);
}

float HG(float cosTheta, float g) {
    float g2 = g * g;
    return ((1.0 - g2) / pow((1.0 + g2 - 2.0 * g * cosTheta), 3.0 / 2.0)) / (4.0 * 3.14159265);
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

float terrainDepth(vec2 uv)
{
    float d = texture(depthTex, uv).r;
    if (d <= 0.0001 || d >= 0.9999) return 1e9; // sem geometria
    
    vec4 ndcPos   = vec4(uv * 2.0 - 1.0, d * 2.0 - 1.0, 1.0);
    vec4 worldPos = invProjView * ndcPos;
    worldPos /= worldPos.w;
    return length(worldPos.xyz - camPos);
}

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


float getCloudShape(vec3 p)
{
    // UVW escalado + offset de vento para o shape
    vec3 shapeUVW = (p / shapeScale) + windDirection * time * windSpeed * 0.95;

    vec4 noise = texture(shapeNoise, shapeUVW);

    float perlinWorley    = noise.r * shapeNoiseWeights.r;

    return perlinWorley;
}

float getCloudDetail(vec3 p)
{
    vec3 detailUVW = (p / detailScale) + windDirection * time * windSpeed * 1.2;

    vec3 s = texture(detailNoise, detailUVW).rgb;
    return s.r * 0.625 + s.g * 0.25 + s.b * 0.125;
}

float bottomTypeProfile(float h, float x)
{
    float curve = 0.05 * x + 0.2;
    //float bottomType = h < curve? 0.0 : clamp((h - 0.4) / 0.6, 0.0, 1.0);
    float bottomType = clamp((h - curve) / (1.0 - curve), 0.0, 1.0);
    return bottomType;
}

float topTypeProfile(float h, float x)
{
    float curve = ((pow((x - 0.25) * 2.0, 3.0))/0.1) + 0.6;

    float gradient = 1.0 - clamp((h - 0.2) / 0.8, 0.0, 1.0);

    float topGradient = h < 0.2 ? gradient : h > curve ? 0.0 : gradient;

    return topGradient;
}



float cloudDensity(vec3 p, bool light = false)
{
    float dist = length(p - planetCenter());
    if (dist < innerCloudRadius || dist > outerCloudRadius) return 0.0; //Check atmosphere bounds

    vec2  weather  = texture(weatherMap, weatherUV(p)).rg;
    float coverage = weather.r;

    if(coverage < 0.01) return 0.0; 


    float topType = topTypeProfile(altitudeOf(p)/atmosphereHeight, cloudTopType);
    float botType = bottomTypeProfile(altitudeOf(p)/atmosphereHeight, cloudBottomType);
    float verticalProfile = topType * botType * weather.g;
    dimensionalProfile = coverage * verticalProfile;

    if(dimensionalProfile < 0.01) return 0.0;
    if(light) return dimensionalProfile;

    float density = dimensionalProfile;

    float shape = getCloudShape(p);
    density *= shape;

    if(density < 0.01) return 0.0;

    // Erosão
    if (enableDetailErosion) {
        float detail = getCloudDetail(p);
        float oneMinusShape = 1.0 - shape;
        float erodeWeight   = oneMinusShape * oneMinusShape * oneMinusShape;
        float detailFBM     = detail;
        density            -= detailFBM * erodeWeight * detailNoiseWeight;
    }

    return density;
}

// ════════════════════════════════════════
//  Light Marching
// ════════════════════════════════════════

float lightMarching(vec3 pos)
{
    float totalDensity = 0.0;
    float stepSize = (outerCloudRadius - innerCloudRadius) / float(lightSteps) * 2.0;

    for (int i = 0; i < lightSteps; i++)
    {
        vec3  p    = pos + normalize(lightDirection) * stepSize * float(i + 1);
        float dist = length(p - planetCenter());

        if (dist > outerCloudRadius || dist < innerCloudRadius) break;

        totalDensity += max(0.0, cloudDensity(p, true)) * stepSize;
        stepSize     *= 1.5;
    }

    return totalDensity;    
}


vec3 rayMarch(vec3 ro, vec3 rd)
{
    vec3 skyColor = texture(skybox, rd).rgb;

    vec2  range  = getAtmosphereRange(ro, rd);
    float tStart = range.x;
    float tEnd   = range.y;
    tEnd = min(tEnd, terrainDepth(TexCoord));

    if (tStart >= tEnd) return skyColor;

    float minDepth  = min(tEnd - tStart, atmosphereMaxDepth);
    float stepSize  = minDepth / float(cloudMaxSteps);
    float jitter    = rand(gl_FragCoord.xy) * stepSize;
    float t         = tStart + jitter;

    float transmittance = 1.0;
    vec3  scatteredLight = vec3(0.0);
    float cosTheta = dot(normalize(lightDirection), normalize(rd));

    float basePhaseFn = HG(cosTheta, phaseG);
    float silverPhaseFn = enableSilverSheen ? (silver_intensity * HG(cosTheta, 0.99 - silver_spread)) : 0.0;
    float phaseValue = max(basePhaseFn, silverPhaseFn);

    for (int i = 0; i < cloudMaxSteps; i++) 
    {
        if (t >= tEnd || transmittance < 0.1) break;

        vec3  p         = ro + rd * t;
        if (p.y < 0.0) break;

        float cloudDens = cloudDensity(p);

        if (cloudDens > 0.01)
        {
            float densityToLight = enableLightMarching ? lightMarching(p) : 0.0;

            float beersLaw = 1.0;
            if (enableBeersLaw) {
                beersLaw = max(exp(-densityToLight), exp(-densityToLight * 0.25) * 0.7);
            }

            float powderEffect = 1.0;
            if (enablePowderEffect) {
                powderEffect = 1.0 - exp(-densityToLight * 2.0);
            }

            float phaseComponent = enablePhaseFunction ? phaseValue : 1.0;
            float energy = beersLaw * phaseComponent * powderEffect;

            float altitude = altitudeOf(p) - atmosphereStart;
            float heightFraction = clamp(altitude / atmosphereHeight, 0.0, 1.0);
            
            vec3 ambient = mix(skyColor, ambientColor, 0.6) * ambientIntensity * (1.0 - cloudDens);

            vec3 direct = lightColor;
            vec3 S = (direct * energy + ambient ) * cloudDens;

            scatteredLight += transmittance * S * stepSize;
            transmittance *= exp(-cloudDens * extinctionCoef * stepSize);
        }

        t += stepSize;
    }

    return scatteredLight + skyColor * transmittance;
}
// ═════════════════════════════════════════════════════════════════════════════
//  Main
// ═════════════════════════════════════════════════════════════════════════════

void main()
{
    float d = texture(depthTex, TexCoord).r;
    if (d > 0.0001 && d < 0.9999)
    {
        discard;
    }

    vec2 uv        = TexCoord * 2.0 - 1.0;
    vec4 ray_clip  = vec4(uv, -1.0, 1.0);
    vec4 ray_world = inverse(camMatrix) * ray_clip;
    vec3 rd        = normalize(ray_world.xyz / ray_world.w - camPos);
    vec3 ro        = camPos;
    
    FragColor = vec4(rayMarch(ro, rd), 1.0);
}
