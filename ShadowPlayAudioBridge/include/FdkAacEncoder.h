#pragma once

#include <windows.h>
#include <aacenc_lib.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ShadowPlayAudioBridge
{
	struct FdkEncodeResult
	{
		std::size_t consumedInputBytes = 0;
		std::vector<std::uint8_t> payload;
	};

	class FdkAacEncoder final
	{
	public:
		FdkAacEncoder() = default;
		~FdkAacEncoder();

		FdkAacEncoder(const FdkAacEncoder&) = delete;
		FdkAacEncoder& operator=(const FdkAacEncoder&) = delete;

		/** Loads and initializes the AAC-LC encoder. */
		HRESULT Initialize();

		/** Closes the active encoder while retaining the loaded runtime. */
		void Reset() noexcept;

		/** Encodes buffered PCM and returns consumed input plus AAC payload. */
		HRESULT Encode(
			const std::uint8_t* input,
			std::size_t inputBytes,
			FdkEncodeResult& result);

	private:
		bool LoadRuntime();
		void UnloadRuntime() noexcept;

		HMODULE module_ = nullptr;
		HANDLE_AACENCODER encoder_ = nullptr;

		decltype(&aacEncOpen) open_ = nullptr;
		decltype(&aacEncoder_SetParam) setParameter_ = nullptr;
		decltype(&aacEncEncode) encode_ = nullptr;
		decltype(&aacEncClose) close_ = nullptr;
	};
}