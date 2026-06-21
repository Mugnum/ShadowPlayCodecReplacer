#pragma once

#include <windows.h>
#include <mftransform.h>
#include <mfidl.h>

#include <cstdint>
#include <deque>
#include <vector>

#include "FdkAacEncoder.h"

namespace ShadowPlayAudioBridge
{
	class AacEncoderTransform final : public IMFTransform
	{
	public:
		/** Creates a configured transform through the requested COM interface. */
		static HRESULT Create(REFIID requestedInterface, void** result);

		HRESULT STDMETHODCALLTYPE QueryInterface(
			REFIID riid,
			void** result) override;

		ULONG STDMETHODCALLTYPE AddRef() override;
		ULONG STDMETHODCALLTYPE Release() override;

		HRESULT STDMETHODCALLTYPE GetStreamLimits(
			DWORD* inputMinimum,
			DWORD* inputMaximum,
			DWORD* outputMinimum,
			DWORD* outputMaximum) override;

		HRESULT STDMETHODCALLTYPE GetStreamCount(
			DWORD* inputStreams,
			DWORD* outputStreams) override;

		HRESULT STDMETHODCALLTYPE GetStreamIDs(
			DWORD inputIdCount,
			DWORD* inputIds,
			DWORD outputIdCount,
			DWORD* outputIds) override;

		HRESULT STDMETHODCALLTYPE GetInputStreamInfo(
			DWORD streamId,
			MFT_INPUT_STREAM_INFO* streamInfo) override;

		HRESULT STDMETHODCALLTYPE GetOutputStreamInfo(
			DWORD streamId,
			MFT_OUTPUT_STREAM_INFO* streamInfo) override;

		HRESULT STDMETHODCALLTYPE GetAttributes(
			IMFAttributes** attributes) override;

		HRESULT STDMETHODCALLTYPE GetInputStreamAttributes(
			DWORD streamId,
			IMFAttributes** attributes) override;

		HRESULT STDMETHODCALLTYPE GetOutputStreamAttributes(
			DWORD streamId,
			IMFAttributes** attributes) override;

		HRESULT STDMETHODCALLTYPE DeleteInputStream(
			DWORD streamId) override;

		HRESULT STDMETHODCALLTYPE AddInputStreams(
			DWORD streamCount,
			DWORD* streamIds) override;

		HRESULT STDMETHODCALLTYPE GetInputAvailableType(
			DWORD streamId,
			DWORD typeIndex,
			IMFMediaType** mediaType) override;

		HRESULT STDMETHODCALLTYPE GetOutputAvailableType(
			DWORD streamId,
			DWORD typeIndex,
			IMFMediaType** mediaType) override;

		HRESULT STDMETHODCALLTYPE SetInputType(
			DWORD streamId,
			IMFMediaType* mediaType,
			DWORD flags) override;

		HRESULT STDMETHODCALLTYPE SetOutputType(
			DWORD streamId,
			IMFMediaType* mediaType,
			DWORD flags) override;

		HRESULT STDMETHODCALLTYPE GetInputCurrentType(
			DWORD streamId,
			IMFMediaType** mediaType) override;

		HRESULT STDMETHODCALLTYPE GetOutputCurrentType(
			DWORD streamId,
			IMFMediaType** mediaType) override;

		HRESULT STDMETHODCALLTYPE GetInputStatus(
			DWORD streamId,
			DWORD* flags) override;

		HRESULT STDMETHODCALLTYPE GetOutputStatus(
			DWORD* flags) override;

		HRESULT STDMETHODCALLTYPE SetOutputBounds(
			LONGLONG lowerBound,
			LONGLONG upperBound) override;

		HRESULT STDMETHODCALLTYPE ProcessEvent(
			DWORD streamId,
			IMFMediaEvent* event) override;

		HRESULT STDMETHODCALLTYPE ProcessMessage(
			MFT_MESSAGE_TYPE message,
			ULONG_PTR parameter) override;

		HRESULT STDMETHODCALLTYPE ProcessInput(
			DWORD streamId,
			IMFSample* sample,
			DWORD flags) override;

		HRESULT STDMETHODCALLTYPE ProcessOutput(
			DWORD flags,
			DWORD outputBufferCount,
			MFT_OUTPUT_DATA_BUFFER* outputBuffers,
			DWORD* status) override;

	private:
		struct EncodedAudioPacket
		{
			std::vector<std::uint8_t> payload;
			LONGLONG timestamp = 0;
			LONGLONG duration = 0;
			bool hasTimestamp = false;
		};

		AacEncoderTransform() = default;
		~AacEncoderTransform();

		HRESULT EncodeBufferedPcm();
		void ResetStreamState();

		LONG referenceCount_ = 1;

		IMFMediaType* inputType_ = nullptr;
		IMFMediaType* outputType_ = nullptr;

		FdkAacEncoder encoder_;

		std::vector<std::uint8_t> pcmBuffer_;
		std::deque<EncodedAudioPacket> outputPackets_;

		LONGLONG nextTimestamp_ = 0;
		bool hasTimestamp_ = false;
	};
}