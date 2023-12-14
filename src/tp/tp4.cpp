//
// Created by lucas on 21/11/23.
//


//! \file tuto_compute_buffer.cpp compute shader + buffers

#include <vector>

#include "app.h"
#include "program.h"
#include "uniforms.h"

#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"

#include "app_time.h"

class ComputeBuffer : public AppTime
{
public:

    ComputeBuffer( ) : AppTime(1280, 768, 4,3) {}

    int init( )
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplSdlGL3_Init(m_window, "#version 330");

        m_program= read_program("../src/tp/shaders/tp4/compute_buffer2.glsl");
        program_print_errors(m_program);

        // initialise un tableau de valeurs
        m_data= std::vector<int>(1024);

        // cree les buffers pour les parametres du shader
        glGenBuffers(1, &m_gpu_buffer1);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_gpu_buffer1);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * m_data.size(), m_data.data(), GL_STATIC_COPY);

        // buffer resultat, meme taille, mais pas de donnees...
        glGenBuffers(1, &m_gpu_buffer2);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_gpu_buffer2);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * m_data.size(), nullptr, GL_STATIC_COPY);
        return 0;
    }

    int quit( )
    {
        release_program(m_program);
        glDeleteBuffers(1, &m_gpu_buffer1);
        glDeleteBuffers(1, &m_gpu_buffer2);

        ImGui_ImplSdlGL3_Shutdown();
        ImGui::DestroyContext();

        return 0;
    }

    int render( )
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_gpu_buffer1);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_gpu_buffer2);

        // execute les shaders
        glUseProgram(m_program);

        // recupere le nombre de threads declare par le shader
        int threads[3]= {};
        glGetProgramiv(m_program, GL_COMPUTE_WORK_GROUP_SIZE, threads);

        int n= m_data.size() / threads[0];
        if(m_data.size() % threads[0])
            n++;

        int value = 12;

        program_uniform(m_program, "value", value);

        // go !
        glDispatchCompute(n, 1, 1);

        // attendre que les resultats soient disponibles
        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

        // relire le resultat
        std::vector<int> tmp(1024);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_gpu_buffer2);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int) * tmp.size(), tmp.data());

        for(unsigned i= 0; i < tmp.size(); i++)
            printf("%d ", tmp[i]);
        printf("\n");

        return 0;
    }

    std::vector<int> m_data;
    GLuint m_gpu_buffer1;
    GLuint m_gpu_buffer2;
    GLuint m_read_buffer;
    GLuint m_program;
};

int main( )
{
    ComputeBuffer app;
    app.run();

    return 0;
}
