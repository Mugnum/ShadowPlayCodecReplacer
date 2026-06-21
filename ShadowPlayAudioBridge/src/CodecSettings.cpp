#include "CodecSettings.h"

#include <mutex>

namespace ShadowPlayAudioBridge::Settings
{
    namespace
    {
        std::mutex g_settingsMutex;

        std::uint32_t g_bitrateKbps =
            kDefaultBitrateKbps;

        bool g_bitrateWasConfigured = false;

        bool IsValidBitrateKbps(
            std::uint32_t bitrateKbps) noexcept
        {
            return bitrateKbps >= kMinimumBitrateKbps &&
                bitrateKbps <= kMaximumBitrateKbps &&
                (bitrateKbps % kBitrateStepKbps) == 0;
        }
    }

    HRESULT ConfigureBitrateKbps(
        std::uint32_t bitrateKbps) noexcept
    {
        if (!IsValidBitrateKbps(bitrateKbps))
        {
            return E_INVALIDARG;
        }

        std::lock_guard<std::mutex> lock(g_settingsMutex);

        if (g_bitrateWasConfigured &&
            g_bitrateKbps != bitrateKbps)
        {
            return HRESULT_FROM_WIN32(
                ERROR_ALREADY_INITIALIZED);
        }

        g_bitrateKbps = bitrateKbps;
        g_bitrateWasConfigured = true;

        return S_OK;
    }

    std::uint32_t GetBitrateBitsPerSecond() noexcept
    {
        std::lock_guard<std::mutex> lock(g_settingsMutex);

        return g_bitrateKbps * 1000;
    }

    std::uint32_t GetAverageBytesPerSecond() noexcept
    {
        return GetBitrateBitsPerSecond() / 8;
    }
}