
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

unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359;
        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y       * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y       * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = indices.size();

        std::vector<float> data;
        for (int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        float stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

bool ReadShaderFile(const char* pFileName, string& outFile)
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
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(render_prog);

    glUniform3f(glGetUniformLocation(render_prog, "albedo"),
                0.5f,0.0f,0.0f);
    glUniform1f(glGetUniformLocation(render_prog, "ao"), 0.5f);
//    static float metallic = 0.01;
//    if(metallic<1){
//        metallic += 0.00005;
//    }else{
//        metallic = 0.01;
//    }
    glUniform1f(glGetUniformLocation(render_prog, "metallic"), 0.1);
    static float roughness = 0.01;
    if(roughness<1){
        roughness += 0.00005;
    }else{
        roughness = 0.01;
    }
    glUniform1f(glGetUniformLocation(render_prog, "roughness"), roughness);
    glUniform3f(glGetUniformLocation(render_prog, "camPos"),
                0.0f, 0.0f, 10.0f);
    glUniform3f(glGetUniformLocation(render_prog, "lightPositions[0]"),
                -5.0f, 0.0f, 5.0f);
    glUniform3f(glGetUniformLocation(render_prog, "lightColors[0]"),
                300.0f, 300.0f, 300.0f);

    //  transform the box
    glm::mat4 projection = glm::perspective(100.0f, (GLfloat)(SCR_WIDTH/SCR_HEIGHT), 0.1f, 100.f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f),
                     glm::vec3(0.0f, 0.0f, 0.0f),
                     glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 model = mat4(1.0f);

    GLuint projectionidx = glGetUniformLocation(render_prog, "projection");
    glUniformMatrix4fv(projectionidx, 1, GL_FALSE, &projection[0][0]);
    GLuint viewidx = glGetUniformLocation(render_prog, "view");
    glUniformMatrix4fv(viewidx, 1, GL_FALSE, &view[0][0]);
    GLuint modelidx = glGetUniformLocation(render_prog, "model");
    glUniformMatrix4fv(modelidx, 1, GL_FALSE, &model[0][0]);
    renderSphere();

    glutSwapBuffers();
}


static void InitializeGlutCallbacks()
{
    glutDisplayFunc(RenderSceneCB);
    glutIdleFunc(RenderSceneCB);
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

    string strvs="",strfs="";
    ReadShaderFile("./shader/pbr.vs",strvs);
    ReadShaderFile("./shader/pbr.fs",strfs);

    AddShader(render_prog, strvs.c_str(), GL_VERTEX_SHADER);
    AddShader(render_prog, strfs.c_str(), GL_FRAGMENT_SHADER);
    glLinkProgram(render_prog);
    glUseProgram(render_prog);
}

int main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(SCR_WIDTH, SCR_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("opengl learn");

    // Must be done after glut is initialized!
    GLenum res = glewInit();
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }

    printf("GL version: %s\n", glGetString(GL_VERSION));

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    InitData();

    InitializeGlutCallbacks();
    glutMainLoop();

    return 0;
}
