#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <QString>
#include <QByteArray>
#include <QDebug>
#include "imageloader.h"

using namespace std;
using glm::mat4;
using glm::vec3;

GLuint m_VBO;//操作顶点缓冲器对象
GLuint m_IBO;// 索引缓冲对象的句柄
GLuint  render_prog;
float g_angle = 0;
float g_windowWidth = 500;
float g_windowHeight = 500;

GLuint texture_buffer;
GLuint m_textureObj;
GLuint texUnit_loc;

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
    //创建shader着色器对象
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }

    //在编译shader对象之前我们必须先定义它的代码源
    const GLchar* p[1];
    p[0] = pShaderText;
    GLint Lengths[1];
    Lengths[0]= strlen(pShaderText);
    glShaderSource(ShaderObj, 1, p, Lengths);

    //编译shader对象
    glCompileShader(ShaderObj);

    //获得编译状态，并且可以打印编译器碰到的所有编译错误
    GLint success;
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }
    //将编译好的shader对象绑定在program object程序对象上
    glAttachShader(ShaderProgram, ShaderObj);
}

void display(){

    glClearColor(0.2f,0.2f,0.2f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    //  transform the box
    glm::mat4 projection = glm::perspective(100.0f, (GLfloat)(g_windowWidth/g_windowHeight), 0.1f, 400.f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f),
                     glm::vec3(0.0f, 0.0f, 0.0f),
                     glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 model = mat4(1.0f);
    model *= glm::rotate((float)g_angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model *= glm::rotate(90.0f, vec3(1.0f, 0.0f, 0.0f));
    //model *= glm::translate(glm::vec3(-0.5f, -0.5f, -0.5f));
    glm::mat4 mvp = projection * view * model;
    GLuint mvpIdx = glGetUniformLocation(render_prog, "MVP");
    if (mvpIdx >= 0)
    {
        glUniformMatrix4fv(mvpIdx, 1, GL_FALSE, &mvp[0][0]);
    }

    glUniform1i(texUnit_loc, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureObj);//绑定纹理

    glEnableVertexAttribArray(0);//开启顶点属性

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, texture_buffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,0, 0);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
    //glDrawElements(GL_QUADS, 24, GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES,0,108);
    glDisableVertexAttribArray(0);//禁用顶点数据
    glutSwapBuffers();// 交换前后缓存
}

void rotateDisplay(){
    g_angle = (g_angle + 0.0005);
    if(g_angle > 360.0){
        g_angle -= 360.0;
    }
    glutPostRedisplay();
}

void initShader(){
    render_prog = glCreateProgram();

    static const char render_vs[] =
        "#version 430 core\n"
        "\n"
        "layout(location = 0) in vec3 VerPos;\n"
        "\n"
        "layout(location = 1) in vec2 vTexCoord;\n"
        "\n"
        "out vec3 Color;;\n"
        "\n"
        "out vec2 texCoord;\n"
        "\n"
        "uniform mat4 MVP;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    texCoord = vTexCoord;\n"
        "    gl_Position = MVP * vec4(VerPos, 1.0);\n"
        "    Color = vec3(VerPos.xyz) + 0.5;\n"
        "}\n";

    static const char render_fs[] =
        "#version 430 core\n"
        "\n"
        "layout (location = 0) out vec4 color;\n"
        "\n"
        "in vec2 texCoord;\n"
        "\n"
        "uniform sampler2D theTexture;\n"
        "\n"
        "in vec3 Color;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    color = texture(theTexture, texCoord);\n"
        "}\n";//vec4(Color, 1.0);

    AddShader(render_prog, render_vs, GL_VERTEX_SHADER);
    AddShader(render_prog, render_fs, GL_FRAGMENT_SHADER);
    glLinkProgram(render_prog);
    glUseProgram(render_prog);

    texUnit_loc = glGetUniformLocation(render_prog, "theTexture");
}

bool LoadTexture()
{
    Image* tmpimg = loadBMP("./volume2/world.bmp");

    glGenTextures(1, &m_textureObj);//生成纹理队形
    glBindTexture(GL_TEXTURE_2D, m_textureObj);//绑定对象
    //指定一个二维纹理图像
    qDebug() << tmpimg->width << tmpimg->height;
    glTexImage2D(GL_TEXTURE_2D, 0, 3, tmpimg->width, tmpimg->height,
                 0, GL_RGB, GL_UNSIGNED_BYTE, tmpimg->pixels);
    //放大和缩小指定过滤方式
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //纹理环绕方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    return true;
}

void init(){
    GLfloat vertices[108] = {
        -0.5, -0.5, -0.5,
        -0.5, -0.5,  0.5,
        -0.5,  0.5, -0.5,

        -0.5, -0.5,  0.5,
        -0.5,  0.5, -0.5,
        -0.5,  0.5,  0.5,

        -0.5, -0.5, -0.5,
        -0.5, -0.5,  0.5,
         0.5, -0.5, -0.5,

        -0.5, -0.5,  0.5,
         0.5, -0.5, -0.5,
         0.5, -0.5,  0.5,

         0.5,  0.5, -0.5,
         0.5,  0.5,  0.5,
        -0.5,  0.5, -0.5,

         0.5,  0.5,  0.5,
        -0.5,  0.5, -0.5,
        -0.5,  0.5,  0.5,

         0.5,  0.5, -0.5,
         0.5,  0.5,  0.5,
         0.5, -0.5, -0.5,

         0.5,  0.5,  0.5,
         0.5, -0.5, -0.5,
         0.5, -0.5,  0.5,

        -0.5, -0.5, 0.5,
         0.5, -0.5, 0.5,
         0.5,  0.5, 0.5,

        -0.5, -0.5, 0.5,
        -0.5,  0.5, 0.5,
         0.5,  0.5, 0.5,

        -0.5, -0.5, -0.5,
         0.5, -0.5, -0.5,
         0.5,  0.5, -0.5,

        -0.5, -0.5, -0.5,
        -0.5,  0.5, -0.5,
         0.5,  0.5, -0.5,
    };
    GLuint indices[24] = {
        1,5,7,3,
        0,2,6,4,
        0,1,3,2,
        7,5,4,6,
        2,3,7,6,
        1,0,4,5
    };
    vector<glm::vec2> vecTex;
    vecTex.push_back(glm::vec2(0.25,0.60));
    vecTex.push_back(glm::vec2(0.25,0.30));
    vecTex.push_back(glm::vec2(0.00,0.60));

    vecTex.push_back(glm::vec2(0.25,0.30));
    vecTex.push_back(glm::vec2(0.00,0.60));
    vecTex.push_back(glm::vec2(0.00,0.30));

    vecTex.push_back(glm::vec2(0.25,0.60));
    vecTex.push_back(glm::vec2(0.25,0.30));
    vecTex.push_back(glm::vec2(0.50,0.60));

    vecTex.push_back(glm::vec2(0.25,0.30));
    vecTex.push_back(glm::vec2(0.50,0.60));
    vecTex.push_back(glm::vec2(0.50,0.30));

    vecTex.push_back(glm::vec2(0.75,0.60));
    vecTex.push_back(glm::vec2(0.75,0.30));
    vecTex.push_back(glm::vec2(1.00,0.60));

    vecTex.push_back(glm::vec2(0.75,0.30));
    vecTex.push_back(glm::vec2(1.00,0.60));
    vecTex.push_back(glm::vec2(1.00,0.30));

    vecTex.push_back(glm::vec2(0.75,0.60));
    vecTex.push_back(glm::vec2(0.75,0.30));
    vecTex.push_back(glm::vec2(0.50,0.60));

    vecTex.push_back(glm::vec2(0.75,0.30));
    vecTex.push_back(glm::vec2(0.50,0.60));
    vecTex.push_back(glm::vec2(0.50,0.30));

    vecTex.push_back(glm::vec2(0.25,0.30));
    vecTex.push_back(glm::vec2(0.50,0.30));
    vecTex.push_back(glm::vec2(0.50,0.00));

    vecTex.push_back(glm::vec2(0.25,0.30));
    vecTex.push_back(glm::vec2(0.25,0.00));
    vecTex.push_back(glm::vec2(0.50,0.00));

    vecTex.push_back(glm::vec2(0.25,1.00));
    vecTex.push_back(glm::vec2(0.50,1.00));
    vecTex.push_back(glm::vec2(0.50,0.60));

    vecTex.push_back(glm::vec2(0.25,1.00));
    vecTex.push_back(glm::vec2(0.25,0.60));
    vecTex.push_back(glm::vec2(0.50,0.60));

    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, 108*sizeof(GLfloat), vertices, GL_STATIC_DRAW);

//    glGenBuffers(1, &m_IBO);
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 24*sizeof(GLuint), indices, GL_STATIC_DRAW);

    glGenBuffers(1, &texture_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, texture_buffer);
    glBufferData(GL_ARRAY_BUFFER, vecTex.size() * sizeof(glm::vec2), &vecTex[0], GL_STATIC_DRAW);

    LoadTexture();
    initShader();
}
void reshape(int w, int h)
{
    g_windowWidth = w;
    g_windowHeight = h;
}
int main(int argc, char** argv)
{

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(g_windowWidth, g_windowHeight);
    glutCreateWindow("GLUT Volume");
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
    /* Problem: glewInit failed, something is seriously wrong. */
    fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }

    glutDisplayFunc(&display);
    glutIdleFunc(&rotateDisplay);
    glutReshapeFunc(&reshape);
    init();
    glutMainLoop();
    return EXIT_SUCCESS;
}
