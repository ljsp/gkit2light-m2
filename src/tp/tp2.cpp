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

class TP : public AppCamera
{
public:
    TP( ) : AppCamera(1920, 1080) {}

    int init( )
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplSdlGL3_Init(m_window, "#version 330");

        show_demo_window = false;
        lightPosition= Point(0, 10, 10);
        objetPosition= Point(1, 0, 0);
        objectRotation = 0;
        objetScale = 0.5f;

        m_repere= make_grid(10);
        m_cube = read_mesh("../data/cube.obj");
        if(m_cube.materials().count() == 0) return -1;
        if(!m_cube.vertex_count()) return -1;

        m_objet = read_mesh("../data/cube.obj");
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
        m_groups = m_objet.groups();

        m_buffers.create(m_objet);

        m_program = read_program("../data/shaders/shader.glsl");
        program_print_errors(m_program);

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
        release_program(m_program);
        ImGui_ImplSdlGL3_Shutdown();
        ImGui::DestroyContext();
        return 0;
    }

    int render( )
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //draw(m_repere, Identity(), camera());

        scene_bistro();

        handleKeys();

        imguiWindow();

        return 1;
    }

    void scene_bistro() {

        Transform view= camera().view();
        Transform projection= camera().projection();
        Transform model= Identity();

        Transform mvp= projection * view * model;
        Transform mv= view * model;

        glUseProgram(m_program);

        program_uniform(m_program, "mvpMatrix", mvp);
        program_uniform(m_program, "mvMatrix", mv);
        program_uniform(m_program, "lightPosition", lightPosition);

        const Materials& materials= m_objet.materials();
        for(unsigned i= 0; i < m_groups.size(); i++)
        {
            const Material& material= materials(m_groups[i].index);

            program_uniform(m_program, "material_color", material.diffuse);

            if(material.diffuse_texture != -1)
                program_use_texture(m_program, "material_texture", 0, m_textures[material.diffuse_texture]);
            else
                program_use_texture(m_program, "material_texture", 0, m_white_texture);

            frustumCulling();

            m_objet.draw(m_groups[i].first, m_groups[i].n, m_program, true,
                         true, true, false, false);
        }
    }

    bool frustumCulling() {
        // Faire la bounding box de l'objet
        Point p000, p111;
        m_objet.bounds(p000, p111);

        Point p001 = Point(p000.x, p000.y, p111.z);
        Point p010 = Point(p000.x, p111.y, p000.z);
        Point p011 = Point(p000.x, p111.y, p111.z);
        Point p100 = Point(p111.x, p000.y, p000.z);
        Point p101 = Point(p111.x, p000.y, p111.z);
        Point p110 = Point(p111.x, p111.y, p000.z);

        Point boundingBox[8] = {p000, p001, p010, p011,p100, p101, p110, p111};

        // Passer ses coordonnées dans le repère projectif
        Transform pvMatrix = camera().projection() * camera().view();
        vec4 projectedBoundingBox[8];
        for(int i = 0; i < 8; i++) {
            projectedBoundingBox[i] = pvMatrix(boundingBox[i]);
        }

        // Faire le test des coordonnées pour savoir si l'objet est visible dans le repère projectif
        if(!isVisibleInProjectedSpace(projectedBoundingBox)) {
            std::cout << "Cube non visible" << std::endl;
            return false;
        }

        //Si on a pas pu déterminer on regarde dans le repère de l'objet
        Transform pvInverse = Inverse(pvMatrix);
        vec4 inverseBoundingBox[8];
        for(int i = 0; i < 8; i++) {
            inverseBoundingBox[i] = pvInverse(projectedBoundingBox[i]);
        }

        if(!isVisibleInObjectSpace(p000, p111, inverseBoundingBox)) {
            std::cout << "Cube non visible" << std::endl;
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

        ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove);
        ImGui::Text("Application average %.1f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Spacing();
        ImGui::Checkbox("Demo Window", &show_demo_window);
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
    Mesh m_objet;
    Mesh m_repere;
    Mesh m_cube;
    Buffers m_buffers;
    std::vector<GLuint> m_textures;
    GLuint m_white_texture;
    std::vector<TriangleGroup> m_groups;
    int location;

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
