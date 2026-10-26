#include "render/GlDebug.h"

#include <glad/glad.h>

#include <iostream>

namespace gldebug {

#ifndef NDEBUG
namespace {

const char* errorName(GLenum error)
{
    switch (error) {
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    default:
        return "GL_UNKNOWN_ERROR";
    }
}

}
#endif

void checkErrors(const char* context)
{
#ifndef NDEBUG
    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError()) {
        std::cerr << "OpenGL error " << errorName(error) << " at " << context << '\n';
    }
#else
    (void)context;
#endif
}

}
