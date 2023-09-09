//
// Created by lucas on 08/09/23.
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

        // cree et configure un vertex array object: conserve la description des attributs de sommets
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // cree et initialise le buffer stockant les positions des sommets
        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertex_buffer_size(), mesh.vertex_buffer(), GL_STATIC_DRAW);
        // attention : le buffer est implicite, c'est celui qui est selectionne sur GL_ARRAY_BUFFER

        // attribut 0, position des sommets, declare dans le vertex shader : layout(location= 0) in vec3 position;
        glVertexAttribPointer(0, // numero de l'attribut, cf declaration dans le shader
                              3, GL_FLOAT,         // size et type, position est un vec3 dans le vertex shader
                              GL_FALSE,            // pas de normalisation des valeurs
                              0,                   // stride 0, les valeurs sont les unes a la suite des autres
                              0                    // offset 0, les valeurs sont au debut du buffer
        );
        glEnableVertexAttribArray(0);   // numero de l'attribut, cf declaration dans le shader
        // attention : le vertex array selectionne est un parametre implicite
        // attention : le buffer selectionne sur GL_ARRAY_BUFFER est un parametre implicite

        // conserve le nombre de sommets
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
        x= 0, y= 10, z= 10;
        lightPosition= Point(0, 10, 10);
        nbOctaves= 4;
        color= Red();

        m_repere= make_grid(10);
        m_cube = read_mesh("../data/cube.obj");
        if(m_cube.materials().count() == 0) return -1;
        if(!m_cube.vertex_count()) return -1;

        //m_objet= Mesh(GL_TRIANGLES);
        //{ /* ajouter des triplets de sommet == des triangles dans objet... */ }
        m_objet = read_mesh("../data/bigguy.obj");
        if(m_objet.materials().count() == 0) return -1;
        if(!m_objet.vertex_count()) return -1;
        m_triangleGroups = m_objet.groups();

        m_buffers.create(m_objet);
        //m_objet.release();

        m_program = read_program("../data/shaders/shadows.glsl");
        program_print_errors(m_program);

        glClearColor(0.2f, 0.2f, 0.2f, 1.f);  // couleur par defaut de la fenetre
        glClearDepth(1.f);                                  // profondeur par defaut
        glDepthFunc(GL_LESS);                                // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST);                             // activer le ztest

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

        draw(m_repere, Identity(), camera());

        //draw(m_cube, Identity() * Translation(x,y,z), camera());

        glUseProgram(m_program);
        setUniforms();
        //glBindVertexArray(m_buffers.vao);
        //glDrawArrays(GL_TRIANGLES, 0, m_buffers.vertex_count);
        m_objet.draw(m_program, true, false, true, false, false);

        handleKeys();

        imguiWindow();

        return 1;
    }

    void question6()
    {
        const Materials& materials= m_objet.materials();

        for(unsigned i= 0; i < m_triangleGroups.size(); i++)
        {
            const TriangleGroup& group= m_triangleGroups[i];

            glUseProgram(m_program);
            {
                setUniforms();

                glBindVertexArray(m_buffers.vao);
                glDrawArrays(GL_TRIANGLES, group.first, group.n);
            }
        }

    }

    void setUniforms()
    {
        Transform view= camera().view();
        Transform projection= camera().projection();
        Transform model= Identity() * Scale(0.1f) * Translation(0, 9, 0);
        Transform mvp= projection * view * model;

        //Color color = materials.materials.at(group.index).diffuse;

        location= glGetUniformLocation(m_program, "mvpMatrix");
        glUniformMatrix4fv(location, 1, GL_TRUE, mvp.data());

        location= glGetUniformLocation(m_program, "lightPosition");
        glUniform3f(location, lightPosition.x, lightPosition.y, lightPosition.z);

        location= glGetUniformLocation(m_program, "diffuse");
        glUniform4f(location, color.r, color.g, color.b, color.a);

        location= glGetUniformLocation(m_program, "nbOctaves");
        glUniform1i(location, nbOctaves);
    }

    void handleKeys( )
    {
        if(key_state(SDLK_LEFT) || key_state(SDLK_q))
            lightPosition = lightPosition - Vector(0.1f, 0, 0);
        if(key_state(SDLK_RIGHT) || key_state(SDLK_d))
            lightPosition = lightPosition + Vector(0.1f, 0, 0);
        if(key_state(SDLK_UP) || key_state(SDLK_z))
            lightPosition = lightPosition + Vector(0, 0, 0.1f);
        if(key_state(SDLK_DOWN) || key_state(SDLK_s))
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

        ImGui::Begin("Settings");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Spacing();
        ImGui::Checkbox("Demo Window", &show_demo_window);
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGui::SeparatorText("Light Position");
        ImGui::SliderFloat("x", &lightPosition.x, -10.0f, 10.0f);
        ImGui::SliderFloat("y", &lightPosition.y, -10.0f, 10.0f);
        ImGui::SliderFloat("z", &lightPosition.z, -10.0f, 10.0f);

        ImGui::SeparatorText("Toon Subdivision");
        ImGui::SliderInt("nbOctaves", &nbOctaves, 1, 30, "%d", ImGuiSliderFlags_Logarithmic);

        ImGui::SeparatorText("Model Color");
        ImGui::ColorEdit3("color", (float*)&color);

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
    std::vector<TriangleGroup> m_triangleGroups;
    int location;

    // Imgui variables
    bool show_demo_window;
    float x, y, z;
    Point lightPosition;
    int nbOctaves;
    Color color;
};


int main( int argc, char **argv )
{
    TP tp;
    tp.run();
    return 0;
}
