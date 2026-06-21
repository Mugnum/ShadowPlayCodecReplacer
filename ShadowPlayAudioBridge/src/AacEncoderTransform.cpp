#include "AacEncoderTransform.h"

#include "AudioFormat.h"
#include "CodecSettings.h"

#include <cstring>
#include <new>
#include <utility>

#include <mferror.h>

namespace ShadowPlayAudioBridge
{
	HRESULT AacEncoderTransform::Create(
		REFIID requestedInterface,
		void** result)
	{
		if (!result)
		{
			return E_POINTER;
		}

		*result = nullptr;

		auto* transform = new (std::nothrow) AacEncoderTransform();

		if (!transform)
		{
			return E_OUTOFMEMORY;
		}

		HRESULT hr = transform->encoder_.Initialize();

		if (SUCCEEDED(hr))
		{
			hr = transform->QueryInterface(
				requestedInterface,
				result);
		}

		transform->Release();

		return hr;
	}

	AacEncoderTransform::~AacEncoderTransform()
	{
		if (inputType_)
		{
			inputType_->Release();
		}

		if (outputType_)
		{
			outputType_->Release();
		}
	}

	HRESULT AacEncoderTransform::QueryInterface(
		REFIID riid,
		void** result)
	{
		if (!result)
		{
			return E_POINTER;
		}

		*result = nullptr;

		if (riid == IID_IUnknown ||
			riid == __uuidof(IMFTransform))
		{
			*result = static_cast<IMFTransform*>(this);
			AddRef();
			return S_OK;
		}

		return E_NOINTERFACE;
	}

	ULONG AacEncoderTransform::AddRef()
	{
		return static_cast<ULONG>(
			InterlockedIncrement(&referenceCount_));
	}

	ULONG AacEncoderTransform::Release()
	{
		ULONG count = static_cast<ULONG>(
			InterlockedDecrement(&referenceCount_));

		if (count == 0)
		{
			delete this;
		}

		return count;
	}

	HRESULT AacEncoderTransform::GetStreamLimits(
		DWORD* inputMinimum,
		DWORD* inputMaximum,
		DWORD* outputMinimum,
		DWORD* outputMaximum)
	{
		if (!inputMinimum || !inputMaximum ||
			!outputMinimum || !outputMaximum)
		{
			return E_POINTER;
		}

		*inputMinimum = 1;
		*inputMaximum = 1;
		*outputMinimum = 1;
		*outputMaximum = 1;

		return S_OK;
	}

	HRESULT AacEncoderTransform::GetStreamCount(
		DWORD* inputStreams,
		DWORD* outputStreams)
	{
		if (!inputStreams || !outputStreams)
		{
			return E_POINTER;
		}

		*inputStreams = 1;
		*outputStreams = 1;

		return S_OK;
	}

	HRESULT AacEncoderTransform::GetStreamIDs(
		DWORD,
		DWORD*,
		DWORD,
		DWORD*)
	{
		return E_NOTIMPL;
	}

	HRESULT AacEncoderTransform::GetInputStreamInfo(
		DWORD streamId,
		MFT_INPUT_STREAM_INFO* streamInfo)
	{
		if (streamId != 0)
		{
			return MF_E_INVALIDSTREAMNUMBER;
		}

		if (!streamInfo)
		{
			return E_POINTER;
		}

		ZeroMemory(streamInfo, sizeof(*streamInfo));

		streamInfo->cbSize = 1920;
		streamInfo->cbAlignment = 1;
		streamInfo->dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES;

		return S_OK;
	}

	HRESULT AacEncoderTransform::GetOutputStreamInfo(
		DWORD streamId,
		MFT_OUTPUT_STREAM_INFO* streamInfo)
	{
		if (streamId != 0)
		{
			return MF_E_INVALIDSTREAMNUMBER;
		}

		if (!streamInfo)
		{
			return E_POINTER;
		}

		ZeroMemory(streamInfo, sizeof(*streamInfo));

		streamInfo->cbSize = static_cast<DWORD>(
			Settings::kEncodedPacketBufferBytes);

		streamInfo->cbAlignment = 1;

		return S_OK;
	}

	HRESULT AacEncoderTransform::GetAttributes(
		IMFAttributes** attributes)
	{
		if (!attributes)
		{
			return E_POINTER;
		}

		*attributes = nullptr;
		return E_NOTIMPL;
	}

	HRESULT AacEncoderTransform::GetInputStreamAttributes(
		DWORD,
		IMFAttributes** attributes)
	{
		if (!attributes)
		{
			return E_POINTER;
		}

		*attributes = nullptr;
		return E_NOTIMPL;
	}

	HRESULT AacEncoderTransform::GetOutputStreamAttributes(
		DWORD,
		IMFAttributes** attributes)
	{
		if (!attributes)
		{
			return E_POINTER;
		}

		*attributes = nullptr;
		return E_NOTIMPL;
	}

	HRESULT AacEncoderTransform::DeleteInputStream(
		DWORD)
	{
		return E_NOTIMPL;
	}

	HRESULT AacEncoderTransform::AddInputStreams(
		DWORD,
		DWORD*)
	{
		return E_NOTIMPL;
	}

	HRESULT AacEncoderTransform::GetInputAvailableType(
		DWORD streamId,
		DWORD typeIndex,
		IMFMediaType** mediaType)
	{
		if (streamId != 0)
		{
			return MF_E_INVALIDSTREAMNUMBER;
		}

		if (typeIndex != 0)
		{
			return MF_E_NO_MORE_TYPES;
		}

		return CreatePcmInputType(mediaType);
	}

	HRESULT AacEncoderTransform::GetOutputAvailableType(
		DWORD streamId,
		DWORD typeIndex,
		IMFMediaType** mediaType)
	{
		if (streamId != 0)
		{
			return MF_E_INVALIDSTREAMNUMBER;
		}

		if (typeIndex != 0)
		{
			return MF_E_NO_MORE_TYPES;
		}

		return CreateAacOutputType(mediaType);
	}

	HRESULT AacEncoderTransform::SetInputType(
		DWORD streamId,
		IMFMediaType* mediaType,
		DWORD flags)
	{
		if (streamId != 0)
		{
			return MF_E_INVALIDSTREAMNUMBER;
		}

		if (!mediaType)
		{
			if ((flags & MFT_SET_TYPE_TEST_ONLY) != 0)
			{
				return S_OK;
			}

			if (inputType_)
			{
				inputType_->Release();
				inputType_ = nullptr;
			}

			ResetStreamState();
			return S_OK;
		}

		HRESULT hr = ValidatePcmInputType(mediaType);

		if (FAILED(hr))
		{
			return hr;
		}

		if ((flags & MFT_SET_TYPE_TEST_ONLY) != 0)
		{
			return S_OK;
		}

		mediaType->AddRef();

		if (inputType_)
		{
			inputType_->Release();
		}

		inputType_ = mediaType;

		return S_OK;
	}

	HRESULT AacEncoderTransform::SetOutputType(
		DWORD streamId,
		IMFMediaType* mediaType,
		DWORD flags)
	{
		if (streamId != 0)
		{
			return MF_E_INVALIDSTREAMNUMBER;
		}

		if (!mediaType)
		{
			if ((flags & MFT_SET_TYPE_TEST_ONLY) != 0)
			{
				return S_OK;
			}

			encoder_.Reset();
			ResetStreamState();

			if (outputType_)
			{
				outputType_->Release();
				outputType_ = nullptr;
			}

			return S_OK;
		}

		if ((flags & MFT_SET_TYPE_TEST_ONLY) != 0)
		{
			return ValidateAacOutputType(mediaType);
		}

		IMFMediaType* configuredType = nullptr;

		HRESULT hr = CreateConfiguredAacOutputType(
			mediaType,
			&configuredType);

		if (FAILED(hr))
		{
			return hr;
		}

		encoder_.Reset();

		hr = encoder_.Initialize();

		if (FAILED(hr))
		{
			configuredType->Release();
			return hr;
		}

		ResetStreamState();

		if (outputType_)
		{
			outputType_->Release();
		}

		outputType_ = configuredType;

		return S_OK;
	}

	HRESULT AacEncoderTransform::GetInputCurrentType(
		DWORD streamId,
		IMFMediaType** mediaType)
	{
		if (!mediaType)
		{
			return E_POINTER;
		}

		*mediaType = nullptr;

		if (streamId != 0)
		{
			return MF_E_INVALIDSTREAMNUMBER;
		}

		if (!inputType_)
		{
			return MF_E_TRANSFORM_TYPE_NOT_SET;
		}

		inputType_->AddRef();
		*mediaType = inputType_;

		return S_OK;
	}

	HRESULT AacEncoderTransform::GetOutputCurrentType(
		DWORD streamId,
		IMFMediaType** mediaType)
	{
		if (!mediaType)
		{
			return E_POINTER;
		}

		*mediaType = nullptr;

		if (streamId != 0)
		{
			return MF_E_INVALIDSTREAMNUMBER;
		}

		if (!outputType_)
		{
			return MF_E_TRANSFORM_TYPE_NOT_SET;
		}

		outputType_->AddRef();
		*mediaType = outputType_;

		return S_OK;
	}

	HRESULT AacEncoderTransform::GetInputStatus(
		DWORD streamId,
		DWORD* flags)
	{
		if (streamId != 0)
		{
			return MF_E_INVALIDSTREAMNUMBER;
		}

		if (!flags)
		{
			return E_POINTER;
		}

		*flags = outputPackets_.empty()
			? MFT_INPUT_STATUS_ACCEPT_DATA
			: 0;

		return S_OK;
	}

	HRESULT AacEncoderTransform::GetOutputStatus(
		DWORD* flags)
	{
		if (!flags)
		{
			return E_POINTER;
		}

		*flags = outputPackets_.empty()
			? 0
			: MFT_OUTPUT_STATUS_SAMPLE_READY;

		return S_OK;
	}

	HRESULT AacEncoderTransform::SetOutputBounds(
		LONGLONG,
		LONGLONG)
	{
		return E_NOTIMPL;
	}

	HRESULT AacEncoderTransform::ProcessEvent(
		DWORD,
		IMFMediaEvent*)
	{
		return E_NOTIMPL;
	}

	HRESULT AacEncoderTransform::ProcessMessage(
		MFT_MESSAGE_TYPE message,
		ULONG_PTR)
	{
		if (message == MFT_MESSAGE_COMMAND_FLUSH)
		{
			ResetStreamState();
		}

		return S_OK;
	}

	HRESULT AacEncoderTransform::ProcessInput(
		DWORD streamId,
		IMFSample* sample,
		DWORD flags)
	{
		if (streamId != 0)
		{
			return MF_E_INVALIDSTREAMNUMBER;
		}

		if (!sample || flags != 0)
		{
			return E_INVALIDARG;
		}

		if (!inputType_ || !outputType_)
		{
			return MF_E_TRANSFORM_TYPE_NOT_SET;
		}

		if (!outputPackets_.empty())
		{
			return MF_E_NOTACCEPTING;
		}

		IMFMediaBuffer* mediaBuffer = nullptr;

		HRESULT hr = sample->ConvertToContiguousBuffer(
			&mediaBuffer);

		if (FAILED(hr))
		{
			return hr;
		}

		BYTE* data = nullptr;
		DWORD maximumLength = 0;
		DWORD currentLength = 0;

		hr = mediaBuffer->Lock(
			&data,
			&maximumLength,
			&currentLength);

		if (FAILED(hr))
		{
			mediaBuffer->Release();
			return hr;
		}

		if ((currentLength % Settings::kBlockAlignment) != 0)
		{
			mediaBuffer->Unlock();
			mediaBuffer->Release();
			return E_INVALIDARG;
		}

		if (!hasTimestamp_)
		{
			LONGLONG inputTimestamp = 0;

			if (SUCCEEDED(sample->GetSampleTime(
				&inputTimestamp)))
			{
				nextTimestamp_ = inputTimestamp;
				hasTimestamp_ = true;
			}
		}

		pcmBuffer_.insert(
			pcmBuffer_.end(),
			data,
			data + currentLength);

		mediaBuffer->Unlock();
		mediaBuffer->Release();

		return EncodeBufferedPcm();
	}

	HRESULT AacEncoderTransform::ProcessOutput(
		DWORD flags,
		DWORD outputBufferCount,
		MFT_OUTPUT_DATA_BUFFER* outputBuffers,
		DWORD* status)
	{
		if (flags != 0)
		{
			return E_INVALIDARG;
		}

		if (!outputBuffers || outputBufferCount != 1)
		{
			return E_INVALIDARG;
		}

		if (status)
		{
			*status = 0;
		}

		if (outputPackets_.empty())
		{
			return MF_E_TRANSFORM_NEED_MORE_INPUT;
		}

		IMFSample* sample = outputBuffers[0].pSample;

		if (!sample)
		{
			return E_INVALIDARG;
		}

		const EncodedAudioPacket& packet =
			outputPackets_.front();

		IMFMediaBuffer* mediaBuffer = nullptr;

		HRESULT hr = sample->ConvertToContiguousBuffer(
			&mediaBuffer);

		if (FAILED(hr))
		{
			return hr;
		}

		BYTE* destination = nullptr;
		DWORD maximumLength = 0;
		DWORD currentLength = 0;

		hr = mediaBuffer->Lock(
			&destination,
			&maximumLength,
			&currentLength);

		if (FAILED(hr))
		{
			mediaBuffer->Release();
			return hr;
		}

		if (maximumLength < packet.payload.size())
		{
			mediaBuffer->Unlock();
			mediaBuffer->Release();
			return MF_E_BUFFERTOOSMALL;
		}

		std::memcpy(
			destination,
			packet.payload.data(),
			packet.payload.size());

		hr = mediaBuffer->SetCurrentLength(
			static_cast<DWORD>(
				packet.payload.size()));

		mediaBuffer->Unlock();
		mediaBuffer->Release();

		if (FAILED(hr))
		{
			return hr;
		}

		if (packet.hasTimestamp)
		{
			sample->SetSampleTime(packet.timestamp);
		}

		sample->SetSampleDuration(packet.duration);

		outputPackets_.pop_front();

		outputBuffers[0].dwStatus =
			outputPackets_.empty()
			? 0
			: MFT_OUTPUT_DATA_BUFFER_INCOMPLETE;

		return S_OK;
	}

	HRESULT AacEncoderTransform::EncodeBufferedPcm()
	{
		while (!pcmBuffer_.empty())
		{
			FdkEncodeResult encoded = {};

			HRESULT hr = encoder_.Encode(
				pcmBuffer_.data(),
				pcmBuffer_.size(),
				encoded);

			if (FAILED(hr))
			{
				return hr;
			}

			if (encoded.consumedInputBytes >
				pcmBuffer_.size())
			{
				return E_FAIL;
			}

			if (encoded.consumedInputBytes > 0)
			{
				pcmBuffer_.erase(
					pcmBuffer_.begin(),
					pcmBuffer_.begin() +
					encoded.consumedInputBytes);
			}

			if (!encoded.payload.empty())
			{
				EncodedAudioPacket packet = {};

				packet.payload = std::move(encoded.payload);
				packet.timestamp = nextTimestamp_;
				packet.duration = static_cast<LONGLONG>(
					Settings::kAacFrameDuration100ns);
				packet.hasTimestamp = hasTimestamp_;

				outputPackets_.push_back(std::move(packet));

				if (hasTimestamp_)
				{
					nextTimestamp_ += static_cast<LONGLONG>(
						Settings::kAacFrameDuration100ns);
				}

				return S_OK;
			}

			if (encoded.consumedInputBytes == 0)
			{
				return S_OK;
			}
		}

		return S_OK;
	}

	void AacEncoderTransform::ResetStreamState()
	{
		pcmBuffer_.clear();
		outputPackets_.clear();
		nextTimestamp_ = 0;
		hasTimestamp_ = false;
	}
}