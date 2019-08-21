
#define GLEW_STATIC
#include <sstream>
#include "include/display.hpp"
#include "include/shaders.hpp"
#include "include/scene.hpp"
#include "include/init.hpp"

int main(int ac, char **av) {
    Display             *d;
    GLuint              computeProgram;
    GLuint              renderProgram;
    GLuint              buffer;
    GLuint              vao;
    GLuint              texture;
    sf::Shader          *shader;
    Scene               *scene;
    sf::Clock           *clock;
    glm::ivec2          resolution;
    float               frames;
    float               time;

    clock = new sf::Clock();

    d = new Display("Test", 1024, 768);
    initGL();

    computeProgram = LoadShaders("shaders/raytrace.cs");

    shader = new sf::Shader();
    std::cout << "Shader Loaded: " << shader->loadFromFile("shaders/vertex.vs", "shaders/fragment.fs") << std::endl;
    renderProgram = shader->getNativeHandle();

    resolution = glm::ivec2(400, 225);

    glActiveTexture(GL_TEXTURE0);
    initBuffers(&vao, &buffer);
    initTexture(&texture, resolution.x, resolution.y);
    char* filename = "gg.png";
    initScene(&scene, filename);

    frames = 0;
    time = 0;
    clock->restart();

    while (d->isClosed() == false) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // COMPUTE PHASE
        glUseProgram(computeProgram);

        glUniform2f(glGetUniformLocation(computeProgram, "uSize"), resolution.x, resolution.y);
        // glUniform1f(glGetUniformLocation(computeProgram, "uTime"), time);

        setSceneUniforms(computeProgram, scene);
        glDispatchCompute(resolution.x, resolution.y, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glUseProgram(0);
        // ---

        // DISPLAY PHASE
        sf::Shader::bind(shader);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(0);

        sf::Shader::bind(0);
        // ---

        d->update(&scene->camera.pos, &scene->camera.rot, &texture, &resolution);

        time += 1;
        frames += 1;

        if (clock->getElapsedTime().asSeconds() >= 1.F) {
            std::cout << "FPS Count: " << frames << std::endl;
            frames = 0;
            clock->restart();
        }
    }
    glDeleteBuffers(1, &buffer);
    return 0;
}
