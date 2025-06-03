#include "MyApp.h"
#include "SDL_GLDebugMessageCallback.h"
#include "ObjParser.h"
#include "ParametricSurfaceMesh.hpp"
#include "ProgramBuilder.h"
#include "Buildings.hpp"

#include <imgui.h>

#include <string>
#include <array>
#include <algorithm>
#include <chrono>
#include <iostream>

CMyApp::CMyApp()
{
}

CMyApp::~CMyApp()
{
}

void CMyApp::SetupDebugCallback()
{
	// Enable and configure the debug callback function if we are in a debug context
	GLint context_flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
	if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, GL_DONT_CARE, 0, nullptr, GL_FALSE);
		glDebugMessageCallback(SDL_GLDebugMessageCallback, nullptr);
	}
}

void CMyApp::InitShaders()
{
	m_programID = glCreateProgram();
	ProgramBuilder{m_programID}
			.ShaderStage(GL_VERTEX_SHADER, "Shaders/Vert_PosNormTex.vert")
			.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/Frag_LightingNoFaceCull.frag")
			.Link();

	m_programWaterID = glCreateProgram();
	ProgramBuilder{m_programWaterID}
			.ShaderStage(GL_VERTEX_SHADER, "Shaders/Vert_Water.vert")
			.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/Frag_Water.frag")
			.Link();

	m_programSkyboxID = glCreateProgram();
	ProgramBuilder{m_programSkyboxID}
			.ShaderStage(GL_VERTEX_SHADER, "Shaders/Vert_Skybox.vert")
			.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/Frag_Skybox.frag")
			.Link();

	m_pickProgramID = glCreateProgram();
	ProgramBuilder{m_pickProgramID}
			.ShaderStage(GL_VERTEX_SHADER, "Shaders/Vert_Pick.vert")
			.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/Frag_Pick.frag")
			.Link();

	m_terrainProgram = glCreateProgram();
	ProgramBuilder{m_terrainProgram}
			.ShaderStage(GL_VERTEX_SHADER, "Shaders/Vert_Terrain.vert")
			.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/Frag_Terrain.frag")
			.Link();

	m_ulTerrainWorld = glGetUniformLocation(m_terrainProgram, "world");
	m_ulTerrainWorldIT = glGetUniformLocation(m_terrainProgram, "worldInvTransp");
	m_ulTerrainViewProj = glGetUniformLocation(m_terrainProgram, "viewProj");
	m_ulTerrainHeightScale = glGetUniformLocation(m_terrainProgram, "heightScale");
	m_ulTerrainTexScale = glGetUniformLocation(m_terrainProgram, "texScale");
}

void CMyApp::InitTerrainTextures()
{
	// Load ground textures
	std::string groundTexPaths[4] = {
			"Assets/ground1.jpg", // Grass/dirt
			"Assets/ground2.jpg", // Dry grass
			"Assets/ground3.jpg", // Mud
			"Assets/ground4.jpg"	// Forest floor
	};

	for (int i = 0; i < 4; ++i)
	{
		ImageRGBA groundImage = ImageFromFile(groundTexPaths[i]);
		glCreateTextures(GL_TEXTURE_2D, 1, &m_groundTextures[i]);
		glTextureStorage2D(m_groundTextures[i], NumberOfMIPLevels(groundImage), GL_RGBA8, groundImage.width, groundImage.height);
		glTextureSubImage2D(m_groundTextures[i], 0, 0, 0, groundImage.width, groundImage.height, GL_RGBA, GL_UNSIGNED_BYTE, groundImage.data());
		glGenerateTextureMipmap(m_groundTextures[i]);
	}

	// Load special textures
	ImageRGBA rockImage = ImageFromFile("Assets/rock.jpg");
	glCreateTextures(GL_TEXTURE_2D, 1, &m_rockTexture);
	glTextureStorage2D(m_rockTexture, NumberOfMIPLevels(rockImage), GL_RGBA8, rockImage.width, rockImage.height);
	glTextureSubImage2D(m_rockTexture, 0, 0, 0, rockImage.width, rockImage.height, GL_RGBA, GL_UNSIGNED_BYTE, rockImage.data());
	glGenerateTextureMipmap(m_rockTexture);

	ImageRGBA sandImage = ImageFromFile("Assets/sand.jpg");
	glCreateTextures(GL_TEXTURE_2D, 1, &m_sandTexture);
	glTextureStorage2D(m_sandTexture, NumberOfMIPLevels(sandImage), GL_RGBA8, sandImage.width, sandImage.height);
	glTextureSubImage2D(m_sandTexture, 0, 0, 0, sandImage.width, sandImage.height, GL_RGBA, GL_UNSIGNED_BYTE, sandImage.data());
	glGenerateTextureMipmap(m_sandTexture);

	ImageRGBA snowImage = ImageFromFile("Assets/snow.jpg");
	glCreateTextures(GL_TEXTURE_2D, 1, &m_snowTexture);
	glTextureStorage2D(m_snowTexture, NumberOfMIPLevels(snowImage), GL_RGBA8, snowImage.width, snowImage.height);
	glTextureSubImage2D(m_snowTexture, 0, 0, 0, snowImage.width, snowImage.height, GL_RGBA, GL_UNSIGNED_BYTE, snowImage.data());
	glGenerateTextureMipmap(m_snowTexture);

	ImageRGBA concreteImage = ImageFromFile("Assets/concrete.jpg");
	glCreateTextures(GL_TEXTURE_2D, 1, &m_concreteTexture);
	glTextureStorage2D(m_concreteTexture, NumberOfMIPLevels(concreteImage), GL_RGBA8, concreteImage.width, concreteImage.height);
	glTextureSubImage2D(m_concreteTexture, 0, 0, 0, concreteImage.width, concreteImage.height, GL_RGBA, GL_UNSIGNED_BYTE, concreteImage.data());
	glGenerateTextureMipmap(m_concreteTexture);
}

void CMyApp::GenerateHeightmap()
{
	const int width = 1000;
	const int height = 1000;

	std::vector<float> heightData(width * height);

	unsigned seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());

	PerlinNoise pn(seed);
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			double nx = x / (double)width - 0.5;
			double ny = y / (double)height - 0.5;

			// Calculate distance from center (0-1)
			float distFromCenter = glm::length(glm::vec2(nx, ny)) * 2.0f; // *2 to normalize to 0-1

			// Generate multi-octave Perlin noise
			double value = pn.octaveNoise(nx * 5, ny * 5, 6, 0.5);

			// Normalize to 0-1 range
			value = (value + 1.0) * 0.5;

			// Apply island mask - reduce height near edges
			float islandMask = 1.0f - smoothstep(0.6f, 1.0f, distFromCenter);
			value *= islandMask;

			// Add some extra noise to the edges to make them more interesting
			if (distFromCenter > 0.7f)
			{
				double edgeNoise = pn.octaveNoise(nx * 10, ny * 10, 2, 0.5) * 0.2;
				value += edgeNoise * (1.0 - islandMask);
			}

			heightData[y * width + x] = static_cast<float>(value);
		}
	}

	// Create texture (same as before)
	glCreateTextures(GL_TEXTURE_2D, 1, &m_heightmapTexture);
	glTextureStorage2D(m_heightmapTexture, 1, GL_R32F, width, height);
	glTextureSubImage2D(m_heightmapTexture, 0, 0, 0, width, height, GL_RED, GL_FLOAT, heightData.data());
	glTextureParameteri(m_heightmapTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_heightmapTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_heightmapTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_heightmapTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void CMyApp::GenerateSplatmap()
{
	const int width = 1000;
	const int height = 1000;

	std::vector<glm::vec4> splatData(width * height);

	unsigned seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());

	PerlinNoise pn(seed);
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			double nx = x / (double)width;
			double ny = y / (double)height;

			// Generate multiple noise layers
			double noise1 = pn.octaveNoise(nx * 5, ny * 5, 3, 0.5);
			double noise2 = pn.octaveNoise(nx * 10, ny * 10, 4, 0.5);
			double noise3 = pn.octaveNoise(nx * 20, ny * 20, 2, 0.5);

			// Create interesting patterns
			glm::vec4 weights(0.0f);
			weights.r = (float)((noise1 + 1) * 0.5);											 // Texture 0
			weights.g = (float)((noise2 + 1) * 0.5);											 // Texture 1
			weights.b = (float)((noise3 + 1) * 0.5);											 // Texture 2
			weights.a = 1.0f - (weights.r + weights.g + weights.b) / 3.0f; // Texture 3

			// Normalize weights
			float sum = weights.r + weights.g + weights.b + weights.a;
			weights /= sum;

			splatData[y * width + x] = weights;
		}
	}

	glCreateTextures(GL_TEXTURE_2D, 1, &m_splatmapTexture);
	glTextureStorage2D(m_splatmapTexture, 1, GL_RGBA32F, width, height);
	glTextureSubImage2D(m_splatmapTexture, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, splatData.data());
	glTextureParameteri(m_splatmapTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_splatmapTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_splatmapTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_splatmapTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void CMyApp::GenerateTerrain()
{
	GenerateHeightmap();
	GenerateSplatmap();

	const int gridSize = 256;
	const float gridSpacing = 1.0f;

	std::vector<glm::vec2> vertices;
	std::vector<GLuint> indices;

	// Generate vertices
	for (int z = 0; z < gridSize; ++z)
	{
		for (int x = 0; x < gridSize; ++x)
		{
			float u = x / (float)(gridSize - 1);
			float v = z / (float)(gridSize - 1);
			vertices.emplace_back(u, v);
		}
	}

	// Generate indices
	for (int z = 0; z < gridSize - 1; ++z)
	{
		for (int x = 0; x < gridSize - 1; ++x)
		{
			int topLeft = z * gridSize + x;
			int topRight = topLeft + 1;
			int bottomLeft = (z + 1) * gridSize + x;
			int bottomRight = bottomLeft + 1;

			indices.push_back(topLeft);
			indices.push_back(bottomLeft);
			indices.push_back(topRight);

			indices.push_back(topRight);
			indices.push_back(bottomLeft);
			indices.push_back(bottomRight);
		}
	}

	m_terrainIndexCount = indices.size();

	// Create VAO, VBO and IBO
	glCreateVertexArrays(1, &m_terrainVAO);
	glCreateBuffers(1, &m_terrainVBO);
	glCreateBuffers(1, &m_terrainIBO);

	// Upload vertex data
	glNamedBufferStorage(m_terrainVBO, vertices.size() * sizeof(glm::vec2), vertices.data(), 0);

	// Upload index data
	glNamedBufferStorage(m_terrainIBO, indices.size() * sizeof(GLuint), indices.data(), 0);

	// Set up VAO
	glVertexArrayVertexBuffer(m_terrainVAO, 0, m_terrainVBO, 0, sizeof(glm::vec2));
	glVertexArrayElementBuffer(m_terrainVAO, m_terrainIBO);

	glEnableVertexArrayAttrib(m_terrainVAO, 0);
	glVertexArrayAttribFormat(m_terrainVAO, 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(m_terrainVAO, 0, 0);
}

void CMyApp::RenderTerrain()
{
	glUseProgram(m_terrainProgram);

	glUniform1f(glGetUniformLocation(m_terrainProgram, "verticalOffset"), m_terrainVerticalOffset);

	// Set texture unit indices first
	glUniform1i(glGetUniformLocation(m_terrainProgram, "splatmap"), 1);
	glUniform1i(glGetUniformLocation(m_terrainProgram, "groundTextures[0]"), 2);
	glUniform1i(glGetUniformLocation(m_terrainProgram, "groundTextures[1]"), 3);
	glUniform1i(glGetUniformLocation(m_terrainProgram, "groundTextures[2]"), 4);
	glUniform1i(glGetUniformLocation(m_terrainProgram, "groundTextures[3]"), 5);
	glUniform1i(glGetUniformLocation(m_terrainProgram, "rockTexture"), 6);
	glUniform1i(glGetUniformLocation(m_terrainProgram, "sandTexture"), 7);
	glUniform1i(glGetUniformLocation(m_terrainProgram, "snowTexture"), 8);
	glUniform1i(glGetUniformLocation(m_terrainProgram, "concreteTexture"), 9);

	// Sky and light colors
	glUniform3fv(ul("sunColor"), 1, glm::value_ptr(m_sunColor));
	glUniform3fv(ul("moonColor"), 1, glm::value_ptr(m_moonColor));
	glUniform3fv(ul("skyTopColor"), 1, glm::value_ptr(m_skyTopColor));
	glUniform3fv(ul("skyBottomColor"), 1, glm::value_ptr(m_skyBottomColor));

	// Time and water properties
	glUniform1f(ul("timeOfDay"), m_timeOfDay);
	glUniform1f(ul("waterReflectivity"), 0.5f);
	glUniform1f(ul("waterWaveIntensity"), 0.1f);

	// Rest of your rendering code...
	glm::mat4 world = glm::mat4(1.0f);
	world = glm::scale(world, glm::vec3(100.0f, 1.0f, 100.0f));

	glUniformMatrix4fv(m_ulTerrainWorld, 1, GL_FALSE, glm::value_ptr(world));
	glUniformMatrix4fv(m_ulTerrainWorldIT, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(world))));
	glUniformMatrix4fv(m_ulTerrainViewProj, 1, GL_FALSE, glm::value_ptr(m_camera.GetViewProj()));
	glUniform1f(m_ulTerrainHeightScale, m_terrainHeightScale);
	glUniform1f(m_ulTerrainTexScale, m_terrainTexScale);

	// Bind textures to correct units
	glBindTextureUnit(0, m_heightmapTexture);
	glBindTextureUnit(1, m_splatmapTexture);
	for (int i = 0; i < 4; ++i)
	{
		glBindTextureUnit(2 + i, m_groundTextures[i]);
	}
	glBindTextureUnit(6, m_rockTexture);
	glBindTextureUnit(7, m_sandTexture);
	glBindTextureUnit(8, m_snowTexture);
	glBindTextureUnit(9, m_concreteTexture);

	// Bind samplers
	for (int i = 0; i < 9; ++i)
	{
		glBindSampler(i, m_SamplerID);
	}

	// Set lighting uniforms
	SetLightingUniforms(32.0f, glm::vec3(0.1f), glm::vec3(1.0f), glm::vec3(0.5f));

	// Draw terrain
	glBindVertexArray(m_terrainVAO);
	glDrawElements(GL_TRIANGLES, m_terrainIndexCount, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);

	// Unbind textures and samplers
	for (int i = 0; i < 9; ++i)
	{
		glBindTextureUnit(i, 0);
		glBindSampler(i, 0);
	}
}

void CMyApp::CleanShaders()
{
	glDeleteProgram(m_programID);
	glDeleteProgram(m_programWaterID);
	glDeleteProgram(m_programSkyboxID);
}

struct Param
{
	glm::vec3 GetPos(float u, float v) const noexcept
	{
		return glm::vec3(u, v, 0.0);
	}

	glm::vec3 GetNorm(float u, float v) const noexcept
	{
		return glm::vec3(0.0, 0.0, 1.0);
	}

	glm::vec2 GetTex(float u, float v) const noexcept
	{
		return glm::vec2(u, v);
	}
};

struct Water
{
	glm::vec3 GetPos(float u, float v) const noexcept
	{
		glm::vec3 pos = glm::vec3(-10.0, 0.0, 10.0) + glm::vec3(20.0, 0.0, -20.0) * glm::vec3(u, 0.0, v);
		pos.y = sinf(pos.z);

		return pos;
	}

	glm::vec3 GetNorm(float u, float v) const noexcept
	{
		glm::vec3 du = GetPos(u + 0.01f, v) - GetPos(u - 0.01f, v);
		glm::vec3 dv = GetPos(u, v + 0.01f) - GetPos(u, v - 0.01f);

		return glm::normalize(glm::cross(du, dv));
	}

	glm::vec2 GetTex(float u, float v) const noexcept
	{
		return glm::vec2(u, v);
	}
};

void CMyApp::InitGeometry()
{

	const std::initializer_list<VertexAttributeDescriptor> vertexAttribList =
			{
					{0, offsetof(Vertex, position), 3, GL_FLOAT},
					{1, offsetof(Vertex, normal), 3, GL_FLOAT},
					{2, offsetof(Vertex, texcoord), 2, GL_FLOAT},
			};

	// quad
	MeshObject<Vertex> quadMeshCPU;
	quadMeshCPU.vertexArray =
			{
					{glm::vec3(-1.0, -1.0, 0.0), glm::vec3(0.0, 0.0, 1.0), glm::vec2(0.0, 0.0)}, // front face
					{glm::vec3(1.0, -1.0, 0.0), glm::vec3(0.0, 0.0, 1.0), glm::vec2(1.0, 0.0)},
					{glm::vec3(1.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0), glm::vec2(1.0, 1.0)},
					{glm::vec3(-1.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0), glm::vec2(0.0, 1.0)}};

	quadMeshCPU.indexArray =
			{
					0, 1, 2, // front face
					2, 3, 0};

	m_quadGPU = CreateGLObjectFromMesh(quadMeshCPU, vertexAttribList);

	// Skybox
	InitSkyboxGeometry();

	// Water
	MeshObject<glm::vec2> waterCPU;
	{
		MeshObject<Vertex> surfaceMeshCPU = GetParamSurfMesh(Param(), 160, 80);
		for (const Vertex &v : surfaceMeshCPU.vertexArray)
		{
			waterCPU.vertexArray.emplace_back(glm::vec2(v.position.x, v.position.y));
		}
		waterCPU.indexArray = surfaceMeshCPU.indexArray;
	}
	m_waterGPU = CreateGLObjectFromMesh(waterCPU, {{0, offsetof(glm::vec2, x), 2, GL_FLOAT}});
}

void CMyApp::CleanGeometry()
{
	CleanOGLObject(m_SkyboxGPU);
}

void CMyApp::InitSkyboxGeometry()
{
	// skybox geo
	MeshObject<glm::vec3> skyboxCPU =
			{
					std::vector<glm::vec3>{
							// back face
							glm::vec3(-1, -1, -1),
							glm::vec3(1, -1, -1),
							glm::vec3(1, 1, -1),
							glm::vec3(-1, 1, -1),
							// front face
							glm::vec3(-1, -1, 1),
							glm::vec3(1, -1, 1),
							glm::vec3(1, 1, 1),
							glm::vec3(-1, 1, 1),
					},

					std::vector<GLuint>{
							// back face
							0,
							1,
							2,
							2,
							3,
							0,
							// front face
							4,
							6,
							5,
							6,
							4,
							7,
							// laft face
							0,
							3,
							4,
							4,
							3,
							7,
							// right face
							1,
							5,
							2,
							5,
							6,
							2,
							// bottom face
							1,
							0,
							4,
							1,
							4,
							5,
							// top face
							3,
							2,
							6,
							3,
							6,
							7,
					}};

	m_SkyboxGPU = CreateGLObjectFromMesh(skyboxCPU, {{0, offsetof(glm::vec3, x), 3, GL_FLOAT}});
}

void CMyApp::InitTextures()
{
	glCreateSamplers(1, &m_SamplerID);

	glSamplerParameteri(m_SamplerID, GL_TEXTURE_WRAP_S, GL_REPEAT); // Changed to REPEAT
	glSamplerParameteri(m_SamplerID, GL_TEXTURE_WRAP_T, GL_REPEAT); // Changed to REPEAT
	glSamplerParameteri(m_SamplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glSamplerParameteri(m_SamplerID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Water texture only
	ImageRGBA waterImage = ImageFromFile("Assets/water_texture.png");
	glCreateTextures(GL_TEXTURE_2D, 1, &m_waterTextureID);
	glTextureStorage2D(m_waterTextureID, NumberOfMIPLevels(waterImage), GL_RGBA8, waterImage.width, waterImage.height);
	glTextureSubImage2D(m_waterTextureID, 0, 0, 0, waterImage.width, waterImage.height, GL_RGBA, GL_UNSIGNED_BYTE, waterImage.data());
	glGenerateTextureMipmap(m_waterTextureID);

	InitSkyboxTextures();

	// Load building texture
	ImageRGBA buildingImage = ImageFromFile("Assets/House1_Diffuse.png");
	glCreateTextures(GL_TEXTURE_2D, 1, &m_buildingTextureID);
	glTextureStorage2D(m_buildingTextureID, NumberOfMIPLevels(buildingImage), GL_RGBA8, buildingImage.width, buildingImage.height);
	glTextureSubImage2D(m_buildingTextureID, 0, 0, 0, buildingImage.width, buildingImage.height, GL_RGBA, GL_UNSIGNED_BYTE, buildingImage.data());
	glGenerateTextureMipmap(m_buildingTextureID);
}

void CMyApp::CleanTextures()
{
	glDeleteTextures(1, &m_waterTextureID);
	glDeleteTextures(1, &m_SkyboxTextureID);
	glDeleteTextures(1, &m_buildingTextureID);
	glDeleteSamplers(1, &m_SamplerID);
}

void CMyApp::InitSkyboxTextures()
{

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_SkyboxTextureID);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

bool CMyApp::Init()
{
	SetupDebugCallback();

	// Set the clear color to a bluish tone
	glClearColor(0.125f, 0.25f, 0.5f, 1.0f);

	InitShaders();
	InitGeometry();
	InitTextures();

	// Additional initialization

	glEnable(GL_CULL_FACE); // Enable back-face culling
	glCullFace(GL_BACK);		// GL_BACK: faces pointing away from the camera, GL_FRONT: faces pointing toward the camera

	glEnable(GL_DEPTH_TEST); // Enable depth testing (occlusion)

	// Camera setup
	m_camera.SetView(
			glm::vec3(0.0, 150.0, 150.0), // Eye position, where we view the scene from
			glm::vec3(0.0, 0.0, 0.0),			// Point in the scene we are looking at
			glm::vec3(0.0, 1.0, 0.0));		// Up direction in the world

	m_cameraManipulator.SetCamera(&m_camera);

	m_sunColor = glm::vec3(1.0f, 1.0f, 0.8f);
	m_moonColor = glm::vec3(0.7f, 0.7f, 0.8f);
	m_skyTopColor = glm::vec3(0.5f, 0.7f, 1.0f);
	m_skyBottomColor = glm::vec3(0.9f, 0.9f, 1.0f);

	m_lightPosition = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f); // Directional light initially pointing downward

	m_moonLightPosition = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f); // Initially pointing downward
	m_moonLa = glm::vec3(0.05f, 0.05f, 0.1f);
	m_moonLd = glm::vec3(0.2f, 0.2f, 0.3f);
	m_moonLs = glm::vec3(0.3f, 0.3f, 0.4f);

	InitTerrainTextures();
	GenerateTerrain();

	Buildings::Initialize();
	m_pickData = new glm::vec3;
	m_buildingColor = glm::vec3(1.0f, 1.0f, 1.0f); // Default white
	CreateFrameBuffer(800, 600);

	return true;
}

void CMyApp::Clean()
{
	CleanShaders();
	CleanGeometry();
	Buildings::Cleanup();
	delete m_pickData;
	if (m_frameBufferCreated)
	{
		glDeleteRenderbuffers(1, &m_depthBuffer);
		glDeleteTextures(1, &m_colorBuffer);
		glDeleteFramebuffers(1, &m_frameBuffer);
	}
	CleanTextures();
}

void CMyApp::Update(const SUpdateInfo &updateInfo)
{
	m_ElapsedTimeInSec = updateInfo.ElapsedTimeInSec;

	UpdateDayNightCycle(updateInfo.DeltaTimeInSec);

	m_cameraManipulator.Update(updateInfo.DeltaTimeInSec);

	m_waterWorldTransform = glm::translate(glm::vec3(0.0f, -2.0f, 0.0f)) * glm::scale(glm::vec3(50.0f, 1.0f, 50.0f));
}

void CMyApp::SetLightingUniforms(float Shininess, glm::vec3 Ka, glm::vec3 Kd, glm::vec3 Ks)
{
	// - Set light sources
	glUniform3fv(ul("cameraPosition"), 1, glm::value_ptr(m_camera.GetEye()));

	// Sun light
	glUniform4fv(ul("lightPosition"), 1, glm::value_ptr(m_lightPosition));
	glUniform3fv(ul("La"), 1, glm::value_ptr(m_La));
	glUniform3fv(ul("Ld"), 1, glm::value_ptr(m_Ld));
	glUniform3fv(ul("Ls"), 1, glm::value_ptr(m_Ls));

	// Moon light
	glUniform4fv(ul("moonLightPosition"), 1, glm::value_ptr(m_moonLightPosition));
	glUniform3fv(ul("moonLa"), 1, glm::value_ptr(m_moonLa));
	glUniform3fv(ul("moonLd"), 1, glm::value_ptr(m_moonLd));
	glUniform3fv(ul("moonLs"), 1, glm::value_ptr(m_moonLs));

	glUniform1f(ul("lightConstantAttenuation"), m_lightConstantAttenuation);
	glUniform1f(ul("lightLinearAttenuation"), m_lightLinearAttenuation);
	glUniform1f(ul("lightQuadraticAttenuation"), m_lightQuadraticAttenuation);

	// - Set material properties
	glUniform3fv(ul("Ka"), 1, glm::value_ptr(Ka));
	glUniform3fv(ul("Kd"), 1, glm::value_ptr(Kd));
	glUniform3fv(ul("Ks"), 1, glm::value_ptr(Ks));

	glUniform1f(ul("Shininess"), Shininess);
}

void CMyApp::Render()
{
	// First pass - render to FBO for picking
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
	// Clear the framebuffer (GL_COLOR_BUFFER_BIT)...
	// ... and the depth Z-buffer (GL_DEPTH_BUFFER_BIT)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Render terrain to FBO with pick shader
	glUseProgram(m_pickProgramID);
	glm::mat4 world = glm::mat4(1.0f);
	world = glm::scale(world, glm::vec3(100.0f, 1.0f, 100.0f));
	world = glm::translate(world, glm::vec3(-0.5f, 0.0f, -0.5f));

	glUniformMatrix4fv(glGetUniformLocation(m_pickProgramID, "world"), 1, GL_FALSE, glm::value_ptr(world));
	glUniformMatrix4fv(glGetUniformLocation(m_pickProgramID, "viewProj"), 1, GL_FALSE, glm::value_ptr(m_camera.GetViewProj()));

	glBindVertexArray(m_terrainVAO);
	glDrawElements(GL_TRIANGLES, m_terrainIndexCount, GL_UNSIGNED_INT, nullptr);

	// Second pass - normal rendering to screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// =========== SKYBOX ===========
	GLint prevDepthFnc;
	glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFnc);
	glDepthFunc(GL_LEQUAL);

	glUseProgram(m_programSkyboxID);

	glUniformMatrix4fv(ul("viewProj"), 1, GL_FALSE, glm::value_ptr(m_camera.GetViewProj()));
	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(glm::translate(m_camera.GetEye())));

	// Pass day-night cycle parameters to skybox fragment shader
	glUniform3fv(ul("sunDirection"), 1, glm::value_ptr(glm::normalize(glm::vec3(m_lightPosition))));
	glUniform3fv(ul("moonDirection"), 1, glm::value_ptr(-glm::normalize(glm::vec3(m_lightPosition))));
	glUniform3fv(ul("sunColor"), 1, glm::value_ptr(m_sunColor));
	glUniform3fv(ul("moonColor"), 1, glm::value_ptr(m_moonColor));
	glUniform3fv(ul("skyTopColor"), 1, glm::value_ptr(m_skyTopColor));
	glUniform3fv(ul("skyBottomColor"), 1, glm::value_ptr(m_skyBottomColor));
	glUniform1f(ul("timeOfDay"), m_timeOfDay); // Pass current time of day

	glBindVertexArray(m_SkyboxGPU.vaoID);
	glDrawElements(GL_TRIANGLES, m_SkyboxGPU.count, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);

	glDepthFunc(prevDepthFnc);

	// =========== WATER ===========
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standard alpha blending

	glUseProgram(m_programWaterID);

	glUniformMatrix4fv(ul("viewProj"), 1, GL_FALSE, glm::value_ptr(m_camera.GetViewProj()));
	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(m_waterWorldTransform));
	glUniformMatrix4fv(ul("worldInvTransp"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(m_waterWorldTransform))));

	// Set water material properties with lower alpha
	glm::vec3 waterKa = m_Ka;
	glm::vec3 waterKd = m_Kd;
	glm::vec3 waterKs = m_Ks;
	float waterShininess = m_Shininess;
	SetLightingUniforms(waterShininess, waterKa, waterKd, waterKs);

	glUniform1f(ul("Alpha"), 0.5f); // 0.7 = 70% opacity

	glUniform1f(ul("ElapsedTimeInSec"), m_ElapsedTimeInSec);

	glBindTextureUnit(0, m_waterTextureID);
	glBindSampler(0, m_SamplerID);

	glBindVertexArray(m_waterGPU.vaoID);
	glDrawElements(GL_TRIANGLES, m_waterGPU.count, GL_UNSIGNED_INT, nullptr);

	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	// =========== TERRAIN ===========
	RenderTerrain();

	// =========== BUILDINGS ===========
	// Render building preview if active
	if (m_showBuildingPreview)
	{
		glUseProgram(m_programID);
		glm::mat4 previewWorld = glm::translate(m_buildingPreviewPos);
		glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(previewWorld));
		glUniformMatrix4fv(ul("worldInvTransp"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(previewWorld))));

		const BuildingData &data = Buildings::GetBuildingData(m_selectedBuildingType);
		glBindVertexArray(data.vao);
		glDrawArrays(GL_TRIANGLES, 0, data.vertexCount);
	}

	// Render all placed buildings
	RenderBuildings();

	// ===========================

	// Disable shader
	glUseProgram(0);

	// - Disable textures for each unit separately
	glBindTextureUnit(0, 0);
	glBindSampler(0, 0);

	// Disable VAO
	glBindVertexArray(0);
}

void CMyApp::RenderGUI()
{
	if (ImGui::Begin("Day/Night Cycle"))
	{
		// Apply 6-hour offset (0.25 in 0-1 range) to make 6:00 AM the starting point
		const float timeOffset = 0.25f; // 6/24 = 0.25
		float offsetTime = fmod(m_timeOfDay + timeOffset, 1.0f);

		// Convert to hours (0-24)
		float hours = offsetTime * 24.0f;

		// Display current time in HH:MM format
		int totalMinutes = static_cast<int>(hours * 60);
		int displayHours = totalMinutes / 60;
		int displayMinutes = totalMinutes % 60;
		ImGui::Text("Current Time: %02d:%02d", displayHours, displayMinutes);

		// Slider that shows 0-24 hours with 0.01 precision
		if (ImGui::SliderFloat("Time of Day", &hours, 0.0f, 24.0f, "%.2f"))
		{
			// Convert back to 0-1 range and remove offset
			offsetTime = hours / 24.0f;
			m_timeOfDay = fmod(offsetTime - timeOffset + 1.0f, 1.0f); // +1.0f to ensure positive

			// Update light position immediately when slider changes
			float sunAngle = m_timeOfDay * 2.0f * glm::pi<float>();
			glm::vec3 sunDir = glm::vec3(cosf(sunAngle), sinf(sunAngle), 0.0f);
			m_lightPosition = glm::vec4(sunDir, 0.0f);

			// Update moon position (opposite the sun)
			float moonAngle = sunAngle + glm::pi<float>();
			glm::vec3 moonDir = glm::vec3(cosf(moonAngle), sinf(moonAngle), 0.0f);
			m_moonLightPosition = glm::vec4(moonDir, 0.0f);
		}

		// Keep the time speed control
		ImGui::SliderFloat("Time Speed", &m_timeSpeed, 0.0f, 0.1f, "%.4f");
	}
	ImGui::End();

	if (ImGui::Begin("Building Settings"))
	{
		const char *buildingTypes[] = {
				"Studio Flat",
				"Small House",
				"Family House",
				"Tower",
				"Apartment Block"};

		int currentType = static_cast<int>(m_selectedBuildingType);
		if (ImGui::Combo("Building Type", &currentType, buildingTypes, IM_ARRAYSIZE(buildingTypes)))
		{
			m_selectedBuildingType = static_cast<BuildingType>(currentType);
		}

		ImGui::ColorEdit3("Building Color", &m_buildingColor[0]);

		ImGui::Text("Ctrl + Left click to place building");
		ImGui::Text("Buildings placed: %d", m_buildings.size());
	}
	ImGui::End();
}

// https://wiki.libsdl.org/SDL2/SDL_KeyboardEvent
// https://wiki.libsdl.org/SDL2/SDL_Keysym
// https://wiki.libsdl.org/SDL2/SDL_Keycode
// https://wiki.libsdl.org/SDL2/SDL_Keymod

void CMyApp::KeyboardDown(const SDL_KeyboardEvent &key)
{
	if (key.repeat == 0) // Pressed for the first time
	{
		if (key.keysym.sym == SDLK_F5 && key.keysym.mod & KMOD_CTRL)
		{
			CleanShaders();
			InitShaders();
		}
		if (key.keysym.sym == SDLK_F1)
		{
			GLint polygonModeFrontAndBack[2] = {};
			// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGet.xhtml
			glGetIntegerv(GL_POLYGON_MODE, polygonModeFrontAndBack);													// Query the current polygon mode, separate for front and back modes
			GLenum polygonMode = (polygonModeFrontAndBack[0] != GL_FILL ? GL_FILL : GL_LINE); // Toggle between FILL and LINE modes
			// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glPolygonMode.xhtml
			glPolygonMode(GL_FRONT_AND_BACK, polygonMode); // Set the new mode
		}
	}
	m_cameraManipulator.KeyboardDown(key);
}

void CMyApp::KeyboardUp(const SDL_KeyboardEvent &key)
{
	m_cameraManipulator.KeyboardUp(key);
}

// https://wiki.libsdl.org/SDL2/SDL_MouseMotionEvent

void CMyApp::MouseMove(const SDL_MouseMotionEvent &mouse)
{
	m_cameraManipulator.MouseMove(mouse);

	int viewportWidth, viewportHeight;
	GetViewportSize(viewportWidth, viewportHeight);

	// Read from FBO for building preview
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
	glReadPixels(mouse.x, viewportHeight - mouse.y - 1, 1, 1, GL_RGB, GL_FLOAT, m_pickData);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (m_pickData->z < 0.1f)
	{
		float u = m_pickData->x;
		float v = m_pickData->y;

		// Convert UV to world coordinates (terrain spans -50 to 50 on X and Z)
		glm::vec3 worldPos(
				(u * 100.0f) - 50.0f, // X: [0,1] -> [-50,50]
				0.0f,									// Temporary Y, will be set properly
				(v * 100.0f) - 50.0f	// Z: [0,1] -> [-50,50]
		);

		UpdateBuildingPreview(worldPos);
	}
	else if (!(SDL_GetModState() & KMOD_CTRL))
	{
		m_showBuildingPreview = false;
	}
}

// https://wiki.libsdl.org/SDL2/SDL_MouseButtonEvent

void CMyApp::MouseDown(const SDL_MouseButtonEvent &mouse)
{
	if (mouse.button == SDL_BUTTON_LEFT && (SDL_GetModState() & KMOD_CTRL))
	{
		int viewportWidth, viewportHeight;
		GetViewportSize(viewportWidth, viewportHeight);

		// Read from FBO
		glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
		glReadPixels(mouse.x, viewportHeight - mouse.y - 1, 1, 1,
								 GL_RGB, GL_FLOAT, m_pickData);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// If we hit terrain (z coordinate is 0 in our pick shader)
		if (m_pickData->z < 0.1f)
		{
			float u = m_pickData->x;
			float v = m_pickData->y;

			// Convert UV to world coordinates (match terrain scale)
			glm::vec3 worldPos(
					(u - 0.5f) * 100.0f, // X coordinate
					0.0f,								 // Temporary Y, will be set properly
					(v - 0.5f) * 100.0f	 // Z coordinate
			);

			// Place building with proper height
			PlaceBuilding(worldPos);
		}
	}
}

void CMyApp::MouseUp(const SDL_MouseButtonEvent &mouse)
{
}

// https://wiki.libsdl.org/SDL2/SDL_MouseWheelEvent

void CMyApp::MouseWheel(const SDL_MouseWheelEvent &wheel)
{
	m_cameraManipulator.MouseWheel(wheel);
}

// The two parameters contain the new window width (_w) and height (_h)
void CMyApp::Resize(int _w, int _h)
{
	glViewport(0, 0, _w, _h);
	m_camera.SetAspect(static_cast<float>(_w) / _h);
	CreateFrameBuffer(_w, _h);
}

// Handling unprocessed, uncommon events
// https://wiki.libsdl.org/SDL2/SDL_Event

void CMyApp::OtherEvent(const SDL_Event &ev)
{
}

void CMyApp::GetViewportSize(int &width, int &height)
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	width = viewport[2];
	height = viewport[3];
}

void CMyApp::UpdateDayNightCycle(float deltaTime)
{
	// Update time of day (0-1 represents 24 hours)
	m_timeOfDay += deltaTime * m_timeSpeed;
	if (m_timeOfDay >= 1.0f)
		m_timeOfDay -= 1.0f;

	// Calculate sun position (circular path)
	float sunAngle = m_timeOfDay * 2.0f * glm::pi<float>();
	glm::vec3 sunDir = glm::vec3(cosf(sunAngle), sinf(sunAngle), 0.0f);
	m_lightPosition = glm::vec4(sunDir, 0.0f);

	// Calculate normalized sun height [-1, 1] -> [0, 1] where 0=horizon, 1=zenith
	float sunHeight = (sunDir.y + 1.0f) * 0.5f;

	// Sun color - completely smooth gradient using key points
	const glm::vec3 nightColor(0.0f, 0.0f, 0.0f);
	const glm::vec3 sunLowColor(1.0f, 0.2f, 0.0f);		// Deep red
	const glm::vec3 sunMidColor(1.0f, 0.5f, 0.1f);		// Bright orange
	const glm::vec3 sunHighColor(1.0f, 0.8f, 0.5f);		// Warm white
	const glm::vec3 sunZenithColor(1.0f, 1.0f, 1.0f); // Pure white

	// Smooth sun color transition using smoothstep interpolation
	float sunColorBlend1 = glm::smoothstep(0.0f, 0.1f, sunHeight);
	float sunColorBlend2 = glm::smoothstep(0.1f, 0.3f, sunHeight);
	float sunColorBlend3 = glm::smoothstep(0.3f, 0.7f, sunHeight);

	m_sunColor = nightColor;
	if (sunDir.y > -0.2f)
	{
		m_sunColor = glm::mix(m_sunColor, sunLowColor, sunColorBlend1);
		m_sunColor = glm::mix(m_sunColor, sunMidColor, sunColorBlend2);
		m_sunColor = glm::mix(m_sunColor, sunHighColor, sunColorBlend3);
		m_sunColor = glm::mix(m_sunColor, sunZenithColor, glm::smoothstep(0.7f, 1.0f, sunHeight));
	}

	// Moon position (opposite the sun)
	float moonAngle = sunAngle + glm::pi<float>();
	glm::vec3 moonDir = glm::vec3(cosf(moonAngle), sinf(moonAngle), 0.0f);
	m_moonLightPosition = glm::vec4(moonDir, 0.0f);

	// Calculate normalized moon height [-1, 1] -> [0, 1] where 0=horizon, 1=zenith
	float moonHeight = (moonDir.y + 1.0f) * 0.5f;

	// Moon color - completely smooth gradient using key points
	const glm::vec3 dayColor(0.0f, 0.0f, 0.0f);
	const glm::vec3 moonLowColor(0.3f, 0.3f, 0.8f);		 // Rich blue
	const glm::vec3 moonMidColor(0.5f, 0.5f, 0.95f);	 // Bright blue
	const glm::vec3 moonHighColor(0.8f, 0.8f, 1.0f);	 // Pale blue
	const glm::vec3 moonZenithColor(1.0f, 1.0f, 1.0f); // Pure white

	// Smooth moon color transition using smoothstep interpolation
	float moonColorBlend1 = glm::smoothstep(0.0f, 0.2f, moonHeight);
	float moonColorBlend2 = glm::smoothstep(0.2f, 0.5f, moonHeight);
	float moonColorBlend3 = glm::smoothstep(0.5f, 0.8f, moonHeight);

	m_moonColor = dayColor;
	if (moonDir.y > -0.2f)
	{
		m_moonColor = glm::mix(m_moonColor, moonLowColor, moonColorBlend1);
		m_moonColor = glm::mix(m_moonColor, moonMidColor, moonColorBlend2);
		m_moonColor = glm::mix(m_moonColor, moonHighColor, moonColorBlend3);
		m_moonColor = glm::mix(m_moonColor, moonZenithColor, glm::smoothstep(0.7f, 1.0f, moonHeight));
	}

	// Sky colors - completely smooth transition
	const glm::vec3 nightTop(0.02f, 0.02f, 0.1f);
	const glm::vec3 nightBottom(0.0f, 0.0f, 0.05f);
	const glm::vec3 sunriseTop1(0.3f, 0.1f, 0.4f);			// Purple dawn
	const glm::vec3 sunriseTop2(0.8f, 0.3f, 0.2f);			// Red sunrise
	const glm::vec3 dayTop(0.3f, 0.5f, 1.0f);						// Day blue
	const glm::vec3 sunriseBottom1(0.1f, 0.05f, 0.15f); // Dark purple
	const glm::vec3 sunriseBottom2(1.0f, 0.5f, 0.2f);		// Orange sunrise
	const glm::vec3 dayBottom(0.6f, 0.8f, 1.0f);				// Day light blue

	// Calculate smooth blending factors
	float horizonEffect = glm::smoothstep(-0.3f, 0.1f, sunDir.y);
	float daylightEffect = glm::smoothstep(-0.1f, 0.2f, sunDir.y);

	// Sky top - smooth transition through all phases
	if (sunDir.y < 0.1f)
	{
		// Night to sunrise transition
		float dawnBlend = glm::smoothstep(-0.3f, -0.1f, sunDir.y);
		float sunriseBlend = glm::smoothstep(-0.1f, 0.1f, sunDir.y);

		m_skyTopColor = nightTop;
		m_skyTopColor = glm::mix(m_skyTopColor, sunriseTop1, dawnBlend);
		m_skyTopColor = glm::mix(m_skyTopColor, sunriseTop2, sunriseBlend);
		m_skyTopColor = glm::mix(m_skyTopColor, dayTop, daylightEffect);
	}
	else
	{
		// Normal day transition
		m_skyTopColor = glm::mix(sunriseTop2, dayTop, daylightEffect);
	}

	// Sky bottom - smooth transition through all phases
	if (sunDir.y < 0.1f)
	{
		// Night to sunrise transition
		float dawnBlend = glm::smoothstep(-0.3f, -0.1f, sunDir.y);
		float sunriseBlend = glm::smoothstep(-0.1f, 0.1f, sunDir.y);

		m_skyBottomColor = nightBottom;
		m_skyBottomColor = glm::mix(m_skyBottomColor, sunriseBottom1, dawnBlend);
		m_skyBottomColor = glm::mix(m_skyBottomColor, sunriseBottom2, sunriseBlend);
		m_skyBottomColor = glm::mix(m_skyBottomColor, dayBottom, daylightEffect);
	}
	else
	{
		// Normal day transition
		m_skyBottomColor = glm::mix(sunriseBottom2, dayBottom, daylightEffect);
	}

	// Calculate intensities based on height above horizon
	float sunIntensity = (sunDir.y > -0.1f) ? pow(glm::clamp(sunDir.y, 0.0f, 1.0f), 1.5f) : 0.0f;
	float moonIntensity = (moonDir.y > -0.1f) ? pow(glm::clamp(moonDir.y, 0.0f, 1.0f), 1.5f) : 0.0f;

	// Lighting parameters
	m_La = m_sunColor * (0.1f + 0.1f * sunIntensity);
	m_Ld = m_sunColor * (0.3f + 0.5f * sunIntensity);
	m_Ls = m_sunColor * (0.5f + 0.5f * sunIntensity);

	// Moon-specific lighting parameters
	m_moonLa = m_moonColor * (0.02f + 0.03f * moonIntensity);
	m_moonLd = m_moonColor * (0.1f + 0.1f * moonIntensity);
	m_moonLs = m_moonColor * (0.2f + 0.1f * moonIntensity);
}

// Smoothstep function for better transitions
float CMyApp::smoothstep(float edge0, float edge1, float x)
{
	x = glm::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return x * x * (3.0f - 2.0f * x);
}

void CMyApp::CreateFrameBuffer(int width, int height)
{
	// Clean up if this function is not being called for the first time
	if (m_frameBufferCreated)
	{
		glDeleteRenderbuffers(1, &m_depthBuffer);
		glDeleteTextures(1, &m_colorBuffer);
		glDeleteFramebuffers(1, &m_frameBuffer);
	}

	glGenFramebuffers(1, &m_frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

	glGenTextures(1, &m_colorBuffer);
	glBindTexture(GL_TEXTURE_2D, m_colorBuffer);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorBuffer, 0);
	if (glGetError() != GL_NO_ERROR)
	{
		std::cout << "Error creating color attachment" << std::endl;
		exit(1);
	}

	glGenRenderbuffers(1, &m_depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer);
	if (glGetError() != GL_NO_ERROR)
	{
		std::cout << "Error creating depth attachment" << std::endl;
		exit(1);
	}

	// -- Completeness check
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Incomplete framebuffer (";
		switch (status)
		{
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			std::cout << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			std::cout << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			std::cout << "GL_FRAMEBUFFER_UNSUPPORTED";
			break;
		}
		std::cout << ")" << std::endl;
		char ch;
		std::cin >> ch;
		exit(1);
	}

	// -- Unbind framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	m_frameBufferCreated = true;
}

float CMyApp::SampleHeightmap(const glm::vec2 &uv)
{
	// Clamp UV coordinates to avoid edge artifacts
	glm::vec2 clampedUV = glm::clamp(uv, 0.0f, 1.0f);

	// Bind heightmap texture
	glBindTexture(GL_TEXTURE_2D, m_heightmapTexture);

	// Get texture dimensions
	GLint width, height;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	// Calculate exact texture coordinates
	float x = clampedUV.x * (width - 1);
	float y = clampedUV.y * (height - 1);

	// Read single pixel from heightmap
	float pixelValue;
	glGetTextureSubImage(m_heightmapTexture, 0,
											 static_cast<int>(x), static_cast<int>(y), 0,
											 1, 1, 1,
											 GL_RED, GL_FLOAT,
											 sizeof(float), &pixelValue);

	// Apply terrain scaling and offset
	return (pixelValue * m_terrainHeightScale) + m_terrainVerticalOffset - 25;
}

void CMyApp::UpdateBuildingPreview(const glm::vec3 &pos)
{
	// Calculate UV coordinates from world position
	glm::vec2 uv(
			(pos.x + 50.0f) / 100.0f, // Convert from [-50,50] to [0,1]
			(pos.z + 50.0f) / 100.0f	// Convert from [-50,50] to [0,1]
	);

	// Sample height from heightmap
	float height = SampleHeightmap(uv);

	// Only show preview if above water
	m_showBuildingPreview = (height >= WATER_LEVEL);

	if (m_showBuildingPreview)
	{
		m_buildingPreviewPos = glm::vec3(pos.x, height, pos.z);

		// Check for collisions using building dimensions
		glm::vec2 buildingSize = Buildings::GetBuildingSize(m_selectedBuildingType);

		// Create AABB for the new building
		glm::vec2 newMin = glm::vec2(pos.x, pos.z) - buildingSize * 0.5f;
		glm::vec2 newMax = glm::vec2(pos.x, pos.z) + buildingSize * 0.5f;

		for (const auto &building : m_buildings)
		{
			// Get accurate size for existing building
			glm::vec2 existingSize = Buildings::GetBuildingSize(m_selectedBuildingType);

			// Create AABB for existing building
			glm::vec2 existingMin = glm::vec2(building.position.x, building.position.z) - existingSize * 0.5f;
			glm::vec2 existingMax = glm::vec2(building.position.x, building.position.z) + existingSize * 0.5f;

			const float PADDING = 1.2f; // Small padding to prevent buildings from touching

			bool collision = (newMin.x < existingMax.x + PADDING &&
												newMax.x + PADDING > existingMin.x &&
												newMin.y < existingMax.y + PADDING &&
												newMax.y + PADDING > existingMin.y);

			if (collision)
			{
				m_showBuildingPreview = false;
				break;
			}
		}
	}
}

void CMyApp::PlaceBuilding(const glm::vec3 &pos)
{
	// Get building dimensions based on type
	glm::vec2 buildingSize = Buildings::GetBuildingSize(m_selectedBuildingType);

	// Create AABB for the new building
	glm::vec2 newMin = glm::vec2(pos.x, pos.z) - buildingSize * 0.5f;
	glm::vec2 newMax = glm::vec2(pos.x, pos.z) + buildingSize * 0.5f;

	// Check for collisions with existing buildings
	for (const auto &building : m_buildings)
	{
		glm::vec2 existingSize = Buildings::GetBuildingSize(m_selectedBuildingType);

		glm::vec2 existingMin = glm::vec2(building.position.x, building.position.z) - existingSize * 0.5f;
		glm::vec2 existingMax = glm::vec2(building.position.x, building.position.z) + existingSize * 0.5f;

		const float PADDING = 1.2f;
		bool collision = (newMin.x < existingMax.x + PADDING &&
											newMax.x + PADDING > existingMin.x &&
											newMin.y < existingMax.y + PADDING &&
											newMax.y + PADDING > existingMin.y);

		if (collision)
		{
			return; // Don't place if collision detected
		}
	}

	// Calculate UV coordinates from world position
	glm::vec2 uv(
			(pos.x + 50.0f) / 100.0f,
			(pos.z + 50.0f) / 100.0f);

	// Apply concrete texture around the building
	ApplyConcreteTexture(uv, m_selectedBuildingType); // Convert radius to UV space

	// Sample height from heightmap and smooth the terrain
	float height = SmoothTerrainUnderBuilding(uv, buildingSize);

	// Check if position is underwater
	if (height < WATER_LEVEL)
	{
		return; // Don't place building underwater
	}

	// Create new building instance at the correct height
	BuildingInstance newBuilding;
	newBuilding.position = glm::vec3(pos.x, height, pos.z);
	newBuilding.type = m_selectedBuildingType;
	newBuilding.color = m_buildingColor;

	m_buildings.push_back(newBuilding);
}

float CMyApp::SmoothTerrainUnderBuilding(const glm::vec2 &centerUV, const glm::vec2 &size)
{
	// Determine smoothing area
	float radiusX = size.x / 100.0f + 0.005f;
	float radiusY = size.y / 100.0f + 0.005f;

	glm::vec2 minUV = glm::clamp(centerUV - glm::vec2(radiusX, radiusY), 0.0f, 1.0f);
	glm::vec2 maxUV = glm::clamp(centerUV + glm::vec2(radiusX, radiusY), 0.0f, 1.0f);

	// Get texture dimensions
	GLint width, height;
	glBindTexture(GL_TEXTURE_2D, m_heightmapTexture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	int minX = static_cast<int>(minUV.x * (width - 1));
	int maxX = static_cast<int>(maxUV.x * (width - 1));
	int minY = static_cast<int>(minUV.y * (height - 1));
	int maxY = static_cast<int>(maxUV.y * (height - 1));

	// Get current height data
	std::vector<float> heightData((maxX - minX + 1) * (maxY - minY + 1));
	glGetTextureSubImage(m_heightmapTexture, 0, minX, minY, 0,
											 maxX - minX + 1, maxY - minY + 1, 1,
											 GL_RED, GL_FLOAT, heightData.size() * sizeof(float), heightData.data());

	// Check for nearby buildings and find the closest one
	float closestBuildingHeight = 0.0f;
	float minDistance = FLT_MAX;
	float overlapMargin = 0.02f; // in UV space

	for (const auto &b : m_buildings)
	{
		glm::vec2 bCenterUV = glm::vec2(b.position.x, b.position.z);
		float distance = glm::distance(centerUV, bCenterUV);

		if (distance < minDistance)
		{
			minDistance = distance;
			if (!b.originalTerrainHeights.empty())
			{
				float sum = std::accumulate(b.originalTerrainHeights.begin(), b.originalTerrainHeights.end(), 0.0f);
				closestBuildingHeight = sum / b.originalTerrainHeights.size();
			}
		}
	}

	// If we're close to an existing building, use its height
	const float MAX_DISTANCE_TO_MATCH_HEIGHT = 0.05f; // in UV space
	if (minDistance < MAX_DISTANCE_TO_MATCH_HEIGHT && !m_buildings.empty())
	{
		// Don't modify terrain, just return the closest building's height
		float finalHeight = (closestBuildingHeight * m_terrainHeightScale) + m_terrainVerticalOffset - 25;
		// Ensure the height is not below water level
		return std::max(finalHeight, WATER_LEVEL);
	}

	// Otherwise, proceed with normal smoothing (for isolated buildings)
	float sum = std::accumulate(heightData.begin(), heightData.end(), 0.0f);
	float averageHeight = sum / heightData.size();

	// Ensure the smoothed height is not below water level (convert water level to heightmap space)
	float waterLevelInHeightmapSpace = (WATER_LEVEL - m_terrainVerticalOffset + 25) / m_terrainHeightScale;
	averageHeight = std::max(averageHeight, waterLevelInHeightmapSpace);

	std::fill(heightData.begin(), heightData.end(), averageHeight);

	// Update texture
	glTextureSubImage2D(m_heightmapTexture, 0, minX, minY,
											maxX - minX + 1, maxY - minY + 1, GL_RED, GL_FLOAT, heightData.data());

	// Store original heights for this new building
	if (!m_buildings.empty())
	{
		m_buildings.back().originalTerrainHeights = heightData;
	}

	float finalHeight = (averageHeight * m_terrainHeightScale) + m_terrainVerticalOffset - 25;
	// Final safety check (shouldn't be needed but just in case)
	return std::max(finalHeight, WATER_LEVEL);
}

void CMyApp::ApplyConcreteTexture(const glm::vec2 &centerUV, BuildingType buildingType)
{
	// Get building dimensions from Buildings class
	glm::vec2 buildingSize = Buildings::GetBuildingSize(buildingType);

	// Add margin around the building (e.g., 0.5 units on each side)
	const float margin = 1.0f;
	glm::vec2 concreteSize = buildingSize + glm::vec2(margin * 2);

	// Convert size from world units to UV space (100 units = 1.0 in UV)
	glm::vec2 sizeUV = concreteSize / 100.0f;

	// Rest of your existing implementation...
	GLint width, height;
	glBindTexture(GL_TEXTURE_2D, m_splatmapTexture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	int minX = static_cast<int>((centerUV.x - sizeUV.x / 2) * width);
	int maxX = static_cast<int>((centerUV.x + sizeUV.x / 2) * width);
	int minY = static_cast<int>((centerUV.y - sizeUV.y / 2) * height);
	int maxY = static_cast<int>((centerUV.y + sizeUV.y / 2) * height);

	// Clamp to texture bounds
	minX = glm::clamp(minX, 0, width - 1);
	maxX = glm::clamp(maxX, 0, width - 1);
	minY = glm::clamp(minY, 0, height - 1);
	maxY = glm::clamp(maxY, 0, height - 1);

	// Read current splatmap data
	std::vector<glm::vec4> splatData((maxX - minX + 1) * (maxY - minY + 1));
	glGetTextureSubImage(m_splatmapTexture, 0,
											 minX, minY, 0,
											 maxX - minX + 1, maxY - minY + 1, 1,
											 GL_RGBA, GL_FLOAT,
											 splatData.size() * sizeof(glm::vec4), splatData.data());

	// Modify splatmap data
	for (int y = minY; y <= maxY; y++)
	{
		for (int x = minX; x <= maxX; x++)
		{
			int idx = (y - minY) * (maxX - minX + 1) + (x - minX);

			// Check if the texel is within the square area (no distance calculation)
			// For a rectangular area, use different radii for X/Y (e.g., radiusX, radiusY)
			splatData[idx].r *= 0.2f; // Reduce other textures
			splatData[idx].g *= 0.2f;
			splatData[idx].b *= 0.2f;
			splatData[idx].a = 0.8f; // Max concrete weight
		}
	}

	// Update texture
	glTextureSubImage2D(m_splatmapTexture, 0,
											minX, minY,
											maxX - minX + 1, maxY - minY + 1,
											GL_RGBA, GL_FLOAT, splatData.data());
}

void CMyApp::RenderBuildings()
{
	glUseProgram(m_programID);

	// Bind building texture
	glBindTextureUnit(0, m_buildingTextureID);
	glBindSampler(0, m_SamplerID);

	// Render all placed buildings
	for (const auto &building : m_buildings)
	{
		glm::mat4 world = glm::translate(building.position);
		glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(world));
		glUniformMatrix4fv(ul("worldInvTransp"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(world))));
		glUniformMatrix4fv(ul("viewProj"), 1, GL_FALSE, glm::value_ptr(m_camera.GetViewProj()));

		// Pass building color to shader
		glUniform3fv(glGetUniformLocation(m_programID, "buildingColor"), 1, glm::value_ptr(building.color));

		// Set material properties for buildings
		SetLightingUniforms(32.0f, glm::vec3(0.1f), glm::vec3(0.8f), glm::vec3(0.5f));

		const BuildingData &data = Buildings::GetBuildingData(building.type);
		glBindVertexArray(data.vao);
		glDrawArrays(GL_TRIANGLES, 0, data.vertexCount);
	}

	// Render building preview with current color
	if (m_showBuildingPreview)
	{
		glm::mat4 world = glm::translate(m_buildingPreviewPos);
		glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(world));
		glUniformMatrix4fv(ul("worldInvTransp"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(world))));

		// Use current building color for preview
		glUniform3fv(glGetUniformLocation(m_programID, "buildingColor"), 1, glm::value_ptr(m_buildingColor));

		const BuildingData &data = Buildings::GetBuildingData(m_selectedBuildingType);
		glBindVertexArray(data.vao);
		glDrawArrays(GL_TRIANGLES, 0, data.vertexCount);
	}

	glBindVertexArray(0);
	glBindTextureUnit(0, 0);
	glBindSampler(0, 0);
}