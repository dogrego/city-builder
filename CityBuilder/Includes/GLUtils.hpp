#pragma once

#include <filesystem>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

/*

 Based on http://www.opengl-tutorial.org/

*/

// Helper structures

struct VertexPosColor
{
    glm::vec3 position;
    glm::vec3 color;
};

struct VertexPosTex
{
    glm::vec3 position;
    glm::vec2 texcoord;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
};

struct ImageRGBA
{
    typedef glm::u8vec4 TexelRGBA;

    static_assert(sizeof(TexelRGBA) == sizeof(std::uint32_t));

    std::vector<TexelRGBA> texelData;
    unsigned int width = 0;
    unsigned int height = 0;

    bool Allocate(unsigned int _width, unsigned int _height)
    {
        width = _width;
        height = _height;

        texelData.resize(width * height);

        return !texelData.empty();
    }

    bool Assign(const std::uint32_t *_TexelData, unsigned int _width, unsigned int _height)
    {
        width = _width;
        height = _height;

        const TexelRGBA *_data = reinterpret_cast<const TexelRGBA *>(_TexelData);

        texelData.assign(_data, _data + width * height);

        return !texelData.empty();
    }

    TexelRGBA GetTexel(unsigned int x, unsigned int y) const
    {
        return texelData[y * width + x];
    }

    void SetTexel(unsigned int x, unsigned int y, const TexelRGBA &texel)
    {
        texelData[y * width + x] = texel;
    }

    const TexelRGBA *data() const
    {
        return texelData.data();
    }
};

template <typename VertexT>
struct MeshObject
{
    std::vector<VertexT> vertexArray;
    std::vector<GLuint> indexArray;
};

struct OGLObject
{
    GLuint vaoID = 0;  // Vertex Array Object resource identifier
    GLuint vboID = 0;  // Vertex Buffer Object resource identifier
    GLuint iboID = 0;  // Index Buffer Object resource identifier
    GLsizei count = 0; // Number of indices/vertices to be drawn
};

struct VertexAttributeDescriptor
{
    GLuint index = -1;
    GLuint strideInBytes = 0;
    GLint numberOfComponents = 0;
    GLenum glType = GL_NONE;
};

// Helper functions

GLuint AttachShader(const GLuint programID, GLenum shaderType, const std::filesystem::path &_fileName);
GLuint AttachShaderCode(const GLuint programID, GLenum shaderType, std::string_view shaderCode);
void LinkProgram(const GLuint programID, bool OwnShaders = true);

template <typename VertexT>
[[nodiscard]] OGLObject CreateGLObjectFromMesh(const MeshObject<VertexT> &mesh, std::initializer_list<VertexAttributeDescriptor> vertexAttrDescList)
{
    OGLObject meshGPU = {0};

    // Create a new VBO resource name
    glCreateBuffers(1, &meshGPU.vboID);

    // Fill the VBO with data
    glNamedBufferData(meshGPU.vboID,                             // Load data into the VBO
                      mesh.vertexArray.size() * sizeof(VertexT), // With this byte size
                      mesh.vertexArray.data(),                   // From this system memory address
                      GL_STATIC_DRAW);                           // Assuming we do not plan to write further into the VBO, and its data is used in each rendering

    // Create the index buffer
    glCreateBuffers(1, &meshGPU.iboID);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshGPU.iboID);
    glNamedBufferData(meshGPU.iboID, mesh.indexArray.size() * sizeof(GLuint), mesh.indexArray.data(), GL_STATIC_DRAW);

    meshGPU.count = static_cast<GLsizei>(mesh.indexArray.size());

    // Allocate one VAO
    glCreateVertexArrays(1, &meshGPU.vaoID);
    // Set it active

    glVertexArrayVertexBuffer(meshGPU.vaoID, 0, meshGPU.vboID, 0, sizeof(VertexT));

    // Set up attributes
    for (const auto &vertexAttrDesc : vertexAttrDescList)
    {
        glEnableVertexArrayAttrib(meshGPU.vaoID, vertexAttrDesc.index);     // Enable the attribute
        glVertexArrayAttribBinding(meshGPU.vaoID, vertexAttrDesc.index, 0); // Specify which VBO the attribute should read from

        switch (vertexAttrDesc.glType)
        {

        case GL_FLOAT: // The attribute contains float32 values
            glVertexArrayAttribFormat(
                meshGPU.vaoID,                     // Setting attributes for the VAO
                vertexAttrDesc.index,              // Setting attributes for the next index in the VB
                vertexAttrDesc.numberOfComponents, // Number of components
                vertexAttrDesc.glType,             // Data type
                GL_FALSE,                          // Whether it should be normalized
                vertexAttrDesc.strideInBytes       // Where the attribute starts within sizeof(VertexT)
            );
            break;
        case GL_UNSIGNED_INT: // The attribute contains uint values
            glVertexArrayAttribIFormat(
                meshGPU.vaoID,                     // Setting attributes for the VAO
                vertexAttrDesc.index,              // Setting attributes for the next index in the VB
                vertexAttrDesc.numberOfComponents, // Number of components
                vertexAttrDesc.glType,             // Data type
                vertexAttrDesc.strideInBytes       // Where the attribute starts within sizeof(VertexT)
            );
            break;
        case GL_DOUBLE: // The attribute contains double values
            glVertexArrayAttribLFormat(
                meshGPU.vaoID,                     // Setting attributes for the VAO
                vertexAttrDesc.index,              // Setting attributes for the next index in the VB
                vertexAttrDesc.numberOfComponents, // Number of components
                vertexAttrDesc.glType,             // Data type
                vertexAttrDesc.strideInBytes       // Where the attribute starts within sizeof(VertexT)
            );
            break;
        default: // For all other cases, assume normalization is needed for compressed integer values
            glVertexArrayAttribFormat(
                meshGPU.vaoID,                     // Setting attributes for the VAO
                vertexAttrDesc.index,              // Setting attributes for the next index in the VB
                vertexAttrDesc.numberOfComponents, // Number of components
                vertexAttrDesc.glType,             // Data type
                GL_TRUE,                           // Whether it should be normalized
                vertexAttrDesc.strideInBytes       // Where the attribute starts within sizeof(VertexT)
            );
            break;
        }
    }
    glVertexArrayElementBuffer(meshGPU.vaoID, meshGPU.iboID);

    return meshGPU;
}

void CleanOGLObject(OGLObject &ObjectGPU);

[[nodiscard]] ImageRGBA ImageFromFile(const std::filesystem::path &fileName, bool needsFlip = true);
GLsizei NumberOfMIPLevels(const ImageRGBA &);

// Retrieve uniform location in the specified program
inline GLint ul(GLuint programID, const GLchar *uniformName) noexcept
{
    // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
    return glGetUniformLocation(programID, uniformName);
}
// Retrieve uniform location in the active program
inline GLint ul(const GLchar *uniformName) noexcept
{
    GLint prog;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    if (prog == 0)
    {
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 1, GL_DEBUG_SEVERITY_HIGH, -1, "Trying to get uniform location but no shader is active.");
        return -1;
    }
    return ul(prog, uniformName);
}
