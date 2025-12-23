#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <iostream>


#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#include "KMTInterceptor.h"
// 顶点结构
struct SimpleVertex {
    float Position[3];
    float Color[4];
};

// HLSL 着色器代码 for D3D11
const char* hlslShaderCode = R"(
struct VS_INPUT {
        float3 Position : POSITION;
        float4 Color : COLOR;
    };

    struct PS_INPUT {
        float4 Position : SV_POSITION;
        float4 Color : COLOR;
    };

    PS_INPUT VS(VS_INPUT input) {
        PS_INPUT output;
        output.Position = float4(input.Position,1.0f);
        output.Color = input.Color;
        return output;
    }

    float4 PS(PS_INPUT input) : SV_TARGET {
        return input.Color;
    }
)";

class D3D11TriangleRenderer {
private:
    // D3D11
    ID3D11Device* m_pd3dDevice;
    ID3D11DeviceContext* m_pImmediateContext;
    IDXGISwapChain* m_pSwapChain;
    ID3D11RenderTargetView* m_pRenderTargetView;

    ID3D11InputLayout* m_pInputLayout;
    ID3D11Buffer* m_pVertexBuffer;
    ID3D11VertexShader* m_pVertexShader;
    ID3D11PixelShader* m_pPixelShader;

    HWND m_hWnd;

public:
    D3D11TriangleRenderer() :
        m_pd3dDevice(nullptr),
        m_pImmediateContext(nullptr),
        m_pSwapChain(nullptr),
        m_pRenderTargetView(nullptr),
        m_pInputLayout(nullptr),
        m_pVertexBuffer(nullptr),
        m_pVertexShader(nullptr),
        m_pPixelShader(nullptr),
        m_hWnd(nullptr) {
    }

    ~D3D11TriangleRenderer() {
        Cleanup();
    }

    HRESULT Initialize(HWND hWnd, UINT width, UINT height) {
        m_hWnd = hWnd;
        std::cout << "Initializing D3D11 renderer..." << std::endl;

        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 4;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = m_hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

        UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;
        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
        D3D_FEATURE_LEVEL selectedFL;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            NULL,
            D3D_DRIVER_TYPE_HARDWARE,
            NULL,
            createDeviceFlags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            &sd,
            &m_pSwapChain,
            &m_pd3dDevice,
            &selectedFL,
            &m_pImmediateContext
        );

        if (FAILED(hr)) {
            std::cerr << "D3D11CreateDeviceAndSwapChain failed: " << std::hex << hr << std::endl;
            return hr;
        }

        if (SUCCEEDED(hr)) {
            return SetupRenderTarget(width, height);
        }
        return hr;
    }

    void CheckDriverInfo() {
        if (!m_pd3dDevice) return;
        IDXGIDevice* pDXGIDevice = nullptr;
        if (SUCCEEDED(m_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice))) {
            IDXGIAdapter* pAdapter = nullptr;
            if (SUCCEEDED(pDXGIDevice->GetAdapter(&pAdapter))) {
                DXGI_ADAPTER_DESC desc;
                if (SUCCEEDED(pAdapter->GetDesc(&desc))) {
                    std::wcout << L"Current adapter: " << desc.Description << std::endl;
                    std::wcout << L"VendorId: " << desc.VendorId << std::endl;
                    std::wcout << L"DeviceId: " << desc.DeviceId << std::endl;
                }
                pAdapter->Release();
            }
            pDXGIDevice->Release();
        }
    }

private:
    HRESULT SetupRenderTarget(UINT width, UINT height) {
        ID3D11Texture2D* pBackBuffer = nullptr;
        HRESULT hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        if (FAILED(hr)) {
            std::cerr << "Failed to get back buffer: " << std::hex << hr << std::endl;
            return hr;
        }

        hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pRenderTargetView);
        pBackBuffer->Release();
        if (FAILED(hr)) {
            std::cerr << "Failed to create render target view: " << std::hex << hr << std::endl;
            return hr;
        }

        m_pImmediateContext->OMSetRenderTargets(1, &m_pRenderTargetView, NULL);

        D3D11_VIEWPORT vp;
        vp.Width = (FLOAT)width;
        vp.Height = (FLOAT)height;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        m_pImmediateContext->RSSetViewports(1, &vp);

        return InitializeShaders();
    }

    HRESULT InitializeShaders() {
        std::cout << "Compiling shaders..." << std::endl;
        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;
        ID3DBlob* errBlob = nullptr;

        // Compile vertex shader
        HRESULT hr = D3DCompile(hlslShaderCode, strlen(hlslShaderCode), nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vsBlob, &errBlob);
        if (FAILED(hr)) {
            if (errBlob) {
                std::cerr << "Vertex shader compile error: " << (char*)errBlob->GetBufferPointer() << std::endl;
                errBlob->Release();
            }
            return hr;
        }

        // Compile pixel shader
        hr = D3DCompile(hlslShaderCode, strlen(hlslShaderCode), nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, &psBlob, &errBlob);
        if (FAILED(hr)) {
            if (errBlob) {
                std::cerr << "Pixel shader compile error: " << (char*)errBlob->GetBufferPointer() << std::endl;
                errBlob->Release();
            }
            if (vsBlob) vsBlob->Release();
            return hr;
        }

        hr = m_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pVertexShader);
        if (FAILED(hr)) {
            std::cerr << "CreateVertexShader failed: " << std::hex << hr << std::endl;
            vsBlob->Release(); psBlob->Release();
            return hr;
        }

        hr = m_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pPixelShader);
        if (FAILED(hr)) {
            std::cerr << "CreatePixelShader failed: " << std::hex << hr << std::endl;
            vsBlob->Release(); psBlob->Release();
            return hr;
        }

        // Define input layout
        D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
            { "POSITION",0, DXGI_FORMAT_R32G32B32_FLOAT,0,0, D3D11_INPUT_PER_VERTEX_DATA,0 },
            { "COLOR",0, DXGI_FORMAT_R32G32B32A32_FLOAT,0,12, D3D11_INPUT_PER_VERTEX_DATA,0 }
        };

        hr = m_pd3dDevice->CreateInputLayout(layoutDesc, ARRAYSIZE(layoutDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_pInputLayout);
        vsBlob->Release();
        if (FAILED(hr)) {
            std::cerr << "CreateInputLayout failed: " << std::hex << hr << std::endl;
            psBlob->Release();
            return hr;
        }

        m_pImmediateContext->IASetInputLayout(m_pInputLayout);

        // Create vertex buffer
        SimpleVertex vertices[] = {
            { {0.0f,0.5f,0.0f }, {1.0f,0.0f,0.0f,1.0f } },
            { {0.5f, -0.5f,0.0f }, {0.0f,1.0f,0.0f,1.0f } },
            { { -0.5f, -0.5f,0.0f },{0.0f,0.0f,1.0f,1.0f } }
        };

        D3D11_BUFFER_DESC bd;
        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(SimpleVertex) * 3;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;

        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem = vertices;

        hr = m_pd3dDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
        if (FAILED(hr)) {
            std::cerr << "CreateBuffer failed: " << std::hex << hr << std::endl;
            psBlob->Release();
            return hr;
        }

        psBlob->Release();

        // Set shaders
        m_pImmediateContext->VSSetShader(m_pVertexShader, nullptr, 0);
        m_pImmediateContext->PSSetShader(m_pPixelShader, nullptr, 0);

        return S_OK;
    }

public:
    void Render() {
        if (!m_pd3dDevice) return;

        m_pImmediateContext->OMSetRenderTargets(1, &m_pRenderTargetView, NULL);
        float clearColor[4] = { 0.0f,0.125f,0.3f,1.0f };
        m_pImmediateContext->ClearRenderTargetView(m_pRenderTargetView, clearColor);

        UINT stride = sizeof(SimpleVertex);
        UINT offset = 0;
        m_pImmediateContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
        m_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


        m_pImmediateContext->Draw(3, 0);

        HRESULT hr = m_pSwapChain->Present(0, 0);
        if (hr == DXGI_STATUS_OCCLUDED) {
            std::cout << "DXGI_STATUS_OCCLUDED." << std::endl;
            return;
        }
    }

    void Cleanup() {
        std::cout << "Cleaning up D3D11 resources..." << std::endl;
        if (m_pVertexBuffer) m_pVertexBuffer->Release();
        if (m_pInputLayout) m_pInputLayout->Release();
        if (m_pPixelShader) m_pPixelShader->Release();
        if (m_pVertexShader) m_pVertexShader->Release();
        if (m_pRenderTargetView) m_pRenderTargetView->Release();
        if (m_pSwapChain) m_pSwapChain->Release();
        if (m_pImmediateContext) m_pImmediateContext->Release();
        if (m_pd3dDevice) m_pd3dDevice->Release();

        m_pVertexBuffer = nullptr;
        m_pInputLayout = nullptr;
        m_pPixelShader = nullptr;
        m_pVertexShader = nullptr;
        m_pRenderTargetView = nullptr;
        m_pSwapChain = nullptr;
        m_pImmediateContext = nullptr;
        m_pd3dDevice = nullptr;

        std::cout << "Cleanup completed!" << std::endl;
    }
};

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 程序入口点
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 启用控制台输出以便调试
    SetDllDirectoryA(".");
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);

    std::cout << "Starting D3D11 Triangle Renderer..." << std::endl;

    // 注册窗口类
    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"D3D11WindowClass";

    if (!RegisterClassExW(&wcex)) {
        MessageBoxW(nullptr, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // 创建窗口
    HWND hWnd = CreateWindowExW(
        0,
        L"D3D11WindowClass",
        L"D3D11 Triangle (App -> D3D11 Runtime) - Press ESC to exit",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hWnd) {
        MessageBoxW(nullptr, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    KMTInterceptor::Initialize();

    // 初始化 D3D11 渲染器
    D3D11TriangleRenderer renderer;
    HRESULT hr = renderer.Initialize(hWnd, 800, 600);
    if (FAILED(hr)) {
        MessageBoxW(hWnd, L"Failed to initialize D3D11!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    renderer.CheckDriverInfo();
    std::cout << "Renderer initialized successfully! Starting main loop..." << std::endl;

    // 主消息循环
    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            renderer.Render();
        }
    }

    KMTInterceptor::Shutdown();
    std::cout << "Application exiting..." << std::endl;
    return (int)msg.wParam;
}
