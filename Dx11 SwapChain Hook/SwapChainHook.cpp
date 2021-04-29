#include "SwapChainHook.h"

typedef HRESULT(__fastcall* D3D11PresentHook) (IDXGISwapChain* pChain, UINT syncInterval, UINT flags);
D3D11PresentHook gamesPresent = nullptr;

IDXGISwapChain* pSwapChain = nullptr;
ID3D11Device* pDevice = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11Texture2D* pBackBuffer = nullptr;
ID3D11RenderTargetView* renderTargetView = nullptr;

IFW1Factory* pFW1Factory = nullptr;
IFW1FontWrapper* pFontWrapper = nullptr;

bool init = false;

HRESULT __fastcall hkPresent(IDXGISwapChain* pChain, UINT syncInterval, UINT flags) {
    if (!init) {
        pSwapChain = pChain;

        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(pDevice), reinterpret_cast<void**>(&pDevice)))) {
            pSwapChain->GetDevice(__uuidof(pDevice), reinterpret_cast<void**>(&pDevice));
            pDevice->GetImmediateContext(&pContext);

            FW1CreateFactory(FW1_VERSION, &pFW1Factory);
            pFW1Factory->CreateFontWrapper(pDevice, L"Bahnschrift", &pFontWrapper);
            pFW1Factory->Release();
        }

        init = true;
    };

    ID3D11Texture2D* renderTargetTexture = nullptr;

    if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&renderTargetTexture))))
    {
        pDevice->CreateRenderTargetView(renderTargetTexture, NULL, &renderTargetView);
        renderTargetTexture->Release();
    }

    pContext->OMSetRenderTargets(1, &renderTargetView, nullptr);

    const wchar_t* text = L"Dx11 SwapChain Hook :P";
    float size = 23.0f;
    float x = 20.0f;
    float y = 20.0f;
    unsigned int color = 0xff25db9c;

    pFontWrapper->DrawString(pContext, text, size, x, y, color, FW1_RESTORESTATE);

    renderTargetView->Release();

    return gamesPresent(pChain, syncInterval, flags);
}

void SwapChainHook::InitHook() {
    WNDCLASSEX wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProc;
    wc.lpszClassName = TEXT("TemporaryWindow");
    RegisterClassEx(&wc);

    HWND hWnd = CreateWindow(wc.lpszClassName, TEXT(""), WS_DISABLED, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    DXGI_SWAP_CHAIN_DESC sd;
    {
        memset((&sd), 0, (sizeof(sd)));
        sd.BufferCount = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    }

    HRESULT hrD3D11CreateDeviceAndSwapChain = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, &featureLevel, 1, D3D11_SDK_VERSION, &sd,
        &pSwapChain, &pDevice, nullptr, &pContext);

    if (FAILED(hrD3D11CreateDeviceAndSwapChain)) {
        DestroyWindow(sd.OutputWindow);
        UnregisterClass(wc.lpszClassName, GetModuleHandle(nullptr));
        return;
    }

    uintptr_t** pSwapChainVTable = *reinterpret_cast<uintptr_t***>(pSwapChain);

    if (MH_Initialize() == MH_OK) {
        MH_CreateHook((void*)pSwapChainVTable[8], &hkPresent, reinterpret_cast<LPVOID*>(&gamesPresent));
        MH_EnableHook((void*)pSwapChainVTable[8]);
    }

    pDevice->Release();
    pContext->Release();
    pSwapChain->Release();

    DestroyWindow(sd.OutputWindow);
    UnregisterClass(wc.lpszClassName, GetModuleHandle(nullptr));
}