#version 330

#ifdef VERTEX_SHADER
layout(location= 0) in vec3 position;
//layout(location= 1) in vec2 texcoord;
//layout(location= 2) in vec3 normal;
//layout(location= 3) in vec4 color;
//layout(location= 4) in uint material;
uniform mat4 mvpMatrix;

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);
}
#endif

#ifdef FRAGMENT_SHADER
uniform vec4 color;
uniform vec3 camPos;
out vec4 fragment_color;



void main( )
{
    fragment_color= color;
}
#endif
