#include "Game/Game.h"
#include "Game/GameCommon.h"
#include "Game/App.h"
#include "Game/Player.hpp"

#include "Engine/Input/InputSystem.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Window/Window.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Rgba8.h"
#include "Engine/Core/Vertex_PCU.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/VertexUtils.h"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/OBJLoader.hpp"
#include "Engine/Math/MathUtils.h"
#include "Engine/Math/AABB3.hpp"

Game::Game(App* owner)
	: m_app(owner)
{
}

Game::~Game()
{
}

void Game::StartUp()
{
	// Load MetaData from XML
	LoadXMLMetaData("Data/Models/Woman.xml");
	std::string womanOBJFile = g_gameConfigBlackboard.GetValue("objFile", "");
	std::string phongShader = g_gameConfigBlackboard.GetValue("shader", "");
	std::string diffuseMap = g_gameConfigBlackboard.GetValue("diffuseMap", "");
	std::string normalMap = g_gameConfigBlackboard.GetValue("normalMap", "");
	float unitsPerMeter = g_gameConfigBlackboard.GetValue("unitsPerMeter", 0.f);
	std::string orientationX = g_gameConfigBlackboard.GetValue("x", "left");
	std::string orientationY = g_gameConfigBlackboard.GetValue("y", "up");
	std::string orientationZ = g_gameConfigBlackboard.GetValue("z", "forward");

	// Create and push back the entities
	m_player = new Player(this, Vec3(-1.f, 0.f, 0.5f));

	// Get Blinn Phong shader
	m_shader = g_theRenderer->CreateOrGetShader(phongShader.c_str(), VertexType::VERTEX_PCUTBN);

	// Get model Textures
	m_womanDiffuseTexture = g_theRenderer->CreateOrGetTextureFromFile(diffuseMap.c_str());
	m_womanNormalTexture = g_theRenderer->CreateOrGetTextureFromFile(normalMap.c_str());

	// Load the model
	bool loaded = LoadOBJMeshFile(m_modelMeshVerts, "Data/Models/cube_vni.obj");
	GUARANTEE_OR_DIE(loaded, "Failed to load cube_vni.obj!");

	bool wloaded = LoadOBJMeshFile(m_modelMeshVerts, womanOBJFile.c_str());
	GUARANTEE_OR_DIE(wloaded, "Failed to load Woman.obj!");

	// Scale and orient the model
	m_modelToWorldTransform.Append(Mat44::MakeUniformScale3D(unitsPerMeter));
	m_modelToWorldTransform.Append(ApplyOrientation(orientationX, orientationY, orientationZ));

	// Create buffers
	CreateBuffers();

	// Adding a plus crosshair with infinite duration
	DebugAddScreenText("+", AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), 20.f, Vec2::ONEHALF, -1.f);

	// Initialize the grid
	InitializeGrid();
}

void Game::CreateBuffers()
{
	// Create buffers and copy to GPU
	m_modelVBO = g_theRenderer->CreateVertexBuffer(static_cast<unsigned int>(m_modelMeshVerts.size()) * sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	//m_modelIBO = g_theRenderer->CreateIndexBuffer(static_cast<unsigned int>(m_modelMeshIndices.size()) * sizeof(unsigned int), sizeof(unsigned int));
	g_theRenderer->CopyCPUToGPU(m_modelMeshVerts.data(), m_modelVBO->GetSize(), m_modelVBO);
	//g_theRenderer->CopyCPUToGPU(m_modelMeshIndices.data(), m_modelIBO->GetSize(), m_modelIBO);
}

void Game::LoadXMLMetaData(char const* filePath)
{
	XmlDocument metaDataXML;
	XmlError result = metaDataXML.LoadFile(filePath);
	if (result == tinyxml2::XML_SUCCESS)
	{
		XmlElement* rootElement = metaDataXML.RootElement();
		if (rootElement)
		{
			g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*rootElement);
		}
		else
		{
			DebuggerPrintf("WARNING: MetaData file \"%s\" was invalid (missing root element)\n", filePath);
		}
	}
	else
	{
		DebuggerPrintf("WARNING: Failed to load MetaData from file \"%s\"\n", filePath);
	}
}

Mat44 Game::ApplyOrientation(std::string const& orientationX, std::string const& orientationY, std::string const& orientationZ)
{
	Mat44 orientationTransform = Mat44();
	Vec3 xAxis, yAxis, zAxis;

	if (orientationX == "left") 
	{
		xAxis = Vec3::YAXE;
	}
	else if (orientationX == "right") 
	{
		xAxis = -Vec3::YAXE;
	}

	if (orientationY == "up") 
	{
		yAxis = Vec3::ZAXE;  
	}
	else if (orientationY == "down") 
	{
		yAxis = -Vec3::ZAXE; 
	}

	if (orientationZ == "forward") 
	{
		zAxis = Vec3::XAXE;  
	}
	else if (orientationZ == "backward") 
	{
		zAxis = -Vec3::XAXE;
	}

	Mat44 orientationMatrix;
	orientationMatrix.m_values[Mat44::Ix] = xAxis.x;
	orientationMatrix.m_values[Mat44::Iy] = xAxis.y;
	orientationMatrix.m_values[Mat44::Iz] = xAxis.z;

	orientationMatrix.m_values[Mat44::Jx] = yAxis.x;
	orientationMatrix.m_values[Mat44::Jy] = yAxis.y;
	orientationMatrix.m_values[Mat44::Jz] = yAxis.z;

	orientationMatrix.m_values[Mat44::Kx] = zAxis.x;
	orientationMatrix.m_values[Mat44::Ky] = zAxis.y;
	orientationMatrix.m_values[Mat44::Kz] = zAxis.z;

	return orientationMatrix;
}

void Game::Update()
{
	// Setting clock time variables
	double deltaSeconds = m_gameClock.GetDeltaSeconds();

	// Set debug text
	std::string debugText = Stringf("Debug Mode [%d]: %s", m_debugInt, GetDebugRenderModeDesc(m_debugInt));
	DebugAddScreenText(debugText, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), 10.f, Vec2(0.0f, 0.97f), 0.f);
	g_theRenderer->SetPerFrameConstants(m_debugInt, 0.f);

	UpdatePlayer(static_cast<float>(deltaSeconds));

	AdjustForPauseAndTimeDistortion(static_cast<float>(deltaSeconds));
	KeyInputPresses();

	UpdateCameras();
}

void Game::Render() const
{
	if (m_isAttractMode == true)
	{
	}
	if (m_isAttractMode == false)
	{
		g_theRenderer->BeginCamera(m_player->GetPlayerCamera());
		g_theRenderer->ClearScreen(Rgba8(70, 70, 70, 255));
		RenderGrid();
		RenderModel();
		g_theRenderer->EndCamera(m_player->GetPlayerCamera());

		DebugRenderWorld(m_player->GetPlayerCamera());
		DebugRenderScreen(m_screenCamera);
	}
}

void Game::Shutdown()
{
	delete m_player;
	m_player = nullptr;

	delete m_modelVBO;
	m_modelVBO = nullptr;
}

void Game::InitializeGrid()
{
	// Layout
	for (int gridIndex = 0; gridIndex < 100; ++gridIndex)
	{
		AddVertsForAABB3D(m_gridVerts, AABB3(-50.f, -50.01f + gridIndex, -0.005f, 50.f, -49.99f + gridIndex, 0.005f), Rgba8::DARKGRAY);
		AddVertsForAABB3D(m_gridVerts, AABB3(-50.01f + gridIndex, -50.0f, -0.005f, -49.99f + gridIndex, 50.f, 0.005f), Rgba8::DARKGRAY);
	}

	// Y axis
	for (int x = 0; x < 105; x += 5)
	{
		if (x == 50)
		{
			AddVertsForAABB3D(m_gridVerts, AABB3(-50.05f + x, -50.f, -0.05f, -49.95f + x, 50.f, 0.05f), Rgba8::GREEN);
		}
		else
		{
			AddVertsForAABB3D(m_gridVerts, AABB3(-50.05f + x, -50.f, -0.05f, -49.95f + x, 50.f, 0.05f), Rgba8::SEAWEED);
		}
	}

	// X axis
	for (int y = 0; y < 105; y += 5)
	{
		if (y == 50)
		{
			AddVertsForAABB3D(m_gridVerts, AABB3(-50.f, -50.05f + y, -0.05f, 50.f, -49.95f + y, 0.05f), Rgba8::RED);
		}
		else
		{
			AddVertsForAABB3D(m_gridVerts, AABB3(-50.f, -50.05f + y, -0.05f, 50.f, -49.95f + y, 0.05f), Rgba8::DARKRED);
		}
	}
}

void Game::KeyInputPresses()
{
	// Attract Mode
	if (g_theInput->WasKeyJustPressed(' '))
	{
		m_isAttractMode = false;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
	{
		m_isAttractMode = true;
	}

	// Debug Visualization Keys
	DebugVisuals();
}

void Game::AdjustForPauseAndTimeDistortion(float deltaSeconds) {

	UNUSED(deltaSeconds);

	if (g_theInput->IsKeyDown('T'))
	{
		m_gameClock.SetTimeScale(0.1);
	}
	else
	{
		m_gameClock.SetTimeScale(1.0);
	}

	if (g_theInput->WasKeyJustPressed('P'))
	{
		m_gameClock.TogglePause();
	}

	if (g_theInput->WasKeyJustPressed('O'))
	{
		m_gameClock.StepSingleFrame();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) && m_isAttractMode)
	{
		g_theEventSystem->FireEvent("Quit");
	}
}

void Game::UpdateCameras()
{
	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void Game::UpdatePlayer(float deltaSeconds)
{
	if (m_player != nullptr)
	{
		m_player->Update(deltaSeconds);
	}
}

void Game::RenderGrid() const
{
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray(m_gridVerts);
}

void Game::RenderModel() const
{
	if (!m_modelVBO)
	{
		return;
	}

	g_theRenderer->SetModelConstants(m_modelToWorldTransform);
	g_theRenderer->SetLightingConstants(m_sunDirection, m_sunIntensity, m_ambientIntensity);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindSampler(SamplerMode::POINT_CLAMP, 0);
	g_theRenderer->BindSampler(SamplerMode::BILINEAR_WRAP, 1);
	g_theRenderer->BindSampler(SamplerMode::BILINEAR_WRAP, 2);
	g_theRenderer->BindTexture(m_womanDiffuseTexture, 0);
	g_theRenderer->BindTexture(m_womanNormalTexture, 1);
	g_theRenderer->BindShader(m_shader);
	g_theRenderer->DrawVertexArray(m_modelMeshVerts);
}

void Game::DebugVisuals()
{
	if (g_theInput->WasKeyJustPressed('0'))
	{
		m_debugInt = 0;
	}
	if (g_theInput->WasKeyJustPressed('1'))
	{
		m_debugInt = 1;
	}
	if (g_theInput->WasKeyJustPressed('2'))
	{
		m_debugInt = 2;
	}
	if (g_theInput->WasKeyJustPressed('3'))
	{
		m_debugInt = 3;
	}
	if (g_theInput->WasKeyJustPressed('7'))
	{
		m_debugInt = 7;
	}
	if (g_theInput->WasKeyJustPressed('8'))
	{
		m_debugInt = 8;
	}
	if (g_theInput->WasKeyJustPressed('9'))
	{
		m_debugInt = 9;
	}

	if (g_theInput->WasKeyJustPressed('K'))
	{
		m_debugInt = 10;
	}
	if (g_theInput->WasKeyJustPressed('L'))
	{
		m_debugInt = 11;
	}

	// Debug Tangent/Bitangent/Normal
	if (g_theInput->WasKeyJustPressed('T'))
	{
		m_debugInt = 4;
	}
	if (g_theInput->WasKeyJustPressed('B'))
	{
		m_debugInt = 5;
	}
	if (g_theInput->WasKeyJustPressed('N'))
	{
		m_debugInt = 6;
	}

	// Debug Specular/Glossiness/Emissive
	// Need to enable Nums Lock to use numpad keys
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUMPAD0))
	{
		m_debugInt = 14;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUMPAD1))
	{
		m_debugInt = 15;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUMPAD2))
	{
		m_debugInt = 16;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUMPAD3))
	{
		m_debugInt = 17;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUMPAD4))
	{
		m_debugInt = 18;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUMPAD5))
	{
		m_debugInt = 19;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_NUMPAD6))
	{
		m_debugInt = 20;
	}
}
