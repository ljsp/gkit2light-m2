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

uniform vec4 material_color;
uniform sampler2D material_texture;
uniform sampler2D decal_texture;
uniform vec3 lightPosition;

void main( )
{
    // Light
    vec3 rayDirection = lightPosition - v_position;
    float cos = dot(normalize(v_normal), normalize(rayDirection));

    vec4 tex = texture(material_texture, v_texcoord);
    if(tex.a < 0.5) {
        discard;
    }

    // Shadows
    vec3 texcoord = v_decal_position.xyz / v_decal_position.w;
    float shadowDepth = texture(decal_texture, texcoord.xy).r;
    //vec3 decal_color= textureProj(decal_texture, v_decal_position).rgb;

    float shadow = 0.0;

    // Hard shadows
    //shadow = texcoord.z - 0.005 > shadowDepth ? 0.3 : 1.0;

    /*
    // PCF
    vec2 texelSize = 1.0 / textureSize(decal_texture, 0);
    for(int x = -1; x <= 1; ++x ) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(decal_texture, texcoord.xy + vec2(x,y) * texelSize).r;
            shadow += texcoord.z - 0.005 > pcfDepth ? 0.3 : 1.0;
        }
    }
    shadow /= 9.0;
    */


    // PCF with Poisson disk
    const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845),
    vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554),
    vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507),
    vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367),
    vec2(0.14383161, -0.14100790)
    );

    vec2 texelSize = 1.0 / textureSize(decal_texture, 0);
    for(int i = 0; i < 16; ++i) {
        float pcfDepth = texture(decal_texture, texcoord.xy + poissonDisk[i] * texelSize).r;
        shadow += texcoord.z - 0.005 > pcfDepth ? 0.3 : 1.0;
    }
    shadow /= 16.0;


    gl_FragColor = vec4(material_color.rgb * tex.rgb * shadow, 1) * cos;
}
#endif

