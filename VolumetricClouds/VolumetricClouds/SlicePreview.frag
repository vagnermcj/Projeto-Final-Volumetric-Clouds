#version 410 core
out vec4 FragColor;
in  vec2 TexCoord;

uniform sampler3D noiseTex;
uniform float slice;     // qual fatia Z mostrar
uniform int   channel;   // 0=RGBA, 1=R, 2=G, 3=B, 4=A

void main() {
    vec4 s = texture(noiseTex, vec3(TexCoord, slice));
    if      (channel == 1) FragColor = vec4(vec3(s.r), 1.0);
    else if (channel == 2) FragColor = vec4(vec3(s.g), 1.0);
    else if (channel == 3) FragColor = vec4(vec3(s.b), 1.0);
    else if (channel == 4) FragColor = vec4(vec3(s.a), 1.0);
    else                   FragColor = vec4(s.rgb, 1.0);
}