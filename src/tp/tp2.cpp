//
// Created by lucas on 19/09/23.
//

#include "wavefront.h"
#include "texture.h"

#include "orbiter.h"
#include "draw.h"
#include "app_camera.h"        // classe Application a deriver

#include "program.h"
#include "uniforms.h"

#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include "app_time.h"

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

        // parametres de filtrage et de "clamping" de la texture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0); // obligatoire
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // obligatoire
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // obligatoire
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // obligatoire
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // obligatoire

        // creer un framebuffer pour contenir la shadow map
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture, 0);

        // ne pas oublier de verifier... (c'est trop souvent oublie)
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

class TP : public AppTime
{
public:
    TP( ) : AppTime(1920, 1080) {}

    int init( )
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplSdlGL3_Init(m_window, "#version 330");

        m_camera.projection(window_width(), window_height(), 45);

        show_demo_window = false;
        lightPosition= Point(0, 10, 10);
        objetPosition= Point(1, 0, 0);
        objectRotation = 0;
        objetScale = 0.5f;

        m_position = Identity();
        m_repere= make_grid(10);
        m_texture= read_texture(0, "../data/grid.png");
        m_cube = read_mesh("../data/cube.obj");
        if(m_cube.materials().count() == 0) return -1;
        if(!m_cube.vertex_count()) return -1;
        m_frustum = make_frustum();

        m_objet = read_mesh("../data/bistro-small/export.obj");
        if(m_objet.materials().count() == 0) return -1;
        if(!m_objet.vertex_count()) return -1;
        Materials& materials= m_objet.materials();
        m_textures.resize(materials.filename_count(), 0);
        {
            for(unsigned i= 0; i < m_textures.size(); i++)
            {
                m_textures[i]= read_texture(0, materials.filename(i));

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
            }

            printf("%d textures.\n", materials.filename_count());
        }

        m_white_texture= read_texture(0, "../data/blanc.jpg");  // !! utiliser une vraie image blanche...
        // Faire la bounding box de l'objet
        //Point pMin, pMax;
        //m_objet.bounds(pMin, pMax);
        //unsigned partition[m_objet.triangle_count()];
        //memset(partition, 0, sizeof(partition));
        //blocs = blocDivision(pMin, pMax);

        m_groups = m_objet.groups();

        m_shadow_map.create(1024, 1024);
        m_shadow_map.clear_color(White());
        m_shadow_map.clear_depth(1);

        m_buffers.create(m_objet);

        m_shadow_program= read_program("../data/shaders/draw_shadows.glsl");
        program_print_errors(m_shadow_program);

        m_program = read_program("../data/shaders/shader_tp2.glsl");
        program_print_errors(m_program);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_shadow_map.framebuffer);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_shadow_map.texture, 0);
        GLenum status= glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
        if(status != GL_FRAMEBUFFER_COMPLETE)
            printf("[error] framebuffer is not complete\n");

        GLenum buffers[]= { GL_DEPTH_ATTACHMENT };
        glDrawBuffers(0, buffers);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glClearColor(0.2f, 0.2f, 0.2f, 1.f);
        glClearDepth(1.f);
        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        return 0;
    }

    int quit( )
    {
        m_repere.release();
        m_objet.release();
        m_cube.release();

        ImGui_ImplSdlGL3_Shutdown();
        ImGui::DestroyContext();

        m_buffers.release();
        m_shadow_map.release();

        release_program(m_shadow_program);
        release_program(m_program);

        return 0;
    }

    int render( )
    {
        cameraUpdate();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        scene_bistro();

        handleKeys();

        imguiWindow();

        return 1;
    }

    void scene_bistro() {
        Transform r= RotationX(-90);
        Transform t= Translation(0,0, 8);
        Transform m= r * t;

        Transform decal_view= Inverse(m_position * m);
        Transform decal_projection= Ortho(-2, 2, -2, 2, float(0.1), float(10));

        // transformations de la camera de l'application
        Transform view= m_camera.view();
        Transform projection= m_camera.projection();

        if(key_state('e'))
        {
            view= decal_view;
            projection= decal_projection;
        }


        // 1ère passse
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_shadow_map.framebuffer);
        glViewport(0, 0, 1024, 1024);
        glClear(GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(m_buffers.vao);
        glUseProgram(m_shadow_program);
        program_uniform(m_shadow_program, "mvpMatrix", decal_projection * decal_view);
        m_objet.draw(m_shadow_program, true, false, false, false, false);


        // 2ème passe

        Transform model= Identity() * Scale(objetScale);
        Transform mvp= projection * view * model;
        Transform mv= view * model;

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glViewport(0, 0, window_width(), window_height());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, m_shadow_map.texture);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindVertexArray(m_buffers.vao);
        glUseProgram(m_program);

        if (key_state('r')) {
            Transform decal_m= Inverse(decal_projection * decal_view);
            draw(m_frustum, decal_m, view, projection);
        }

        program_uniform(m_program, "mvpMatrix", mvp);
        program_uniform(m_program, "mvMatrix", mv);
        program_uniform(m_program, "lightPosition", lightPosition);

        Transform decal_viewport= Viewport(1, 1);
        Transform decal= decal_viewport * decal_projection * decal_view;

        program_uniform(m_program, "decalMatrix", decal);
        program_use_texture(m_program, "texture", 0, m_shadow_map.texture);

        const Materials& materials= m_objet.materials();
        for(unsigned i= 0; i < m_groups.size(); i++)
        {
            const Material& material= materials(m_groups[i].index);

            program_uniform(m_program, "material_color", material.diffuse);

            if(material.diffuse_texture != -1)
                program_use_texture(m_program, "material_texture", 0, m_textures[material.diffuse_texture]);
            else
                program_use_texture(m_program, "material_texture", 0, m_white_texture);

            // Faire frustum culling des blocs

            m_objet.draw(m_groups[i].first, m_groups[i].n, m_program, true,
                         true, true, false, false);
        }

    }

    std::vector<std::pair<Point,Point>> blocDivision(Point pMin, Point pMax) {

        // Trouver les coordonnées médianes
        float midX = (pMin.x + pMax.x) / 2.0;
        float midY = (pMin.y + pMax.y) / 2.0;
        float midZ = (pMin.z + pMax.z) / 2.0;

        // Diviser la bounding box en 8 sous-boxes
        std::vector<std::pair<Point, Point>> blocs;

        // Bloc du bas
        blocs.emplace_back(pMin, Point(midX, midY, midZ));
        blocs.emplace_back(Point(midX, pMin.y, pMin.z), Point(pMax.x, midY, midZ));
        blocs.emplace_back(Point(pMin.x, midY, pMin.z), Point(midX, pMax.y, midZ));
        blocs.emplace_back(Point(midX, midY, pMin.z), Point(pMax.x, pMax.y, midZ));

        // Bloc du haut
        blocs.emplace_back(Point(pMin.x, pMin.y, midZ), Point(midX, midY, pMax.z));
        blocs.emplace_back(Point(midX, pMin.y, midZ), Point(pMax.x, midY, pMax.z));
        blocs.emplace_back(Point(pMin.x, midY, midZ), Point(midX, pMax.y, pMax.z));
        blocs.emplace_back(Point(midX, midY, midZ), pMax);

        return blocs;
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

    bool frustumCulling(Point pMin, Point pMax) {
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

    void cameraUpdate() {

        // recupere les mouvements de la souris
        int mx, my;
        unsigned int mb= SDL_GetRelativeMouseState(&mx, &my);
        int mousex, mousey;
        SDL_GetMouseState(&mousex, &mousey);
        ImGuiIO& io = ImGui::GetIO();

        // deplace la camera
        if(!io.WantCaptureMouse && mb & SDL_BUTTON(1))
            m_camera.rotation(mx, my);      // tourne autour de l'objet
        else if(mb & SDL_BUTTON(3))
            m_camera.translation((float) mx / (float) window_width(), (float) my / (float) window_height()); // deplace le point de rotation
        else if(mb & SDL_BUTTON(2))
            m_camera.move(mx);           // approche / eloigne l'objet

        SDL_MouseWheelEvent wheel= wheel_event();
        if(wheel.y != 0)
        {
            clear_wheel_event();
            m_camera.move(8.f * wheel.y);  // approche / eloigne l'objet
        }

        const char *orbiter_filename= "app_orbiter.txt";
        // copy / export / write orbiter
        if(key_state('c'))
        {
            clear_key_state('c');
            m_camera.write_orbiter(orbiter_filename);

        }
        // paste / read orbiter
        if(key_state('v'))
        {
            clear_key_state('v');

            Orbiter tmp;
            if(tmp.read_orbiter(orbiter_filename) < 0)
                // ne pas modifer la camera en cas d'erreur de lecture...
                tmp= m_camera;

            m_camera= tmp;
        }

        // screenshot
        if(key_state('s'))
        {
            static int calls= 1;
            clear_key_state('s');
            screenshot("app", calls++);
        }
    }

    void handleKeys( )
    {
        if(key_state(SDLK_LEFT) || key_state(SDLK_q))
            lightPosition = lightPosition - Vector(0.1f, 0, 0);
        if(key_state(SDLK_RIGHT) || key_state(SDLK_d))
            lightPosition = lightPosition + Vector(0.1f, 0, 0);
        if(key_state(SDLK_DOWN) || key_state(SDLK_z))
            lightPosition = lightPosition + Vector(0, 0, 0.1f);
        if(key_state(SDLK_UP) || key_state(SDLK_s))
            lightPosition = lightPosition - Vector(0, 0, 0.1f);
        if(key_state(SDLK_SPACE))
            lightPosition = lightPosition + Vector(0, 0.1f, 0);
        if(key_state(SDLK_LSHIFT))
            lightPosition = lightPosition - Vector(0, 0.1f, 0);
    }

    void imguiWindow( )
    {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui_ImplSdlGL3_NewFrame(m_window);
        ImGui::NewFrame();

        m_cpu_stop= std::chrono::high_resolution_clock::now();
        int cpu_time= std::chrono::duration_cast<std::chrono::microseconds>(m_cpu_stop - m_cpu_start).count();

        ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove);
        ImGui::Text("Application average %.1f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("CPU  %02dms %03dus", cpu_time / 1000, cpu_time % 1000);
        ImGui::Text("GPU  %02dms %03dus", int(m_frame_time / 1000000), int((m_frame_time / 1000) % 1000));
        ImGui::Spacing();
        ImGui::Checkbox("Demo Window", &show_demo_window);

        ImVec2 texture_size(350, 350);  // You can adjust this size as per your requirements
        ImGui::Image((void*)(intptr_t)m_shadow_map.texture, texture_size);

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGui::SeparatorText("Light Position");
        ImGui::SliderFloat("light x", &lightPosition.x, -10.0f, 10.0f);
        ImGui::SliderFloat("light y", &lightPosition.y, -10.0f, 10.0f);
        ImGui::SliderFloat("light z", &lightPosition.z, -10.0f, 10.0f);

        ImGui::SeparatorText("Model Position");
        ImGui::SliderFloat("x", &objetPosition.x, -10.0f, 10.0f);
        ImGui::SliderFloat("y", &objetPosition.y, -10.0f, 10.0f);
        ImGui::SliderFloat("z", &objetPosition.z, -10.0f, 10.0f);
        ImGui::SliderFloat("rotation", &objectRotation, -180.0f, 180.0f, "%.3f");

        ImGui::SeparatorText("Model Scale");
        ImGui::SliderFloat("scale", &objetScale, 0.0f, 5.0f, "%.3f");

        ImGui::End();
        ImGui::Render();
        ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
    }

protected:
    GLuint m_program;
    GLuint m_shadow_program;
    ShadowMap m_shadow_map;
    Mesh m_objet;
    Transform m_position;
    GLuint m_texture;
    Mesh m_repere;
    Mesh m_cube;
    Mesh m_frustum;
    Buffers m_buffers;
    std::vector<GLuint> m_textures;
    GLuint m_white_texture;
    std::vector<TriangleGroup> m_groups;
    std::vector<std::pair<Point, Point>> blocs;
    int location;
    Orbiter m_camera;

    // Imgui variables
    bool show_demo_window;
    Point lightPosition;
    Point objetPosition;
    float objectRotation;
    float objetScale;
};


int main( int argc, char **argv )
{
    TP tp;
    tp.run();
    return 0;
}
