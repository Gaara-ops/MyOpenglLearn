#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
class GLFWwindow;
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <QVector>
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void SetArgInfo(int argc, char *argv[]);
private:
    int Init();
    int CreateVertexInfo(GLuint& VAO,GLuint& VBO);
    int CreateTextureInfo(const char *strPath,GLuint& texID);
private slots:
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QString strResource;
    int argc;
    char **argv;
    GLFWwindow* window;
};

#endif // MAINWINDOW_H
