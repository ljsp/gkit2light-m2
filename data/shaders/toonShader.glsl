#version 330

#ifdef VERTEX_SHADER
uniform mat4 mvpMatrix;
layout(location= 0) in vec3 position;
layout(location= 1) in vec2 texcoord;
layout(location= 2) in vec3 normal;

out vec3 v_position;
out vec2 v_texcoord;
out vec3 v_normal;

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);       // centre de la fenetre

    v_position= position;
    v_texcoord= texcoord;
    v_normal= normal;
}
#endif

#ifdef FRAGMENT_SHADER
in vec3 v_position;
in vec2 v_texcoord;
in vec3 v_normal;

uniform int toonLevel;
uniform vec3 lightPosition;
uniform vec4 diffuse;

float toonShading(float cos) {
    for(int i = 0; i < toonLevel; i++)
    {
        float octave = 1.0 / toonLevel;
        if(cos > (octave * i) && cos < (octave * (i + 1))) {
            cos = clamp(cos, octave * (i + 1) , octave * (i + 1));
        }
    }
    return cos;
}

void main( )
{
    vec3 rayDirection= lightPosition - v_position;
    float cos = dot(normalize(v_normal), normalize(rayDirection));

    cos = toonShading(cos);

    gl_FragColor= diffuse * cos;
}
#endif

