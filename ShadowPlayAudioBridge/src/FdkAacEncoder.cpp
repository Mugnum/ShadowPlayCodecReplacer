#define NOMINMAX

#include "FdkAacEncoder.h"
#include "CodecSettings.h"

#include <array>
#include <limits>
#include <string>
#include <vector>

namespace ShadowPlayAudioBridge
{
	namespace
	{
		constexpr wchar_t kFdkRuntimeFileName[] = L"libfdk-aac-2.dll";

		void ModuleAnchor()
		{}

		std::wstring GetRuntimePath()
		{
			HMODULE module = nullptr;

			BOOL foundModule = GetModuleHandleExW(
				GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
				GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
				reinterpret_cast<LPCWSTR>(&ModuleAnchor),
				&module);

			if (!foundModule || !module)
			{
				return kFdkRuntimeFileName;
			}

			std::vector<wchar_t> modulePath(32768);

			DWORD length = GetModuleFileNameW(
				module,
				modulePath.data(),
				static_cast<DWORD>(modulePath.size())
			);

			if (length == 0 || length >= modulePath.size())
			{
				return kFdkRuntimeFileName;
			}

			std::wstring path(modulePath.data(), length);

			std::size_t separator = path.find_last_of(L"\\/");

			if (separator == std::wstring::npos)
			{
				return kFdkRuntimeFileName;
			}

			return path.substr(0, separator + 1) +
				kFdkRuntimeFileName;
		}
	}

	FdkAacEncoder::~FdkAacEncoder()
	{
		Reset();
		UnloadRuntime();
	}

	HRESULT FdkAacEncoder::Initialize()
	{
		if (encoder_)
		{
			return S_OK;
		}

		if (!LoadRuntime())
		{
			DWORD error = GetLastError();

			return error
				? HRESULT_FROM_WIN32(error)
				: E_FAIL;
		}

		AACENC_ERROR error = open_(
			&encoder_,
			0,
			Settings::kChannels);

		if (error != AACENC_OK)
		{
			encoder_ = nullptr;
			return E_FAIL;
		}

		auto set = [this](AACENC_PARAM parameter, UINT value)
			{
				return setParameter_(
					encoder_,
					parameter,
					value) == AACENC_OK;
			};

		bool configured =
			set(AACENC_AOT, 2) &&
			set(AACENC_SAMPLERATE, Settings::kSampleRate) &&
			set(AACENC_CHANNELMODE, MODE_2) &&
			set(AACENC_CHANNELORDER, 1) &&
			set(AACENC_BITRATE, Settings::GetBitrateBitsPerSecond()) &&
			set(AACENC_TRANSMUX, TT_MP4_RAW);

		if (!configured)
		{
			Reset();
			return E_FAIL;
		}

		error = encode_(
			encoder_,
			nullptr,
			nullptr,
			nullptr,
			nullptr);

		if (error != AACENC_OK)
		{
			Reset();
			return E_FAIL;
		}

		return S_OK;
	}

	void FdkAacEncoder::Reset() noexcept
	{
		if (encoder_ && close_)
		{
			close_(&encoder_);
		}

		encoder_ = nullptr;
	}

	HRESULT FdkAacEncoder::Encode(
		const std::uint8_t* input,
		std::size_t inputBytes,
		FdkEncodeResult& result)
	{
		result = {};

		if (!encoder_ || !input || inputBytes == 0)
		{
			return E_INVALIDARG;
		}

		if ((inputBytes % sizeof(INT_PCM)) != 0)
		{
			return E_INVALIDARG;
		}

		if (inputBytes > static_cast<std::size_t>(
			(std::numeric_limits<INT>::max)()))
		{
			return E_INVALIDARG;
		}

		void* inputBuffer = const_cast<std::uint8_t*>(input);

		INT inputIdentifier = IN_AUDIO_DATA;
		INT inputSize = static_cast<INT>(inputBytes);
		INT inputElementSize = sizeof(INT_PCM);

		AACENC_BufDesc inputDescription = {};
		inputDescription.numBufs = 1;
		inputDescription.bufs = &inputBuffer;
		inputDescription.bufferIdentifiers = &inputIdentifier;
		inputDescription.bufSizes = &inputSize;
		inputDescription.bufElSizes = &inputElementSize;

		std::array<UCHAR, Settings::kEncodedPacketBufferBytes> outputBytes = {};

		void* outputBuffer = outputBytes.data();

		INT outputIdentifier = OUT_BITSTREAM_DATA;
		INT outputSize = static_cast<INT>(outputBytes.size());
		INT outputElementSize = sizeof(UCHAR);

		AACENC_BufDesc outputDescription = {};
		outputDescription.numBufs = 1;
		outputDescription.bufs = &outputBuffer;
		outputDescription.bufferIdentifiers = &outputIdentifier;
		outputDescription.bufSizes = &outputSize;
		outputDescription.bufElSizes = &outputElementSize;

		AACENC_InArgs inputArguments = {};
		inputArguments.numInSamples = static_cast<INT>(
			inputBytes / sizeof(INT_PCM));

		AACENC_OutArgs outputArguments = {};

		AACENC_ERROR error = encode_(
			encoder_,
			&inputDescription,
			&outputDescription,
			&inputArguments,
			&outputArguments);

		if (error != AACENC_OK)
		{
			return E_FAIL;
		}

		if (outputArguments.numInSamples < 0 ||
			outputArguments.numOutBytes < 0)
		{
			return E_FAIL;
		}

		result.consumedInputBytes =
			static_cast<std::size_t>(
				outputArguments.numInSamples) *
			sizeof(INT_PCM);

		if (result.consumedInputBytes > inputBytes)
		{
			return E_FAIL;
		}

		if (static_cast<std::size_t>(
			outputArguments.numOutBytes) > outputBytes.size())
		{
			return E_FAIL;
		}

		result.payload.assign(
			outputBytes.begin(),
			outputBytes.begin() +
			outputArguments.numOutBytes);

		return S_OK;
	}

	bool FdkAacEncoder::LoadRuntime()
	{
		if (module_)
		{
			return true;
		}

		std::wstring runtimePath = GetRuntimePath();

		module_ = LoadLibraryExW(
			runtimePath.c_str(),
			nullptr,
			LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);

		if (!module_)
		{
			return false;
		}

		open_ = reinterpret_cast<decltype(open_)>(
			GetProcAddress(module_, "aacEncOpen"));

		setParameter_ = reinterpret_cast<decltype(setParameter_)>(
			GetProcAddress(module_, "aacEncoder_SetParam"));

		encode_ = reinterpret_cast<decltype(encode_)>(
			GetProcAddress(module_, "aacEncEncode"));

		close_ = reinterpret_cast<decltype(close_)>(
			GetProcAddress(module_, "aacEncClose"));

		if (!open_ || !setParameter_ || !encode_ || !close_)
		{
			UnloadRuntime();
			return false;
		}

		return true;
	}

	void FdkAacEncoder::UnloadRuntime() noexcept
	{
		if (module_)
		{
			FreeLibrary(module_);
		}

		module_ = nullptr;
		open_ = nullptr;
		setParameter_ = nullptr;
		encode_ = nullptr;
		close_ = nullptr;
	}
}