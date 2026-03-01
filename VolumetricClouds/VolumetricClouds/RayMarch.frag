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
uniform vec3 windDirection;
uniform float cloudCoverage;
uniform sampler3D shapeNoise;
uniform sampler3D detailNoise;
uniform samplerCube skybox;

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

float HG(float costheta, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * 3.1415 * pow(1.0 + g2 - 2.0 * g * costheta, 1.5));
}

float getCloudShape(vec3 q)
{
    vec4 noiseSample = texture(shapeNoise, q );
    
    float perlin = noiseSample.r;
    float worleyLow = noiseSample.g;  
    float worleyMid = noiseSample.b; 
    float worleyHigh = noiseSample.a; 
    
    float baseCloud =  perlin + (worleyLow * 0.625 + worleyMid * 0.25 + worleyHigh * 0.125);
    
    return baseCloud;
}

float getCloudDetail(vec3 q)
{
    vec3 detailSample = texture(detailNoise, q).rgb;
    
    float worleyLow =  detailSample.r;
    float worleyMid =  detailSample.g;
    float worleyHigh = detailSample.b;

    float detailCloud = worleyLow * 0.625 + worleyMid * 0.25 + worleyHigh * 0.125;
    return detailCloud;
}


float cloudDensity(vec3 p, float sdf)
{
    vec3 q = (p - cloudPosition) - windDirection * time * windSpeed;
    
    float baseCloud = getCloudShape(q);

    float density = remap(baseCloud, cloudCoverage, 1.0, 0.0, 1.0);

    if (density <= 0.0) return 0.0;
    
    density -= getCloudDetail(q);

    return density;
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
            totalDensity += max(0.0, cloudDensity(p, d));
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
            float cloudDens = cloudDensity(p, d);
            if(cloudDens > 0.0)
            {
                float lightTransmittance = lightMarching(p);
                
                vec3 Light = lightColor  * lightTransmittance * cloudDens  * phaseVal;

                lightEnergy += Light * 0.4 * transmittance;

                transmittance *= exp(-absorptionCoefficient * cloudDens * cloudStepSize);
            }
            
            t += cloudStepSize;
        }
        else
        {
            t += min(d, t);
        }

        if(transmittance < 0.1) break;
        if (t > 100.0) break;
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

    int debug = 2; //0 : Clouds | 1 : Whole Noise | 2: Specific Chanel 
       
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
