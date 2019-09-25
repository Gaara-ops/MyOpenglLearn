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

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <shader.h>

using namespace std;
using glm::mat4;
using glm::vec3;
using glm::vec4;

GLuint m_VBO;//操作顶点缓冲器对象
GLuint m_IBO;// 索引缓冲对象的句柄
GLuint  render_prog;
float g_angle = 0;
float g_windowWidth = 500;
float g_windowHeight = 500;

GLuint renderTextureFront, renderTextureBack;
GLuint firstPassFrameBuffer = 0;

// Setup and compile our shaders
Shader shader("shaders/advanced.vs", "shaders/advanced.frag");
Shader skyboxShader("shaders/skybox.vs", "shaders/skybox.frag");

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

GLuint setUpEmptyTexture()
{
    GLuint textureID;
    glGenTextures(1, &textureID);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    //set texture filtering paramaters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glGenerateMipmap(GL_TEXTURE_2D);
    // Give an empty image to OpenGL ( the last "0" )
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_windowWidth, g_windowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    return textureID;
}
void saveTextureToBMP(GLuint &textureID, string outputFileName)
{
    glBindTexture(GL_TEXTURE_2D, textureID);
    unsigned char* imageData = (unsigned char *)malloc((int)(g_windowWidth * g_windowHeight * (3)));
    //glReadPixels(0, 0, SCR_WIDTH, SCR_WIDTH, GL_RGB, GL_UNSIGNED_BYTE, imageData);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
    stbi_write_bmp(outputFileName.c_str(), g_windowWidth, g_windowHeight, 3, imageData);
    delete imageData;
}
#include<QImage>
void PrintBmpInfo(QString filename){
    QImage image;
    image.load(filename);
}

void display(){

    glBindFramebuffer(GL_FRAMEBUFFER, firstPassFrameBuffer);
    glClearColor(0.2f,0.2f,0.2f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    //  transform the box
    glm::mat4 projection = glm::ortho(-1.0,1.0,-1.0,1.0,0.0,10.0);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, -5.0f),
                     glm::vec3(0.0f, 0.0f, 0.0f),
                     glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 model = mat4(1.0f);
    model *= glm::rotate((float)45, glm::vec3(0.0f, 1.0f, 0.0f));
    model *= glm::rotate(90.0f, vec3(1.0f, 0.0f, 0.0f));
    model *= glm::translate(glm::vec3(-0.5f, -0.5f, -0.5f));
    glm::mat4 mvp = projection * view * model;

    glm::vec4 tt1 = mvp * vec4(1.0,1.0,-1.0, 1.0);
    glm::vec4 tt2 = mvp * vec4(1.0,1.0,1.0, 1.0);
    glm::vec4 tt3 = mvp * vec4(1.0,1.0,2.0, 1.0);
    qDebug() << "tt1"<<tt1[0]<<tt1[1]<<tt1[2]<<tt1[3];
    qDebug() << "tt2"<<tt2[0]<<tt2[1]<<tt2[2]<<tt2[3];
    qDebug() << "tt3"<<tt3[0]<<tt3[1]<<tt3[2]<<tt3[3];

    GLuint mvpIdx = glGetUniformLocation(render_prog, "MVP");
    if (mvpIdx >= 0)
    {
        glUniformMatrix4fv(mvpIdx, 1, GL_FALSE, &mvp[0][0]);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    glEnableVertexAttribArray(0);//开启顶点属性
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
    glDrawElements(GL_QUADS, 24, GL_UNSIGNED_INT, 0);
    glDisableVertexAttribArray(0);//禁用顶点数据

    glutSwapBuffers();// 交换前后缓存

    saveTextureToBMP(renderTextureFront,"./front.bmp");
    saveTextureToBMP(renderTextureBack,"./back.bmp");
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
        "out vec3 Color;\n"
        "\n"
        "out vec4 clip_position;\n"
        "\n"
        "uniform mat4 MVP;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = MVP * vec4(VerPos, 1.0);\n"
        "    clip_position = gl_Position;\n"
        "    Color = vec3(VerPos.xyz);\n"
        "}\n";

    static const char render_fs[] =
        "#version 430 core\n"
        "\n"
        "layout(location = 0) out vec3 start_point;\n"
        "layout(location = 1) out vec3 end_point;\n"
        "\n"
        "in vec4 clip_position;\n"
        "in vec3 Color;\n"
        "\n"
        "void main(void)\n"
        "{\n"
            "vec3 ndc_position = clip_position.xyz;\n"
            "if (gl_FrontFacing){\n"
            "    start_point = ndc_position + 0.5;\n"
            "    end_point = vec3(0);\n"
            "}else{\n"
            "    start_point = vec3(0);\n"
            "    end_point = ndc_position + 0.5;\n"
            "}\n"
        "}\n";

    AddShader(render_prog, render_vs, GL_VERTEX_SHADER);
    AddShader(render_prog, render_fs, GL_FRAGMENT_SHADER);
    glLinkProgram(render_prog);
    glUseProgram(render_prog);
}

GLuint loadCubemap(vector<const GLchar*> faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width,height;
    unsigned char* image;

    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    for(GLuint i = 0; i < faces.size(); i++)
    {
        image = SOIL_load_image(faces[i], &width, &height, 0, SOIL_LOAD_RGB);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        SOIL_free_image_data(image);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return textureID;
}

void init(){
    GLfloat cubeVertices[] = {
       // Positions          // Texture Coords
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
   GLfloat skyboxVertices[] = {
       // Positions
       -1.0f,  1.0f, -1.0f,
       -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
       -1.0f,  1.0f, -1.0f,

       -1.0f, -1.0f,  1.0f,
       -1.0f, -1.0f, -1.0f,
       -1.0f,  1.0f, -1.0f,
       -1.0f,  1.0f, -1.0f,
       -1.0f,  1.0f,  1.0f,
       -1.0f, -1.0f,  1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

       -1.0f, -1.0f,  1.0f,
       -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
       -1.0f, -1.0f,  1.0f,

       -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
       -1.0f,  1.0f,  1.0f,
       -1.0f,  1.0f, -1.0f,

       -1.0f, -1.0f, -1.0f,
       -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
       -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
   };

   // Setup cube VAO
   GLuint cubeVAO, cubeVBO;
   glGenVertexArrays(1, &cubeVAO);
   glGenBuffers(1, &cubeVBO);
   glBindVertexArray(cubeVAO);
   glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
   glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
   glBindVertexArray(0);
   // Setup skybox VAO
   GLuint skyboxVAO, skyboxVBO;
   glGenVertexArrays(1, &skyboxVAO);
   glGenBuffers(1, &skyboxVBO);
   glBindVertexArray(skyboxVAO);
   glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
   glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
   glBindVertexArray(0);

   // Load textures
   GLuint cubeTexture = loadTexture("./textures/container.jpg");
   #pragma endregion

   // Cubemap (Skybox)
   vector<const GLchar*> faces;
   faces.push_back("./skybox/right.jpg");
   faces.push_back("./skybox/left.jpg");
   faces.push_back("./skybox/top.jpg");
   faces.push_back("./skybox/bottom.jpg");
   faces.push_back("./skybox/back.jpg");
   faces.push_back("./skybox/front.jpg");
   GLuint cubemapTexture = loadCubemap(faces);
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
//    glutIdleFunc(&rotateDisplay);
//    glutReshapeFunc(&reshape);
    init();
    glutMainLoop();
    return EXIT_SUCCESS;
}
