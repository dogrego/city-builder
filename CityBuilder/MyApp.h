#pragma once

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

// GLEW
#include <GL/glew.h>

// SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

// Utils
#include "GLUtils.hpp"
#include "Camera.h"
#include "CameraManipulator.h"

#include "Perlin.h"
#include "buildings.hpp"

struct SUpdateInfo
{
	float ElapsedTimeInSec = 0.0f; // Time elapsed since program start
	float DeltaTimeInSec = 0.0f;	 // Time elapsed since previous update
};

class CMyApp
{
public:
	CMyApp();
	~CMyApp();

	bool Init();
	void Clean();

	void Update(const SUpdateInfo &);
	void Render();
	void RenderGUI();

	void KeyboardDown(const SDL_KeyboardEvent &);
	void KeyboardUp(const SDL_KeyboardEvent &);
	void MouseMove(const SDL_MouseMotionEvent &);
	void MouseDown(const SDL_MouseButtonEvent &);
	void MouseUp(const SDL_MouseButtonEvent &);
	void MouseWheel(const SDL_MouseWheelEvent &);
	void Resize(int, int);

	void OtherEvent(const SDL_Event &);

protected:
	void SetupDebugCallback();

	// Data variables

	float m_ElapsedTimeInSec = 0.0f;

	glm::mat4 m_waterWorldTransform;

	// Camera
	Camera m_camera;
	CameraManipulator m_cameraManipulator;

	// OpenGL-related elements

	// Shader-related variables
	GLuint m_programID = 0;				// Shader program
	GLuint m_programSkyboxID = 0; // Skybox program
	GLuint m_programWaterID = 0;	// Water program

	// Light source properties
	glm::vec4 m_lightPosition = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

	glm::vec3 m_La = glm::vec3(0.125f);
	glm::vec3 m_Ld = glm::vec3(1.0, 1.0, 1.0);
	glm::vec3 m_Ls = glm::vec3(1.0, 1.0, 1.0);

	glm::vec4 m_moonLightPosition;
	glm::vec3 m_moonLa, m_moonLd, m_moonLs;

	float m_lightConstantAttenuation = 1.0;
	float m_lightLinearAttenuation = 0.0;
	float m_lightQuadraticAttenuation = 0.0;

	// Material properties
	glm::vec3 m_Ka = glm::vec3(1.0);
	glm::vec3 m_Kd = glm::vec3(1.0);
	glm::vec3 m_Ks = glm::vec3(1.0);

	float m_Shininess = 1.0;

	// Shader initialization and cleanup
	void InitShaders();
	void CleanShaders();

	// Geometry-related variables
	OGLObject m_SkyboxGPU = {};
	OGLObject m_waterGPU = {};
	OGLObject m_quadGPU = {};

	// Geometry initialization and cleanup
	void InitGeometry();
	void CleanGeometry();
	void InitSkyboxGeometry();

	// Texturing-related variables
	GLuint m_SamplerID = 0;

	GLuint m_SkyboxTextureID = 0;
	GLuint m_waterTextureID = 0;
	GLuint m_buildingTextureID = 0;

	void InitTextures();
	void CleanTextures();
	void InitSkyboxTextures();

	void SetLightingUniforms(float Shininess, glm::vec3 Ka = glm::vec3(1.0), glm::vec3 Kd = glm::vec3(1.0), glm::vec3 Ks = glm::vec3(1.0));

	// Skybox
	glm::vec3 m_sunColor;
	glm::vec3 m_moonColor;
	glm::vec3 m_skyTopColor;
	glm::vec3 m_skyBottomColor;
	float m_timeOfDay = 0.0f;	 // 0-1 representing 24 hours
	float m_timeSpeed = 0.01f; // Speed of time progression

	void UpdateDayNightCycle(float deltaTime);
	float smoothstep(float edge0, float edge1, float x);

	// Terrain
	GLuint m_terrainVAO = 0;
	GLuint m_terrainVBO = 0;
	GLuint m_terrainIBO = 0;
	size_t m_terrainIndexCount = 0;

	// Textures
	GLuint m_heightmapTexture = 0;
	GLuint m_splatmapTexture = 0;
	GLuint m_groundTextures[4] = {0};
	GLuint m_rockTexture = 0;
	GLuint m_sandTexture = 0;
	GLuint m_snowTexture = 0;
	GLuint m_concreteTexture = 0;

	// Uniform locations
	GLint m_ulTerrainWorld = -1;
	GLint m_ulTerrainWorldIT = -1;
	GLint m_ulTerrainViewProj = -1;
	GLint m_ulTerrainHeightScale = -1;
	GLint m_ulTerrainTexScale = -1;

	// Terrain parameters
	float m_terrainVerticalOffset = 4.0f;
	float m_terrainHeightScale = 50.0f;
	float m_terrainTexScale = 10.0f;

	// Shader program
	GLuint m_terrainProgram = 0;

	void GenerateTerrain();
	void GenerateHeightmap();
	void GenerateSplatmap();
	void InitTerrainTextures();
	void RenderTerrain();
	void RenderBuildings();

	struct BuildingInstance
	{
		glm::vec3 position;
		BuildingType type;
		glm::vec3 color;
		std::vector<float> originalTerrainHeights; // Stores original terrain heights under building
	};

	std::vector<BuildingInstance> m_buildings;
	BuildingType m_selectedBuildingType = SMALL_HOUSE;
	glm::vec3 *m_pickData = nullptr; // For reading FBO data
	bool m_showBuildingPreview = true;
	glm::vec3 m_buildingPreviewPos;
	glm::vec3 m_buildingColor;

	// FBO related
	bool m_frameBufferCreated = false;
	GLuint m_depthBuffer;
	GLuint m_colorBuffer;
	GLuint m_frameBuffer;
	GLuint m_pickProgramID;

	void CreateFrameBuffer(int width, int height);
	void UpdateBuildingPreview(const glm::vec3 &pos);
	void PlaceBuilding(const glm::vec3 &pos);
	void GetViewportSize(int &width, int &height);
	float SampleHeightmap(const glm::vec2 &uv);
	void ApplyConcreteTexture(const glm::vec2 &centerUV, BuildingType buildingType);
	float SmoothTerrainUnderBuilding(const glm::vec2 &centerUV, const glm::vec2 &size);

	const float WATER_LEVEL = -0.8f;
};
