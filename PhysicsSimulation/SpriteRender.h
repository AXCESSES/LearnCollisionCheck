#pragma once

#include "GLShader.h"
#include "GLTexture.h"

class SpriteRender
{
public:
    SpriteRender(MyOpenGL::Shader& shader) {
        this->shader = shader;
        initRenderData();
    }
    ~SpriteRender() {
        glDeleteVertexArrays(1, &quadVAO);
    }
    void DrawSprite(const MyOpenGL::Texture2D& texture, glm::vec2 position, glm::vec2 size = glm::vec2(10, 10), GLfloat rotate = 1.0f, glm::vec3 color = glm::vec3(1.0f)) const;

private:
    const static GLfloat vertices[];
    MyOpenGL::Shader shader;
    GLuint quadVAO;

    void initRenderData();

};

const GLfloat SpriteRender::vertices[] = {
    // Œª÷√     // Œ∆¿Ì
    0.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,

    0.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 0.0f
};

inline void SpriteRender::initRenderData() {
    GLuint VBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(quadVAO);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

inline void SpriteRender::DrawSprite(const MyOpenGL::Texture2D& texture, glm::vec2 position, glm::vec2 size, GLfloat rotate, glm::vec3 color) const {
    using namespace glm;
    shader.Use();
    
    mat4 model(1.0f);
    model = translate(model, vec3(position, 0.0f));

    model = translate(model, vec3(0.5f * size.x, 0.5f * size.y, 0.0f));
    model = glm::rotate(model, rotate, vec3(0.0f, 0.0f, 1.0f));
    model = translate(model, vec3(-0.5f * size.x, -0.5f * size.y, 0.0f));

    model = scale(model, vec3(size, 1.0f));

    shader.SetMatrix4("model", model);
    shader.SetVector3f("spriteColor", color);

    glActiveTexture(GL_TEXTURE0);
    texture.Bind();

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}