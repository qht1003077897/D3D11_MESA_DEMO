#include <windows.h>
#include <d3d10_1.h>
#include <d3d10.h>
#include <d3d11.h>
#include <dxgi.h>
#include <iostream>


#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#include "KMTInterceptor.h"
// 顶点结构
struct SimpleVertex {
    float Position[3];
    float Color[4];
};

//
// HLSL 着色器代码
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
        output.Position = float4(input.Position, 1.0f);
        output.Color = input.Color;
        return output;
    }

    float4 PS(PS_INPUT input) : SV_TARGET {
        return input.Color;
    }

    technique10 Render {
        pass P0 {
            SetVertexShader(CompileShader(vs_4_0, VS()));
            SetPixelShader(CompileShader(ps_4_0, PS()));
            SetGeometryShader(NULL);
        }
    }
)";

class D3D10TriangleRenderer {
private:
    // Direct3D 11
    ID3D11Device* m_pd3dDevice;                    // D3D11设备
    ID3D11DeviceContext* m_pd3dImmediateContext;   // D3D11设备上下文
    IDXGISwapChain* m_pSwapChainD3D11;                  // D3D11交换链

    ID3D10Device* m_pDevice;
    IDXGISwapChain* m_pSwapChain;
    ID3D10RenderTargetView* m_pRenderTargetView;
    ID3D10Effect* m_pEffect;
    ID3D10InputLayout* m_pInputLayout;
    ID3D10Buffer* m_pVertexBuffer;
    ID3D10EffectTechnique* m_pTechnique;
    HWND m_hWnd;

public:
    D3D10TriangleRenderer() :
        m_pDevice(nullptr),
        m_pd3dDevice(nullptr),
        m_pd3dImmediateContext(nullptr),
        m_pSwapChainD3D11(nullptr),
        m_pSwapChain(nullptr),
        m_pRenderTargetView(nullptr),
        m_pEffect(nullptr),
        m_pInputLayout(nullptr),
        m_pVertexBuffer(nullptr),
        m_pTechnique(nullptr),
        m_hWnd(nullptr) {
    }

    ~D3D10TriangleRenderer() {
        Cleanup();
    }

    HRESULT Initialize(HWND hWnd, UINT width, UINT height) {
        m_hWnd = hWnd;

        std::cout << "Initializing D3D10 with Gallium backend..." << std::endl;

        HRESULT hr = InitializeStandard(width, height);


        if (SUCCEEDED(hr)) {
            std::cout << "D3D10 initialized successfully!" << std::endl;
        }


        return hr;
    }


    void CheckDriverInfo() {
        if (!m_pDevice) 
                return;

        // 获取设备信息
        IDXGIDevice* pDXGIDevice = nullptr;
        if (SUCCEEDED(m_pDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice))) {
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
    HRESULT InitializeStandard(UINT width, UINT height) {
        std::cout << "Using standard D3D10 initialization..." << std::endl;

        // 创建交换链描述
        DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
        swapChainDesc.BufferCount =1;
        swapChainDesc.BufferDesc.Width = width;
        swapChainDesc.BufferDesc.Height = height;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = m_hWnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = TRUE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
        HRESULT hr;
        //HMODULE hGallium = LoadLibraryA("rosumd.dll");
        //if (!hGallium) {
        //    std::wcout << "rosumd load not success!";
        //}
        //else 
        {
            // 特性等级数组
            D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_1
            };
            UINT numFeatureLevels = ARRAYSIZE(featureLevels);
            D3D_FEATURE_LEVEL selectedFeatureLevel;

            //UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
            //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
            //    hr = D3D11CreateDevice(
            //        nullptr, 
            //        D3D_DRIVER_TYPE_HARDWARE, 
            //        nullptr, 
            //        createDeviceFlags, 
            //        featureLevels,
            //        ARRAYSIZE(featureLevels),
            //        D3D11_SDK_VERSION,      
            //        &m_pd3dDevice, 
            //        &selectedFeatureLevel,
            //        &m_pd3dImmediateContext);

            //    if (SUCCEEDED(hr))
            //    {
            //        std::cout << "D3D11设备创建成功!" << std::endl;
            //        std::cout << "驱动类型: " << D3D_DRIVER_TYPE_HARDWARE << std::endl;
            //    }

            //UINT createDeviceFlags = D3D10_CREATE_DEVICE_BGRA_SUPPORT;
            //createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
            hr = D3D10CreateDeviceAndSwapChain(
                NULL, // 使用默认适配器
                D3D10_DRIVER_TYPE_HARDWARE,
                NULL,
                0, // 标志
                D3D10_SDK_VERSION,
                &swapChainDesc,
                &m_pSwapChain,
                &m_pDevice
            );
        }

        if (SUCCEEDED(hr)) {
            return SetupRenderTarget(width, height);
        }

        return hr;
    }

    bool IsGalliumAdapter(const DXGI_ADAPTER_DESC& desc) {
        // 检查适配器描述，识别 Gallium 驱动
        // 这里需要根据您的 Gallium 驱动的实际特征来识别

        std::wstring adapterName = desc.Description;
        std::wcout << "LLVM?= " << adapterName;
        // 示例检查：通过名称识别
        if (adapterName.find(L"Gallium") != std::wstring::npos ||
            adapterName.find(L"LLVM") != std::wstring::npos ||
            adapterName.find(L"Mesa") != std::wstring::npos) {
            return true;
        }

        // 或者通过 VendorID 识别
        if (desc.VendorId == 0x1002 || // AMD
            desc.VendorId == 0x10DE || // NVIDIA
            desc.VendorId == 0x8086) { // Intel
            // 这些可能是 Gallium 驱动的目标硬件
            return true;
        }

        return false;
    }

    /**
        你可以把这个过程想象成一个艺术画廊的布展流程：

        交换链（SwapChain）：就像一个拥有多个画室的画廊管理员。每个画室（Back Buffer）里都有一个空画布（Texture2D）。

        GetBuffer：你向管理员申请：“请把1号画室的钥匙给我（GetBuffer(0, ...)）”。管理员把钥匙（ID3D11Texture2D）交给你。

        CreateRenderTargetView：你拿着钥匙进入画室，然后为这个画布安装一个特制的“画框”（RTV）。这个画框定义了绘画的有效区域和方式。

        OMSetRenderTargets：你告诉你的画家团队（D3D11 Device Context）：“接下来所有的创作，都请直接画在这个带画框的画布上”。

        绘制：画家团队开始在画布上作画。

        Present：绘画完成后，你把钥匙还给管理员，并说：“可以展出了！”。管理员就会将1号画室（现在的后台缓冲区）和0号画室（现在的前台缓冲区，即屏幕）进行“交换”，让观众看到新画作。

        如果没有 GetBuffer 这一步，你的渲染管线就无法知道应该把图形绘制到交换链管理的哪一个具体的纹理资源上。
    */
    HRESULT SetupRenderTarget(UINT width, UINT height) {
        // 创建渲染目标视图
        ID3D10Texture2D* pBackBuffer = nullptr;
        HRESULT hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&pBackBuffer);//后台缓冲区索引
        if (FAILED(hr)) {
            std::cerr << "Failed to get back buffer: " << std::hex << hr << std::endl;
            return hr;
        }

        hr = m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pRenderTargetView);
        pBackBuffer->Release();
        if (FAILED(hr)) {
            std::cerr << "Failed to create render target view: " << std::hex << hr << std::endl;
            return hr;
        }

        m_pDevice->OMSetRenderTargets(1, &m_pRenderTargetView, NULL);

        // 设置视口
        D3D10_VIEWPORT viewport;
        viewport.Width = width;
        viewport.Height = height;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        m_pDevice->RSSetViewports(1, &viewport);

        return InitializeShaders();
    }

    HRESULT InitializeShaders() {
        std::cout << "Initializing shaders..." << std::endl;

        // 编译着色器
        ID3D10Blob* pErrorBlob = nullptr;
        ID3D10Blob* pShaderBlob = nullptr;

        HRESULT hr = D3D10CompileEffectFromMemory(
            (void*)hlslShaderCode,
            strlen(hlslShaderCode),
            "Shader",
            nullptr,
            nullptr,
            0,
            0,
            &pShaderBlob,
            &pErrorBlob
        );

        if (FAILED(hr)) {
            if (pErrorBlob) {
                std::cerr << "Shader compilation error: " << (char*)pErrorBlob->GetBufferPointer() << std::endl;
                pErrorBlob->Release();
            }
            return hr;
        }

        hr = D3D10CreateEffectFromMemory(
            pShaderBlob->GetBufferPointer(),
            pShaderBlob->GetBufferSize(),
            0,
            m_pDevice,
            nullptr,
            &m_pEffect
        );

        if (pShaderBlob) pShaderBlob->Release();
        if (pErrorBlob) pErrorBlob->Release();

        if (FAILED(hr)) {
            std::cerr << "Failed to create effect: " << std::hex << hr << std::endl;
            return hr;
        }

        // 获取技术
        m_pTechnique = m_pEffect->GetTechniqueByName("Render");
        if (!m_pTechnique || !m_pTechnique->IsValid()) {
            std::cerr << "Failed to get technique" << std::endl;
            return E_FAIL;
        }

        // 创建输入布局
        D3D10_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 }
        };
        UINT numElements = sizeof(layout) / sizeof(layout[0]);

        D3D10_PASS_DESC passDesc;
        m_pTechnique->GetPassByIndex(0)->GetDesc(&passDesc);

        hr = m_pDevice->CreateInputLayout(
            layout,
            numElements,
            passDesc.pIAInputSignature,
            passDesc.IAInputSignatureSize,
            &m_pInputLayout
        );

        if (FAILED(hr)) {
            std::cerr << "Failed to create input layout: " << std::hex << hr << std::endl;
            return hr;
        }

        m_pDevice->IASetInputLayout(m_pInputLayout);

        return CreateVertexBuffer();
    }

    HRESULT CreateVertexBuffer() {
        std::cout << "Creating vertex buffer..." << std::endl;

        // 定义三角形的顶点数据
        SimpleVertex vertices[] = {
            // 位置              // 颜色 (RGBA)
            { { 0.0f, 0.5f, 0.0f },  { 1.0f, 0.0f, 0.0f, 1.0f } },   // 顶点 0: 红色
            { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },   // 顶点 1: 绿色
            { { -0.5f, -0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }    // 顶点 2: 蓝色
        };

        // 创建顶点缓冲区
        D3D10_BUFFER_DESC bufferDesc;
        bufferDesc.Usage = D3D10_USAGE_DEFAULT;
        bufferDesc.ByteWidth = sizeof(SimpleVertex) * 3;
        bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = 0;

        D3D10_SUBRESOURCE_DATA initData;
        initData.pSysMem = vertices;
        initData.SysMemPitch = 0;
        initData.SysMemSlicePitch = 0;

        HRESULT hr = m_pDevice->CreateBuffer(&bufferDesc, &initData, &m_pVertexBuffer);
        if (FAILED(hr)) {
            std::cerr << "Failed to create vertex buffer: " << std::hex << hr << std::endl;
            return hr;
        }

        std::cout << "Vertex buffer created successfully!" << std::endl;
        return S_OK;
    }

public:
    void Render() {
        if (!m_pDevice) return;

        // 清除渲染目标为深蓝色
        float clearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
        m_pDevice->ClearRenderTargetView(m_pRenderTargetView, clearColor);

        // 设置顶点缓冲区
        UINT stride = sizeof(SimpleVertex);
        UINT offset = 0;
        m_pDevice->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
        m_pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 应用技术并绘制
        D3D10_TECHNIQUE_DESC techDesc;
        m_pTechnique->GetDesc(&techDesc);

        for (UINT p = 0; p < techDesc.Passes; ++p) {
            m_pTechnique->GetPassByIndex(p)->Apply(0);
            m_pDevice->Draw(3, 0);  // 绘制3个顶点（一个三角形）
        }

        // 呈现到屏幕 DXGI_DDI_FLIP_INTERVAL_IMMEDIATE

        HRESULT hr = m_pSwapChain->Present(0, DXGI_PRESENT_DO_NOT_WAIT);
        if (hr == DXGI_STATUS_OCCLUDED) {
            std::cout << "DXGI_STATUS_OCCLUDED." << std::endl;
            // 应用程序被遮挡，不需要实际呈现
            return; // 跳过实际Present调用
        }
    }

    void Cleanup() {
        std::cout << "Cleaning up resources..." << std::endl;

        if (m_pVertexBuffer) m_pVertexBuffer->Release();
        if (m_pInputLayout) m_pInputLayout->Release();
        if (m_pEffect) m_pEffect->Release();
        if (m_pRenderTargetView) m_pRenderTargetView->Release();
        if (m_pSwapChain) m_pSwapChain->Release();
        if (m_pDevice) m_pDevice->Release();

        m_pVertexBuffer = nullptr;
        m_pInputLayout = nullptr;
        m_pEffect = nullptr;
        m_pRenderTargetView = nullptr;
        m_pSwapChain = nullptr;
        m_pDevice = nullptr;

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

    std::cout << "Starting D3D10 Triangle Renderer..." << std::endl;
    //std::cout << "Expected call path: App -> D3D10 Runtime -> Gallium Driver" << std::endl;

    // 注册窗口类
    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"D3D10WindowClass";

    if (!RegisterClassExW(&wcex)) {
        MessageBoxW(nullptr, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // 创建窗口
    HWND hWnd = CreateWindowExW(
        0,
        L"D3D10WindowClass",
        L"D3D10 Triangle (App -> D3D10 Runtime -> Gallium Driver) - Press ESC to exit",
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

    // 初始化 D3D10 渲染器
    D3D10TriangleRenderer renderer;
    HRESULT hr = renderer.Initialize(hWnd, 800, 600);
    if (FAILED(hr)) {
        MessageBoxW(hWnd, L"Failed to initialize D3D10!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
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
