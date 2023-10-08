#pragma once


#include <GL/glew.h>

namespace MyOpenGL
{
    class Texture2D
    {
    public:
        GLuint ID;
        GLuint Width, Height;
        GLuint Internal_format;
        GLuint Image_format;
        GLuint Wrap_S, Wrap_T, Filter_min, Filter_max;
        Texture2D() :Width(0), Height(0), Internal_format(GL_RGB), Image_format(GL_RGB), Wrap_S(GL_REPEAT), Wrap_T(GL_REPEAT), Filter_min(GL_LINEAR), Filter_max(GL_LINEAR) {
            glGenTextures(1, &ID);
        }

        void Generate(GLuint width, GLuint height, unsigned char* data) {
            this->Width = width;
            this->Height = height;
            glBindTexture(GL_TEXTURE_2D, ID);
            glTexImage2D(GL_TEXTURE_2D, 0, Internal_format, width, height, 0, Image_format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, Wrap_S);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, Wrap_T);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Filter_min);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Filter_max);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        void Bind() const {
            glBindTexture(GL_TEXTURE_2D, ID);
        }
    };
};