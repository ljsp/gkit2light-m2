#version 330

#ifdef VERTEX_SHADER
layout(location= 0) in vec3 position;
layout(location= 1) in vec2 texcoord;
layout(location= 2) in vec3 normal;

uniform mat4 mvpMatrix;
uniform mat4 mvMatrix;
uniform mat4 decalMatrix;

out vec3 v_position;
out vec2 v_texcoord;
out vec3 v_normal;
out vec4 v_decal_position;

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);       // centre de la fenetre
    v_decal_position= decalMatrix * vec4(position, 1); // position du fragment dans la texture

    v_position= vec3(mvMatrix * vec4(position, 1));
    v_texcoord= texcoord;
    v_normal= normal;
}
#endif

#ifdef FRAGMENT_SHADER
in vec3 v_position;
in vec2 v_texcoord;
in vec3 v_normal;
in vec4 v_decal_position;

uniform vec3 lightPosition;
uniform vec4 material_color;
uniform sampler2D material_texture;
uniform sampler2D decal_texture;

void main( )
{
    //vec3 rayDirection = lightPosition - v_position;
    //float cos = dot(normalize(v_normal), normalize(rayDirection));

    /*
    vec4 tex = texture(material_texture, v_texcoord);
    if(tex.a < 0.5) {
        discard;
    }
    */


    //vec3 texcoord= v_decal_position.xyz / v_decal_position.w;
    //vec3 decal_color= texture(decal_texture, texcoord.xy).rgb;

    vec3 decal_color= textureProj(decal_texture, v_decal_position).rgb;

    gl_FragColor = vec4(material_color.rgb * decal_color.r, 1);
}
#endif

