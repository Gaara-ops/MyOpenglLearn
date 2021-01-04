#include <iostream>
#include <string>
#include <sstream>
#include <QFile>
#include <vector>
#include <math.h>
#include <fstream>
#include <ctime>
#include <QString>
#include <QImage>
#include <glad\glad.h>
#include <GLFW\glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "shader.h"
#include "fixedcamera.h"

#include "CubicSpline.h"
#include "Trackball.h"

using namespace std;

const float SPEED = 5.0f;

//Load 3D Volume as Texture Function
//volume dimensions
const int XDIM = 256;
const int YDIM = 256;
const int ZDIM = 20;

//Prototypes
bool createWindowAndRC(string windowTitle, GLFWwindow **window);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
double calcFPS(GLFWwindow *window, double theTimeInterval = 1.0, string theWindowTitle = "NONE");
bool LoadVolume(string volume_file);
GLuint GenerateVolumeTexture();
void saveTextureToBMP(GLuint &textureID, string outputFileName);
void loadAndCreateTexture(GLuint &texture, string fileName);
void scroll_callback(GLFWwindow * window, double xoffset, double yoffset);
GLuint computeTransferFunction(vector<TransferFunctionControlPoint> colorKnots, vector<TransferFunctionControlPoint> alphaKnots);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow * window, double xpos, double ypos);
GLuint setUpEmptyTexture();
void generateGradients(int sampleSize);
float sampleVolume(int x, int y, int z);
int clip(int n, int lower, int upper);
void filterNxNxN(int n);
glm::vec3 sampleNxNxN(int x, int y, int z, int n);
bool isInBounds(int x, int y, int z);
GLuint loadGradientTexture();
glm::vec3 sampleNxNxN(int x, int y, int z, int n);
glm::vec3 sampleGradients(int x, int y, int z);
bool fileExists(string fileName);
ostream& operator<<(ostream& out, const glm::vec3 &v);
istream& operator>>(istream& in, glm::vec3 &v);
void readGradientsFromFile();
GLuint GenerateNoiseTexture();
GLuint GenerateBackgroundTexture();
GLuint CreateTexInfo();

//settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

//camera
//FixedCamera camera(glm::vec3(-2.5f, 0, 0));
FixedCamera camera(glm::vec3(XDIM/2.0, YDIM/2.0, (ZDIM/2.0)+100),glm::vec3(XDIM/2.0, YDIM/2.0, ZDIM/2.0),glm::vec3(0, 1.0, 0));

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
//const float ZOOM = 45.0f;

double lastposx = 0.0;
double lastposy = 0.0;
float scalerange = 0.0;

//timing
float deltaTime = 0.0f; //Time between current frame and last frame
float lastFrame = 0.0f; //Time of last frame

static Trackball trackball(SCR_WIDTH, SCR_HEIGHT, 1.0f);

string windowTitle = "MultiPass GPU Raycasting";

vector<float> scalars;
vector<glm::vec3> gradients;

int currentShaderIndex = 2;

string gradientFileName = "./volume2/gradients.bin";

bool mouseBtnPressed = false;

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
#undef main
int main()
{
    qDebug() << glm::degrees(3.1416f);//radians to degrees;
    qDebug() << glm::radians(180.0f);// degrees to radians

    glm::mat4 modelgg = glm::mat4(1.0f);
    modelgg *= glm::translate(glm::vec3(1.0, 1.0, 0.0));
    modelgg *= glm::rotate(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    modelgg *= glm::translate(glm::vec3(-1.0, -1.0, 0.0));

    glm::vec4 pos1(1.0,1.0,1.0,1.0);
    glm::vec4 res1 = modelgg*pos1;
    qDebug() << res1.x << res1.y << res1.z << res1.w;
    glm::vec4 pos2(1.0,0.0,0.0,1.0);
    glm::vec4 res2 = modelgg*pos2;
    qDebug() << res2.x << res2.y << res2.z << res2.w;

    FixedCamera cameragg(glm::vec3(0.0, 0.0, 1.0),glm::vec3(0.0, 0.0, 0.0),glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 viewmatrixgg = cameragg.GetViewMatrix();
    qDebug() << "viewmatrixgg:";
    for(int i=0;i<4;i++){
        qDebug() << viewmatrixgg[i][0]<<viewmatrixgg[i][1]<<viewmatrixgg[i][2]<<viewmatrixgg[i][3];
    }
    cameragg.Position = glm::vec3(1.0,-1.0,1.0);
    viewmatrixgg = cameragg.GetViewMatrix();

    qDebug() <<"dir:"<< cameragg.Direction.x << cameragg.Direction.y << cameragg.Direction.z;
    qDebug() <<"up:"<< cameragg.Up.x << cameragg.Up.y << cameragg.Up.z;
    qDebug() <<"right"<< cameragg.Right.x << cameragg.Right.y << cameragg.Right.z;

    stbi_flip_vertically_on_write(true);

    if (!LoadVolume("./volume2/male.raw"))
    {
        cout << "Volume Loading Error" << endl;
        return -1;
    }

    cout << "Loading and setting up volume data" << endl;

    GLFWwindow *window = nullptr;

    if (!createWindowAndRC(windowTitle, &window))
    {
        cout << "Window and Rendering Context Creation Error" << endl;
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_ALPHA_TEST);

    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glBlendFunc(GL_ONE, GL_ONE);

    glEnable(GL_CULL_FACE);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_3D);
    //unit cube vertices
    GLfloat cubeVertices[] = {
        //top
        0.0        , 0.0        , (float)ZDIM,
        (float)XDIM, 0.0        , (float)ZDIM,
        (float)XDIM, (float)YDIM, (float)ZDIM,
        (float)XDIM, (float)YDIM, (float)ZDIM,
        0.0        , (float)YDIM, (float)ZDIM,
        0.0        ,  0.0       , (float)ZDIM,
        //bottom
        0.0        , 0.0        , 0.0,
        0.0        , (float)YDIM, 0.0,
        (float)XDIM, (float)YDIM, 0.0,
        (float)XDIM, (float)YDIM, 0.0,
        (float)XDIM, 0.0        , 0.0,
        0.0        , 0.0        , 0.0,
        //left
        0.0        , 0.0        , 0.0        ,
        0.0        , 0.0        , (float)ZDIM,
        0.0        , (float)YDIM, (float)ZDIM,
        0.0        , (float)YDIM, (float)ZDIM,
        0.0        , (float)YDIM, 0.0        ,
        0.0        , 0.0        , 0.0        ,
        //right
        (float)XDIM, (float)YDIM, (float)ZDIM,
        (float)XDIM, 0.0        , (float)ZDIM,
        (float)XDIM, 0.0        , 0.0        ,
        (float)XDIM, 0.0        , 0.0        ,
        (float)XDIM, (float)YDIM, 0.0        ,
        (float)XDIM, (float)YDIM, (float)ZDIM,
        //back
        0.0        , (float)YDIM, 0.0        ,
        0.0        , (float)YDIM, (float)ZDIM,
        (float)XDIM, (float)YDIM, (float)ZDIM,
        (float)XDIM, (float)YDIM, (float)ZDIM,
        (float)XDIM, (float)YDIM, 0.0        ,
        0.0        , (float)YDIM, 0.0        ,
        //front
        0.0        , 0.0        , (float)ZDIM,
        0.0        , 0.0        , 0.0        ,
        (float)XDIM, 0.0        , 0.0        ,
        (float)XDIM, 0.0        , 0.0        ,
        (float)XDIM, 0.0        , (float)ZDIM,
        0.0        , 0.0        , (float)ZDIM,
    };

    GLuint cubeIndices[] = {
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,
        21,22,23,24,25,26,27,28,29,30,31,32,33,34,35
    };

    GLuint cubeVAO, cubeVBO, cubeEBO;

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glGenBuffers(1, &cubeEBO);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s)
    glBindVertexArray(cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    Shader raycastShader("./volume2/myrendercube.vert", "./volume2/myrendercube.frag");

    //Load Volume Texture from File
    GLuint volumeTex = GenerateVolumeTexture();
    vector<TransferFunctionControlPoint> colorKnots = {
        TransferFunctionControlPoint(1.0f, 0.0f, 0.0f, 0),
        TransferFunctionControlPoint(0.0f, 1.0f, 0.0f, 64),
        TransferFunctionControlPoint(0.0f, 0.0f, 1.0f, 128),
        TransferFunctionControlPoint(0.0f, 1.0f, 1.0f, 192),
        TransferFunctionControlPoint(1.0f, 1.0f, 0.0f, 256)
    };

    vector<TransferFunctionControlPoint> alphaKnots = {
        TransferFunctionControlPoint(0.0f, 0),
        TransferFunctionControlPoint(1.0f, 256)
    };

    GLuint transferFuncTexture = computeTransferFunction(colorKnots, alphaKnots);

    Shader *currentRaycastShader = &raycastShader;
    float scale = 512;
    scalerange = scale*0.75;
    //render loop
    while (!glfwWindowShouldClose(window))
    {
        //per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        //input
        processInput(window);

        glm::mat4 projection = glm::ortho(-scalerange, scalerange,-scalerange, scalerange,-10000.1f, 1000.0f);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        currentRaycastShader->use();
        //Setup Vertex Attributes
        glBindVertexArray(cubeVAO);
        //Setup Uniforms
/*
        glm::mat4 modelrotate = glm::mat4(1.0f);
        modelrotate *= glm::translate(glm::vec3(XDIM/2.0, YDIM/2.0, ZDIM/2.0));
        modelrotate *= glm::rotate(glm::radians(g_angle), glm::vec3(0.0f, 1.0f, 0.0f));
        modelrotate *= glm::translate(glm::vec3(-XDIM/2.0, -YDIM/2.0, -ZDIM/2.0));

        glm::vec4 cameraPos(XDIM/2.0, YDIM/2.0, (ZDIM/2.0)+100,1.0);
        glm::vec4 rotatePos = modelrotate*cameraPos;
        camera.Position = glm::vec3(rotatePos.x,rotatePos.y,rotatePos.z);
*/
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4_cast(cubeTransform.rotQuat);


        currentRaycastShader->setMat4("MVP", projection * view * model);
        currentRaycastShader->setVec3("Dims",glm::vec3(XDIM,YDIM,ZDIM));
        currentRaycastShader->setMat4("modelMatrix", model);
        currentRaycastShader->setVec3("cameraPos",camera.Position);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, volumeTex);
        glUniform1i(glGetUniformLocation(currentRaycastShader->ID, "volumeTex"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, transferFuncTexture);
        glUniform1i(glGetUniformLocation(currentRaycastShader->ID, "transferFuncTex"), 1);
        //Draw Scene
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        //glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
        calcFPS(window, 1.0, windowTitle);
    }

    //optional: de-allocate all resources once they've outlived their purpose:

    //glfw: terminate, clearing all previously allocated GLFW resources
    glfwTerminate();

    return 0;
}


bool createWindowAndRC(string windowTitle, GLFWwindow **window)
{
    //GLFW: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //GLFW: window creation
    *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, windowTitle.c_str(), NULL, NULL);
    if (*window == NULL)
    {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(*window);

    //Limit the frameRate to 60fps.
    glfwSwapInterval(1);

    //glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return false;
    }

    return true;
}

//process all inputs: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        currentShaderIndex = 1;

    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        currentShaderIndex = 2;
}

//glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    //make sure the viewport matches the new window dimensions
    glViewport(0, 0, width, height);
}

double calcFPS(GLFWwindow *window, double theTimeInterval, string theWindowTitle)
{
    // Static values which only get initialised the first time the function runs
    static double t0Value = glfwGetTime(); // Set the initial time to now
    static int    fpsFrameCount = 0;             // Set the initial FPS frame count to 0
    static double fps = 0.0;           // Set the initial FPS value to 0.0

    // Get the current time in seconds since the program started (non-static, so executed every time)
    double currentTime = glfwGetTime();

    // Ensure the time interval between FPS checks is sane (low cap = 0.1s, high-cap = 10.0s)
    // Negative numbers are invalid, 10 fps checks per second at most, 1 every 10 secs at least.
    if (theTimeInterval < 0.1)
    {
        theTimeInterval = 0.1;
    }
    if (theTimeInterval > 10.0)
    {
        theTimeInterval = 10.0;
    }

    // Calculate and display the FPS every specified time interval
    if ((currentTime - t0Value) > theTimeInterval)
    {
        // Calculate the FPS as the number of frames divided by the interval in seconds
        fps = (double)fpsFrameCount / (currentTime - t0Value);

        // If the user specified a window title to append the FPS value to...
        if (theWindowTitle != "NONE")
        {
            // Convert the fps value into a string using an output stringstream
            std::ostringstream stream;
            stream << fps;
            std::string fpsString = stream.str();

            // Append the FPS value to the window title details
            theWindowTitle += " | FPS: " + fpsString;

            // Convert the new window title to a c_str and set it
            const char* pszConstString = theWindowTitle.c_str();
            glfwSetWindowTitle(window, pszConstString);
        }
        else // If the user didn't specify a window to append the FPS to then output the FPS to the console
        {
            std::cout << "FPS: " << fps << std::endl;
        }

        // Reset the FPS frame counter and set the initial time to be now
        fpsFrameCount = 0;
        t0Value = glfwGetTime();
    }
    else // FPS calculation time interval hasn't elapsed yet? Simply increment the FPS frame counter
    {
        fpsFrameCount++;
    }

    // Return the current FPS - doesn't have to be used if you don't want it!
    return fps;
}

bool LoadVolume(string volume_file)
{
    /* 随机生成
    srand((unsigned)time(NULL));
    volumeData.pData = new GLubyte[XDIM*YDIM*ZDIM];
    for(int i=0;i<XDIM*YDIM*ZDIM;i++){
        volumeData.pData[i] = 255.*rand() / (float)RAND_MAX;
    }*/
    /* 从测试文件读取 */
    QFile file("./ctdata.txt");
    if(!file.open(QIODevice::ReadOnly)){
         return false;
    }
    volumeData.pData = new GLubyte[XDIM*YDIM*ZDIM];
    QTextStream in(&file);
    QString alldata = in.readAll();
    QStringList strlist = alldata.split(",");
    if(XDIM*YDIM*ZDIM > strlist.size()){
        qDebug() << strlist.size();
        return false;
    }
    for(int i=0;i<XDIM*YDIM*ZDIM;i++){
        volumeData.pData[i] = strlist.at(i).toShort();
    }
    file.close();
    return true;
/*
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
            volumeData.scalars[i] = (float)volumeData.pData[i]/256;
        }
        return true;
    }
    return false;*/
}

//function that load a volume from the given raw data file and
//generates an OpenGL 3D texture from it
GLuint GenerateVolumeTexture()
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_3D, textureID);

    // set the texture parameters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    //set the mipmap levels (base and max)
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 4);

    //allocate data with internal format and foramt as (GL_RED)
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, XDIM, YDIM, ZDIM, 0, GL_RED, GL_UNSIGNED_BYTE, volumeData.pData);
    //GL_CHECK_ERRORS

    //generate mipmaps
    glGenerateMipmap(GL_TEXTURE_3D);

    return textureID;
}

// Generates gradients using a central differences scheme
void generateGradients(int sampleSize)
{
    int n = sampleSize;
    glm::vec3 normal = glm::vec3(0.0f);
    glm::vec3 s1, s2;

    int index = 0;
    for (int z = 0; z < ZDIM; z++)
    {
        for (int y = 0; y < YDIM; y++)
        {
            for (int x = 0; x < XDIM; x++)
            {
                s1.x = sampleVolume(x - n, y, z);
                s2.x = sampleVolume(x + n, y, z);
                s1.y = sampleVolume(x, y - n, z);
                s2.y = sampleVolume(x, y + n, z);
                s1.z = sampleVolume(x, y, z - n);
                s2.z = sampleVolume(x, y, z + n);

                volumeData.gradients[index++] = glm::normalize(s2 - s1);
                if (isnan(volumeData.gradients[index - 1].x))
                {
                    volumeData.gradients[index - 1] = glm::vec3(0.0f);
                }
            }
        }
    }

    filterNxNxN(3);

    //Now save the gradient
    ofstream fout;
    fout.open(gradientFileName, ios_base::binary);

    glm::vec3 *temp = &(volumeData.gradients[0]);

    fout.write(reinterpret_cast<char*>(temp), XDIM*YDIM*ZDIM *  sizeof(glm::vec3));
    fout.close();
    cout << "Gradients saved into file\n";
}

void readGradientsFromFile()
{
    //Load the gradients
    ifstream fin;
    fin.open(gradientFileName, ios_base::binary);

    glm::vec3 *temp = new glm::vec3[XDIM*YDIM*ZDIM];

    fin.read(reinterpret_cast<char*>(temp), XDIM*YDIM*ZDIM * sizeof(glm::vec3));
    fin.close();

    volumeData.gradients = vector<glm::vec3>(temp, temp + XDIM * YDIM*ZDIM);

    cout << "Gradients loaded from file\n";
}

bool fileExists(string fileName)
{
    ifstream infile(fileName.c_str());

    if (infile.good())
    {
        return true;
    }
    else
    {
        return false;
    }
}

float sampleVolume(int x, int y, int z)
{
    x = (int)clip(x, 0, XDIM - 1);
    y = (int)clip(y, 0, YDIM - 1);
    z = (int)clip(z, 0, ZDIM - 1);
    return volumeData.scalars[x + (y * XDIM) + (z * XDIM * YDIM)];
}

//filter the gradients with an NxNxN box filter
//Should be an odd number of samples. 3 used by default.
void filterNxNxN(int n)
{
    int index = 0;
    for (int z = 0; z < ZDIM; z++)
    {
        for (int y = 0; y < YDIM; y++)
        {
            for (int x = 0; x < XDIM; x++)
            {
                volumeData.gradients[index++] = sampleNxNxN(x, y, z, n);
            }
        }
    }
}

glm::vec3 sampleNxNxN(int x, int y, int z, int n)
{
    n = (n - 1) / 2;
    glm::vec3 avg = glm::vec3(0.0f);
    int num = 0;

    for (int k = z - n; k <= z + n; k++)
    {
        for (int j = y - n; j <= y + n; j++)
        {
            for (int i = x - n; i <= x + n; i++)
            {
                if (isInBounds(i, j, k))
                {
                    avg += sampleGradients(i, j, k);
                    num++;
                }
            }
        }
    }
    avg /= (float)num;

    if (avg != glm::vec3(0.0f))
    {
        glm::normalize(avg);
    }

    return avg;
}

bool isInBounds(int x, int y, int z)
{
    return ((x >= 0 && x < XDIM) && (y >= 0 && y < YDIM) && (z >= 0 && z < ZDIM));
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

glm::vec3 sampleGradients(int x, int y, int z)
{
    return volumeData.gradients[x + (y * XDIM) + (z * XDIM * YDIM)];
}

int clip(int n, int lower, int upper)
{
    return std::max(lower, std::min(n, upper));
}

void saveTextureToBMP(GLuint &textureID, string outputFileName)
{
    glBindTexture(GL_TEXTURE_2D, textureID);
    unsigned char* imageData = (unsigned char *)malloc((int)(SCR_WIDTH * SCR_HEIGHT * (3)));
    //glReadPixels(0, 0, SCR_WIDTH, SCR_WIDTH, GL_RGB, GL_UNSIGNED_BYTE, imageData);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
    stbi_write_bmp(outputFileName.c_str(), SCR_WIDTH, SCR_HEIGHT, 3, imageData);
    delete imageData;
}

void loadAndCreateTexture(GLuint &texture, string fileName)
{
    //texture
    glBindTexture(GL_TEXTURE_2D, texture);
    //set the texture wrapping paramaters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //set texture filtering paramaters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //load image, create texture and generate mipmaps
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char *data = stbi_load(fileName.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        cout << "Failed to load texture" << endl;
    }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

//whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    //camera.ProcessMouseScroll(yoffset);
    if(yoffset > 0){
        scalerange *= 1.05;
    }else if(yoffset < 0){
        scalerange *= 0.95;
    }
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    return textureID;
}

//Stochastic Jittering Noise Texture
GLuint GenerateNoiseTexture()
{
    GLuint noiseTex;
//    int size = 32;
//    unsigned char* buffer = new unsigned char[size*size*3];
//    srand((unsigned)time(NULL));
//    for (int i = 0; i < (size*size*3); i++)
//        buffer[i] = 255.*rand() / (float)RAND_MAX;
    std::cout << "GenerateNoiseTexture begin" << std::endl;
    int size = 256;
    unsigned char* buffer = new unsigned char[size*size*3];
    srand((unsigned)time(NULL));
    for (int i = 0; i < size; i++){
        for(int j=0;j < size; j++){
            buffer[i*size*3 + j*3] = j;
            buffer[i*size*3 + j*3+1] = i;
            buffer[i*size*3 + j*3+2] = 0;
        }
    }
    std::cout << "GenerateNoiseTexture end" << std::endl;

    glGenTextures(1, &noiseTex);
    glBindTexture(GL_TEXTURE_2D, noiseTex);
    glTexImage2D(
        GL_TEXTURE_2D, // target
        0, // level
        GL_RGB, // internal
        size, // width
        size, // height
        0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    delete buffer;

    return noiseTex;
}

GLuint GenerateBackgroundTexture()
{
    GLuint backgroundTex;

    QImage image;
    image.load("./volume2/sky.png");
    std::cout << "byteCount:" << image.byteCount() << " " << image.width() <<  " " <<  image.height()<< std::endl;
    glGenTextures(1, &backgroundTex);
    glBindTexture(GL_TEXTURE_2D, backgroundTex);
    glTexImage2D(
        GL_TEXTURE_2D, // target
        0, // level
        GL_RGBA, // internal
        image.width(), // width
        image.height(), // height
        0, GL_BGRA, GL_UNSIGNED_BYTE, image.bits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return backgroundTex;
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

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    //set texture filtering paramaters
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glGenerateMipmap(GL_TEXTURE_2D);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_FLOAT, &transferFunc[0].x);
/*
    unsigned char* imageData = (unsigned char *)malloc((int)(256 * 10 * (4)));
    glGetTexImage(GL_TEXTURE_1D, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

    for (int i = 1; i < 10; i++)
    {
        copy(imageData, imageData + 256 * 4, (imageData + i * (256 * 4)));
    }

    stbi_write_png("transfer.png", 256, 10, 4, imageData, 0);
    delete imageData;
*/
    return textureID;
}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            //std::cout << "pos:" << xpos << " " << ypos << std::endl;
            //trackball.mouseButtonCallback(true, xpos, ypos);
            lastposx = xpos; lastposy = ypos;
            mouseBtnPressed = true;
        }
        if (action == GLFW_RELEASE)
        {
            //trackball.mouseButtonCallback(false, 0, 0);
            mouseBtnPressed = false;
        }
    }
}

void cursor_position_callback(GLFWwindow * window, double xpos, double ypos)
{
    //trackball.cursorCallback(window, xpos, ypos);

    if (mouseBtnPressed)
    {
        float dX = xpos - lastposx;
        float dY = ypos - lastposy;
        lastposx = xpos; lastposy = ypos;

        glm::mat4 viewmatrix = camera.GetViewMatrix();

        glm::mat4 modelrotate = glm::mat4(1.0f);
        modelrotate *= glm::translate(glm::vec3(XDIM/2.0, YDIM/2.0, ZDIM/2.0));
        modelrotate *= glm::rotate(glm::radians(-dX), camera.Up);
        modelrotate *= glm::translate(glm::vec3(-XDIM/2.0, -YDIM/2.0, -ZDIM/2.0));

        glm::vec4 cameraPos(camera.Position.x,camera.Position.y,camera.Position.z,1.0);
        glm::vec4 rotatePos = modelrotate*cameraPos;
        camera.Position = glm::vec3(rotatePos.x,rotatePos.y,rotatePos.z);
        viewmatrix = camera.GetViewMatrix();
        modelrotate = glm::mat4(1.0f);
        modelrotate *= glm::translate(glm::vec3(XDIM/2.0, YDIM/2.0, ZDIM/2.0));
        modelrotate *= glm::rotate(glm::radians(-dY), camera.Right);
        modelrotate *= glm::translate(glm::vec3(-XDIM/2.0, -YDIM/2.0, -ZDIM/2.0));
        cameraPos = glm::vec4(camera.Position.x,camera.Position.y,camera.Position.z,1.0);
        rotatePos = modelrotate*cameraPos;
        camera.Position = glm::vec3(rotatePos.x,rotatePos.y,rotatePos.z);
        viewmatrix = camera.GetViewMatrix();
        //qDebug() << xpos << ypos;
        /*
        bool flag = false;
        std::cout << "deltaTime:" << deltaTime << std::endl;
        glm::quat newRotation = trackball.createWorldRotationQuat(camera.GetViewMatrix(), deltaTime, flag);
        //glm::mat4 newRotation = trackball.createViewRotationMatrix(deltaTime);
        if (flag)
            cubeTransform.rotQuat = newRotation * cubeTransform.rotQuat;
        */
    }
}

GLuint CreateTexInfo(){
    vector<glm::vec2> vecTex;
    vecTex.push_back(glm::vec2(1.00,0.60));
    vecTex.push_back(glm::vec2(0.75,0.60));
    vecTex.push_back(glm::vec2(0.75,0.30));
    vecTex.push_back(glm::vec2(0.75,0.30));
    vecTex.push_back(glm::vec2(1.00,0.30));
    vecTex.push_back(glm::vec2(1.00,0.60));

    vecTex.push_back(glm::vec2(0.25,0.60));
    vecTex.push_back(glm::vec2(0.25,0.30));
    vecTex.push_back(glm::vec2(0.50,0.30));
    vecTex.push_back(glm::vec2(0.50,0.30));
    vecTex.push_back(glm::vec2(0.50,0.60));
    vecTex.push_back(glm::vec2(0.25,0.60));

    vecTex.push_back(glm::vec2(0.25,0.60));
    vecTex.push_back(glm::vec2(0.00,0.60));
    vecTex.push_back(glm::vec2(0.00,0.30));
    vecTex.push_back(glm::vec2(0.00,0.30));
    vecTex.push_back(glm::vec2(0.25,0.30));
    vecTex.push_back(glm::vec2(0.25,0.60));

    vecTex.push_back(glm::vec2(0.75,0.30));
    vecTex.push_back(glm::vec2(0.75,0.60));
    vecTex.push_back(glm::vec2(0.50,0.60));
    vecTex.push_back(glm::vec2(0.50,0.60));
    vecTex.push_back(glm::vec2(0.50,0.30));
    vecTex.push_back(glm::vec2(0.75,0.30));

    vecTex.push_back(glm::vec2(0.25,0.30));
    vecTex.push_back(glm::vec2(0.25,0.00));
    vecTex.push_back(glm::vec2(0.50,0.00));
    vecTex.push_back(glm::vec2(0.50,0.00));
    vecTex.push_back(glm::vec2(0.50,0.30));
    vecTex.push_back(glm::vec2(0.25,0.30));

    vecTex.push_back(glm::vec2(0.25,1.00));
    vecTex.push_back(glm::vec2(0.25,0.60));
    vecTex.push_back(glm::vec2(0.50,0.60));
    vecTex.push_back(glm::vec2(0.50,0.60));
    vecTex.push_back(glm::vec2(0.50,1.00));
    vecTex.push_back(glm::vec2(0.25,1.00));

    GLuint tex;
    glGenBuffers(1, &tex);
    glBindBuffer(GL_ARRAY_BUFFER, tex);
    glBufferData(GL_ARRAY_BUFFER, vecTex.size() * sizeof(glm::vec2), &vecTex[0], GL_STATIC_DRAW);

    return tex;
}

ostream& operator<<(ostream& out, const glm::vec3 &v)
{
    out << v.x << " " << v.y << " " << v.z;
    return out;
}

istream& operator>>(istream& in, glm::vec3 &v)
{
    in >> v.x >> v.y >> v.z;
    return in;
}
