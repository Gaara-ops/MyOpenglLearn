//#include "mainwindow.h"
#include <QApplication>

#include <QDebug>
#include <QList>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#define random(x) (rand()%x)


#include <stdio.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <fstream>
#include <iostream>
#include <math.h>
#include <QImage>
using namespace std;
using namespace glm;

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

GLuint  render_prog;
GLuint  render_vao;
GLuint m_textureObj;
unsigned int framebuffer;

// 使用shader文本编译shader对象，并绑定shader都想到着色器程序中
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
static void RenderSceneCB()
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(render_prog);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(render_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureObj);//绑定纹理

    //  transform the box
    glm::mat4 projection = glm::perspective(100.0f, (GLfloat)(SCR_WIDTH/SCR_HEIGHT), 0.1f, 100.f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f),
                     glm::vec3(0.0f, 0.0f, 0.0f),
                     glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 model = mat4(1.0f);
    model *= glm::rotate((float)45, glm::vec3(0.0f, 1.0f, 0.0f));
    model *= glm::rotate(90.0f, vec3(1.0f, 0.0f, 0.0f));
    GLuint projectionidx = glGetUniformLocation(render_prog, "projection");
    glUniformMatrix4fv(projectionidx, 1, GL_FALSE, &projection[0][0]);
    GLuint viewidx = glGetUniformLocation(render_prog, "view");
    glUniformMatrix4fv(viewidx, 1, GL_FALSE, &view[0][0]);
    GLuint modelidx = glGetUniformLocation(render_prog, "model");
    glUniformMatrix4fv(modelidx, 1, GL_FALSE, &model[0][0]);

    // 告诉管线怎样解析bufer中的数据
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDisableVertexAttribArray(0);//禁用顶点数据
    glDisableVertexAttribArray(1);//禁用顶点数据
    //读取frameBUffer测试
    uchar *data = new uchar[SCR_WIDTH*SCR_HEIGHT*3];
//        glReadBuffer(GL_COLOR_ATTACHMENT0);
//        glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, SCR_WIDTH, SCR_HEIGHT, GLenum(GL_RGB),GLenum(GL_UNSIGNED_BYTE), data);
    QImage image(data, SCR_WIDTH, SCR_HEIGHT, QImage::Format_RGB888);
    image.save("./Capture_tt.jpg", "JPG");
    delete[] data;
    //end
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//    glBindVertexArray(0);
//    glutSwapBuffers();
}


static void InitializeGlutCallbacks()
{
    glutDisplayFunc(RenderSceneCB);
//    glutIdleFunc(RenderSceneCB);
}
void CreateFrameBuffer(){
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // create a color attachment texture
    unsigned int textureColorbuffer;
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
    // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
    // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool LoadTexture()
{
    QString filename ="F:/opengl/learn/ogldev-source/Content/test.png";
    QImage m_image;
    m_image.load(filename);

    glGenTextures(1, &m_textureObj);//生成纹理队形
    glBindTexture(GL_TEXTURE_2D, m_textureObj);//绑定对象
    //指定一个二维纹理图像
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_image.width(), m_image.height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, m_image.bits());
    //放大和缩小指定过滤方式
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //纹理环绕方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    return true;
}

static void InitData(){
    render_prog = glCreateProgram();

    static const char render_vs[] =
        "#version 430 core\n"
        "\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec2 aTexCoords;\n"
        "out vec2 TexCoords;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    TexCoords = aTexCoords;  \n"
        "    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "}\n";

    static const char render_fs[] =
        "#version 430 core\n"
        "\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoords;\n"
        "\n"
        "uniform sampler2D texture1;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    FragColor = texture(texture1, TexCoords) ;\n"// vec4(1.0,0.0,0.0,1.0)
        "}\n";

    AddShader(render_prog, render_vs, GL_VERTEX_SHADER);
    AddShader(render_prog, render_fs, GL_FRAGMENT_SHADER);
    glLinkProgram(render_prog);


    float cubeVertices[] = {
        // positions          // texture Coords
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    unsigned int  cubeVBO;
//    glGenVertexArrays(1, &render_vao);
    glGenBuffers(1, &cubeVBO);
//    glBindVertexArray(render_vao);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);

    LoadTexture();
}

int main(int argc, char *argv[])
{
//    QApplication a(argc, argv);
//    MainWindow w;
//    w.SetArgInfo(argc,argv);
//    w.show();
//    return a.exec();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(SCR_WIDTH, SCR_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("opengl learn");

    InitializeGlutCallbacks();

    // Must be done after glut is initialized!
    GLenum res = glewInit();
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }

    printf("GL version: %s\n", glGetString(GL_VERSION));

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    InitData();

    glutMainLoop();

    return 0;
}
