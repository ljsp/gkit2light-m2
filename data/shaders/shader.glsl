#version 330
 
#ifdef VERTEX_SHADER
// doit calculer la position d'un sommet dans le repere projectif
// indice du sommet : gl_VertexID
uniform mat4 mvpMatrix;
layout(location= 0) in vec3 position;

void main( )
{
        gl_Position= mvpMatrix * vec4(position, 1);       // centre de la fenetre
}
#endif
 
#ifdef FRAGMENT_SHADER
// doit calculer la couleur du fragment
void main( )
{
        gl_FragColor= vec4(1, 0, 1, 1);       // blanc opaque
}
#endif

