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
#include <QImage>
#include "imageloader.h"
#include "fixedcamera.h"
#include "Trackball.h"

#define M_PI 3.1415926

using namespace std;
using glm::mat4;
using glm::vec3;
using glm::vec2;

GLuint m_VBO;//操作顶点缓冲器对象
GLuint m_IBO;// 索引缓冲对象的句柄
GLuint texture_buffer;
GLuint  render_prog;
GLuint texUnit_loc;
GLuint m_textureObj;
float g_angle = 0;
float g_windowWidth = 512;
float g_windowHeight = 512;

vector<vec3> vecVerticePos;
vector<unsigned short> vecIndex;
vector<vec2> texSphere;

struct Transform
{
    glm::quat rotQuat;
}cubeTransform;

FixedCamera camera(glm::vec3(0.0f, 0, 15),glm::vec3(0, 0, 0),glm::vec3(0, 1, 0.0));
static Trackball trackball(g_windowWidth, g_windowHeight, 0.5);

void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
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

float toRadian(float degree){
    return degree*M_PI/180.0;
}

void display(){

    glClearColor(0.2f,0.2f,0.2f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                            (float)g_windowWidth / (float)g_windowWidth, 0.1f, 100.0f);
//    glm::mat4 projection = glm::ortho(-50,50,-50,50,-50,50);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 model = glm::mat4_cast(cubeTransform.rotQuat);


    //  transform the box
//    glm::mat4 projection = glm::perspective(65.0f, (GLfloat)(g_windowWidth/g_windowHeight), 0.1f, 400.f);
//    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f),
//                     glm::vec3(0.0f, 0.0f, 0.0f),
//                     glm::vec3(0.0f, 1.0f, 0.0f));
//    glm::mat4 model = mat4(1.0f);
//    model *= glm::rotate(toRadian(g_angle), glm::vec3(0.0f, 1.0f, 0.0f));
//    model *= glm::rotate(90.0f, vec3(1.0f, 0.0f, 0.0f));
    //model *= glm::translate(glm::vec3(-0.5f, -0.5f, -0.5f));
    glm::mat4 mvp = projection * view * model;
    GLuint mvpIdx = glGetUniformLocation(render_prog, "MVP");
    if (mvpIdx >= 0)
    {
        glUniformMatrix4fv(mvpIdx, 1, GL_FALSE, &mvp[0][0]);
    }

    glEnableVertexAttribArray(0);//开启顶点属性

    glUniform1i(texUnit_loc, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureObj);//绑定纹理

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,0, 0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, texture_buffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
    //glDrawElements(GL_QUADS, 24, GL_UNSIGNED_INT, 0);
    glDrawElements(GL_TRIANGLES, vecIndex.size(), GL_UNSIGNED_SHORT, 0);
    glDisableVertexAttribArray(0);//禁用顶点数据
    glDisableVertexAttribArray(1);//禁用顶点数据
    glutSwapBuffers();// 交换前后缓存
}

void rotateDisplay(){
    g_angle = (g_angle + 0.05);
    if(g_angle > 180.0){
        g_angle -= 180.0;
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
        "out vec3 Color;\n"
        "\n"
        "out vec2 texCoord;\n"
        "\n"
        "uniform mat4 MVP;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    texCoord = vTexCoord;\n"
        "    gl_Position = MVP * vec4(VerPos, 1.0);\n"
        "    Color = normalize(VerPos);\n"
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
        "}\n";//vec4(Color,1.0f);

    AddShader(render_prog, render_vs, GL_VERTEX_SHADER);
    AddShader(render_prog, render_fs, GL_FRAGMENT_SHADER);
    glLinkProgram(render_prog);
    glUseProgram(render_prog);
    texUnit_loc = glGetUniformLocation(render_prog, "theTexture");
}


void createSphereVerticesAndIndex(){
    int col = 50;
    int row = 25;
    float radius = 3;

    vecVerticePos.push_back(vec3(0, 0, radius));
    texSphere.push_back(vec2(0.0, 0));
    float rowAngle = 180.0/(row+1);
    float colAngle = 360.0/col;
    for(int j = 1;j <row+1;j++){
        float tmpRowAngle = rowAngle*j;
        float tmpRadius = radius*sin(toRadian(tmpRowAngle));
        float tmpz = radius*cos(toRadian(tmpRowAngle));
        for(int i= 0; i<=col;i++){
            float tmpColAngle = colAngle*i;
            float tmpx = tmpRadius*cos(toRadian(tmpColAngle));
            float tmpy = tmpRadius*sin(toRadian(tmpColAngle));
            vecVerticePos.push_back(vec3(tmpx, tmpy, tmpz));
            texSphere.push_back(vec2(1.0-i/((float)col), j/((float)(row+1))));
        }
        if(j == 1){
            int startindex = vecVerticePos.size()-(col+1);
            for(int i=startindex;i<vecVerticePos.size()-1;i++){
                vecIndex.push_back(0);
                vecIndex.push_back(i);
                vecIndex.push_back(i+1);
            }
        }else{
            int startindex = vecVerticePos.size()-(col+1)*2;
            int lastindex = vecVerticePos.size()-(col+1);
            for(int i=startindex;i<lastindex-1;i++){
                vecIndex.push_back(i);
                vecIndex.push_back(i+col+1);
                vecIndex.push_back(i+col+1+1);

                vecIndex.push_back(i);
                vecIndex.push_back(i+1);
                vecIndex.push_back(i+col+1+1);
            }
        }
    }
    int startindex = vecVerticePos.size()-(col+1);
    int lastindex = vecVerticePos.size();
    for(int i=startindex;i<vecVerticePos.size()-1;i++){
        vecIndex.push_back(i);
        vecIndex.push_back(i+1);
        vecIndex.push_back(lastindex);
    }

    vecVerticePos.push_back(vec3(0, 0, -radius));
    texSphere.push_back(vec2(1, 1));
    qDebug() <<"vecVerticePos size:"<< vecVerticePos.size() << vecIndex.size();
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
    createSphereVerticesAndIndex();
//    GLfloat vertices[24] = {
//        0.0, 0.0, 0.0,
//        0.0, 0.0, 1.0,
//        0.0, 1.0, 0.0,
//        0.0, 1.0, 1.0,
//        1.0, 0.0, 0.0,
//        1.0, 0.0, 1.0,
//        1.0, 1.0, 0.0,
//        1.0, 1.0, 1.0
//    };
//    GLuint indices[24] = {
//        1,5,7,3,
//        0,2,6,4,
//        0,1,3,2,
//        7,5,4,6,
//        2,3,7,6,
//        1,0,4,5
//    };
    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vecVerticePos.size()*sizeof(vec3), &vecVerticePos[0], GL_STATIC_DRAW);

    glGenBuffers(1, &m_IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vecIndex.size()*sizeof(GLushort), &vecIndex[0], GL_STATIC_DRAW);

    glGenBuffers(1, &texture_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, texture_buffer);
    glBufferData(GL_ARRAY_BUFFER, texSphere.size() * sizeof(vec2), &texSphere[0], GL_STATIC_DRAW);

    initShader();
    LoadTexture();

    cubeTransform.rotQuat = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
}
void reshape(int w, int h)
{
    g_windowWidth = w;
    g_windowHeight = h;
}
bool mouseBtnPressed = false;
void PassiveMouseCB(int xpos, int ypos)
{
    trackball.cursorCallback(NULL, xpos, ypos);

    if (mouseBtnPressed)
    {
        bool flag = false;
        glm::quat newRotation = trackball.createWorldRotationQuat(camera.GetViewMatrix(), 0.1, flag);
        //glm::mat4 newRotation = trackball.createViewRotationMatrix(deltaTime);
        if (flag)
            cubeTransform.rotQuat = newRotation * cubeTransform.rotQuat;
    }
}
void MouseCB(int Button, int State, int x, int y)
{
    qDebug() << Button<<State<<x<<y;
    if(Button == 3){
        camera.ProcessMouseScroll(-1);
    }else if(Button == 4){
        camera.ProcessMouseScroll(1);
    }else if(Button == 0 && State== 0){
        qDebug() << "left press";
        trackball.mouseButtonCallback(true, x, y);
        mouseBtnPressed = true;
    }else if(Button == 0 && State== 1){
        qDebug() << "left up";
        trackball.mouseButtonCallback(false, 0, 0);
        mouseBtnPressed = false;
    }
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
    glutMotionFunc(PassiveMouseCB);//用于监听鼠标事件
    glutMouseFunc(MouseCB);
    init();
    glutMainLoop();
    return EXIT_SUCCESS;
}
