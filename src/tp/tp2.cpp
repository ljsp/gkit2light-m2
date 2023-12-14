//
// Created by lucas on 19/09/23.
//

#include "wavefront.h"
#include "texture.h"

#include "orbiter.h"
#include "draw.h"

#include "program.h"
#include "uniforms.h"

#include "Utils.h"

#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include "app_time.h"

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
        m_ground= make_ground(20);
        m_cube = read_mesh("../data/cube.obj");
        if(m_cube.materials().count() == 0) return -1;
        if(!m_cube.vertex_count()) return -1;
        m_frustum = make_frustum();

        m_objet = read_mesh("../data/bistro-small/export.obj");
        //m_objet = read_mesh("../data/robot.obj");
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

        m_shadow_program= read_program("../src/tp/shaders/tp2/draw_shadows.glsl");
        program_print_errors(m_shadow_program);

        m_program = read_program("../src/tp/shaders/tp2/shader.glsl");
        //m_program = read_program("../data/shaders/decal.glsl");
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
        m_ground.release();

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

        Transform decal_viewport= Viewport(1, 1);
        Transform decal= decal_viewport * decal_projection * decal_view;

        program_uniform(m_program, "mvpMatrix", mvp);
        program_uniform(m_program, "mvMatrix", mv);
        program_uniform(m_program, "lightPosition", lightPosition);
        program_uniform(m_program, "decalMatrix", decal);
        program_use_texture(m_program, "decal_texture", 0, m_shadow_map.texture);

        const Materials& materials= m_objet.materials();

        for(unsigned i= 0; i < m_groups.size(); i++)
        {
            const Material& material= materials(m_groups[i].index);

            program_uniform(m_program, "material_color", material.diffuse);

            if(material.diffuse_texture != -1) {
                program_use_texture(m_program, "material_texture", 0, m_textures[material.diffuse_texture]);
            }
            else {
                program_use_texture(m_program, "material_texture", 0, m_white_texture);
            }

            // Faire frustum culling des blocs

            m_objet.draw(m_groups[i].first, m_groups[i].n, m_program, true, true, true, false, false);
        }
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
    Mesh m_ground;
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
