//
// Created by lucas on 21/11/23.
//

#include <vector>
#include <chrono>
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

        return 0;
    }

    int quit( )
    {
        release_program(m_program);
        glDeleteBuffers(1, &m_read_buffer);
        glDeleteBuffers(1, &m_write_buffer);

        ImGui_ImplSdlGL3_Shutdown();
        ImGui::DestroyContext();

        return 0;
    }

    void ajouteCPU( const int value, const std::vector<int>& entree, std::vector<int>& sortie )
    {
        sortie.resize(entree.size());
        for(unsigned i= 0; i < entree.size(); i++) {
            sortie[i]= entree[i] + value;
        }
    }

    void ajouteGPU( const int value, const std::vector<int>& entree, std::vector<int>& sortie )
    {
        // cree les buffers pour les parametres du shader
        glGenBuffers(1, &m_read_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_read_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * entree.size(), entree.data(), GL_STATIC_COPY);

        // buffer resultat, meme taille, mais pas de donnees...
        glGenBuffers(1, &m_write_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_write_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * entree.size(), nullptr, GL_STATIC_COPY);

        // execute les shaders
        glUseProgram(m_program);

        // recupere le nombre de threads declare par le shader
        int threads[3]= {};
        glGetProgramiv(m_program, GL_COMPUTE_WORK_GROUP_SIZE, threads);

        int n = m_data.size() / threads[0];
        if(m_data.size() % threads[0]) {
            n++;
        }

        program_uniform(m_program, "value", value);

        // go !
        glDispatchCompute(n, 1, 1);

        // attendre que les resultats soient disponibles
        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

        // relire le resultat
        sortie.resize(entree.size());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_write_buffer);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int) * sortie.size(), sortie.data());
    }

    int render( )
    {
        int value = 15;
        m_data = std::vector<int>(1024);
        std::vector<int> tmp, tmp2;

        auto beg = std::chrono::high_resolution_clock::now();
        ajouteCPU(value, m_data, tmp);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - beg;
        printf("CPU: %f\n", elapsed.count());

        beg = std::chrono::high_resolution_clock::now();
        ajouteGPU(value, m_data, tmp2);
        end = std::chrono::high_resolution_clock::now();
        elapsed = end - beg;
        printf("GPU: %f\n", elapsed.count());

        for(unsigned i= 0; i < tmp.size(); i++) {
            printf("%d ", tmp[i]);
        }
        printf("\n");

        for(unsigned i= 0; i < tmp2.size(); i++) {
            printf("%d ", tmp2[i]);
        }
        printf("\n");

        return 0;
    }

    std::vector<int> m_data;
    GLuint m_read_buffer;
    GLuint m_write_buffer;
    GLuint m_program;
};

int main( )
{
    ComputeBuffer app;
    app.run();

    return 0;
}
