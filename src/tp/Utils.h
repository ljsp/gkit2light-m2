//
// Created by lucas on 14/12/23.
//

#ifndef GKIT2LIGHT_UTILS_H
#define GKIT2LIGHT_UTILS_H

#include "mesh.h"
#include "orbiter.h"

Mesh make_grid( const int n= 10 )
{
    Mesh grid= Mesh(GL_LINES);

    // grille
    grid.color(White());
    for(int x= 0; x < n; x++)
    {
        float px= float(x) - float(n)/2 + .5f;
        grid.vertex(Point(px, 0, - float(n)/2 + .5f));
        grid.vertex(Point(px, 0, float(n)/2 - .5f));
    }

    for(int z= 0; z < n; z++)
    {
        float pz= float(z) - float(n)/2 + .5f;
        grid.vertex(Point(- float(n)/2 + .5f, 0, pz));
        grid.vertex(Point(float(n)/2 - .5f, 0, pz));
    }

    // axes XYZ
    grid.color(Red());
    grid.vertex(Point(0, .1, 0));
    grid.vertex(Point(1, .1, 0));

    grid.color(Green());
    grid.vertex(Point(0, .1, 0));
    grid.vertex(Point(0, 1, 0));

    grid.color(Blue());
    grid.vertex(Point(0, .1, 0));
    grid.vertex(Point(0, .1, 1));

    glLineWidth(2);

    return grid;
}

Mesh make_ground( const int n= 10 )
{
    Mesh grid= Mesh(GL_TRIANGLES);

    grid.normal(0, 1, 0);
    int a= grid.vertex(-n/2, -1, n/2);
    int b= grid.vertex( n/2, -1, n/2);
    int c= grid.vertex( n/2, -1, -n/2);
    int d= grid.vertex(-n/2, -1, -n/2);
    grid.triangle(a, b, c);
    grid.triangle(a, c, d);

    return grid;
}

Mesh make_frustum( )
{
    glLineWidth(2);
    Mesh camera= Mesh(GL_LINES);

    camera.color(Yellow());
    // face avant
    camera.vertex(-1, -1, -1);
    camera.vertex(-1, 1, -1);
    camera.vertex(-1, 1, -1);
    camera.vertex(1, 1, -1);
    camera.vertex(1, 1, -1);
    camera.vertex(1, -1, -1);
    camera.vertex(1, -1, -1);
    camera.vertex(-1, -1, -1);

    // face arriere
    camera.vertex(-1, -1, 1);
    camera.vertex(-1, 1, 1);
    camera.vertex(-1, 1, 1);
    camera.vertex(1, 1, 1);
    camera.vertex(1, 1, 1);
    camera.vertex(1, -1, 1);
    camera.vertex(1, -1, 1);
    camera.vertex(-1, -1, 1);

    // aretes
    camera.vertex(-1, -1, -1);
    camera.vertex(-1, -1, 1);
    camera.vertex(-1, 1, -1);
    camera.vertex(-1, 1, 1);
    camera.vertex(1, 1, -1);
    camera.vertex(1, 1, 1);
    camera.vertex(1, -1, -1);
    camera.vertex(1, -1, 1);

    return camera;
}

struct Buffers
{
    GLuint vao;
    GLuint vertex_buffer;
    int vertex_count;

    Buffers( ) : vao(0), vertex_buffer(0), vertex_count(0) {}

    void create( const Mesh& mesh )
    {
        if(!mesh.vertex_buffer_size()) return;

        std::vector<float> data;
        int vertexSize = mesh.vertex_buffer_size();
        int texSize = mesh.texcoord_buffer_size();
        int normalSize = mesh.normal_buffer_size();

        const float* vertexData = mesh.vertex_buffer();
        const float* texData = mesh.texcoord_buffer();
        const float* normalData = mesh.normal_buffer();

        for(int i = 0; i < vertexSize/sizeof(float); i++) {
            data.push_back(vertexData[i]);
        }

        for(int i = 0; i < texSize/sizeof(float); i++) {
            data.push_back(texData[i]);
        }

        for(int i = 0; i < normalSize/sizeof(float); i++) {
            data.push_back(normalData[i]);
        }

        int bufferSize = vertexSize + texSize + normalSize;

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, bufferSize, data.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0,3, GL_FLOAT,GL_FALSE,0,0);

        int offset =vertexSize; // 12;
        glVertexAttribPointer(1,2, GL_FLOAT,GL_FALSE,0,(const void*) offset);

        offset = offset + texSize;
        glVertexAttribPointer(2,3, GL_FLOAT,GL_FALSE,0,(const void*) offset);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        vertex_count= mesh.vertex_count();
    }

    void draw () {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertex_count);
    }

    void release( )
    {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vertex_buffer);
    }
};

struct ShadowMap {
    GLuint framebuffer;
    GLuint texture;

    ShadowMap( ) : framebuffer(0), texture(0) {}

    void create( const int width, const int height )
    {
        // creer une texture pour contenir la shadow map
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0); // obligatoire
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // obligatoire
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // obligatoire
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // obligatoire
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // obligatoire

        // creer un framebuffer pour contenir la shadow map
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture, 0);

        GLenum status= glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
        if(status != GL_FRAMEBUFFER_COMPLETE)
            printf("[error] framebuffer is not complete\n");

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    void clear_color(const Color & color) {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void clear_depth(const float depth) {
        glClearDepth(depth);
    }

    void release( )
    {
        glDeleteTextures(1, &texture);
        glDeleteFramebuffers(1, &framebuffer);
    }

};

struct BBox
{
    Point pmin, pmax;

    BBox( const Point& p ) : pmin(p), pmax(p) {}
    BBox( const Point& a, const Point& b ) : pmin(a), pmax(b) {}
    BBox( const BBox& b ) : pmin(b.pmin), pmax(b.pmax) {}

    BBox& insert( const Point& p ) { pmin= min(pmin, p); pmax= max(pmax, p); return *this; }
    BBox& insert( const BBox& b ) { pmin= min(pmin, b.pmin); pmax= max(pmax, b.pmax); return *this; }

    float centroid( const int axis ) const { return (pmin(axis) + pmax(axis)) / 2; }
};

BBox EmptyBox( )
{
    return BBox(Point(FLT_MAX, FLT_MAX, FLT_MAX),
                Point(-FLT_MAX, -FLT_MAX, -FLT_MAX));
}

std::vector<std::pair<Point,Point>> blocDivision(Point pMin, Point pMax) {
    // Trouver l'axe le plus long
    // Faire la division sur cet axe


}

/*std::vector<unsigned> splitBox(std::pair<Point, Point> box,  ) {
    std::vector<unsigned> triangles;
    m_objet.triangle_count()
    for(int i = 0; i < m_objet.vertex_count(); i++) {
        Point a = m_objet.triangle(i).a;
        Point b = m_objet.triangle(i).b;
        Point c = m_objet.triangle(i).c;

        Point p = Point((a.x + b.x + c.x) / 3.0, (a.y + b.y + c.y) / 3.0, (a.z + b.z + c.z) / 3.0);
        if(p.x >= pMin.x && p.x <= pMax.x && p.y >= pMin.y && p.y <= pMax.y && p.z >= pMin.z && p.z <= pMax.z) {
            triangles.push_back(i);
        }
    }
    return triangles;
}*/

static bool isVisibleInProjectedSpace(vec4 boundingBox[8]) {

    vec4 v000 = boundingBox[0]; vec4 v001 = boundingBox[1];
    vec4 v010 = boundingBox[2]; vec4 v011 = boundingBox[3];
    vec4 v100 = boundingBox[4]; vec4 v101 = boundingBox[5];
    vec4 v110 = boundingBox[6]; vec4 v111 = boundingBox[7];

    if(v000.x > v000.w && v001.x > v001.w && v010.x > v010.w && v011.x > v011.w &&
       v100.x > v100.w && v101.x > v101.w && v110.x > v110.w && v111.x > v111.w ) {
        return false;
    }

    if(v000.x < -v000.w && v001.x < -v001.w && v010.x < -v010.w && v011.x < -v011.w &&
       v100.x < -v100.w && v101.x < -v101.w && v110.x < -v110.w && v111.x < -v111.w ) {
        return false;
    }

    if(v000.y > v000.w && v001.y > v001.w && v010.y > v010.w && v011.y > v011.w &&
       v100.y > v100.w && v101.y > v101.w && v110.y > v110.w && v111.y > v111.w ) {
        return false;
    }

    if(v000.y < -v000.w && v001.y < -v001.w && v010.y < -v010.w && v011.y < -v011.w &&
       v100.y < -v100.w && v101.y < -v101.w && v110.y < -v110.w && v111.y < -v111.w ) {
        return false;
    }

    if(v000.z > v000.w && v001.z > v001.w && v010.z > v010.w && v011.z > v011.w &&
       v100.z > v100.w && v101.z > v101.w && v110.z > v110.w && v111.z > v111.w ) {
        return false;
    }

    if(v000.z < -v000.w && v001.z < -v001.w && v010.z < -v010.w && v011.z < -v011.w &&
       v100.z < -v100.w && v101.z < -v101.w && v110.z < -v110.w && v111.z < -v111.w ) {
        return false;
    }

    return true;
}

static bool isVisibleInObjectSpace(Point pMin, Point pMax,vec4 boundingBox[8]) {

    vec4 v000 = boundingBox[0]; vec4 v001 = boundingBox[1];
    vec4 v010 = boundingBox[2]; vec4 v011 = boundingBox[3];
    vec4 v100 = boundingBox[4]; vec4 v101 = boundingBox[5];
    vec4 v110 = boundingBox[6]; vec4 v111 = boundingBox[7];

    if(v000.x > pMax.x && v001.x > pMax.x && v010.x > pMax.x && v011.x > pMax.x &&
       v100.x > pMax.x && v101.x > pMax.x && v110.x > pMax.x && v111.x > pMax.x ) {
        return false;
    }

    if(v000.x < pMin.x && v001.x < pMin.x && v010.x < pMin.x && v011.x < pMin.x &&
       v100.x < pMin.x && v101.x < pMin.x && v110.x < pMin.x && v111.x < pMin.x ) {
        return false;
    }

    if(v000.y > pMax.y && v001.y > pMax.y && v010.y > pMax.y && v011.y > pMax.y &&
       v100.y > pMax.y && v101.y > pMax.y && v110.y > pMax.y && v111.y > pMax.y ) {
        return false;
    }

    if(v000.y < pMin.y && v001.y < pMin.y && v010.y < pMin.y && v011.y < pMin.y &&
       v100.y < pMin.y && v101.y < pMin.y && v110.y < pMin.y && v111.y < pMin.y ) {
        return false;
    }

    if(v000.z > pMax.z && v001.z > pMax.z && v010.z > pMax.z && v011.z > pMax.z &&
       v100.z > pMax.z && v101.z > pMax.z && v110.z > pMax.z && v111.z > pMax.z ) {
        return false;
    }

    if(v000.z < pMin.z && v001.z < pMin.z && v010.z < pMin.z && v011.z < pMin.z &&
       v100.z < pMin.z && v101.z < pMin.z && v110.z < pMin.z && v111.z < pMin.z ) {
        return false;
    }

    return true;
}

bool frustumCulling(Point pMin, Point pMax, Orbiter m_camera) {
    // Faire la bounding box de l'objet
    Point p000 = pMin, p111 = pMax;
    Point p001 = Point(p000.x, p000.y, p111.z);
    Point p010 = Point(p000.x, p111.y, p000.z);
    Point p011 = Point(p000.x, p111.y, p111.z);
    Point p100 = Point(p111.x, p000.y, p000.z);
    Point p101 = Point(p111.x, p000.y, p111.z);
    Point p110 = Point(p111.x, p111.y, p000.z);

    Point boundingBox[8] = {p000, p001, p010, p011,p100, p101, p110, p111};

    // Passer ses coordonnées dans le repère projectif
    Transform pvMatrix = m_camera.projection() * m_camera.view();
    vec4 projectedBoundingBox[8];
    for(int i = 0; i < 8; i++) {
        projectedBoundingBox[i] = pvMatrix(boundingBox[i]);
    }

    // Faire le test des coordonnées pour savoir si l'objet est visible dans le repère projectif
    if(!isVisibleInProjectedSpace(projectedBoundingBox)) {
        //std::cout << "Cube non visible" << std::endl;
        return false;
    }

    //Si on a pas pu déterminer on regarde dans le repère de l'objet
    Transform pvInverse = Inverse(pvMatrix);
    vec4 inverseBoundingBox[8];
    for(int i = 0; i < 8; i++) {
        inverseBoundingBox[i] = pvInverse(projectedBoundingBox[i]);
    }

    if(!isVisibleInObjectSpace(p000, p111, inverseBoundingBox)) {
        //std::cout << "Cube non visible" << std::endl;
        return false;
    }

    return true;
}

#endif //GKIT2LIGHT_UTILS_H
