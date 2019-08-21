
#define GLEW_STATIC
#include <sstream>
#include "include/display.hpp"
#include "include/shaders.hpp"
#include "include/scene.hpp"
#include "include/init.hpp"

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
using namespace std;

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768

GLuint              buffer;
GLuint              vao;
GLuint              texture;
Scene               *scene;
GLuint ShaderProgram;
GLuint ShaderProgram2;

static bool ReadFile(const char* pFileName, string& outFile)
{
    ifstream f(pFileName);

    bool ret = false;

    if (f.is_open()) {
        string line;
        while (getline(f, line)) {
            outFile.append(line);
            outFile.append("\n");
        }

        f.close();

        ret = true;
    }
    else {
        printf("read file error!");
    }

    return ret;
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(1);
    }

    const GLchar* p[1];
    p[0] = pShaderText;
    GLint Lengths[1];
    Lengths[0]= strlen(pShaderText);
    glShaderSource(ShaderObj, 1, p, Lengths);
    glCompileShader(ShaderObj);
    GLint success;
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }
    glAttachShader(ShaderProgram, ShaderObj);
}
static void CompileShaders2(){
    std::cout << "glCreateProgram begin" << std::endl;
    ShaderProgram2 = glCreateProgram();
    std::cout << "glCreateProgram end" << std::endl;

    if (ShaderProgram2 == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }
    const char* pCSFileName = "shaders/raytrace.cs";
    string cs;
    if (!ReadFile(pCSFileName, cs)) {
        exit(1);
    };
    std::cout << "add shader begin" << std::endl;
    AddShader(ShaderProgram2, cs.c_str(), GL_COMPUTE_SHADER);
    std::cout << "add shader end" << std::endl;

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };

    glLinkProgram(ShaderProgram2);
    std::cout << "glLinkProgram end" << std::endl;
    glGetProgramiv(ShaderProgram2, GL_LINK_STATUS, &Success);
    if (Success == 0) {
        glGetProgramInfoLog(ShaderProgram2, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
    }

    glValidateProgram(ShaderProgram2);

    std::cout << "glValidateProgram end" << std::endl;
    glGetProgramiv(ShaderProgram2, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(ShaderProgram2, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
}

static void CompileShaders()
{
    ShaderProgram = glCreateProgram();

    if (ShaderProgram == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }
    const char* pVSFileName = "shaders/vertex.vs";
    const char* pFSFileName = "shaders/fragment.fs";
    string vs, fs ;

    if (!ReadFile(pVSFileName, vs)) {
        exit(1);
    };

    if (!ReadFile(pFSFileName, fs)) {
        exit(1);
    };
    AddShader(ShaderProgram, vs.c_str(), GL_VERTEX_SHADER);
    AddShader(ShaderProgram, fs.c_str(), GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };

    glLinkProgram(ShaderProgram);
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success);
    if (Success == 0) {
        glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
    }

    glValidateProgram(ShaderProgram);
    glGetProgramiv(ShaderProgram, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
}

static void RenderSceneCB()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(ShaderProgram2);
    glUniform2f(glGetUniformLocation(ShaderProgram2, "uSize"), 400, 225);

    setSceneUniforms(ShaderProgram2, scene);
    glDispatchCompute(400, 225, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glUseProgram(ShaderProgram);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(0);
    glUseProgram(0);

    glDeleteTextures(1, &texture);
    initTexture(&texture, 400, 225);

    glutSwapBuffers();
}
static void InitializeGlutCallbacks()
{
    glutDisplayFunc(RenderSceneCB);
    glutIdleFunc(RenderSceneCB);
}

int main(int ac, char **av) {

    glutInit(&ac, av);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Tutorial 14");
    GLenum res = glewInit();
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }
    initGL();
    std::cout << "CompileShaders2" << std::endl;
    CompileShaders2();
    std::cout << "CompileShaders" << std::endl;
    CompileShaders();

    glActiveTexture(GL_TEXTURE0);
    initBuffers(&vao, &buffer);
    initTexture(&texture, 400, 225);
    char* filename = "gg.png";
    initScene(&scene, filename);
    std::cout << "InitializeGlutCallbacks" << std::endl;
    InitializeGlutCallbacks();
    std::cout << "glutMainLoop" << std::endl;
    glutMainLoop();

    return 0;
}
