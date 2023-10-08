#pragma once

#ifndef __GLM__
#define __GLM__
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#endif

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

namespace MyOpenGL
{
    struct MyShaderInfo
    {
        GLuint program;
        GLsizei size;
        std::string path;
        GLint* length = nullptr;

        MyShaderInfo(GLuint program, GLsizei size, const char* path, GLint* length = nullptr) :program(program), size(size), path(path), length(length) {}

        std::string toString() const {
            using namespace std;

            return
                "{program:" + to_string(program)
                + ",size:" + to_string(size)
                + ",path:" + path
                + ",length:" + (length == nullptr ? "nullptr" : to_string(*length)) + "}";
        }
    };

    class MyShader
    {
    public:
        static unsigned int compileShader(const GLuint program, GLsizei size, const char* shaderSourceCode, const GLint* length = nullptr) {
            unsigned int shader = glCreateShader(program);
            glShaderSource(shader, size, &shaderSourceCode, length);
            glCompileShader(shader);
            int success;
            char infoLog[1024];
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                std::string errorMsg = "ERROR::SHADER::COMPILATION_FAILED\n";
                errorMsg.append(infoLog);
                throw std::exception(errorMsg.c_str());
            }
            return shader;
        }

        static unsigned int createAndLinkShaderProgram(const std::vector<unsigned int>& shaders) {
            unsigned int shaderProgram = glCreateProgram();
            if (shaders.size() > 0) {
                for (const auto shader : shaders) {
                    glAttachShader(shaderProgram, shader);
                }
            }
            glLinkProgram(shaderProgram);
            int success;
            char infoLog[1024];
            glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shaderProgram, 1024, nullptr, infoLog);
                std::string errorMsg = "ERROR::PROGRAM::CREATE_AND_LINK_FAILED\n";
                errorMsg.append(infoLog);
                throw std::exception(errorMsg.c_str());
            }
            return shaderProgram;
        }

        static unsigned int createAndLinkShaderProgram(std::initializer_list<unsigned int> shaders) {
            unsigned int shaderProgram = glCreateProgram();
            if (shaders.size() > 0) {
                for (const auto shader : shaders) {
                    glAttachShader(shaderProgram, shader);
                }
            }
            glLinkProgram(shaderProgram);
            int success;
            char infoLog[1024];
            glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shaderProgram, 1024, nullptr, infoLog);
                std::string errorMsg = "ERROR::PROGRAM::CREATE_AND_LINK_FAILED\n";
                errorMsg.append(infoLog);
                throw std::exception(errorMsg.c_str());
            }
            return shaderProgram;
        }

        unsigned int id;

        MyShader(): id(0) {}

        MyShader(const std::vector<MyShaderInfo>& shaderInfos) {
            using namespace std;
            string shaderCode;
            ifstream shaderFile;
            shaderFile.exceptions(ifstream::failbit | ifstream::badbit);
            vector<unsigned int> shaders;
            shaders.reserve(shaderInfos.size());
            for (const auto& shaderInfo : shaderInfos) {
                try {
                    shaderFile.open(shaderInfo.path);
                    stringstream ss;
                    ss << shaderFile.rdbuf();
                    shaderFile.close();
                    shaderCode = ss.str();
                    shaders.push_back(compileShader(shaderInfo.program, shaderInfo.size, shaderCode.c_str(), shaderInfo.length));
                }
                catch (std::ifstream::failure e) {
                    std::cerr << "ERROR::SHADER::FILE_READ_FAILED " << e.what() << endl;
                    throw e;
                }
                catch (std::exception e) {
                    std::cerr << "ERROR::SHADER::COMPILE_FAILED " << e.what() << endl;
                    std::cerr << "Error shader info: " << shaderInfo.toString() << endl;
                    throw e;
                }
            }

            try {
                this->id = createAndLinkShaderProgram(shaders);
            }
            catch (exception e) {
                cerr << "ERROR::PROGRAM::CREATE_AND_LINK_FAILED " << e.what() << endl;
                throw e;
            }

            for (const auto shader : shaders)
                glDeleteShader(shader);
        }

        MyShader(const std::initializer_list<MyShaderInfo> shaderInfos) {
            using namespace std;
            string shaderCode;
            ifstream shaderFile;
            shaderFile.exceptions(ifstream::failbit | ifstream::badbit);
            vector<unsigned int> shaders;
            shaders.reserve(shaderInfos.size());
            for (const auto& shaderInfo : shaderInfos) {
                try {
                    shaderFile.open(shaderInfo.path);
                    stringstream ss;
                    ss << shaderFile.rdbuf();
                    shaderFile.close();
                    shaderCode = ss.str();
                    shaders.push_back(compileShader(shaderInfo.program, shaderInfo.size, shaderCode.c_str(), shaderInfo.length));
                }
                catch (std::ifstream::failure e) {
                    std::cerr << "ERROR::SHADER::FILE_READ_FAILED " << e.what() << endl;
                    throw e;
                }
                catch (std::exception e) {
                    std::cerr << "ERROR::SHADER::COMPILE_FAILED " << e.what() << endl;
                    std::cerr << "Error shader info: " << shaderInfo.toString() << endl;
                    throw e;
                }
            }

            try {
                this->id = createAndLinkShaderProgram(shaders);
            }
            catch (exception e) {
                cerr << "ERROR::PROGRAM::CREATE_AND_LINK_FAILED " << e.what() << endl;
                throw e;
            }

            for (const auto shader : shaders)
                glDeleteShader(shader);
        }

        void use() const {
            glUseProgram(id);
        }

        const MyShader& setBool(const std::string& name, bool value) const {
            setInt(name, (int)value);
            return *this;
        }

        const MyShader& setInt(const std::string& name, int value) const {
            glUniform1i(glGetUniformLocation(id, name.c_str()), value);
            return *this;
        }

        const MyShader& setFloat(const std::string& name, float value) const {
            glUniform1f(glGetUniformLocation(id, name.c_str()), value);
            return *this;
        }

        const MyShader& setInt(const std::string& name, const std::initializer_list<int> vals) const {
            if (vals.size() == 1)
                glUniform1i(glGetUniformLocation(id, name.c_str()), *vals.begin());
            else {
                auto size = vals.size();
                if (size > 4 || size < 1) {
                    throw std::exception("Invalid amount of parameters! The amount of parameters should between 1 and 4!");
                    //return;
                }
                auto ptr = vals.begin();
                int* vs = new int[size];
                for (int i = 0; i < size; ++i, ++ptr) {
                    vs[i] = *ptr;
                }
                auto location = glGetUniformLocation(id, name.c_str());
                if (size == 2) {
                    glUniform2i(location, vs[0], vs[1]);
                }
                else if (size == 3) {
                    glUniform3i(location, vs[0], vs[1], vs[2]);
                }
                else {
                    glUniform4i(location, vs[0], vs[1], vs[2], vs[3]);
                }
                delete[] vs; vs = nullptr;
            }
            return *this;
        }

        const MyShader& setFloat(const std::string& name, const std::initializer_list<float> vals) const {
            if (vals.size() == 1)
                glUniform1f(glGetUniformLocation(id, name.c_str()), *vals.begin());
            else {
                auto size = vals.size();
                if (size > 4 || size < 1) {
                    throw std::exception("Invalid amount of parameters! The amount of parameters should between 1 and 4!");
                    //return;
                }
                auto ptr = vals.begin();
                float* vs = new float[size];
                for (int i = 0; i < size; ++i, ++ptr) {
                    vs[i] = *ptr;
                }
                auto location = glGetUniformLocation(id, name.c_str());
                if (size == 2) {
                    glUniform2f(location, vs[0], vs[1]);
                }
                else if (size == 3) {
                    glUniform3f(location, vs[0], vs[1], vs[2]);
                }
                else {
                    glUniform4f(location, vs[0], vs[1], vs[2], vs[3]);
                }
                delete[] vs; vs = nullptr;
            }
            return *this;
        }

        template<int L, int R>
        static glm::mat<L, R, float> toMat(const std::initializer_list<std::initializer_list<float>>& mats) {
            glm::mat<L, R, float> mat;
            auto pptr = mats.begin();
            for (int i = 0; i < L; ++i, ++pptr) {
                auto ptr = pptr->begin();
                for (int j = 0; j < R; ++j, ++ptr) {
                    mat[i][j] = *ptr;
                }
            }
            return mat;
        }

        const MyShader& setMatrix(const std::string& name, const std::initializer_list<std::initializer_list<float>> mat, const GLint count = 1) const {
            using namespace glm;
            int m = (int)mat.size(), n = (int)mat.begin()->size();
            if (m < 2 || n < 2 || m > 4 || n > 4) {
                throw std::exception("ERROR::PROGRAM::INVAILD_MATRIX!\nInvalid matrix size! The rows and columns of matrix both should between 2 and 4!");
            }

            auto location = glGetUniformLocation(id, name.c_str());

            switch (m) {
            case 2:
                switch (n) {
                case 2:
                {
                    mat2x2 matrix = toMat<2, 2>(mat);
                    glUniformMatrix2fv(location, count, GL_FALSE, value_ptr(matrix));
                }
                break;
                case 3:
                {
                    mat2x3 matrix = toMat<2, 3>(mat);
                    glUniformMatrix2x3fv(location, count, GL_FALSE, value_ptr(matrix));
                }
                break;
                case 4:
                {
                    mat2x4 matrix = toMat<2, 4>(mat);
                    glUniformMatrix2x4fv(location, count, GL_FALSE, value_ptr(matrix));
                }
                break;
                }
                break;
            case 3:
                switch (n) {
                case 2:
                {
                    mat3x2 matrix = toMat<3, 2>(mat);
                    glUniformMatrix3x2fv(location, count, GL_FALSE, value_ptr(matrix));
                }
                break;
                case 3:
                {
                    mat3x3 matrix = toMat<3, 3>(mat);
                    glUniformMatrix3fv(location, count, GL_FALSE, value_ptr(matrix));
                }
                break;
                case 4:
                {
                    mat3x4 matrix = toMat<3, 4>(mat);
                    glUniformMatrix3x4fv(location, count, GL_FALSE, value_ptr(matrix));
                }
                break;
                }
                break;
            case 4:
                switch (n) {
                case 2:
                {
                    mat4x2 matrix = toMat<4, 2>(mat);
                    glUniformMatrix4x2fv(location, count, GL_FALSE, value_ptr(matrix));
                }
                break;
                case 3:
                {
                    mat4x3 matrix = toMat<4, 3>(mat);
                    glUniformMatrix4x3fv(location, count, GL_FALSE, value_ptr(matrix));
                }
                break;
                case 4:
                {
                    mat4x4 matrix = toMat<4, 4>(mat);
                    glUniformMatrix4fv(location, count, GL_FALSE, value_ptr(matrix));
                }
                break;
                }
            }
            return *this;
        }

        template<int L, int R>
        const MyShader& setMatrix(const std::string& name, const glm::mat<L, R, float> mat, const GLint count = 1)const {
            auto location = glGetUniformLocation(id, name.c_str());
            switch (L) {
            case 2:
                switch (R) {
                case 2:
                    glUniformMatrix2fv(location, count, GL_FALSE, value_ptr(mat));
                    break;
                case 3:
                    glUniformMatrix2x3fv(location, count, GL_FALSE, value_ptr(mat));
                    break;
                case 4:
                    glUniformMatrix2x4fv(location, count, GL_FALSE, value_ptr(mat));
                    break;
                }
                break;
            case 3:
                switch (R) {
                case 2:
                    glUniformMatrix3x2fv(location, count, GL_FALSE, value_ptr(mat));
                    break;
                case 3:
                    glUniformMatrix3fv(location, count, GL_FALSE, value_ptr(mat));
                    break;
                case 4:
                    glUniformMatrix3x4fv(location, count, GL_FALSE, value_ptr(mat));
                    break;
                }
                break;
            case 4:
                switch (R) {
                case 2:
                    glUniformMatrix4x2fv(location, count, GL_FALSE, value_ptr(mat));
                    break;
                case 3:
                    glUniformMatrix4x3fv(location, count, GL_FALSE, value_ptr(mat));
                    break;
                case 4:
                    glUniformMatrix4fv(location, count, GL_FALSE, value_ptr(mat));
                    break;
                }
                break;
            }
            return *this;
        }

        template<int L>
        const MyShader& setVecf(const std::string& name, const glm::vec<L, float> v, const GLint count = 1)const {
            auto location = glGetUniformLocation(id, name.c_str());
            switch (L) {
            case 1:
                glUniform1fv(location, count, glm::value_ptr(v));
                break;
            case 2:
                glUniform2fv(location, count, glm::value_ptr(v));
                break;
            case 3:
                glUniform3fv(location, count, glm::value_ptr(v));
                break;
            case 4:
                glUniform4fv(location, count, glm::value_ptr(v));
                break;
            }
            return *this;
        }

        const MyShader& setVecf(const std::string& name, std::initializer_list<float> lst, const GLint count = 1) const {
            int size = lst.size();
            if (size < 1 || size > 4) {
                std::string msg = "ERROR::PROGRAM::INVALID_VECTOR!\nInvalid vector size " + std::to_string(size) + " !\n";
                msg += "The length of vector is expected to be between 1 and 4!";
                throw std::exception(msg.c_str());
            }
            float* vec = new float[size];
            auto ptr = lst.begin();
            for (int i = 0; i < size; ++i, ptr++)
                vec[i] = *ptr;
            auto location = glGetUniformLocation(id, name.c_str());
            switch (size) {
            case 1:
                glUniform1fv(location, count, vec);
                break;
            case 2:
                glUniform2fv(location, count, vec);
                break;
            case 3:
                glUniform3fv(location, count, vec);
                break;
            case 4:
                glUniform4fv(location, count, vec);
                break;
            }
            delete[] vec; vec = nullptr;
            return *this;
        }

        struct _hash
        {
            size_t operator()(const MyShader& shader)const {
                return std::hash<unsigned int>()(shader.id);
            }
        };
    };
}