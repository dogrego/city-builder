#include "GLUtils.hpp"

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include <SDL2/SDL_image.h>

/*

 Based on http://www.opengl-tutorial.org/

*/

static void loadShaderCode(std::string &shaderCode, const std::filesystem::path &_fileName)
{
	// Loading shader code from _fileName
	shaderCode = "";

	// Opening _fileName
	std::ifstream shaderStream(_fileName);
	if (!shaderStream.is_open())
	{
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
									 SDL_LOG_PRIORITY_ERROR,
									 "Error while opening shader code file %s!", _fileName.string().c_str());
		return;
	}

	// Loading the file content into the shaderCode string
	std::string line = "";
	while (std::getline(shaderStream, line))
	{
		shaderCode += line + "\n";
	}

	shaderStream.close();
}

GLuint AttachShader(const GLuint programID, GLenum shaderType, const std::filesystem::path &_fileName)
{
	// Loading shader code from _fileName
	std::string shaderCode;
	loadShaderCode(shaderCode, _fileName);

	return AttachShaderCode(programID, shaderType, shaderCode);
}

GLuint AttachShaderCode(const GLuint programID, GLenum shaderType, std::string_view shaderCode)
{
	if (programID == 0)
	{
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
									 SDL_LOG_PRIORITY_ERROR,
									 "Program needs to be inited before loading!");
		return 0;
	}

	// Creating the shader
	GLuint shaderID = glCreateShader(shaderType);

	// Assigning code to the shader
	const char *sourcePointer = shaderCode.data();
	GLint sourceLength = static_cast<GLint>(shaderCode.length());

	glShaderSource(shaderID, 1, &sourcePointer, &sourceLength);

	// Compiling the shader
	glCompileShader(shaderID);

	// Checking if everything is okay
	GLint result = GL_FALSE;
	int infoLogLength;

	// Querying the compilation status
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (GL_FALSE == result || infoLogLength != 0)
	{
		// Retrieving and printing the error message
		std::string ErrorMessage(infoLogLength, '\0');
		glGetShaderInfoLog(shaderID, infoLogLength, NULL, ErrorMessage.data());

		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
									 (result) ? SDL_LOG_PRIORITY_WARN : SDL_LOG_PRIORITY_ERROR,
									 "[glCompileShader]: %s", ErrorMessage.data());
	}

	// Attaching the shader to the program
	glAttachShader(programID, shaderID);

	return shaderID;
}

void LinkProgram(const GLuint programID, bool OwnShaders)
{
	// Linking the shaders (matching input-output variables etc)
	glLinkProgram(programID);

	// Checking linking status
	GLint infoLogLength = 0, result = 0;

	glGetProgramiv(programID, GL_LINK_STATUS, &result);
	glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (GL_FALSE == result || infoLogLength != 0)
	{
		std::string ErrorMessage(infoLogLength, '\0');
		glGetProgramInfoLog(programID, infoLogLength, nullptr, ErrorMessage.data());
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
									 (result) ? SDL_LOG_PRIORITY_WARN : SDL_LOG_PRIORITY_ERROR,
									 "[glLinkProgram]: %s", ErrorMessage.data());
	}

	// In this case, the program object owns the shader object
	// This means that the shader objects can be "deleted"
	// According to the standard (https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDeleteShader.xhtml)
	// Shader objects are only deleted if they are not attached to any program objects
	// When the program object is deleted, the shader objects are deleted as well
	if (OwnShaders)
	{
		// Retrieve the shaders attached to the program object
		GLint attachedShaders = 0;
		glGetProgramiv(programID, GL_ATTACHED_SHADERS, &attachedShaders);
		std::vector<GLuint> shaders(attachedShaders);

		glGetAttachedShaders(programID, attachedShaders, nullptr, shaders.data());

		// Delete them
		for (GLuint shader : shaders)
		{
			glDeleteShader(shader);
		}
	}
}

static inline ImageRGBA::TexelRGBA *get_image_row(ImageRGBA &image, int rowIndex)
{
	return &image.texelData[rowIndex * image.width];
}

static void invert_image_RGBA(ImageRGBA &image)
{
	int height_div_2 = image.height / 2;

	for (int index = 0; index < height_div_2; index++)
	{
		std::uint32_t *lower_data = reinterpret_cast<std::uint32_t *>(get_image_row(image, index));
		std::uint32_t *higher_data = reinterpret_cast<std::uint32_t *>(get_image_row(image, image.height - 1 - index));

		for (unsigned int rowIndex = 0; rowIndex < image.width; rowIndex++)
		{
			lower_data[rowIndex] ^= higher_data[rowIndex];
			higher_data[rowIndex] ^= lower_data[rowIndex];
			lower_data[rowIndex] ^= higher_data[rowIndex];
		}
	}
}

GLsizei NumberOfMIPLevels(const ImageRGBA &image)
{
	GLsizei targetlevel = 1;
	unsigned int index = std::max(image.width, image.height);

	while (index >>= 1)
		++targetlevel;

	return targetlevel;
}

[[nodiscard]] ImageRGBA ImageFromFile(const std::filesystem::path &fileName, bool needsFlip)
{
	ImageRGBA img;

	// Loading the image
	std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> loaded_img(IMG_Load(fileName.string().c_str()), SDL_FreeSurface);
	if (!loaded_img)
	{
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
									 SDL_LOG_PRIORITY_ERROR,
									 "[ImageFromFile] Error while loading image file: %s", fileName.string().c_str());
		return img;
	}

	// SDL stores colors in Uint32, so byte order matters
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	Uint32 format = SDL_PIXELFORMAT_ABGR8888;
#else
	Uint32 format = SDL_PIXELFORMAT_RGBA8888;
#endif

	// Convert to 32-bit RGBA format if it was in a different format
	std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> formattedSurf(SDL_ConvertSurfaceFormat(loaded_img.get(), format, 0), SDL_FreeSurface);

	if (!formattedSurf)
	{
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
									 SDL_LOG_PRIORITY_ERROR,
									 "[ImageFromFile] Error while processing texture");
		return img;
	}

	// Transfer SDL Surface to ImageRGBA
	img.Assign(reinterpret_cast<const std::uint32_t *>(formattedSurf->pixels), formattedSurf->w, formattedSurf->h);

	// Convert from SDL coordinate system ((0,0) top-left) to OpenGL texture coordinate system ((0,0) bottom-left)

	if (needsFlip)
		invert_image_RGBA(img);

	return img;
}

void CleanOGLObject(OGLObject &ObjectGPU)
{
	glDeleteBuffers(1, &ObjectGPU.vboID);
	ObjectGPU.vboID = 0;
	glDeleteBuffers(1, &ObjectGPU.iboID);
	ObjectGPU.iboID = 0;
	glDeleteVertexArrays(1, &ObjectGPU.vaoID);
	ObjectGPU.vaoID = 0;
}
