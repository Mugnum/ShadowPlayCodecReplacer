#include "AudioFormat.h"
#include "CodecSettings.h"

#include <mfapi.h>
#include <mferror.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

namespace ShadowPlayAudioBridge
{
	namespace
	{
		HRESULT ValidateGuid(
			IMFMediaType* mediaType,
			REFGUID attribute,
			REFGUID expectedValue)
		{
			GUID value = {};

			HRESULT result = mediaType->GetGUID(attribute, &value);

			if (FAILED(result) || value != expectedValue)
			{
				return MF_E_INVALIDTYPE;
			}

			return S_OK;
		}

		HRESULT ValidateUInt32(
			IMFMediaType* mediaType,
			REFGUID attribute,
			UINT32 expectedValue)
		{
			UINT32 value = 0;

			HRESULT result = mediaType->GetUINT32(attribute, &value);

			if (FAILED(result) || value != expectedValue)
			{
				return MF_E_INVALIDTYPE;
			}

			return S_OK;
		}
	}

	HRESULT CreatePcmInputType(IMFMediaType** result)
	{
		if (!result)
		{
			return E_POINTER;
		}

		*result = nullptr;

		IMFMediaType* mediaType = nullptr;

		HRESULT hr = MFCreateMediaType(&mediaType);

		if (SUCCEEDED(hr))
		{
			hr = mediaType->SetGUID(
				MF_MT_MAJOR_TYPE,
				MFMediaType_Audio);
		}

		if (SUCCEEDED(hr))
		{
			hr = mediaType->SetGUID(
				MF_MT_SUBTYPE,
				MFAudioFormat_PCM);
		}

		if (SUCCEEDED(hr))
		{
			hr = mediaType->SetUINT32(
				MF_MT_AUDIO_SAMPLES_PER_SECOND,
				Settings::kSampleRate);
		}

		if (SUCCEEDED(hr))
		{
			hr = mediaType->SetUINT32(
				MF_MT_AUDIO_NUM_CHANNELS,
				Settings::kChannels);
		}

		if (SUCCEEDED(hr))
		{
			hr = mediaType->SetUINT32(
				MF_MT_AUDIO_BITS_PER_SAMPLE,
				Settings::kBitsPerSample);
		}

		if (SUCCEEDED(hr))
		{
			hr = mediaType->SetUINT32(
				MF_MT_AUDIO_BLOCK_ALIGNMENT,
				Settings::kBlockAlignment);
		}

		if (SUCCEEDED(hr))
		{
			hr = mediaType->SetUINT32(
				MF_MT_AUDIO_AVG_BYTES_PER_SECOND,
				Settings::kSampleRate * Settings::kBlockAlignment);
		}

		if (FAILED(hr))
		{
			mediaType->Release();
			return hr;
		}

		*result = mediaType;
		return S_OK;
	}

	HRESULT CreateAacOutputType(IMFMediaType** result)
	{
		if (!result)
		{
			return E_POINTER;
		}

		*result = nullptr;

		IMFMediaType* mediaType = nullptr;

		HRESULT hr = MFCreateMediaType(&mediaType);

		if (SUCCEEDED(hr))
		{
			hr = mediaType->SetGUID(
				MF_MT_MAJOR_TYPE,
				MFMediaType_Audio);
		}

		if (SUCCEEDED(hr))
		{
			hr = mediaType->SetGUID(
				MF_MT_SUBTYPE,
				MFAudioFormat_AAC);
		}

		if (SUCCEEDED(hr))
		{
			hr = mediaType->SetUINT32(
				MF_MT_AUDIO_SAMPLES_PER_SECOND,
				Settings::kSampleRate);
		}

		if (SUCCEEDED(hr))
		{
			hr = mediaType->SetUINT32(
				MF_MT_AUDIO_NUM_CHANNELS,
				Settings::kChannels);
		}

		if (SUCCEEDED(hr))
		{
			hr = mediaType->SetUINT32(
				MF_MT_AVG_BITRATE,
				Settings::GetBitrateBitsPerSecond());
		}

		if (SUCCEEDED(hr))
		{
			hr = mediaType->SetUINT32(
				MF_MT_AUDIO_AVG_BYTES_PER_SECOND,
				Settings::GetAverageBytesPerSecond());
		}

		if (SUCCEEDED(hr))
		{
			hr = mediaType->SetUINT32(
				MF_MT_AAC_PAYLOAD_TYPE,
				0);
		}

		if (FAILED(hr))
		{
			mediaType->Release();
			return hr;
		}

		*result = mediaType;
		return S_OK;
	}

	HRESULT ValidatePcmInputType(IMFMediaType* mediaType)
	{
		if (!mediaType)
		{
			return E_POINTER;
		}

		HRESULT hr = ValidateGuid(
			mediaType,
			MF_MT_MAJOR_TYPE,
			MFMediaType_Audio);

		if (SUCCEEDED(hr))
		{
			hr = ValidateGuid(
				mediaType,
				MF_MT_SUBTYPE,
				MFAudioFormat_PCM);
		}

		if (SUCCEEDED(hr))
		{
			hr = ValidateUInt32(
				mediaType,
				MF_MT_AUDIO_SAMPLES_PER_SECOND,
				Settings::kSampleRate);
		}

		if (SUCCEEDED(hr))
		{
			hr = ValidateUInt32(
				mediaType,
				MF_MT_AUDIO_NUM_CHANNELS,
				Settings::kChannels);
		}

		if (SUCCEEDED(hr))
		{
			hr = ValidateUInt32(
				mediaType,
				MF_MT_AUDIO_BITS_PER_SAMPLE,
				Settings::kBitsPerSample);
		}

		return hr;
	}

	HRESULT ValidateAacOutputType(IMFMediaType* mediaType)
	{
		if (!mediaType)
		{
			return E_POINTER;
		}

		HRESULT hr = ValidateGuid(
			mediaType,
			MF_MT_MAJOR_TYPE,
			MFMediaType_Audio);

		if (SUCCEEDED(hr))
		{
			hr = ValidateGuid(
				mediaType,
				MF_MT_SUBTYPE,
				MFAudioFormat_AAC);
		}

		if (SUCCEEDED(hr))
		{
			hr = ValidateUInt32(
				mediaType,
				MF_MT_AUDIO_SAMPLES_PER_SECOND,
				Settings::kSampleRate);
		}

		if (SUCCEEDED(hr))
		{
			hr = ValidateUInt32(
				mediaType,
				MF_MT_AUDIO_NUM_CHANNELS,
				Settings::kChannels);
		}

		return hr;
	}

	HRESULT CreateConfiguredAacOutputType(
		IMFMediaType* requestedType,
		IMFMediaType** result)
	{
		if (!result)
		{
			return E_POINTER;
		}

		*result = nullptr;

		HRESULT hr = ValidateAacOutputType(requestedType);

		if (FAILED(hr))
		{
			return hr;
		}

		IMFMediaType* configuredType = nullptr;

		hr = MFCreateMediaType(&configuredType);

		if (SUCCEEDED(hr))
		{
			hr = requestedType->CopyAllItems(configuredType);
		}

		if (SUCCEEDED(hr))
		{
			hr = configuredType->SetUINT32(
				MF_MT_AVG_BITRATE,
				Settings::GetBitrateBitsPerSecond());
		}

		if (SUCCEEDED(hr))
		{
			hr = configuredType->SetUINT32(
				MF_MT_AUDIO_AVG_BYTES_PER_SECOND,
				Settings::GetAverageBytesPerSecond());
		}

		if (SUCCEEDED(hr))
		{
			hr = configuredType->SetUINT32(
				MF_MT_AAC_PAYLOAD_TYPE,
				0);
		}

		if (FAILED(hr))
		{
			configuredType->Release();
			return hr;
		}

		*result = configuredType;
		return S_OK;
	}
}