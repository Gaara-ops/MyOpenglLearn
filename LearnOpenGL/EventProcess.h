#ifndef EVENTPROCESS_H
#define EVENTPROCESS_H

#include "GLFW/glfw3.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window){
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){

}

void processInput(GLFWwindow *window){

}

#endif // EVENTPROCESS_H
