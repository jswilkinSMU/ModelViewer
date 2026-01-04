#pragma once
#include "Game/GameCommon.h"
#include "Engine/Renderer/Camera.h"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Vertex_PCU.h"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include <string>
// -----------------------------------------------------------------------------
class Player;
class VertexBuffer;
class IndexBuffer;
class Shader;
class Texture;
// -----------------------------------------------------------------------------
class Game
{
public:
	App* m_app;
	Game(App* owner);
	~Game();
	void StartUp();
	void CreateBuffers();
	void LoadXMLMetaData(char const* filePath);

	Mat44 ApplyOrientation(std::string const& orientationX, std::string const& orientationY, std::string const& orientationZ);

	void Update();
	void UpdateCameras();
	void UpdatePlayer(float deltaSeconds);

	void Render() const;
	void RenderGrid() const;
	void RenderModel() const;
	void DebugVisuals();

	void Shutdown();

	void InitializeGrid();
	void KeyInputPresses();
	void AdjustForPauseAndTimeDistortion(float deltaSeconds);
	bool		m_isAttractMode = true;

private:
	Camera		m_screenCamera;
	Camera      m_gameWorldCamera;
	Clock		m_gameClock;
	Mat44		m_modelToWorldTransform = Mat44();

	Player* m_player = nullptr;
	Shader* m_shader = nullptr;
	int m_debugInt = 0;
	Vec3 m_sunDirection = Vec3(3.f, 1.f, -2.f);
	float m_sunIntensity = 0.35f;
	float m_ambientIntensity = 0.25f;

	std::vector<Vertex_PCU> m_gridVerts;

	// Model Loading
	std::vector<Vertex_PCUTBN> m_modelMeshVerts;
	std::vector<unsigned int>  m_modelMeshIndices;
	VertexBuffer* m_modelVBO = nullptr;
	IndexBuffer* m_modelIBO = nullptr;
	Texture* m_womanDiffuseTexture = nullptr;
	Texture* m_womanNormalTexture = nullptr;
};