	#include "MyApp.h"
	#include "SDL_GLDebugMessageCallback.h"
	#include "ObjParser.h"
	#include "ParametricSurfaceMesh.hpp"
	#include "ProgramBuilder.h"

	#include <imgui.h>

	#include <string>
	#include <array>
	#include <algorithm>

	CMyApp::CMyApp()
	{
	}

	CMyApp::~CMyApp()
	{
	}

	void CMyApp::SetupDebugCallback()
	{
		// engedélyezzük és állítsuk be a debug callback függvényt ha debug context-ben vagyunk 
		GLint context_flags;
		glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
		if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
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
		ProgramBuilder{ m_programID }
			.ShaderStage(GL_VERTEX_SHADER, "Shaders/Vert_PosNormTex.vert")
			.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/Frag_LightingNoFaceCull.frag")
			.Link();

		m_programWaterID = glCreateProgram();
		ProgramBuilder{ m_programWaterID }
			.ShaderStage(GL_VERTEX_SHADER, "Shaders/Vert_Water.vert")
			.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/Frag_Water.frag")
			.Link();

		InitSkyboxShaders();
	}

	void CMyApp::InitSkyboxShaders()
	{
		m_programSkyboxID = glCreateProgram();
		ProgramBuilder{ m_programSkyboxID }
			.ShaderStage(GL_VERTEX_SHADER, "Shaders/Vert_Skybox.vert")
			.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/Frag_Skybox.frag")
			.Link();
	}

	void CMyApp::CleanShaders()
	{
		glDeleteProgram(m_programID);
		glDeleteProgram(m_programWaterID);

		CleanSkyboxShaders();
	}

	void CMyApp::CleanSkyboxShaders()
	{
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
			{ 0, offsetof(Vertex, position), 3, GL_FLOAT },
			{ 1, offsetof(Vertex, normal), 3, GL_FLOAT },
			{ 2, offsetof(Vertex, texcoord), 2, GL_FLOAT },
		};

		// quad
		MeshObject<Vertex> quadMeshCPU;
		quadMeshCPU.vertexArray =
		{
			{ glm::vec3(-1.0, -1.0, 0.0), glm::vec3(0.0, 0.0,  1.0), glm::vec2(0.0,0.0) }, // első lap
			{ glm::vec3(1.0, -1.0, 0.0), glm::vec3(0.0, 0.0,  1.0), glm::vec2(1.0,0.0) },
			{ glm::vec3(1.0,  1.0, 0.0), glm::vec3(0.0, 0.0,  1.0), glm::vec2(1.0,1.0) },
			{ glm::vec3(-1.0,  1.0, 0.0), glm::vec3(0.0, 0.0,  1.0), glm::vec2(0.0,1.0) }
		};

		quadMeshCPU.indexArray =
		{
			0, 1, 2, // első lap
			2, 3, 0
		};

		m_quadGPU = CreateGLObjectFromMesh(quadMeshCPU, vertexAttribList);

		// Skybox
		InitSkyboxGeometry();

		// Water
		MeshObject<glm::vec2> waterCPU;
		{
			MeshObject<Vertex> surfaceMeshCPU = GetParamSurfMesh(Param(), 160, 80);
			for (const Vertex& v : surfaceMeshCPU.vertexArray)
			{
				waterCPU.vertexArray.emplace_back(glm::vec2(v.position.x, v.position.y));
			}
			waterCPU.indexArray = surfaceMeshCPU.indexArray;
		}
		m_waterGPU = CreateGLObjectFromMesh(waterCPU, { { 0, offsetof(glm::vec2,x), 2, GL_FLOAT} });
	}

	void CMyApp::CleanGeometry()
	{
		CleanSkyboxGeometry();
	}

	void CMyApp::InitSkyboxGeometry()
	{
		// skybox geo
		MeshObject<glm::vec3> skyboxCPU =
		{
			std::vector<glm::vec3>
			{
			// hátsó lap
			glm::vec3(-1, -1, -1),
			glm::vec3(1, -1, -1),
			glm::vec3(1,  1, -1),
			glm::vec3(-1,  1, -1),
				// elülső lap
				glm::vec3(-1, -1, 1),
				glm::vec3(1, -1, 1),
				glm::vec3(1,  1, 1),
				glm::vec3(-1,  1, 1),
			},

			std::vector<GLuint>
			{
			// hátsó lap
			0, 1, 2,
			2, 3, 0,
				// elülső lap
				4, 6, 5,
				6, 4, 7,
				// bal
				0, 3, 4,
				4, 3, 7,
				// jobb
				1, 5, 2,
				5, 6, 2,
				// alsó
				1, 0, 4,
				1, 4, 5,
				// felső
				3, 2, 6,
				3, 6, 7,
			}
		};

		m_SkyboxGPU = CreateGLObjectFromMesh(skyboxCPU, { { 0, offsetof(glm::vec3,x), 3, GL_FLOAT } });
	}

	void CMyApp::CleanSkyboxGeometry()
	{
		CleanOGLObject(m_SkyboxGPU);
	}

	void CMyApp::InitTextures()
	{
		glCreateSamplers(1, &m_SamplerID);
		glSamplerParameteri(m_SamplerID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(m_SamplerID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(m_SamplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glSamplerParameteri(m_SamplerID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Water texture only
		ImageRGBA waterImage = ImageFromFile("Assets/water_texture.png");
		glCreateTextures(GL_TEXTURE_2D, 1, &m_waterTextureID);
		glTextureStorage2D(m_waterTextureID, NumberOfMIPLevels(waterImage), GL_RGBA8, waterImage.width, waterImage.height);
		glTextureSubImage2D(m_waterTextureID, 0, 0, 0, waterImage.width, waterImage.height, GL_RGBA, GL_UNSIGNED_BYTE, waterImage.data());
		glGenerateTextureMipmap(m_waterTextureID);

		InitSkyboxTextures();
	}

	void CMyApp::CleanTextures()
	{
		glDeleteTextures(1, &m_waterTextureID);

		CleanSkyboxTextures();

		glDeleteSamplers(1, &m_SamplerID);
	}

	void CMyApp::InitSkyboxTextures()
	{

		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_SkyboxTextureID);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	}

	void CMyApp::CleanSkyboxTextures()
	{
		glDeleteTextures(1, &m_SkyboxTextureID);
	}

	bool CMyApp::Init()
	{
		SetupDebugCallback();

		// törlési szín legyen kékes
		glClearColor(0.125f, 0.25f, 0.5f, 1.0f);

		InitShaders();
		InitGeometry();
		InitTextures();

		// egyéb inicializálás

		glEnable(GL_CULL_FACE); // kapcsoljuk be a hátrafelé néző lapok eldobását
		glCullFace(GL_BACK);    // GL_BACK: a kamerától "elfelé" néző lapok, GL_FRONT: a kamera felé néző lapok

		glEnable(GL_DEPTH_TEST); // mélységi teszt bekapcsolása (takarás)

		// kamera
		m_camera.SetView(
			glm::vec3(0.0, 7.0, 7.0),	// honnan nézzük a színteret	   - eye
			glm::vec3(0.0, 0.0, 0.0),   // a színtér melyik pontját nézzük - at
			glm::vec3(0.0, 1.0, 0.0));  // felfelé mutató irány a világban - up

		m_cameraManipulator.SetCamera(&m_camera);

		m_sunColor = glm::vec3(1.0f, 1.0f, 0.8f);
		m_moonColor = glm::vec3(0.7f, 0.7f, 0.8f);
		m_skyTopColor = glm::vec3(0.5f, 0.7f, 1.0f);
		m_skyBottomColor = glm::vec3(0.9f, 0.9f, 1.0f);

		m_lightPosition = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f); // Directional light initially pointing down

		m_moonLightPosition = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f); // Initially pointing down
		m_moonLa = glm::vec3(0.05f, 0.05f, 0.1f);
		m_moonLd = glm::vec3(0.2f, 0.2f, 0.3f);
		m_moonLs = glm::vec3(0.3f, 0.3f, 0.4f);

		return true;
	}

	void CMyApp::Clean()
	{
		CleanShaders();
		CleanGeometry();
		CleanTextures();
	}

	void CMyApp::Update(const SUpdateInfo& updateInfo)
	{
		m_ElapsedTimeInSec = updateInfo.ElapsedTimeInSec;

		UpdateDayNightCycle(updateInfo.DeltaTimeInSec);

		m_cameraManipulator.Update(updateInfo.DeltaTimeInSec);

		m_waterWorldTransform = glm::translate(glm::vec3(0.0f, -2.0f, 0.0f));
	}

void CMyApp::SetLightingUniforms(float Shininess, glm::vec3 Ka, glm::vec3 Kd, glm::vec3 Ks)
{
    // - Fényforrások beállítása
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

    // - Anyagjellemzők beállítása
    glUniform3fv(ul("Ka"), 1, glm::value_ptr(Ka));
    glUniform3fv(ul("Kd"), 1, glm::value_ptr(Kd));
    glUniform3fv(ul("Ks"), 1, glm::value_ptr(Ks));

    glUniform1f(ul("Shininess"), Shininess);
}

	void CMyApp::Render()
	{
		// töröljük a frampuffert (GL_COLOR_BUFFER_BIT)...
		// ... és a mélységi Z puffert (GL_DEPTH_BUFFER_BIT)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// =========== SKYBOX ===========
		GLint prevDepthFnc;
		glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFnc);
		glDepthFunc(GL_LEQUAL);

		glUseProgram(m_programSkyboxID);

		glUniformMatrix4fv(ul("viewProj"), 1, GL_FALSE, glm::value_ptr(m_camera.GetViewProj()));
		glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(glm::translate(m_camera.GetEye())));

		// Pass day-night cycle parameters - these will be used in the skybox fragment shader
		glUniform3fv(ul("sunDirection"), 1, glm::value_ptr(glm::normalize(glm::vec3(m_lightPosition))));
		glUniform3fv(ul("moonDirection"), 1, glm::value_ptr(-glm::normalize(glm::vec3(m_lightPosition))));
		glUniform3fv(ul("sunColor"), 1, glm::value_ptr(m_sunColor));
		glUniform3fv(ul("moonColor"), 1, glm::value_ptr(m_moonColor));
		glUniform3fv(ul("skyTopColor"), 1, glm::value_ptr(m_skyTopColor));
		glUniform3fv(ul("skyBottomColor"), 1, glm::value_ptr(m_skyBottomColor));
		glUniform1f(ul("timeOfDay"), m_timeOfDay); // Pass the current time of day

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

		glUniform1f(ul("Alpha"), 1.0f); // 0.7 = 70% opacity

		glUniform1f(ul("ElapsedTimeInSec"), m_ElapsedTimeInSec);

		glBindTextureUnit(0, m_waterTextureID);
		glBindSampler(0, m_SamplerID);

		glBindVertexArray(m_waterGPU.vaoID);
		glDrawElements(GL_TRIANGLES, m_waterGPU.count, GL_UNSIGNED_INT, nullptr);

		glDisable(GL_BLEND);
		glEnable(GL_CULL_FACE);

		// ===========================

		// shader kikapcsolasa
		glUseProgram(0);

		// - Textúrák kikapcsolása, minden egységre külön
		glBindTextureUnit(0, 0);
		glBindSampler(0, 0);

		// VAO kikapcsolása
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
	}

	// https://wiki.libsdl.org/SDL2/SDL_KeyboardEvent
	// https://wiki.libsdl.org/SDL2/SDL_Keysym
	// https://wiki.libsdl.org/SDL2/SDL_Keycode
	// https://wiki.libsdl.org/SDL2/SDL_Keymod

	void CMyApp::KeyboardDown(const SDL_KeyboardEvent& key)
	{
		if (key.repeat == 0) // Először lett megnyomva
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
				glGetIntegerv(GL_POLYGON_MODE, polygonModeFrontAndBack); // Kérdezzük le a jelenlegi polygon módot! Külön adja a front és back módokat.
				GLenum polygonMode = (polygonModeFrontAndBack[0] != GL_FILL ? GL_FILL : GL_LINE); // Váltogassuk FILL és LINE között!
				// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glPolygonMode.xhtml
				glPolygonMode(GL_FRONT_AND_BACK, polygonMode); // Állítsuk be az újat!
			}
		}
		m_cameraManipulator.KeyboardDown(key);
	}

	void CMyApp::KeyboardUp(const SDL_KeyboardEvent& key)
	{
		m_cameraManipulator.KeyboardUp(key);
	}

	// https://wiki.libsdl.org/SDL2/SDL_MouseMotionEvent

	void CMyApp::MouseMove(const SDL_MouseMotionEvent& mouse)
	{
		m_cameraManipulator.MouseMove(mouse);
	}

	// https://wiki.libsdl.org/SDL2/SDL_MouseButtonEvent

	void CMyApp::MouseDown(const SDL_MouseButtonEvent& mouse)
	{
	}

	void CMyApp::MouseUp(const SDL_MouseButtonEvent& mouse)
	{
	}

	// https://wiki.libsdl.org/SDL2/SDL_MouseWheelEvent

	void CMyApp::MouseWheel(const SDL_MouseWheelEvent& wheel)
	{
		m_cameraManipulator.MouseWheel(wheel);
	}


	// a két paraméterben az új ablakméret szélessége (_w) és magassága (_h) található
	void CMyApp::Resize(int _w, int _h)
	{
		glViewport(0, 0, _w, _h);
		m_camera.SetAspect(static_cast<float>(_w) / _h);
	}

	// Le nem kezelt, egzotikus esemény kezelése
	// https://wiki.libsdl.org/SDL2/SDL_Event

	void CMyApp::OtherEvent(const SDL_Event& ev)
	{

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
	const glm::vec3 sunriseColor1(1.0f, 0.2f, 0.0f);   // Deep red
	const glm::vec3 sunriseColor2(1.0f, 0.5f, 0.1f);   // Bright orange
	const glm::vec3 daylightColor1(1.0f, 0.8f, 0.5f);  // Warm white
	const glm::vec3 daylightColor2(1.0f, 1.0f, 1.0f);  // Pure white

	// Smooth sun color transition using smoothstep interpolation
	float sunColorBlend1 = glm::smoothstep(0.0f, 0.1f, sunHeight);
	float sunColorBlend2 = glm::smoothstep(0.1f, 0.3f, sunHeight);
	float sunColorBlend3 = glm::smoothstep(0.3f, 0.7f, sunHeight);

	m_sunColor = nightColor;
	if (sunDir.y > -0.2f) {
		m_sunColor = glm::mix(m_sunColor, sunriseColor1, sunColorBlend1);
		m_sunColor = glm::mix(m_sunColor, sunriseColor2, sunColorBlend2);
		m_sunColor = glm::mix(m_sunColor, daylightColor1, sunColorBlend3);
		m_sunColor = glm::mix(m_sunColor, daylightColor2, glm::smoothstep(0.7f, 1.0f, sunHeight));
	}

	// Moon position (opposite the sun)
	float moonAngle = sunAngle + glm::pi<float>();
	glm::vec3 moonDir = glm::vec3(cosf(moonAngle), sinf(moonAngle), 0.0f);
	m_moonLightPosition = glm::vec4(moonDir, 0.0f);

	// Moon color - smooth gradient
	float moonHeight = (moonDir.y + 1.0f) * 0.5f;
	float moonBlend1 = glm::smoothstep(0.0f, 0.25f, moonHeight);
	float moonBlend2 = glm::smoothstep(0.25f, 0.5f, moonHeight);
	float moonBlend3 = glm::smoothstep(0.5f, 0.75f, moonHeight);

	m_moonColor = glm::vec3(0.2f, 0.2f, 0.5f); // Base night color
	if (moonDir.y > -0.1f) {
		m_moonColor = glm::mix(m_moonColor, glm::vec3(0.5f, 0.5f, 0.8f), moonBlend1);
		m_moonColor = glm::mix(m_moonColor, glm::vec3(0.8f, 0.8f, 0.9f), moonBlend2);
		m_moonColor = glm::mix(m_moonColor, glm::vec3(0.9f, 0.9f, 1.0f), moonBlend3);
	}

	// Sky colors - completely smooth transition
	const glm::vec3 nightTop(0.02f, 0.02f, 0.1f);
	const glm::vec3 nightBottom(0.0f, 0.0f, 0.05f);
	const glm::vec3 sunriseTop1(0.3f, 0.1f, 0.4f);    // Purple dawn
	const glm::vec3 sunriseTop2(0.8f, 0.3f, 0.2f);    // Red sunrise
	const glm::vec3 dayTop(0.3f, 0.5f, 1.0f);         // Day blue
	const glm::vec3 sunriseBottom1(0.1f, 0.05f, 0.15f); // Dark purple
	const glm::vec3 sunriseBottom2(1.0f, 0.5f, 0.2f);  // Orange sunrise
	const glm::vec3 dayBottom(0.6f, 0.8f, 1.0f);       // Day light blue

	// Calculate smooth blending factors
	float horizonEffect = glm::smoothstep(-0.3f, 0.1f, sunDir.y);
	float daylightEffect = glm::smoothstep(-0.1f, 0.2f, sunDir.y);

	// Sky top - smooth transition through all phases
	if (sunDir.y < 0.1f) {
		// Night to sunrise transition
		float dawnBlend = glm::smoothstep(-0.3f, -0.1f, sunDir.y);
		float sunriseBlend = glm::smoothstep(-0.1f, 0.1f, sunDir.y);

		m_skyTopColor = nightTop;
		m_skyTopColor = glm::mix(m_skyTopColor, sunriseTop1, dawnBlend);
		m_skyTopColor = glm::mix(m_skyTopColor, sunriseTop2, sunriseBlend);
		m_skyTopColor = glm::mix(m_skyTopColor, dayTop, daylightEffect);
	}
	else {
		// Normal day transition
		m_skyTopColor = glm::mix(sunriseTop2, dayTop, daylightEffect);
	}

	// Sky bottom - smooth transition through all phases
	if (sunDir.y < 0.1f) {
		// Night to sunrise transition
		float dawnBlend = glm::smoothstep(-0.3f, -0.1f, sunDir.y);
		float sunriseBlend = glm::smoothstep(-0.1f, 0.1f, sunDir.y);

		m_skyBottomColor = nightBottom;
		m_skyBottomColor = glm::mix(m_skyBottomColor, sunriseBottom1, dawnBlend);
		m_skyBottomColor = glm::mix(m_skyBottomColor, sunriseBottom2, sunriseBlend);
		m_skyBottomColor = glm::mix(m_skyBottomColor, dayBottom, daylightEffect);
	}
	else {
		// Normal day transition
		m_skyBottomColor = glm::mix(sunriseBottom2, dayBottom, daylightEffect);
	}

	// Calculate intensities based on height above horizon
	float sunIntensity = (sunDir.y > -0.1f) ? pow(glm::clamp(sunDir.y, 0.0f, 1.0f), 1.5f) : 0.0f;
	float moonIntensity = (moonDir.y > -0.1f) ? pow(glm::clamp(moonDir.y, 0.0f, 1.0f), 1.5f) : 0.0f;

	// Lighting parameters
	m_La = m_sunColor * (0.1f + 0.1f * sunIntensity) +
		m_moonColor * (0.02f + 0.03f * moonIntensity);

	m_Ld = m_sunColor * (0.3f + 0.5f * sunIntensity) +
		m_moonColor * (0.1f + 0.1f * moonIntensity);

	m_Ls = m_sunColor * (0.5f + 0.5f * sunIntensity) +
		m_moonColor * (0.2f + 0.1f * moonIntensity);

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