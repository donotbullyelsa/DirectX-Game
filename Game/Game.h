//
// Game.h
//

#pragma once

#include "StepTimer.h"
#include <list>
#include "CommonStates.h"
#include "../DirectXTK/Inc/Effects.h" //this clashes with a SDK file so must explitily state it
#include "Keyboard.h"
#include "Mouse.h"
#include "Audio.h"
#include "CMOGO.h"
#include "Collectable.h"
#include "Player.h"
#include "Targets.h"
#include "TextGO2D.h"

using std::list;

// Forward declarations
struct GameData;
struct DrawData;
struct DrawData2D;
class GameObject;
class GameObject2D;
class Camera;
class TPSCamera;
class Light;
class Sound;

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game
{
public:

    Game() noexcept;
    ~Game() = default;

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void Initialize(HWND _window, int _width, int _height);

    // Basic game loop
    void Tick();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged(int _width, int _height);

    // Properties
    void GetDefaultSize( int& _width, int& _height ) const noexcept;

private:

    int winX;
    int winY;

    void Update(DX::StepTimer const& _timer);
    void Render();
    void AddToDraw();

    void Clear();
    void Present();

    void CreateDevice();
    void CreateResources();

    void OnDeviceLost();

    // Device resources.
    HWND                                            m_window;
    int                                             m_outputWidth;
    int                                             m_outputHeight;

    D3D_FEATURE_LEVEL                               m_featureLevel;
    Microsoft::WRL::ComPtr<ID3D11Device1>           m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_d3dContext;

    Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;

    // Rendering loop timer.
    DX::StepTimer                                   m_timer;

    //Scarle Added stuff
    GameData* m_GD = NULL;			//Data to be shared to all Game Objects as they are ticked
    DrawData* m_DD = NULL;			//Data to be shared to all 3D Game Objects as they are drawn
    DrawData2D* m_DD2D = NULL;	    //Data to be passed by game to all 2D Game Objects via Draw 

    //Basic 3D renderers
    std::shared_ptr <TPSCamera> m_cam = NULL; //principle camera
    std::shared_ptr <TPSCamera> m_TPScam = NULL;//TPS cam
    std::shared_ptr <Light> m_light = NULL; //base light

    //required for the CMO model rendering system
    DirectX::CommonStates* m_states = NULL;
    DirectX::IEffectFactory* m_fxFactory = NULL;

    //basic keyboard and mouse input system
    void ReadInput(); //Get current Mouse and Keyboard states
    std::unique_ptr<DirectX::Keyboard> m_keyboard;
    std::unique_ptr<DirectX::Mouse> m_mouse;

    list<std::weak_ptr<GameObject>> m_GameObjects; //data structure to hold pointers to the 3D Game Objects
    list<std::shared_ptr<GameObject2D>> m_GameObjects2D; //data structure to hold pointers to the 2D Game Objects 
    list<std::weak_ptr<GameObject2D>> m_MenuObjects2D;
    std::shared_ptr<TextGO2D> title_text;
    void Draw2D(list<std::weak_ptr<GameObject2D>> list_of_GO2D);

    std::vector<std::shared_ptr<CMOGO>> m_ColliderObjects;
    std::vector<std::weak_ptr<CMOGO>> m_PhysicsObjects;

    void SpawnRandomObject(string _s);
    void BuildMap();
    std::vector<std::shared_ptr<CMOGO>> m_random_obj;
    const Vector2 MAP_SIZE = { 700.0f, 700.0f };

    void DoPhysics();
    void CheckCollision();
    void CollisionHandling(std::shared_ptr<CMOGO> _PO, std::shared_ptr<CMOGO> _CO);

    std::shared_ptr<Player> pPlayer;
    std::shared_ptr<TextGO2D> score_text;
    int score = 0;

    std::shared_ptr<TextGO2D> ammo_text;
    std::vector<std::shared_ptr<Targets>> m_targets; 
    void EnemySpawn(int _health, float _speed);
    float enemy_spawn_clock = 0.0f;
    const float ENEMY_SPAWN_TIME = 1.0f;
    float endless_difficulty = 0.0f;
    int difficulty = 0;
    const int NUM_LEVEL = 5;
    bool basic_game_done = false;
                                         
    //sound stuff
	//This uses a simple system, but a better pipeline can be used using Wave Banks
	//See here: https://github.com/Microsoft/DirectXTK/wiki/Creating-and-playing-sounds Using wave banks Section
    std::unique_ptr<DirectX::AudioEngine> m_audioEngine;
    list<Sound*>m_Sounds;
};
