
//! tuto_mdi_elements.cpp exemple d'utilisation de multidrawindirect pour des triangles indexes.

#include "app_camera.h"

#include "mesh.h"
#include "wavefront.h"

#include "orbiter.h"
#include "program.h"
#include "uniforms.h"

#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"

#include <chrono>

// representation des parametres de multidrawELEMENTSindirect
struct IndirectParam
{
    unsigned index_count;
    unsigned instance_count;
    unsigned first_index;
    unsigned vertex_base;
    unsigned instance_base;
};


class TP : public AppCamera
{
public:
    TP( ) : AppCamera(1024, 640, 4, 3) {}     // openGL version 4.3, ne marchera pas sur mac.

    int init( )
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplSdlGL3_Init(m_window, "#version 430");

        // charge un objet indexe
        m_objet= read_indexed_mesh("../data/bistro-small/export.obj");

        Point pmin, pmax;
        m_objet.bounds(pmin, pmax);
        m_camera.lookat(pmin, pmax);

        // trie les triangles de l'objet par matiere.
        std::vector<TriangleGroup> groups= m_objet.groups();

        /* parcours chaque groupe et construit les parametres du draw correspondant
            on peut afficher les groupes avec glDrawElements, comme d'habitude :

            glBindVertexArray(m_vao);
            glUseProgram(m_program);
            glUniform( ... );

            for(unsigned i= 0; i < groups.size(); i++)
                glDrawElements(GL_TRIANGLES, /count/ groups[i].n, /type/ GL_UNSIGNED_INT, /offset/ groups[i].first * sizeof(unsigned));

            on peut remplir une structure IndirectParam par draw... et utiliser un seul appel a multidrawindirect
        */

        std::vector<IndirectParam> params;
        for(unsigned i= 0; i < groups.size(); i++)
        {
            params.push_back({
                                     unsigned(groups[i].n),      // count
                                     1,                          // instance_count
                                     unsigned(groups[i].first),  // first_index
                                     0,                          // vertex_base, pas la peine de renumeroter les sommets
                                     0                           // instance_base, pas d'instances
                             });
        }

        m_draws= params.size();

        // transferer dans un buffer
        glGenBuffers(1, &m_indirect_buffer);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, params.size() * sizeof(IndirectParam), params.data(), GL_STATIC_READ);

        // construire aussi les buffers de l'objet
        m_vao= m_objet.create_buffers(/* use texcoord */ false, /* use normal */ true, /* use color */ false, /* use material index */ false);

        // et charge un shader..
        m_program= read_program("../src/tp/shaders/tp5/shader.glsl");
        program_print_errors(m_program);

        // etat openGL par defaut
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre

        glClearDepth(1.f);                          // profondeur par defaut
        glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST);                    // activer le ztest

        return 0;
    }

    int quit( )
    {
        ImGui_ImplSdlGL3_Shutdown();
        ImGui::DestroyContext();
        return 0;
    }

    int render( )
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // recupere les transformations
        Transform model;
        Transform view= camera().view();
        Transform projection= camera().projection();
        Transform mv= view * model;
        Transform mvp= projection * mv;

        // parametre le shader
        glBindVertexArray(m_vao);
        glUseProgram(m_program);

        program_uniform(m_program, "mvpMatrix", mvp);
        program_uniform(m_program, "mvMatrix", mv);

        // parametre du multidraw indirect
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, m_draws, 0);

        imguiWindow( );

        return 1;
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

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);


        ImGui::End();
        ImGui::Render();
        ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
    }

protected:
    GLuint m_indirect_buffer;
    GLuint m_vao;
    GLuint m_program;
    Transform m_model;

    Mesh m_objet;
    int m_draws;

    // Imgui variables
    bool show_demo_window;

    std::chrono::high_resolution_clock::time_point m_cpu_start;
    std::chrono::high_resolution_clock::time_point m_cpu_stop;
    GLint64 m_frame_time;
};

int main( )
{
    TP app;
    app.run();

    return 0;
}
