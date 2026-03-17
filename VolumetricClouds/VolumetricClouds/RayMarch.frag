#version 410 core
out vec4 FragColor;
in vec2 TexCoord;

uniform mat4 camMatrix;
uniform vec3 camPos;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform int lightSteps;
uniform vec3 backgroundColor;
uniform float absorptionCoefficient;
uniform float cloudStepSize;
uniform int cloudMaxSteps;
uniform vec3 cloudPosition;
uniform vec3 cloudScale;
uniform float time;
uniform float windSpeed;
uniform float cloudMinCoverage;
uniform float weatherScale;
uniform float atmosphereStart;
uniform float atmosphereHeight;
uniform vec4 shapeNoiseWeights;
uniform vec3 windDirection;
uniform float cloudCoverage;
uniform sampler3D shapeNoise;
uniform sampler3D detailNoise;
uniform sampler2D weatherMap;
uniform samplerCube skybox;
uniform float maxCloudHeight;
uniform float maxCloudAltitude;

float sceneSDF(vec3 p) {
    vec3 boxMin = vec3(-cloudScale.x, atmosphereStart, -cloudScale.x); // Box lower bounds
    vec3 boxMax = vec3(cloudScale.x, atmosphereStart + atmosphereHeight, cloudScale.x); // Box upper bounds

    vec3 q = max(boxMin - p, p - boxMax); // Calculate distance to box bounds
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float remap(float value, float start1, float stop1, float start2, float stop2)
{
    float outgoing = start2 + (stop2 - start2) * ((value - start1) / (stop1 - start1));
    return outgoing;
}

float HG(float costheta, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * 3.1415 * pow(1.0 + g2 - 2.0 * g * costheta, 1.5));
}

float getCloudShape(vec3 q)
{
    vec4 noise = texture(shapeNoise, q);

    // Normaliza os pesos para somarem 1
    vec4 normalizedWeights = shapeNoiseWeights / 
                             dot(shapeNoiseWeights, vec4(1.0));

    // Soma ponderada dos 4 canais
    return 1.0 - dot(noise, normalizedWeights);
}

float getCloudDetail(vec3 q)
{
    vec3 detailSample = texture(detailNoise, q).rgb;
    float detailCloud = 1.0 - (detailSample.r * 0.625 + detailSample.g * 0.25 + detailSample.b * 0.125); 

    return detailCloud;
}

float HeightSignal(vec3 p)
{   
    vec2 texPos = (vec2(p.x, p.z) / weatherScale) - vec2(windDirection.x, windDirection.z) * time * windSpeed;
    vec3 weather = texture(weatherMap, texPos).rgb;

    float height   = weather.r; // quão ALTA é a nuvem (tamanho)
    float altitude = weather.b; // onde ela COMEÇA (offset do fundo)

    if (height < 0.001) return 0.0;

    height = height * maxCloudHeight;

    altitude = altitude * maxCloudAltitude;

    float x = p.y - atmosphereStart;

    float oneOverHeight = 1.0 / (height * height);
    float heightSignal  = (x - altitude) * (x - altitude - height) * (oneOverHeight * -4.0);

    return clamp(heightSignal, 0.0, 1.0);
}

float cloudDensity(vec3 p)
{
        //weather = getWeather ( p o s i t i o n . xz )
        //d e n si t y = weather . r
        //d e n si t y ∗= H ei g h t Si g n al ( weather . gb , p o s i t i o n )
        //d e n si t y ∗= getCloudShape ( p o s i t i o n )
        //d e n si t y −= g e tCl o u dD e t ail ( p o s i t i o n )
        //d e n si t y ∗= Hei gh tG r adien t ( weather . gb , p o s i t i o n )

    vec2 texPos = (vec2(p.x, p.z) / weatherScale) - vec2(windDirection.x, windDirection.z) * time * windSpeed;
    float density = texture(weatherMap, texPos).r; //Coverage

    if (density < 0.001) return 0.0;

    float hSignal = HeightSignal(p);
    density *= hSignal;

    if (density < 0.001) return 0.0;

    float shapeFBM = getCloudShape(p);
    float baseShapeDensity = shapeFBM * hSignal;

    if (baseShapeDensity <= 0.0) return 0.0;

    // Detail erode bordas
    float detailFBM     = getCloudDetail(p);
    float oneMinusShape = 1.0 - shapeFBM;
    float erodeWeight   = oneMinusShape * oneMinusShape * oneMinusShape;
    float finalDensity  = baseShapeDensity - detailFBM * erodeWeight;

    return finalDensity;
}

//Light Marching
float lightMarching(vec3 pos)
{
    float totalDensity = 0.0;
    float steps = 1.0;
    for(int i = 0; i < lightSteps; i++)
    {
        vec3 p = pos + lightDirection * float(i) * steps;
        float d = sceneSDF(p);
        steps *= 1.5;
        if(d < 0.001)
        {
            totalDensity += max(0.0, cloudDensity(p));
        }
        else
         break;
    }

    float transmittance = exp(-absorptionCoefficient * totalDensity);
    return transmittance;
}

// Ray marching
vec3 rayMarch(vec3 ro, vec3 rd) {
    
    //Variaveis
    float t = 0.1;
    float transmittance = 1.0;
    vec3 lightEnergy = vec3(0.0); 
    float costheta = dot(normalize(lightDirection), rd);
    float phaseVal = HG(costheta, 0.8);

    for (int i = 0; i < cloudMaxSteps; i++) {
        vec3 p = ro + rd * t;
        float d = sceneSDF(p);
        if (d < 0.001) //Inside the cloud
        {
            float cloudDens = cloudDensity(p);
            if (cloudDens > 0.0f) //Calcula iluminacao 
            {
                    //float lightTransmittance = lightMarching(p);
                    float ambientLight = 0.2f;
                    vec3 Light = lightColor * ambientLight * cloudDens;

                    lightEnergy += Light * transmittance;

                    transmittance *= exp(-absorptionCoefficient * cloudDens * cloudStepSize);
            }
            
            
            t += cloudStepSize;
        }
        else
        {
            t += min(d, t);
        }

        if(transmittance < 0.1) break;
    }
    
    //vec3 skyColor = texture(skybox, rd).rgb;
    vec3 skyColor = mix(vec3(0.5, 0.7, 1.0), vec3(0.0, 0.2, 0.8), max(rd.y, 0.0));

    //Sun
    float sun = pow(max(dot(rd, normalize(lightDirection)), 0.0), 500.0);
    skyColor += vec3(1.0, 0.9, 0.7) * sun;

    vec3 finalColor = lightEnergy + (skyColor * transmittance);
    return finalColor;
}




void main()
{
    vec2 uv = TexCoord * 2.0 - 1.0;

    vec4 ray_clip = vec4(uv, -1.0, 1.0);
    vec4 ray_world = inverse(camMatrix) * ray_clip;
    vec3 rd = normalize(ray_world.xyz / ray_world.w - camPos);
    vec3 ro = camPos;

    int debug = 0; //0 : Clouds | 1 : Whole Noise | 2: Specific Chanel 
       
    switch(debug) {
        case 0: // Clouds 
            vec3 color = rayMarch(ro, rd);
            FragColor = vec4(color, 1.0);
            return;
        case 1: // Whole Noise
             FragColor = texture(shapeNoise, vec3(TexCoord, 0.5));
            return;
        case 2: // Specific Chanel 
            float v = texture(shapeNoise, vec3(TexCoord, 0.5)).r;
            FragColor = vec4(vec3(v), 1.0);
            return;
    }
    
}
