#pragma once

#include <cstddef>
#include <cstdint>
#include <windows.h>

namespace ShadowPlayAudioBridge::Settings
{
    inline constexpr bool kEnableReplacement = true;

    /** Fixed NVIDIA PCM input format currently supported by the bridge. */
    inline constexpr std::uint32_t kSampleRate = 48000;
    inline constexpr std::uint32_t kChannels = 2;
    inline constexpr std::uint32_t kBitsPerSample = 16;
    inline constexpr std::uint32_t kBlockAlignment =
        kChannels * (kBitsPerSample / 8);

    /** Default AAC bitrate used when Windhawk does not override it. */
    inline constexpr std::uint32_t kDefaultBitrateKbps = 384;

    /** Supported public bitrate range, per encoded AAC stream. */
    inline constexpr std::uint32_t kMinimumBitrateKbps = 64;
    inline constexpr std::uint32_t kMaximumBitrateKbps = 576;
    inline constexpr std::uint32_t kBitrateStepKbps = 8;

    inline constexpr std::int64_t kAacFrameDuration100ns =
        (1024LL * 10000000LL) / kSampleRate;

    inline constexpr std::size_t kEncodedPacketBufferBytes = 2048;

    /**
     * Configures the AAC bitrate before the first replacement encoder is created.
     *
     * @param bitrateKbps Bitrate in kilobits per second.
     * @return S_OK for a valid setting; E_INVALIDARG for an unsupported value.
     */
    HRESULT ConfigureBitrateKbps(
        std::uint32_t bitrateKbps) noexcept;

    /** Returns the configured bitrate in bits per second. */
    std::uint32_t GetBitrateBitsPerSecond() noexcept;

    /** Returns the configured bitrate in bytes per second. */
    std::uint32_t GetAverageBytesPerSecond() noexcept;
}