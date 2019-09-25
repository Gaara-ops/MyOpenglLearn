#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <QString>
#include <QByteArray>
#include <QDebug>
#include "imageloader.h"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <math.h>
#include <fstream>
#include <ctime>
#include <QImage>
//#include <glad\glad.h>
//#include <GLFW\glfw3.h>


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "shader.h"
#include "fixedcamera.h"

#include "CubicSpline.h"
#include "Trackball.h"

const float SPEED = 5.0f;

//Load 3D Volume as Texture Function
//volume dimensions
const int XDIM = 128;
const int YDIM = 256;
const int ZDIM = 256;

using namespace std;
using glm::mat4;
using glm::vec3;

GLuint g_vao;
GLuint g_programHandle;
GLuint g_winWidth = 400;
GLuint g_winHeight = 400;
GLint g_angle = 0;
GLuint g_frameBuffer;
// transfer function
GLuint g_tffTexObj;
GLuint g_bfTexObj;
GLuint g_texWidth;
GLuint g_texHeight;
GLuint g_volTexObj;
GLuint g_rcVertHandle;
GLuint g_rcFragHandle;
GLuint g_bfVertHandle;
GLuint g_bfFragHandle;
float g_stepSize = 0.001f;

float step_length = 0.0f;
float g_windowWidth = 500;
float g_windowHeight = 500;

GLuint m_textureObj;
GLuint texUnit_loc;

struct Volume
{
    GLubyte* pData;
    vector<float> scalars;
    vector<glm::vec3> gradients;
}volumeData;

struct Transform
{
    glm::quat rotQuat;
}cubeTransform;

FixedCamera camera(glm::vec3(0.0f, 0.0f, 2.0f),glm::vec3(0, 0, 0),glm::vec3(0, 1, 0.0));
static Trackball trackball(g_windowWidth, g_windowHeight, 0.5);

GLuint volumeTex;
GLuint gradientTexture;
GLuint transferFuncTexture;

void CreateCubeInfo();
bool LoadVolume(string volume_file);
void readGradientsFromFile();
GLuint GenerateVolumeTexture();
GLuint loadGradientTexture();
GLuint computeTransferFunction(vector<TransferFunctionControlPoint> colorKnots, vector<TransferFunctionControlPoint> alphaKnots);

GLuint initShaderObj(const GLchar* srcfile, GLenum shaderType)
{
    ifstream inFile(srcfile, ifstream::in);
    // use assert?
    if (!inFile)
    {
    cerr << "Error openning file: " << srcfile << endl;
    exit(EXIT_FAILURE);
    }

    const int MAX_CNT = 10000;
    GLchar *shaderCode = (GLchar *) calloc(MAX_CNT, sizeof(GLchar));
    inFile.read(shaderCode, MAX_CNT);
    if (inFile.eof())
    {
    size_t bytecnt = inFile.gcount();
    *(shaderCode + bytecnt) = '\0';
    }
    else if(inFile.fail())
    {
    cout << srcfile << "read failed " << endl;
    }
    else
    {
    cout << srcfile << "is too large" << endl;
    }
    // create the shader Object
    GLuint shader = glCreateShader(shaderType);
    if (0 == shader)
    {
    cerr << "Error creating vertex shader." << endl;
    }
    // cout << shaderCode << endl;
    // cout << endl;
    const GLchar* codeArray[] = {shaderCode};
    glShaderSource(shader, 1, codeArray, NULL);
    free(shaderCode);

    // compile the shader
    glCompileShader(shader);

    GLint err;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &err);
    if (GL_FALSE == err){
        GLint logLen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 0)
        {
            char* log = (char *)malloc(logLen);
            GLsizei written;
            glGetShaderInfoLog(shader, logLen, &written, log);
            cerr << "Shader log: " << log << endl;
            free(log);
        }
    }
    if (GL_FALSE == err)
    {
        cerr << "shader compilation failed" << endl;
    }
    return shader;
}

GLuint initFace2DTex(GLuint bfTexWidth, GLuint bfTexHeight)
{
    GLuint backFace2DTex;
    glGenTextures(1, &backFace2DTex);
    glBindTexture(GL_TEXTURE_2D, backFace2DTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bfTexWidth, bfTexHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    return backFace2DTex;
}

void render(GLenum cullFace)
{
    glClearColor(0.2f,0.2f,0.2f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //  transform the box
    glm::mat4 projection = glm::perspective(camera.Zoom, (GLfloat)g_winWidth/g_winHeight, 0.1f, 100.f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 model = glm::mat4_cast(cubeTransform.rotQuat);
    //glm::mat4 model = mat4(1.0f);
    //model *= glm::rotate((float)g_angle, glm::vec3(0.0f, 1.0f, 0.0f));
    // to make the "head256.raw" i.e. the volume data stand up.
    //model *= glm::rotate(90.0f, vec3(1.0f, 0.0f, 0.0f));
    model *= glm::translate(glm::vec3(-0.5f, -0.5f, -0.5f));
    // notice the multiplication order: reverse order of transform
    glm::mat4 mvp = projection * view * model;
    GLuint mvpIdx = glGetUniformLocation(g_programHandle, "MVP");
    if (mvpIdx >= 0)
    {
        glUniformMatrix4fv(mvpIdx, 1, GL_FALSE, &mvp[0][0]);
    }
    else
    {
        cerr << "can't get the MVP" << endl;
    }
    glEnable(GL_CULL_FACE);
    glCullFace(cullFace);
    glBindVertexArray(g_vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (GLuint *)NULL);
    glDisable(GL_CULL_FACE);
}

void linkShader(GLuint shaderPgm, GLuint newVertHandle, GLuint newFragHandle)
{
    const GLsizei maxCount = 2;
    GLsizei count;
    GLuint shaders[maxCount];
    glGetAttachedShaders(shaderPgm, maxCount, &count, shaders);
    for (int i = 0; i < count; i++) {
        glDetachShader(shaderPgm, shaders[i]);
    }
    // Bind index 0 to the shader input variable "VerPos"
    glBindAttribLocation(shaderPgm, 0, "VerPos");
    // Bind index 1 to the shader input variable "VerClr"
    glBindAttribLocation(shaderPgm, 1, "VerClr");

    glAttachShader(shaderPgm,newVertHandle);
    glAttachShader(shaderPgm,newFragHandle);

    glLinkProgram(shaderPgm);

    GLint status;
    glGetProgramiv(shaderPgm, GL_LINK_STATUS, &status);
    if (GL_FALSE == status)
    {
        GLint logLen;
        glGetProgramiv(shaderPgm, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 0)
        {
            GLchar * log = (GLchar *)malloc(logLen);
            GLsizei written;
            glGetProgramInfoLog(shaderPgm, logLen, &written, log);
            cerr << "Program log: " << log << endl;
        }
    }

    if (GL_FALSE == status)
    {
        cerr << "Failed to relink shader program!" << endl;
        exit(EXIT_FAILURE);
    }
}

void rcSetUinforms()
{
    GLint screenSizeLoc = glGetUniformLocation(g_programHandle, "ScreenSize");
    if (screenSizeLoc >= 0){
        glUniform2f(screenSizeLoc, (float)g_winWidth, (float)g_winHeight);
    }
    GLint stepSizeLoc = glGetUniformLocation(g_programHandle, "StepSize");

    if (stepSizeLoc >= 0){
        glUniform1f(stepSizeLoc, g_stepSize);
    }
    GLint transferFuncLoc = glGetUniformLocation(g_programHandle, "TransferFunc");
    if (transferFuncLoc >= 0){
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_1D, g_tffTexObj);
        glUniform1i(transferFuncLoc, 0);
    }
    GLint backFaceLoc = glGetUniformLocation(g_programHandle, "ExitPoints");
    if (backFaceLoc >= 0){
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, g_bfTexObj);
        glUniform1i(backFaceLoc, 1);
    }
    GLint volumeLoc = glGetUniformLocation(g_programHandle, "VolumeTex");
    if (volumeLoc >= 0){
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_3D, g_volTexObj);
        glUniform1i(volumeLoc, 2);
    }
}

void display(){


    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_frameBuffer);
    glViewport(0, 0, g_winWidth, g_winHeight);
    linkShader(g_programHandle, g_bfVertHandle, g_bfFragHandle);
    glUseProgram(g_programHandle);
    render(GL_FRONT);

    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_winWidth, g_winHeight);

    linkShader(g_programHandle, g_rcVertHandle, g_rcFragHandle);
    glUseProgram(g_programHandle);
    rcSetUinforms();
    render(GL_BACK);

    glUseProgram(0);
    glutSwapBuffers();// 交换前后缓存
}

void rotateDisplay(){
//    g_angle = (g_angle + 0.0005);
//    if(g_angle > 360.0){
//        g_angle -= 360.0;
//    }

    g_angle = (g_angle + 1) % 360;

    glutPostRedisplay();
}

void initShader(){
    // vertex shader object for first pass
    g_bfVertHandle = initShaderObj("./volume/shader/backface.vert", GL_VERTEX_SHADER);
    // fragment shader object for first pass
    g_bfFragHandle = initShaderObj("./volume/shader/backface.frag", GL_FRAGMENT_SHADER);
    // vertex shader object for second pass
    g_rcVertHandle = initShaderObj("./volume/shader/raycasting.vert", GL_VERTEX_SHADER);
    // fragment shader object for second pass
    g_rcFragHandle = initShaderObj("./volume/shader/raycasting.frag", GL_FRAGMENT_SHADER);
    // create the shader program , use it in an appropriate time
    g_programHandle = glCreateProgram();
    if (0 == g_programHandle)
    {
        cerr << "Error create shader program" << endl;
        exit(EXIT_FAILURE);
    }
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
    g_texWidth = g_winWidth;
    g_texHeight = g_winHeight;

    CreateCubeInfo();
    initShader();

    vector<TransferFunctionControlPoint> colorKnots = {
        TransferFunctionControlPoint(.91f, .7f, .61f, 0),
        TransferFunctionControlPoint(.91f, .7f, .61f, 80),
        TransferFunctionControlPoint(1.0f, 1.0f, .85f, 82),
        TransferFunctionControlPoint(1.0f, 1.0f, .85f, 256)
    };

    vector<TransferFunctionControlPoint> alphaKnots = {
        TransferFunctionControlPoint(0.0f, 0),
        TransferFunctionControlPoint(0.0f, 40),
        TransferFunctionControlPoint(0.2f, 60),
        TransferFunctionControlPoint(0.05f, 63),
        TransferFunctionControlPoint(0.0f, 80),
        TransferFunctionControlPoint(0.9f, 82),
        TransferFunctionControlPoint(1.0f, 256)
    };
    g_tffTexObj = computeTransferFunction(colorKnots, alphaKnots);
    g_bfTexObj = initFace2DTex(g_texWidth, g_texHeight);

    if (!LoadVolume("./volume2/male.raw"))
    {
        cout << "Volume Loading Error" << endl;
        return;
    }
    readGradientsFromFile();
    cout << "Volume Data setup complete" << endl;
    g_volTexObj = GenerateVolumeTexture();

    GLuint depthBuffer;
    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, g_texWidth, g_texHeight);

    // attach the texture and the depth buffer to the framebuffer
    glGenFramebuffers(1, &g_frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, g_frameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_bfTexObj, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

    GLenum complete = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (complete != GL_FRAMEBUFFER_COMPLETE){
        cout << "framebuffer is not complete" << endl;
        exit(EXIT_FAILURE);
    }

    glEnable(GL_DEPTH_TEST);

    cubeTransform.rotQuat = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);

//    gradientTexture = loadGradientTexture();
//    LoadTexture();
}
void reshape(int w, int h)
{
    g_winWidth = w;
    g_winHeight = h;
    g_texWidth = w;
    g_texHeight = h;
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

bool LoadVolume(string volume_file)
{
    ifstream infile(volume_file.c_str(), ios_base::binary);

    if (infile.good())
    {
        //read the volume data file
        volumeData.pData = new GLubyte[XDIM*YDIM*ZDIM];
        infile.read(reinterpret_cast<char*>(volumeData.pData), XDIM*YDIM*ZDIM * sizeof(GLubyte));
        infile.close();

        volumeData.scalars.resize(XDIM*YDIM*ZDIM);
        volumeData.gradients.resize(XDIM*YDIM*ZDIM);

        for (int i = 0; i < volumeData.scalars.size(); i++)
        {
            volumeData.scalars[i] = (float)volumeData.pData[i] / 255;
        }
        return true;
    }
    return false;
}
void readGradientsFromFile()
{
    //Load the gradients
    ifstream fin;
    fin.open("./volume2/gradients.bin", ios_base::binary);

    glm::vec3 *temp = new glm::vec3[XDIM*YDIM*ZDIM];

    fin.read(reinterpret_cast<char*>(temp), XDIM*YDIM*ZDIM * sizeof(glm::vec3));
    fin.close();

    volumeData.gradients = vector<glm::vec3>(temp, temp + XDIM * YDIM*ZDIM);

    cout << "Gradients loaded from file\n";
}
GLuint GenerateVolumeTexture()
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_3D, textureID);

    // set the texture parameters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    //set the mipmap levels (base and max)
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 4);

    //glPixelStorei(GL_UNPACK_ALIGNMENT,1);

    //allocate data with internal format and foramt as (GL_RED)
    glTexImage3D(GL_TEXTURE_3D, 0, GL_INTENSITY, XDIM, YDIM, ZDIM, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, volumeData.pData);

    return textureID;
}
GLuint loadGradientTexture()
{
    //write to texture
    GLuint textureID;
    glGenTextures(1, &textureID);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_3D, textureID);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    //set texture filtering paramaters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    //glGenerateMipmap(GL_TEXTURE_2D);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, XDIM, YDIM, ZDIM, 0, GL_RGB, GL_FLOAT, &volumeData.gradients[0].x);

    return textureID;
}

GLuint computeTransferFunction(vector<TransferFunctionControlPoint> colorKnots, vector<TransferFunctionControlPoint> alphaKnots)
{
    glm::vec4 transferFunc[256];

    vector<CubicSplineSegment> colorCubicSpline = CalculateCubicSpline(colorKnots);
    vector<CubicSplineSegment> alphaCubicSpline = CalculateCubicSpline(alphaKnots);

    int numTF = 0; //Each isoVal from 0 to 255 will be mapped to a color and alpha using transfer func

    for (int i = 0; i < colorKnots.size() - 1; i++)
    {
        int steps = colorKnots[i + 1].isoValue - colorKnots[i].isoValue;

        for (int j = 0; j < steps; j++)
        {
            float k = (float)j / (float)(steps - 1);

            transferFunc[numTF++] = colorCubicSpline[i].GetPointOnSpline(k);
        }
    }

    numTF = 0; //Each isoVal from 0 to 255 will be mapped to a color and alpha using transfer func

    for (int i = 0; i < alphaKnots.size() - 1; i++)
    {
        int steps = alphaKnots[i + 1].isoValue - alphaKnots[i].isoValue;

        for (int j = 0; j < steps; j++)
        {
            float k = (float)j / (float)(steps - 1);

            transferFunc[numTF++].w = alphaCubicSpline[i].GetPointOnSpline(k).w;
        }
    }

    //write to texture
    GLuint textureID;
    glGenTextures(1, &textureID);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_1D, textureID);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_FLOAT, &transferFunc[0].x);

    return textureID;
}
void CreateCubeInfo(){

    GLfloat vertices[24] = {
    0.0, 0.0, 0.0,
    0.0, 0.0, 1.0,
    0.0, 1.0, 0.0,
    0.0, 1.0, 1.0,
    1.0, 0.0, 0.0,
    1.0, 0.0, 1.0,
    1.0, 1.0, 0.0,
    1.0, 1.0, 1.0
    };
    GLuint indices[36] = {
    1,5,7,
    7,3,1,
    0,2,6,
    6,4,0,
    0,1,3,
    3,2,0,
    7,5,4,
    4,6,7,
    2,3,7,
    7,6,2,
    1,0,4,
    4,5,1
    };
    GLuint gbo[2];
    glGenBuffers(2, gbo);
    GLuint vertexdat = gbo[0];
    GLuint veridxdat = gbo[1];
    glBindBuffer(GL_ARRAY_BUFFER, vertexdat);
    glBufferData(GL_ARRAY_BUFFER, 24*sizeof(GLfloat), vertices, GL_STATIC_DRAW);
    // used in glDrawElement()
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, veridxdat);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36*sizeof(GLuint), indices, GL_STATIC_DRAW);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    // vao like a closure binding 3 buffer object: verlocdat vercoldat and veridxdat
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0); // for vertexloc
    glEnableVertexAttribArray(1); // for vertexcol

    // the vertex location is the same as the vertex color
    glBindBuffer(GL_ARRAY_BUFFER, vertexdat);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLfloat *)NULL);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLfloat *)NULL);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, veridxdat);

    g_vao = vao;
}




