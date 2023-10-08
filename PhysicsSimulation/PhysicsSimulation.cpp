
#include <iostream>
#include "PhysicalObject.hpp"

//#include <GL/glu.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Physics.hpp"
#include "Renderer.hpp"

constexpr int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 800;
constexpr int WORLD_WIDTH = 300, WORLD_HEIGHT = 300;

constexpr int MAX_ELEMENTS = 80000;

std::atomic_bool emit = true;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        emit.store(!emit.load());
}

void run(GLFWwindow* const window, std::function<void(float)> render, std::function<void(float)> fixedUpdate = nullptr) {
    using namespace std;

    atomic_bool running;
    running.store(true);

    if (fixedUpdate) {
        function<void()> updateFunc = [&]() {
            static constexpr float interval = 1.0f / 60.0f;
            while (running.load()) {
                GLfloat currentFrame = static_cast<GLfloat>(glfwGetTime());
                fixedUpdate(interval);
                while (static_cast<GLfloat>(glfwGetTime()) - currentFrame < interval) {
                    std::this_thread::yield();
                }
            }
            };

        thread fixedUpdateThread(updateFunc);
        fixedUpdateThread.detach();
    }

    GLfloat lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        GLfloat deltaTime = static_cast<GLfloat>(glfwGetTime()) - lastFrame;
        render(deltaTime);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    running.store(false);
}

glm::vec3 getColor(float seed) {
    seed /= 1000.0f;
    const float r = sin(seed);
    const float g = sin(seed + 0.33f * 2.0f * PI);
    const float b = sin(seed + 0.66f * 2.0f * PI);
    return { r * r, g * g, b * b };
}

float getFPS() {
    static float fps = 0.0f;
    static int frameCount = 0;
    static float currenttime = 0.0f;
    static float lasttime = 0.0f;
    frameCount++;
    currenttime = (float)glfwGetTime();
    if (currenttime - lasttime > 1.0f) {
        fps = (float)frameCount / (currenttime - lasttime);
        lasttime = currenttime;
        frameCount = 0;
    }
    return fps;
}

// 注意在Release模式下运行，否则Debug模式下的性能很低的
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Physics Simulation", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();
    glGetError(); // Call it once to catch glewInit() bug, all other errors are now from our application.

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    glfwSetKeyCallback(window, keyCallback);

    SafeSimpleThreadPool threadPool(15);
    //FastThreadPool threadPool(10);

    const glm::vec2 world_size{ WORLD_WIDTH, WORLD_HEIGHT };
    PhysicsSolver solver{ world_size, threadPool };
    Renderer render(solver, threadPool, "./circle.png", {
        {GL_VERTEX_SHADER, 1, "./vertex.vert"},
        {GL_FRAGMENT_SHADER, 1, "./fragment.frag"}
                    },
                    MAX_ELEMENTS,
                    [](MyOpenGL::MyShader& shader) {
                        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(WORLD_WIDTH), static_cast<float>(WORLD_HEIGHT), 0.0f, -1.0f, 1.0f);

                        shader.use();
                        shader
                            .setInt("image", 0) // 设置纹理
                            .setMatrix("projection", projection); // 设置投影
                    }
    );

    int idx = 0;

    // 如果希望不加锁的话，就需要将物理处理放在渲染帧中
    run(window, 
        [&](float deltaTime) {
            if (solver.objects.size() < MAX_ELEMENTS && emit.load()) {
                for (uint32_t i = 20; i--;) {
                    const auto id = solver.create({ 2.0f, 10.0f + 1.1f * i });
                    solver.objects[id].last_position.x -= 0.2f;
                    solver.objects[id].color = getColor(idx);
                }
                idx++;
            }

            static constexpr float physicsDeltaTime = 1.0f / 60.0f;
            solver.update(physicsDeltaTime);
            std::cout << getFPS() << "\r\n";

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            render.render(); // 应当修改为实例化渲染
            //std::cout << getFPS() << "\r\n";
        }
    );

    glfwTerminate();
    return 0;
}

