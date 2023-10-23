#version 330
#ifdef VERTEX_SHADER

layout(location= 0) in vec3 position;

uniform mat4 mvpMatrix;
uniform mat4 decalMatrix;

out vec4 decal_position;

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);
    decal_position= decalMatrix * vec4(position, 1);
}

#endif

#ifdef FRAGMENT_SHADER

out vec4 fragment_color;

uniform sampler2D decal;        // texture du decal
in vec4 decal_position;         // position dans la projection du decal

void main() {

    vec3 color= vec3(1.0) ;           // calcul habituel de la couleur de l'objet

    // recupere la couleur du decal
    // termine la projection : passage repere homogene 4d vers repere reel 3d
    vec3 decal_texcoord= decal_position.xyz / decal_position.w;
    vec3 decal_color= texture(decal, decal_texcoord.xy).rgb;

    fragment_color= vec4(color * decal_color, 1);

}

#endif
