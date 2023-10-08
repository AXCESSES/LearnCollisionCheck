/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#pragma once

#include <string>
#include <iostream>

#include <GL/glew.h>

#ifndef __GLM__
#define __GLM__
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#endif

namespace MyOpenGL
{// General purpsoe shader object. Compiles from file, generates
// compile/link-time error messages and hosts several utility 
// functions for easy management.
    class Shader
    {
    public:
        // State
        GLuint ID;
        // Constructor
        Shader() : ID(0) {}
        // Sets the current shader as active
        Shader& Use() {
            glUseProgram(this->ID);
            return *this;
        }
        void Use() const {
            glUseProgram(this->ID);
        }
        // Compiles the shader from given source code
        void Compile(const GLchar* vertexSource, const GLchar* fragmentSource, const GLchar* geometrySource = nullptr) {
            GLuint sVertex, sFragment, gShader;
            // Vertex Shader
            sVertex = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(sVertex, 1, &vertexSource, NULL);
            glCompileShader(sVertex);
            checkCompileErrors(sVertex, "VERTEX");
            // Fragment Shader
            sFragment = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(sFragment, 1, &fragmentSource, NULL);
            glCompileShader(sFragment);
            checkCompileErrors(sFragment, "FRAGMENT");
            // If geometry shader source code is given, also compile geometry shader
            if (geometrySource != nullptr) {
                gShader = glCreateShader(GL_GEOMETRY_SHADER);
                glShaderSource(gShader, 1, &geometrySource, NULL);
                glCompileShader(gShader);
                checkCompileErrors(gShader, "GEOMETRY");
            }
            // Shader Program
            this->ID = glCreateProgram();
            glAttachShader(this->ID, sVertex);
            glAttachShader(this->ID, sFragment);
            if (geometrySource != nullptr)
                glAttachShader(this->ID, gShader);
            glLinkProgram(this->ID);
            checkCompileErrors(this->ID, "PROGRAM");
            // Delete the shaders as they're linked into our program now and no longer necessery
            glDeleteShader(sVertex);
            glDeleteShader(sFragment);
            if (geometrySource != nullptr)
                glDeleteShader(gShader);
        }
        // Utility functions
        const Shader& SetFloat(const GLchar* name, GLfloat value, GLboolean useShader = false) const {
            if (useShader)
                this->Use();
            glUniform1f(glGetUniformLocation(this->ID, name), value);
            return *this;
        }
        const Shader& SetInteger(const GLchar* name, GLint value, GLboolean useShader = false) const {
            if (useShader)
                this->Use();
            glUniform1i(glGetUniformLocation(this->ID, name), value);
            return *this;
        }
        const Shader& SetVector2f(const GLchar* name, GLfloat x, GLfloat y, GLboolean useShader = false) const {
            if (useShader)
                this->Use();
            glUniform2f(glGetUniformLocation(this->ID, name), x, y);
            return *this;
        }
        const Shader& SetVector2f(const GLchar* name, const glm::vec2& value, GLboolean useShader = false) const {
            if (useShader)
                this->Use();
            glUniform2f(glGetUniformLocation(this->ID, name), value.x, value.y);
            return *this;
        }
        const Shader& SetVector3f(const GLchar* name, GLfloat x, GLfloat y, GLfloat z, GLboolean useShader = false) const {
            if (useShader)
                this->Use();
            glUniform3f(glGetUniformLocation(this->ID, name), x, y, z);
            return *this;
        }
        const Shader& SetVector3f(const GLchar* name, const glm::vec3& value, GLboolean useShader = false) const {
            if (useShader)
                this->Use();
            glUniform3f(glGetUniformLocation(this->ID, name), value.x, value.y, value.z);
            return *this;
        }
        const Shader& SetVector4f(const GLchar* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w, GLboolean useShader = false) const {
            if (useShader)
                this->Use();
            glUniform4f(glGetUniformLocation(this->ID, name), x, y, z, w);
            return *this;
        }
        const Shader& SetVector4f(const GLchar* name, const glm::vec4& value, GLboolean useShader = false) const {
            if (useShader)
                this->Use();
            glUniform4f(glGetUniformLocation(this->ID, name), value.x, value.y, value.z, value.w);
            return *this;
        }
        const Shader& SetMatrix4(const GLchar* name, const glm::mat4& matrix, GLboolean useShader = false) const {
            if (useShader)
                this->Use();
            glUniformMatrix4fv(glGetUniformLocation(this->ID, name), 1, GL_FALSE, glm::value_ptr(matrix));
            return *this;
        }


    private:
        // Checks if compilation or linking failed and if so, print the error logs
        void checkCompileErrors(GLuint object, std::string type) {
            GLint success;
            GLchar infoLog[1024];
            if (type != "PROGRAM") {
                glGetShaderiv(object, GL_COMPILE_STATUS, &success);
                if (!success) {
                    glGetShaderInfoLog(object, 1024, NULL, infoLog);
                    std::cout << "| ERROR::SHADER: Compile-time error: Type: " << type << "\n"
                        << infoLog << "\n -- --------------------------------------------------- -- "
                        << std::endl;
                }
            }
            else {
                glGetProgramiv(object, GL_LINK_STATUS, &success);
                if (!success) {
                    glGetProgramInfoLog(object, 1024, NULL, infoLog);
                    std::cout << "| ERROR::Shader: Link-time error: Type: " << type << "\n"
                        << infoLog << "\n -- --------------------------------------------------- -- "
                        << std::endl;
                }
            }
        }
    };

}
