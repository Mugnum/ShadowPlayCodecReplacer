// ==WindhawkMod==
// @id              shadowplay-audio-codec-replacer
// @name            Shadowplay Audio Codec Replacer
// @description     Replaces NVIDIA's AAC encoder with a custom in-process encoder
// @version         0.3.0
// @author          Mugnum
// @include         nvcontainer.exe
// ==/WindhawkMod==

// ==WindhawkModSettings==
/*
- BridgeDllPath: C:\PathToFiles\ShadowPlayAudioBridge.dll
*/
// ==/WindhawkModSettings==

#include <windows.h>
#include <objbase.h>
#include <string>

using CoCreateInstance_t = HRESULT(WINAPI*)(
    REFCLSID classId,
    LPUNKNOWN outer,
    DWORD context,
    REFIID requestedInterface,
    LPVOID* result
);

using CreateReplacementAudioEncoder_t = HRESULT(WINAPI*)(
    IUnknown* originalEncoder,
    REFIID requestedInterface,
    void** result
);

static CoCreateInstance_t CoCreateInstance_Original = nullptr;

// Microsoft AAC encoder:
// {93AF0C51-2275-45D2-A35B-F2BA21CAED00}
static const CLSID kMicrosoftAacEncoderClsid =
{
    0x93AF0C51,
    0x2275,
    0x45D2,
    { 0xA3, 0x5B, 0xF2, 0xBA, 0x21, 0xCA, 0xED, 0x00 }
};

static std::wstring g_bridgeDllPath;

HRESULT WINAPI CoCreateInstance_Hook(
    REFCLSID classId,
    LPUNKNOWN outer,
    DWORD context,
    REFIID requestedInterface,
    LPVOID* result)
{
    HRESULT hr = CoCreateInstance_Original(
        classId,
        outer,
        context,
        requestedInterface,
        result
    );

    if (FAILED(hr) ||
        !result ||
        !*result ||
        !IsEqualCLSID(classId, kMicrosoftAacEncoderClsid))
    {
        return hr;
    }

    HMODULE bridgeModule = LoadLibraryW(g_bridgeDllPath.c_str());

    if (!bridgeModule)
    {
        Wh_Log(L"[Audio bridge] Failed to load bridge DLL. Path=%s Error=%lu",
            g_bridgeDllPath.c_str(),
            GetLastError());

        return hr;
    }

    auto CreateReplacementAudioEncoder =
        reinterpret_cast<CreateReplacementAudioEncoder_t>(
            GetProcAddress(
                bridgeModule,
                "CreateReplacementAudioEncoder"
            )
        );

    if (!CreateReplacementAudioEncoder)
    {
        Wh_Log(
            L"[Audio bridge] Export CreateReplacementAudioEncoder was not found"
        );

        return hr;
    }

    void* replacement = nullptr;

    HRESULT replacementHr = CreateReplacementAudioEncoder(
        static_cast<IUnknown*>(*result),
        requestedInterface,
        &replacement
    );

    if (FAILED(replacementHr) || !replacement)
    {
        Wh_Log(
            L"[Audio bridge] Bridge declined replacement. HRESULT=0x%08X",
            replacementHr
        );

        return hr;
    }

    static_cast<IUnknown*>(*result)->Release();
    *result = replacement;

    Wh_Log(L"[Audio bridge] AAC encoder instance replaced");
    return S_OK;
}

BOOL Wh_ModInit()
{
    PCWSTR configuredPath = Wh_GetStringSetting(L"BridgeDllPath");

    if (!configuredPath || !*configuredPath)
    {
        Wh_Log(L"[Audio bridge] BridgeDllPath is empty");
        return FALSE;
    }

    g_bridgeDllPath = configuredPath;

    HMODULE combase = GetModuleHandleW(L"combase.dll");

    if (!combase)
    {
        Wh_Log(L"[Audio bridge] combase.dll is not loaded");
        return FALSE;
    }

    void* target = reinterpret_cast<void*>(
        GetProcAddress(combase, "CoCreateInstance")
    );

    if (!target)
    {
        Wh_Log(L"[Audio bridge] Could not locate CoCreateInstance");
        return FALSE;
    }

    if (!Wh_SetFunctionHook(
        target,
        reinterpret_cast<void*>(CoCreateInstance_Hook),
        reinterpret_cast<void**>(&CoCreateInstance_Original)
    ))
    {
        Wh_Log(L"[Audio bridge] Failed to hook CoCreateInstance");
        return FALSE;
    }

    Wh_Log(L"[Audio bridge] CoCreateInstance hook installed");
    return TRUE;
}
