// ==WindhawkMod==
// @id              shadowplay-audio-codec-replacer
// @name            Shadowplay Audio Codec Replacer
// @description     Replaces NVIDIA's AAC encoder with a custom in-process encoder.
// @version         0.4.1
// @author          Mugnum
// @include         nvcontainer.exe
// ==/WindhawkMod==

// ==WindhawkModSettings==
/*
- BridgeDllPath: '%LOCALAPPDATA%\ShadowPlayAudioCodecReplacer\ShadowPlayAudioBridge.dll'
  $name: Bridge DLL path
  $description: Full path to ShadowPlayAudioBridge.dll.

- BitrateKbps: 384
  $name: AAC bitrate per track (kb/s)
  $description: >
    AAC-LC bitrate for each ShadowPlay audio stream. Allowed values are
    64 through 576 kb/s, in 8 kb/s increments. Restart nvcontainer.exe
    after changing this setting.
*/
// ==/WindhawkModSettings==

#include <windows.h>
#include <objbase.h>
#include <string>
#include <utility>

using CoCreateInstance_t = HRESULT(WINAPI*)(
    REFCLSID classId,
    LPUNKNOWN outer,
    DWORD context,
    REFIID requestedInterface,
    LPVOID* result);

using ConfigureReplacementAudioEncoder_t = HRESULT(WINAPI*)(
    UINT32 bitrateKbps);

using CreateReplacementAudioEncoder_t = HRESULT(WINAPI*)(
    IUnknown* originalEncoder,
    REFIID requestedInterface,
    void** result);

static CoCreateInstance_t CoCreateInstance_Original = nullptr;
static HMODULE g_bridgeModule = nullptr;
static CreateReplacementAudioEncoder_t
g_createReplacementAudioEncoder = nullptr;

// Microsoft AAC encoder:
// {93AF0C51-2275-45D2-A35B-F2BA21CAED00}
static const CLSID kMicrosoftAacEncoderClsid =
{
    0x93AF0C51,
    0x2275,
    0x45D2,
    { 0xA3, 0x5B, 0xF2, 0xBA, 0x21, 0xCA, 0xED, 0x00 }
};

bool TryExpandEnvironmentVariables(
    const std::wstring& value,
    std::wstring& expandedValue)
{
    DWORD requiredCharacters = ExpandEnvironmentStringsW(value.c_str(), nullptr, 0);

    if (requiredCharacters == 0)
    {
        Wh_Log(L"[Audio bridge] Failed to expand environment variables. Error=%lu", GetLastError());
        return false;
    }

    std::wstring buffer(requiredCharacters, L'\0');
    DWORD writtenCharacters = ExpandEnvironmentStringsW(value.c_str(), buffer.data(), requiredCharacters);

    if (writtenCharacters == 0 || writtenCharacters != requiredCharacters)
    {
        Wh_Log(L"[Audio bridge] Failed to expand environment variables. Error=%lu", GetLastError());
        return false;
    }

    // ExpandEnvironmentStringsW includes the terminating null in its count.
    buffer.resize(requiredCharacters - 1);
    expandedValue = std::move(buffer);

    return true;
}

bool LoadAndConfigureBridge()
{
    PCWSTR configuredPath = Wh_GetStringSetting(L"BridgeDllPath");

    if (!configuredPath || !*configuredPath)
    {
        Wh_Log(L"[Audio bridge] BridgeDllPath is empty");
        return false;
    }

    std::wstring bridgeDllPath;
    std::wstring configuredBridgeDllPath = configuredPath;
    Wh_FreeStringSetting(configuredPath);

    if (!TryExpandEnvironmentVariables(configuredBridgeDllPath, bridgeDllPath))
    {
        return false;
    }

    int bitrateKbps = Wh_GetIntSetting(L"BitrateKbps");

    if (bitrateKbps < 64 || bitrateKbps > 576 || (bitrateKbps % 8) != 0)
    {
        Wh_Log(L"[Audio bridge] Invalid BitrateKbps=%d. " L"Use 64-576 in 8 kb/s increments.", bitrateKbps);
        return false;
    }

    g_bridgeModule = LoadLibraryW(bridgeDllPath.c_str());

    if (!g_bridgeModule)
    {
        Wh_Log(L"[Audio bridge] Failed to load bridge DLL. " L"Path=%s Error=%lu",
            bridgeDllPath.c_str(), GetLastError());

        return false;
    }

    auto ConfigureReplacementAudioEncoder = reinterpret_cast<ConfigureReplacementAudioEncoder_t>(
        GetProcAddress(g_bridgeModule, "ConfigureReplacementAudioEncoder"));

    g_createReplacementAudioEncoder = reinterpret_cast<CreateReplacementAudioEncoder_t>(
        GetProcAddress(g_bridgeModule, "CreateReplacementAudioEncoder"));

    if (!ConfigureReplacementAudioEncoder || !g_createReplacementAudioEncoder)
    {
        Wh_Log(L"[Audio bridge] Required bridge export was not found");
        return false;
    }

    HRESULT configurationResult = ConfigureReplacementAudioEncoder(static_cast<UINT32>(bitrateKbps));

    if (FAILED(configurationResult))
    {
        Wh_Log(L"[Audio bridge] Bitrate configuration failed. " L"HRESULT=0x%08X", configurationResult);
        return false;
    }

    Wh_Log(L"[Audio bridge] Bridge configured for %d kb/s per audio track", bitrateKbps);
    return true;
}

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
        result);

    if (FAILED(hr) ||
        !result ||
        !*result ||
        !IsEqualCLSID(classId, kMicrosoftAacEncoderClsid))
    {
        return hr;
    }

    void* replacement = nullptr;

    HRESULT replacementResult = g_createReplacementAudioEncoder(
        static_cast<IUnknown*>(*result), requestedInterface, &replacement);

    if (FAILED(replacementResult) || !replacement)
    {
        Wh_Log(L"[Audio bridge] Bridge declined replacement. " L"HRESULT=0x%08X", replacementResult);
        return hr;
    }

    static_cast<IUnknown*>(*result)->Release();
    *result = replacement;
    Wh_Log(L"[Audio bridge] AAC encoder instance replaced");

    return S_OK;
}

BOOL Wh_ModInit()
{
    if (!LoadAndConfigureBridge())
    {
        return FALSE;
    }

    HMODULE combase = GetModuleHandleW(L"combase.dll");

    if (!combase)
    {
        Wh_Log(L"[Audio bridge] combase.dll is not loaded");
        return FALSE;
    }

    void* target = reinterpret_cast<void*>(GetProcAddress(combase, "CoCreateInstance"));

    if (!target)
    {
        Wh_Log(L"[Audio bridge] Could not locate CoCreateInstance");
        return FALSE;
    }

    if (!Wh_SetFunctionHook(
        target,
        reinterpret_cast<void*>(CoCreateInstance_Hook),
        reinterpret_cast<void**>(&CoCreateInstance_Original)))
    {
        Wh_Log(L"[Audio bridge] Failed to hook CoCreateInstance");
        return FALSE;
    }

    Wh_Log(L"[Audio bridge] CoCreateInstance hook installed");
    return TRUE;
}
