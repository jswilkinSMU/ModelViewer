#pragma once
#include "Engine/Renderer/Camera.h"
// -----------------------------------------------------------------------------
class Game;
// -----------------------------------------------------------------------------
class Player
{
public:
	Player(Game* owner, Vec3 const& position);
	~Player();

	void Update(float deltaSeconds);
	void Render() const;
	Vec3 GetForwardNormal() const;

	Camera GetPlayerCamera() const;
	Mat44  GetModelToWorldTransform() const;

	Vec3 m_position = Vec3::ZERO;
	EulerAngles m_orientation = EulerAngles::ZERO;

private:
	void CameraKeyPresses(float deltaSeconds);
	void CameraControllerPresses(float deltaSeconds);
	Camera m_playerCamera;
	Game* m_theGame = nullptr;
};