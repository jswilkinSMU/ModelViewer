#include "Game/Player.hpp"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Math/MathUtils.h"
#include <Engine/Core/DevConsole.hpp>
#include <Engine/Core/DebugRender.hpp>

Player::Player(Game* owner, Vec3 const& position)
	:m_theGame(owner),
	 m_position(position)
{
	m_position = position;
	m_orientation = EulerAngles(0.f, 0.f, 0.f);
	Mat44 cameraToRender(Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(0.f, 0.f, 0.f));
	m_playerCamera.SetCameraToRenderTransform(cameraToRender);
}

Player::~Player()
{
}

void Player::Update(float deltaSeconds)
{
	CameraKeyPresses(deltaSeconds);
	CameraControllerPresses(deltaSeconds);

	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.f, 85.f);
	m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.f, 45.f);

	m_playerCamera.SetPositionAndOrientation(m_position, m_orientation);

	m_playerCamera.SetPerspectiveView(2.f, 60.f, 0.1f, 300.f);
}

void Player::Render() const
{
}

Vec3 Player::GetForwardNormal() const
{
	return Vec3::MakeFromPolarDegrees(m_orientation.m_pitchDegrees, m_orientation.m_yawDegrees, 2.f);
}

Camera Player::GetPlayerCamera() const
{
	return m_playerCamera;
}

Mat44 Player::GetModelToWorldTransform() const
{
	Mat44 modelToWorldMatrix;
	modelToWorldMatrix.SetTranslation3D(m_position);
	EulerAngles orientation;
	modelToWorldMatrix.Append(orientation.GetAsMatrix_IFwd_JLeft_KUp());
	return modelToWorldMatrix;
}

void Player::CameraKeyPresses(float deltaSeconds)
{
	// Yaw and Pitch with mouse
	m_orientation.m_yawDegrees += 0.08f * g_theInput->GetCursorClientDelta().x;
	m_orientation.m_pitchDegrees -= 0.08f * g_theInput->GetCursorClientDelta().y;

	float movementSpeed = 2.f;
	// Increase speed by a factor of 10
	if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
	{
		movementSpeed *= 10.f;
	}

	// Move left or right
	if (g_theInput->IsKeyDown('A'))
	{
		m_position += movementSpeed * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetJBasis3D() * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('D'))
	{
		m_position += -movementSpeed * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetJBasis3D() * deltaSeconds;
	}

	// Move Forward and Backward
	if (g_theInput->IsKeyDown('W'))
	{
		m_position += movementSpeed * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D() * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('S'))
	{
		m_position += -movementSpeed * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D() * deltaSeconds;
	}

	// Move Up and Down
	if (g_theInput->IsKeyDown('Z'))
	{
		m_position += -movementSpeed * Vec3::ZAXE * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('C'))
	{
		m_position += movementSpeed * Vec3::ZAXE * deltaSeconds;
	}

	// Reset position and orientation to zero
	if (g_theInput->WasKeyJustPressed('H'))
	{
		m_position = Vec3::ZERO;
		m_orientation = EulerAngles(0.f, 0.f, 0.f);
	}
}

void Player::CameraControllerPresses(float deltaSeconds)
{
	XboxController const& controller = g_theInput->GetController(0);
	float movementSpeed = 2.f;

	// Increase speed by a factor of 10
	if (controller.IsButtonDown(XBOX_BUTTON_A))
	{
		movementSpeed *= 10.f;
	}

	// Rolling
	if (controller.GetLeftTrigger())
	{
		m_orientation.m_rollDegrees = -90.f * deltaSeconds;
	}
	if (controller.GetRightTrigger())
	{
		m_orientation.m_rollDegrees = 90.f * deltaSeconds;
	}

	//// Move left, right, forward, and backward
	if (controller.GetLeftStick().GetMagnitude() > 0.f)
	{
		m_position += (-movementSpeed * controller.GetLeftStick().GetPosition().x * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetJBasis3D() * deltaSeconds);
		m_position += (movementSpeed * controller.GetLeftStick().GetPosition().y * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D() * deltaSeconds);
	}

	//// Move Up and Down
	if (controller.IsButtonDown(XBOX_BUTTON_LSHOULDER))
	{
		m_position += -movementSpeed * Vec3::ZAXE * deltaSeconds;
	}
	if (controller.IsButtonDown(XBOX_BUTTON_RSHOULDER))
	{
		m_position += movementSpeed * Vec3::ZAXE * deltaSeconds;
	}

	//// Reset position and orientation to zero
	if (controller.WasButtonJustPressed(XBOX_BUTTON_START))
	{
		m_position = Vec3::ZERO;
		m_orientation = EulerAngles(0.f, 0.f, 0.f);
	}
}
