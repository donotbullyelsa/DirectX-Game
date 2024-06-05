//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include <time.h>

#include <iostream>

//Scarle Headers
#include "GameData.h"
#include "GameState.h"
#include "DrawData.h"
#include "DrawData2D.h"
#include "ObjectList.h"

#include "CMOGO.h"
#include <DirectXCollision.h>
#include "Collision.h"

extern void ExitGame() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept :
    m_window(nullptr),
    m_outputWidth(800),
    m_outputHeight(600),
    m_featureLevel(D3D_FEATURE_LEVEL_11_0)
{
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND _window, int _width, int _height)
{
    m_window = _window;
    m_outputWidth = std::max(_width, 1);
    m_outputHeight = std::max(_height, 1);

    //get window size
    GetDefaultSize(winX, winY);

    CreateDevice();

    CreateResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */

    //seed the random number generator
    srand((UINT)time(NULL));

    //set up keyboard and mouse system
    //documentation here: https://github.com/microsoft/DirectXTK/wiki/Mouse-and-keyboard-input
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(_window);
    m_mouse->SetMode(Mouse::MODE_RELATIVE);
    //Hide the mouse pointer
    ShowCursor(false);

    //create GameData struct and populate its pointers
    m_GD = new GameData;
    m_GD->m_GS = GS_MENU;

    //set up systems for 2D rendering
    m_DD2D = new DrawData2D();
    m_DD2D->m_Sprites.reset(new SpriteBatch(m_d3dContext.Get()));
    m_DD2D->m_Font.reset(new SpriteFont(m_d3dDevice.Get(), L"..\\Assets\\italic.spritefont"));
    m_states = new CommonStates(m_d3dDevice.Get());

    //set up DirectXTK Effects system
    m_fxFactory = new EffectFactory(m_d3dDevice.Get());
    //Tell the fxFactory to look to the correct build directory to pull stuff in from
    ((EffectFactory*)m_fxFactory)->SetDirectory(L"..\\Assets");
    //init render system for VBGOs
    VBGO::Init(m_d3dDevice.Get());

    //set audio system
    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif
    m_audioEngine = std::make_unique<AudioEngine>(eflags);

    //create a set of dummy things to show off the engine

    //create a base light
    m_light = std::make_shared<Light>(Vector3(0.0f, 100.0f, 160.0f), Color(1.0f, 1.0f, 1.0f, 1.0f), Color(0.4f, 0.1f, 0.1f, 1.0f));
    m_GameObjects.push_back(m_light);

    //find how big my window is to correctly calculate my aspect ratio
    float AR = (float)_width / (float)_height;

    //add Player
    pPlayer = std::make_shared<Player>("BirdModelV1", m_d3dDevice.Get(), m_fxFactory);
    pPlayer->SetScale(10.0f);
    pPlayer->SetPos(Vector3(rand() % int(MAP_SIZE.x) - 350.0f, -100.0f, rand() % int(MAP_SIZE.y) - 350.0f));
    //pPlayer->SetRoll(100.0f);
    m_GameObjects.push_back(pPlayer);
    m_PhysicsObjects.push_back(pPlayer);
    m_GD->score = 0;

    //create a base camera
    m_cam = std::make_shared<TPSCamera>(0.25f * XM_PI, AR, 1.0f, 10000.0f, pPlayer, Vector3::UnitY, Vector3(0.0f, 100.0f, 200.0f));
    m_GameObjects.push_back(m_cam);

    //add a secondary camera
    m_TPScam = std::make_shared<TPSCamera>(0.25f * XM_PI, AR, 1.0f, 10000.0f, pPlayer, Vector3::UnitY, Vector3(0.0f, 0.0f, 0.02f));
    m_GameObjects.push_back(m_TPScam);

    //create DrawData struct and populate its pointers
    m_DD = new DrawData;
    m_DD->m_pd3dImmediateContext = nullptr;
    m_DD->m_states = m_states;
    m_DD->m_cam = m_cam.get();
    m_DD->m_light = m_light.get();

    title_text = std::make_shared<TextGO2D>("DirectX Game");
    title_text->SetPos(Vector2(winX / 5, winY / 5));
    title_text->SetColour(Color((float*)&Colors::Yellow));
    m_MenuObjects2D.push_back(title_text);

    ammo_text = std::make_shared<TextGO2D>("Ammo: " + to_string(pPlayer->AMMO_LIMIT) + " / " + to_string(pPlayer->AMMO_LIMIT));
    ammo_text->SetPos(Vector2(50, winY - 100));
    ammo_text->SetColour(Color((float*)&Colors::Yellow));
    m_GameObjects2D.push_back(ammo_text);
    pPlayer->ammo_text = ammo_text;

    score_text = std::make_shared<TextGO2D>("Score: " + to_string(score));
    score_text->SetPos(Vector2(50, winY - 200));
    score_text->SetColour(Color((float*)&Colors::Yellow));
    m_GameObjects2D.push_back(score_text);

    BuildMap();
}

void Game::SpawnRandomObject(string _s)
{
    //make a seperate collision layer for this
    int x = rand() % int(MAP_SIZE.x);
    int z = rand() % int(MAP_SIZE.y);
    std::shared_ptr<Terrain> temp_GO = std::make_shared<Terrain>(_s, m_d3dDevice.Get(), m_fxFactory, Vector3(x - 350.0f, -100.0f, z - 350.0f), rand() % 360, rand() % 360, rand() % 360, 20.0f * Vector3::One);
    m_GameObjects.push_back(temp_GO);
    m_random_obj.push_back(temp_GO);
}

void Game::BuildMap()
{
    //ground
    for (int i = 0; i < 7; i++) for (int j = 0; j < 7; j++)
    {
        std::shared_ptr<Terrain> temp_GO = std::make_shared<Terrain>("block", m_d3dDevice.Get(), m_fxFactory, Vector3(100.0f * (j - 3), -200.0f, 100.0f * (i - 3)), 0.0f, 0.0f, 0.0f, 10.0f * Vector3::One);
        m_GameObjects.push_back(temp_GO);
        m_ColliderObjects.push_back(temp_GO);

    }

    //spawn some objects randomly around the map
    SpawnRandomObject("pistol");
    SpawnRandomObject("uzi");
    SpawnRandomObject("sniper");
    SpawnRandomObject("shotgun");
    SpawnRandomObject("knife_sharp");
    SpawnRandomObject("grenade");

    //Test Sounds
    Loop* loop = new Loop(m_audioEngine.get(), "NightAmbienceSimple_02");
    loop->SetVolume(0.1f);
    //loop->Play();
    //m_Sounds.push_back(loop);

    TestSound* TS = new TestSound(m_audioEngine.get(), "Explo1");
    //m_Sounds.push_back(TS);
}

// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
        {
            Update(m_timer);
        });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& _timer)
{
    float elapsedTime = float(_timer.GetElapsedSeconds());
    m_GD->m_dt = elapsedTime;

    //this will update the audio engine but give us chance to do somehting else if that isn't working
    if (!m_audioEngine->Update())
    {
        if (m_audioEngine->IsCriticalError())
        {
            // We lost the audio device!
        }
    }
    else
    {
        //update sounds playing
        for (list<Sound*>::iterator it = m_Sounds.begin(); it != m_Sounds.end(); it++)
        {
            (*it)->Tick(m_GD);
        }
    }

    //read input
    ReadInput();
    //m_GD->m_GS = GameState::GS_GAME_OVER;
    //basic_game_done = true;

    //gamestates
    switch (m_GD->m_GS)
    {
        //menu
        case GameState::GS_MENU:
        {
            //input
            if (m_GD->m_KBS.Enter)
            {
                m_GD->m_GS = GameState::GS_PLAY_MAIN_CAM;
            }
            break;
        }

        //win state
        case GameState::GS_WON:
        {
            //std::cout << "winner ";
            title_text->SetText("You've won!\nPress Enter to continue\nplaying endless mode.");
            basic_game_done = true;

            if (m_GD->m_KBS.Enter)
            {
                m_GD->m_GS = GameState::GS_PLAY_MAIN_CAM;
            }
            break;
        }

        //lose state
        case GameState::GS_LOST:
        {
            //std::cout << "loser ";
            title_text->SetText("Sorry, you've lost.");

            if (m_GD->m_KBS.Enter)
            {
                std::exit(1);
            }
            break;
        }

        //endless mode
        case GameState::GS_GAME_OVER:
        {
            title_text->SetText("Sorry, the game is over.\nYour final score is " + to_string(score) + ".");

            if (m_GD->m_KBS.Enter)
            {
                std::exit(1);
            }
            break;
        }

        //main game
        default:
        {
            score_text->SetText("Score: " + to_string(score) + "\nEnemy spawn time: " + to_string(1 - endless_difficulty));

            //update spawn timer
            enemy_spawn_clock += elapsedTime;
            if (basic_game_done)
            {
                EnemySpawn(100, (NUM_LEVEL + 1) * 20.0f);
            }
            else
            {
                EnemySpawn(100, (difficulty + 1) * 20.0f);
            }

            //upon space bar switch camera state
            //see docs here for what's going on: https://github.com/Microsoft/DirectXTK/wiki/Keyboard
            if (m_GD->m_KBS_tracker.pressed.G)
            {
                if (m_GD->m_GS == GS_PLAY_MAIN_CAM)
                {
                    m_GD->m_GS = GS_PLAY_TPS_CAM;
                    if (pPlayer != nullptr)
                    {
                        pPlayer->visible = false;
                    }
                }
                else
                {
                    m_GD->m_GS = GS_PLAY_MAIN_CAM;
                    if (pPlayer != nullptr)
                    {
                        pPlayer->visible = true;
                    }
                }
            }

            //player shoot
            else if (m_GD->m_KBS_tracker.pressed.E)
            {
                if (pPlayer->isAmmoRunOut())
                {
                    pPlayer->Reload();
                }
                else if (pPlayer->canSpawnBullet())
                {
                    std::shared_ptr<Bullet> m_bullet = std::make_shared<Bullet>("ammo_pistol", m_d3dDevice.Get(), m_fxFactory);
                    m_bullet->SetScale(10.0f);
                    m_bullet->SetPos(pPlayer->GetPos());
                    m_GameObjects.push_back(m_bullet);
                    m_PhysicsObjects.push_back(m_bullet);
                    pPlayer->Shoot(m_bullet);
                }
            }

            //reload
            else if (m_GD->m_KBS_tracker.pressed.R)
            {
                pPlayer->Reload();
            }

            //update all objects
            for (list<std::weak_ptr<GameObject>>::iterator it = m_GameObjects.begin(); it != m_GameObjects.end(); it++)
            {
                if ((*it).lock() != nullptr)
                (*it).lock()->Tick(m_GD);
            }
            for (list<std::shared_ptr<GameObject2D>>::iterator it = m_GameObjects2D.begin(); it != m_GameObjects2D.end(); it++)
            {
                (*it)->Tick(m_GD);
            }
            for (std::vector<std::shared_ptr<Targets>>::iterator it = m_targets.begin(); it != m_targets.end(); it++)
            {
                (*it)->Tick(m_GD, Vector2(pPlayer->GetPos().x, pPlayer->GetPos().z));
            }

            DoPhysics();
        }
    }
}

// also check if needs to spawn
void Game::EnemySpawn(int _health, float _speed)
{
    if (enemy_spawn_clock >= ENEMY_SPAWN_TIME - endless_difficulty)
    {
        enemy_spawn_clock -= ENEMY_SPAWN_TIME - endless_difficulty;

        //basic game
        if (!basic_game_done)
        {
            if (m_targets.size() == 0)
            {
                difficulty++;
                if (difficulty > NUM_LEVEL)
                {
                    m_GD->m_GS = GS_WON;
                }

                else
                {
                    //make a seperated collision layer for enemies
                    std::shared_ptr<Targets> temp_target = std::make_shared<Targets>("BirdModelV1", m_d3dDevice.Get(), m_fxFactory, _speed);
                    temp_target->health = _health;
                    m_targets.push_back(temp_target);
                    temp_target->SetScale(10.0f);
                    m_GameObjects.push_back(temp_target);
                }
            }
        }

        //after defeating 5 enemies, gamemode change
        //more than one enemy can exist in the world at the same time, all of them can be killed in one hit
        else
        {
            //make a seperated collision layer for enemies
            std::shared_ptr<Targets> temp_target = std::make_shared<Targets>("BirdModelV1", m_d3dDevice.Get(), m_fxFactory, _speed);
            temp_target->health = 1;
            m_targets.push_back(temp_target);
            temp_target->SetScale(10.0f);
            m_GameObjects.push_back(temp_target);
            
            
             endless_difficulty += 0.02f; 
             if (endless_difficulty > 0.9f)
             {
                 endless_difficulty = 0.9f;
             }
        }
    }
}

// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    //set immediate context of the graphics device
    m_DD->m_pd3dImmediateContext = m_d3dContext.Get();

    //set which camera to be used
    m_DD->m_cam = m_cam.get();
    if (m_GD->m_GS == GS_PLAY_TPS_CAM)
    {
        m_DD->m_cam = m_TPScam.get();
    }

    //update the constant buffer for the rendering of VBGOs
    VBGO::UpdateConstantBuffer(m_DD);

    switch (m_GD->m_GS)
    {
        case GameState::GS_MENU:
        case GameState::GS_WON:
        case GameState::GS_LOST:
        case GameState::GS_GAME_OVER:
        {
            // Draw sprite batch stuff 
            Draw2D(m_MenuObjects2D);

            break;
        }

        default:
        {
            //Draw 3D Game Obejects
            for (list <std::weak_ptr<GameObject>> ::iterator it = m_GameObjects.begin(); it != m_GameObjects.end(); it++)
            {
                if ((*it).lock() != nullptr)
                {
                    if ((*it).lock()->visible)
                    {
                        (*it).lock()->Draw(m_DD);
                    }
                }
            }

            // Draw sprite batch stuff 
            list<std::weak_ptr<GameObject2D>> m_GameObjects2D_temp;
            for (std::shared_ptr<GameObject2D> m_GO2D_temp : m_GameObjects2D)
            {
                m_GameObjects2D_temp.push_back(m_GO2D_temp);
            }
            Draw2D(m_GameObjects2D_temp);
        }
    }

    //drawing text screws up the Depth Stencil State, this puts it back again!
    m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

    Present();
}

void Game::Draw2D(list<std::weak_ptr<GameObject2D>> list_of_GO2D)
{
    m_DD2D->m_Sprites->Begin(SpriteSortMode_Deferred, m_states->NonPremultiplied());
    for (list< std::weak_ptr<GameObject2D>>::iterator it = list_of_GO2D.begin(); it != list_of_GO2D.end(); it++)
    {
        (*it).lock()->Draw(m_DD2D);
    }

    m_DD2D->m_Sprites->End();
}


// Helper method to clear the back buffers.
void Game::Clear()
{
    // Clear the views.
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::CornflowerBlue);
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    // Set the viewport.
    CD3D11_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight));
    m_d3dContext->RSSetViewports(1, &viewport);
}

// Presents the back buffer contents to the screen.
void Game::Present()
{
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        OnDeviceLost();
    }
    else
    {
        DX::ThrowIfFailed(hr);
    }
}

// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowSizeChanged(int _width, int _height)
{
    m_outputWidth = std::max(_width, 1);
    m_outputHeight = std::max(_height, 1);

    CreateResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& _width, int& _height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    _width = 800;
    _height = 600;
}

// These are the resources that depend on the device.
void Game::CreateDevice()
{
    UINT creationFlags = 0;

#ifdef _DEBUG
    //creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
    //something missing on the machines in 2Q28
    //this should work!
#endif

    static const D3D_FEATURE_LEVEL featureLevels[] =
    {
        // TODO: Modify for supported Direct3D feature levels
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };

    // Create the DX11 API device object, and get a corresponding context.
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    DX::ThrowIfFailed(D3D11CreateDevice(
        nullptr,                            // specify nullptr to use the default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        creationFlags,
        featureLevels,
        static_cast<UINT>(std::size(featureLevels)),
        D3D11_SDK_VERSION,
        device.ReleaseAndGetAddressOf(),    // returns the Direct3D device created
        &m_featureLevel,                    // returns feature level of device created
        context.ReleaseAndGetAddressOf()    // returns the device immediate context
    ));

#ifndef NDEBUG
    ComPtr<ID3D11Debug> d3dDebug;
    if (SUCCEEDED(device.As(&d3dDebug)))
    {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D11_MESSAGE_ID hide[] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                // TODO: Add more message IDs here as needed.
            };
            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    DX::ThrowIfFailed(device.As(&m_d3dDevice));
    DX::ThrowIfFailed(context.As(&m_d3dContext));

    // TODO: Initialize device dependent objects here (independent of window size).
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
    // Clear the previous window size specific context.
    ID3D11RenderTargetView* nullViews[] = { nullptr };
    m_d3dContext->OMSetRenderTargets(static_cast<UINT>(std::size(nullViews)), nullViews, nullptr);
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_d3dContext->Flush();

    const UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
    const UINT backBufferHeight = static_cast<UINT>(m_outputHeight);
    const DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    const DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    constexpr UINT backBufferCount = 2;

    // If the swap chain already exists, resize it, otherwise create one.
    if (m_swapChain)
    {
        HRESULT hr = m_swapChain->ResizeBuffers(backBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            OnDeviceLost();

            // Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
    }
    else
    {
        // First, retrieve the underlying DXGI Device from the D3D Device.
        ComPtr<IDXGIDevice1> dxgiDevice;
        DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

        // Identify the physical adapter (GPU or card) this device is running on.
        ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

        // And obtain the factory object that created it.
        ComPtr<IDXGIFactory2> dxgiFactory;
        DX::ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = backBufferCount;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        // Create a SwapChain from a Win32 window.
        DX::ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
            m_d3dDevice.Get(),
            m_window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            m_swapChain.ReleaseAndGetAddressOf()
        ));

        // This template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut.
        DX::ThrowIfFailed(dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));
    }

    // Obtain the backbuffer for this window which will be the final 3D rendertarget.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));

    // Create a view interface on the rendertarget to use on bind.
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));

    // Allocate a 2-D surface as the depth/stencil buffer and
    // create a DepthStencil view on this surface to use on bind.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf()));

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    DX::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, m_depthStencilView.ReleaseAndGetAddressOf()));

    // TODO: Initialize windows-size dependent objects here.
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.

    m_depthStencilView.Reset();
    m_renderTargetView.Reset();
    m_swapChain.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();

    CreateDevice();

    CreateResources();
}

void Game::ReadInput()
{
    m_GD->m_KBS = m_keyboard->GetState();
    m_GD->m_KBS_tracker.Update(m_GD->m_KBS);
    //quit game on hiting escape
    if (m_GD->m_KBS.Escape)
    {
        ExitGame();
    }

    m_GD->m_MS = m_mouse->GetState();

    //lock the cursor to the centre of the window
    RECT window;
    GetWindowRect(m_window, &window);
    SetCursorPos((window.left + window.right) >> 1, (window.bottom + window.top) >> 1);
}

void Game::DoPhysics()
{

    CheckCollision();
}

void Game::CollisionHandling(std::shared_ptr<CMOGO> _PO, std::shared_ptr<CMOGO> _CO)
{
    
    if (_PO != nullptr)
    {
        if (_PO->Intersects(*_CO))
        {
            XMFLOAT3 eject_vect = Collision::ejectionCMOGO(*_PO, *_CO);
            //std::cout << eject_vect.x << std::endl << eject_vect.y << std::endl << eject_vect.z << std::endl;
            auto pos = _PO->GetPos();
            {
                _PO->SetPos(pos - eject_vect);
            }
            if (eject_vect.y <= 0.0f)
            {
                _PO->grounded = true;
                _PO->stopGravity();
            }

            //std::cout << eject_vect.y << std::endl;
        }

        else
        {
            _PO->grounded = false;
        }

        //record the current position for next frame
        _PO->last_pos = _PO->GetPos();

    }
}

void Game::CheckCollision()
{

    for (std::weak_ptr<CMOGO> m_PO: m_PhysicsObjects) for (std::shared_ptr<CMOGO> m_CO: m_ColliderObjects)
    {
        CollisionHandling(m_PO.lock(), m_CO);
    }

    int j = 0;
    //dealing with player / enemy collision
    for (std::weak_ptr<Targets> temp_target : m_targets)
    {
        if (m_targets[j] != nullptr)
        {
            //if collide, player lose
            if (pPlayer->Intersects(*temp_target.lock()))
            {
                if (basic_game_done)
                {
                    m_GD->m_GS = GS_GAME_OVER;
                }
                else
                {
                    m_GD->m_GS = GS_LOST;
                }
            }

            bool kill_flag = false;
            //dealing with bullet / enemy collision
            for (auto it = pPlayer->bullet.begin(); it != pPlayer->bullet.end(); it++)
            {
                if ((*it)->Intersects(*temp_target.lock()))
                {
                    temp_target.lock()->health -= (*it)->DAMAGE;
                    it->reset();

                    if (temp_target.lock()->health <= 0)
                    {
                        kill_flag = true;
                    }
                }
            }

            //delete bullet that should be deleted
            int i = 0;
            while (pPlayer->bullet.size() > i)
            {
                std::shared_ptr<Bullet> it = pPlayer->bullet[i];
                if (it == nullptr)
                {
                    pPlayer->bullet.erase(pPlayer->bullet.begin() + i);                    
                }
                else
                {
                    i++;
                }
            }

            //remove dead enemy
            if (kill_flag)
            {
                score += m_targets[j]->SPEED * 5;
                m_targets[j].reset();
            }

            //handle collision for enemy surviving
            else
            {
                CollisionHandling(pPlayer, temp_target.lock());
                for (std::shared_ptr<CMOGO> m_CO : m_ColliderObjects)
                {
                    CollisionHandling(temp_target.lock(), m_CO);
                }
            }
        }

        j++;
    }

    int i = 0;
    while (m_targets.size() > i)
    {
        std::shared_ptr<Targets> it = m_targets[i];
        if (it == nullptr)
        {
            m_targets.erase(m_targets.begin() + i);
        }
        else
        {
            i++;
        }
    }

    //dealing with player / random objects collision
    for (std::shared_ptr<CMOGO> temp_target : m_random_obj)
    {
        CollisionHandling(pPlayer, temp_target);
        for (std::shared_ptr<CMOGO> m_CO : m_ColliderObjects)
        {
            CollisionHandling(temp_target, m_CO);
        }
    }

    //world boundary
    if (pPlayer->GetPos().z > MAP_SIZE.y / 2)
    {
        pPlayer->SetPosZ(MAP_SIZE.y / 2);
    }
    else if (pPlayer->GetPos().z < -MAP_SIZE.y / 2)
    {
        pPlayer->SetPosZ(-MAP_SIZE.y / 2);
    }

    if (pPlayer->GetPos().x > MAP_SIZE.x / 2)
    {
        pPlayer->SetPosX(MAP_SIZE.x / 2);
    }
    else if (pPlayer->GetPos().x < -MAP_SIZE.x / 2)
    {
        pPlayer->SetPosX(-MAP_SIZE.x / 2);
    }
}