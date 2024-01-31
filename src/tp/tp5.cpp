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
        // init GUI
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplSdlGL3_Init(m_window, "#version 330");

        show_demo_window = false;
        isRealTimeShadow = false;
        isMDI = false;
        lightPosition= Point(-50, 50, -15);
        shadowLightPosition = Point(0,0,45);
        shadowLightRotation = Point(-90, 0, 0);
        objetScale = 0.1f;

        // init Scene
        m_camera.projection(window_width(), window_height(), 45);
        m_position = Identity();
        m_frustum = make_frustum();

        m_objet = read_indexed_mesh("../data/bistro-small/export.obj");
        if(m_objet.materials().count() == 0) return -1;
        if(!m_objet.vertex_count()) return -1;
        Materials& materials= m_objet.materials();

        m_textures.resize(materials.filename_count());
        {
            for(unsigned i= 0; i < m_textures.size(); i++)
            {
                m_textures[i] = read_image_data(materials.filename(i));

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
            }
            printf("%d textures.\n", materials.filename_count());
        }
        m_groups = m_objet.groups();

        m_texture_array = make_texture_array(0, m_textures, GL_RGBA);

        std::vector<IndirectParam> indirect_params;
        for(unsigned i= 0; i < m_groups.size(); i++)
        {
            indirect_params.push_back({
                                              unsigned(m_groups[i].n),      // count
                                              1,                          // instance_count
                                              unsigned(m_groups[i].first),  // first_index
                                              0,                          // vertex_base, pas la peine de renumeroter les sommets
                                              0                           // instance_base, pas d'instances
                                      });
        }

        m_mdi_draws = indirect_params.size();

        // transferer dans un buffer
        glGenBuffers(1, &m_indirect_buffer);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, indirect_params.size() * sizeof(IndirectParam), indirect_params.data(), GL_STATIC_READ);

        GLuint ssbo;
        glGenBuffers(1, &ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

        std::vector<unsigned> textureIndices;

        for(int i = 0; i < materials.materials.size(); i++) {
            textureIndices.push_back(materials.materials[i].diffuse_texture);
        }

        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * textureIndices.size(), textureIndices.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

        m_vao= m_objet.create_buffers(/* use texcoord */ true, /* use normal */ true, /* use color */ false, /* use material index */ true);

        m_white_texture= read_texture(0, "../data/blanc.jpg");

        m_objet.bounds(pmin, pmax);
        m_camera.lookat(pmin, pmax);

        // Faire la bounding box de l'objet
        m_bbox = make_bbox(pmin, pmax);
        //m_bounding_box = Box();
        // https://forge.univ-lyon1.fr/m1if27/bvh-etu

        m_shadow_map.create(4096, 4096);
        m_shadow_map.clear_color(White());
        m_shadow_map.clear_depth(1);

        m_shadow_program= read_program("../src/tp/shaders/tp5/depth_shader.glsl");
        program_print_errors(m_shadow_program);

        m_program = read_program("../src/tp/shaders/tp5/shader.glsl");
        program_print_errors(m_program);

        glClearColor(0.2f, 0.2f, 0.2f, 1.f);
        glClearDepth(1.f);
        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        make_shadow_map();

        return 0;
    }

    int quit( )
    {
        ImGui_ImplSdlGL3_Shutdown();
        ImGui::DestroyContext();

        m_objet.release();
        m_frustum.release();

        m_shadow_map.release();

        release_program(m_shadow_program);
        release_program(m_program);

        return 0;
    }

    int render( )
    {
        cameraUpdate();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Point pMin, pMax;
        m_objet.bounds(pMin, pMax);

        scene_bistro();

        handleKeys();

        imguiWindow();

        return 1;
    }

    void scene_bistro() {

        if(isRealTimeShadow) {
            make_shadow_map();
        }

        // 2ème passe
        Transform view= m_camera.view();
        Transform projection= m_camera.projection();

        glViewport(0, 0, window_width(), window_height());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, m_shadow_map.texture);
        glGenerateMipmap(GL_TEXTURE_2D);

        if(key_state('e'))
        {
            view= decal_view;
            projection= decal_projection;
        }

        glBindVertexArray(m_vao);
        glUseProgram(m_program);

        Transform model= Identity();
        Transform mvp= projection * view * model;
        Transform mv= view * model;
        Transform decal= Viewport(1, 1) * decal_projection * decal_view;

        program_uniform(m_program, "mvpMatrix", mvp);
        program_uniform(m_program, "mvMatrix", mv);
        program_uniform(m_program, "decalMatrix", decal);
        program_uniform(m_program, "lightPosition", lightPosition);
        glActiveTexture(GL_TEXTURE0); // Ou une autre unité de texture si nécessaire
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture_array);
        program_use_texture(m_program, "decal_texture", 1, m_shadow_map.texture);

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, m_mdi_draws, 0);

        if (key_state('r')) {
            Transform decal_m= Inverse(decal_projection * decal_view);
            draw(m_frustum, decal_m, view, projection);
        }

        if (key_state('b')) {
            draw(m_bbox, model, view, projection);
        }
    }

    void make_shadow_map() {
        Transform r= RotationX(shadowLightRotation.x) *
                     RotationY(shadowLightRotation.y) *
                     RotationZ(shadowLightRotation.z);

        Transform t= Translation(shadowLightPosition.x,
                                 shadowLightPosition.y,
                                 shadowLightPosition.z);
        Transform m= r * t;

        decal_view= Inverse(m_position * m);
        decal_projection= Ortho(-40, 40, -40, 40, float(0.1), float(75));

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_shadow_map.framebuffer);
        glViewport(0, 0, 4096, 4096);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_shadow_program);
        program_uniform(m_shadow_program, "mvpMatrix", decal_projection * decal_view);
        m_objet.draw(m_shadow_program, true, false, false, false, false);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
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

        ImGui::SeparatorText("Parameters");
        ImGui::Checkbox("Multi Draw Indirect", &isMDI); ImGui::SameLine();
        ImGui::Checkbox("Real Time Shadow", &isRealTimeShadow);

        ImVec2 texture_size(350, 350);
        ImGui::Image((void*)(intptr_t)m_shadow_map.texture, texture_size);

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGui::SeparatorText("Ambient Light Position");
        ImGui::SliderFloat("Pos x", &lightPosition.x, -50.0f, 50.0f);
        ImGui::SliderFloat("Pos y", &lightPosition.y, -50.0f, 50.0f);
        ImGui::SliderFloat("Pos z", &lightPosition.z, -50.0f, 50.0f);

        ImGui::SeparatorText("Shadow Light Position");
        ImGui::SliderFloat("Shadow Pos x", &shadowLightPosition.x, -100.0f, 100.0f);
        ImGui::SliderFloat("Shadow Pos y", &shadowLightPosition.y, -100.0f, 100.0f);
        ImGui::SliderFloat("Shadow Pos z", &shadowLightPosition.z, -100.0f, 100.0f);
        ImGui::SliderFloat("Rot x", &shadowLightRotation.x, -180.0f, 180.0f);
        ImGui::SliderFloat("Rot y", &shadowLightRotation.y, -180.0f, 180.0f);
        ImGui::SliderFloat("Rot z", &shadowLightRotation.z, -180.0f, 180.0f);

        ImGui::End();
        ImGui::Render();
        ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
    }

protected:
    GLuint m_program;
    Mesh m_objet;
    Point pmin, pmax;
    Transform m_position;
    Orbiter m_camera;

    // Textures
    GLuint m_white_texture;
    std::vector<ImageData> m_textures;
    std::vector<TriangleGroup> m_groups;

    // Frustum Culling
    Mesh m_frustum;
    Mesh m_bbox;

    // Shadow Map
    GLuint m_shadow_program;
    ShadowMap m_shadow_map;
    Transform decal_view;
    Transform decal_projection;

    // Multi Draw Indirect
    unsigned long m_mdi_draws;
    GLuint m_indirect_buffer;
    GLuint m_vao;
    GLuint m_texture_array;

    // Imgui variables
    bool show_demo_window;
    bool isRealTimeShadow;
    bool isMDI;
    Point lightPosition;
    Point shadowLightPosition;
    Point shadowLightRotation;
    float objetScale;
};


int main( int argc, char **argv )
{
    TP tp;
    tp.run();
    return 0;
}
