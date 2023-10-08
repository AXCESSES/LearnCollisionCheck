#pragma once

#include "MyShader.h"
#include "GLTexture.h"
#include "ThreadPool.h"
#include "Physics.hpp"
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

constexpr float PI = 3.14159265358979323846;

class Renderer
{
private:
    struct arrayDeleter
    {
        void operator()(void* p) {
            delete[] p; p = nullptr;
        }
    };

    MyOpenGL::MyShader shader;
    MyOpenGL::Texture2D texture;
    PhysicsSolver& solver;
    SafeSimpleThreadPool& threadPool;
    //FastThreadPool& threadPool;

    //std::vector<glm::mat4> modelMatrices;
    std::unique_ptr<glm::mat4, arrayDeleter> modelMatrices;
    //std::vector<glm::vec3> modelColors;
    std::unique_ptr<glm::vec3, arrayDeleter> modelColors;
    GLuint colorBuffer, buffer;

    size_t max_elements;

    GLuint VAO;

    static constexpr float vertex[] = {
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f
    };

    static void stbi_horizontal_flip(unsigned char* data, int width, int height, int channels) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width / 2; ++x) {
                for (int c = 0; c < channels; ++c) {
                    unsigned char pixle = data[(y * width + x) * channels + c];
                    data[(y * width + x) * channels + c] = data[(y * width + (width - 1 - x)) * channels + c];
                    data[(y * width + (width - 1 - x)) * channels + c] = pixle;
                }
            }
        }
    }

    static MyOpenGL::Texture2D loadTextureFromFile(const GLchar* file, const bool vflip = false, const bool hflip = false) {
        using namespace MyOpenGL;
        // Create Texture object
        Texture2D texture;
        // Load image
        int width, height;
        int nrChannels;
        if (vflip)stbi_set_flip_vertically_on_load(true);
        unsigned char* image = stbi_load(file, &width, &height, &nrChannels, 0);
        if (vflip) stbi_set_flip_vertically_on_load(false);
        if (hflip)stbi_horizontal_flip(image, width, height, nrChannels);
        if (image) {
            if (nrChannels == 1)
                texture.Internal_format = GL_RED, texture.Image_format = GL_RED;
            else if (nrChannels == 3)
                texture.Internal_format = GL_RGB, texture.Image_format = GL_RGB;
            else texture.Internal_format = GL_RGBA, texture.Image_format = GL_RGBA;
        }
        // Now generate texture
        texture.Generate(width, height, image);
        // And finally free image data
        stbi_image_free(image);
        return texture;
    }

    void initRenderData() {
        // 绑定纹理坐标
        GLuint VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);

        glBindVertexArray(VAO);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // 绑定实例化颜色
        //GLuint colorBuffer;
        glGenBuffers(1, &colorBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * max_elements, &(modelColors.get()[0]), GL_DYNAMIC_DRAW);

        glBindVertexArray(VAO);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

        glVertexAttribDivisor(1, 1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // 绑定实例化坐标
        //GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * max_elements, &(modelMatrices.get()[0]), GL_DYNAMIC_DRAW);

        glBindVertexArray(VAO);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

        glVertexAttribDivisor(2, 1);
        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        //glBindBuffer(GL_ARRAY_BUFFER, VBO);
    }



public:

    explicit Renderer(PhysicsSolver& solver, SafeSimpleThreadPool& threadPool, std::string textureFilePath, std::initializer_list<MyOpenGL::MyShaderInfo> shaderInfos, size_t max_elements, std::function<void(MyOpenGL::MyShader&)> externalInit = nullptr) : solver(solver), threadPool(threadPool), shader(shaderInfos), max_elements(max_elements), modelMatrices(new glm::mat4[max_elements]), modelColors(new glm::vec3[max_elements])
    {
        texture = loadTextureFromFile(textureFilePath.c_str());
        initRenderData();
        if (externalInit)
            externalInit(std::ref(shader));
    }

    virtual ~Renderer() {
        glDeleteVertexArrays(1, &VAO);
    }

    void render() {
        static std::function<void(size_t, size_t)> func = [this](size_t start, size_t end) {
            //std::shared_lock<std::shared_mutex> lk(solver.mtx);
            for (size_t i = start; i < end; ++i) {
                const auto& object = solver.objects[i];
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(object.position, 1.0f));
                model = glm::scale(model, glm::vec3(SimplePhysicalObject::size, 1.0f));
                modelMatrices.get()[i] = model;
                modelColors.get()[i] = solver.objects[i].color;
            }
            };

        // 使用实例化渲染时，应当将位置数据全部添加至modelMatrices中
        // 使用线程池加速添加过程
        {
            /*
            const size_t count = solver.objects.size();
            const size_t batch_size = count / threadPool.getThreadCount();
            std::vector<std::future<void>> rets;
            for (size_t i = 0; i < threadPool.getThreadCount(); ++i) {
                const size_t start = batch_size * i;
                const size_t end = start + batch_size;
                rets.emplace_back(
                    threadPool.enqueue(
                        std::bind(func, start, end)
                    )
                );
            }

            if (batch_size * threadPool.getThreadCount() < count) {
                const size_t start = batch_size * threadPool.getThreadCount();
                func(start, count);
            }

            for (auto& ret : rets)ret.wait();
            */
            
            threadPool.dispatch
            (
                solver.objects.size(),
                func
            );
            
        }

        shader.use();

        glActiveTexture(GL_TEXTURE0);
        texture.Bind();

        // 使用GL_DYNAMIC_DRAW时应当在每次绘制时重新更新Buffer中的内容
        glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * max_elements, &(modelColors.get()[0]), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * max_elements, &(modelMatrices.get()[0]), GL_DYNAMIC_DRAW);

        glBindVertexArray(VAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, solver.objects.size());
        glBindVertexArray(0);

        /*
        shader.use();
        for (const auto& obj : solver.objects) {
            const auto& position = obj.position;
            const auto& color = obj.color;
            
            using namespace glm;
            mat4 model(1.0f);

            model = translate(model, vec3(position, 0.0f));
            model = scale(model, vec3(SimplePhysicalObject::size, 1.0f));

            shader.setMatrix("model", model);
            shader.setVecf("fragmentColor", color);

            glActiveTexture(GL_TEXTURE0);
            texture.Bind();

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }
*/
    }

};