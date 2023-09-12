#version 330

#ifdef VERTEX_SHADER
// doit calculer la position d'un sommet dans le repere projectif
// indice du sommet : gl_VertexID
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

uniform int nbOctaves;
uniform vec3 lightPosition;
uniform vec4 diffuse;

void main( )
{
    vec3 rayDirection= lightPosition - v_position;
    float cos = dot(normalize(v_normal), normalize(rayDirection));

    // Toon shading
    for(int i = 0; i < nbOctaves; i++)
    {
        float octave = 1.0 / nbOctaves;
        if(cos > (octave * i) && cos < (octave * (i + 1))) {
            cos = clamp(cos, octave * (i + 1) , octave * (i + 1));
        }
    }

    gl_FragColor= diffuse * cos;

}
#endif

