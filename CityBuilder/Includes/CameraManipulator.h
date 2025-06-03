#pragma once

#include <glm/glm.hpp>

class Camera;

struct SDL_KeyboardEvent;
struct SDL_MouseMotionEvent;
struct SDL_MouseWheelEvent;

class CameraManipulator
{
public:
	CameraManipulator();

	~CameraManipulator();

	void SetCamera(Camera *_pCamera);
	void Update(float _deltaTime);

	inline void SetSpeed(float _speed) { m_speed = _speed; }
	inline float GetSpeed() const noexcept { return m_speed; }

	void KeyboardDown(const SDL_KeyboardEvent &key);
	void KeyboardUp(const SDL_KeyboardEvent &key);
	void MouseMove(const SDL_MouseMotionEvent &mouse);
	void MouseWheel(const SDL_MouseWheelEvent &wheel);

private:
	Camera *m_pCamera = nullptr;

	// The u coordinate is part of the spherical coordinate pair (u,v),
	// which defines the viewing direction from the m_eye position
	float m_u = 0.0f;

	// The v coordinate is part of the spherical coordinate pair (u,v),
	// which defines the viewing direction from the m_eye position
	float m_v = 0.0f;

	// The distance between the target position and the camera position
	float m_distance = 0.0f;

	// The center point of the circular model
	glm::vec3 m_center = glm::vec3(0.0f);

	// The world's up vector for the camera
	glm::vec3 m_worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

	// The speed of the camera movement
	float m_speed = 16.0f;

	// Movement indicators for different directions
	float m_goForward = 0.0f;
	float m_goRight = 0.0f;
	float m_goUp = 0.0f;
};
