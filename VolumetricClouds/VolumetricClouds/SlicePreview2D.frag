#version 410 core
out vec4 FragColor;
in  vec2 TexCoord;

uniform sampler2D noiseTex;
uniform int   channel;   // 0=RGBA, 1=R, 2=G, 3=B, 4=A

void main() {
    vec4 s = texture(noiseTex, vec2(TexCoord));
    if      (channel == 1) FragColor = vec4(s.r, 0.0, 0.0, 1.0); // vermelho
    else if (channel == 2) FragColor = vec4(0.0, s.g, 0.0, 1.0); // verde
    else if (channel == 3) FragColor = vec4(vec3(s.b), 1.0); // cinza
    else if (channel == 4) FragColor = vec4(vec3(s.a), 1.0);     // cinza
    else                   FragColor = vec4(s.rgb, 1.0);         // RGB
}