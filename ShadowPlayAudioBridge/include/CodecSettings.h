#pragma once

#include <cstddef>
#include <cstdint>

namespace ShadowPlayAudioBridge::Settings
{
	inline constexpr bool kEnableReplacement = true;

	inline constexpr std::uint32_t kSampleRate = 48000;
	inline constexpr std::uint32_t kChannels = 2;
	inline constexpr std::uint32_t kBitsPerSample = 16;
	inline constexpr std::uint32_t kBlockAlignment = 4;

	inline constexpr std::uint32_t kBitrate = 384000;
	inline constexpr std::uint32_t kAverageBytesPerSecond = kBitrate / 8;

	inline constexpr std::int64_t kAacFrameDuration100ns =
		(1024LL * 10000000LL) / kSampleRate;

	inline constexpr std::size_t kEncodedPacketBufferBytes = 2048;
}