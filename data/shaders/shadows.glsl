#version 330

#ifdef VERTEX_SHADER
layout(location= 0) in vec3 position;
layout(location= 1) in vec2 texcoord;
layout(location= 2) in vec3 normal;

uniform mat4 mvpMatrix;
uniform mat4 mvMatrix;

out vec3 v_position;
out vec2 v_texcoord;
out vec3 v_normal;

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);       // centre de la fenetre

    v_position= vec3(mvMatrix * vec4(position, 1));;
    v_texcoord= texcoord;
    v_normal= normal;
}
#endif

#ifdef FRAGMENT_SHADER
in vec3 v_position;
in vec2 v_texcoord;
in vec3 v_normal;

uniform vec3 lightPosition;
uniform vec4 material_color;

void main( )
{
    vec3 rayDirection= lightPosition - v_position;
    float cos = dot(normalize(v_normal), normalize(rayDirection));

    gl_FragColor= cos * material_color;

}
#endif

