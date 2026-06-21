#pragma once

#include <windows.h>
#include <mfidl.h>

namespace ShadowPlayAudioBridge
{
	/** Creates the fixed PCM input type accepted by the bridge. */
	HRESULT CreatePcmInputType(IMFMediaType** result);

	/** Creates the fixed AAC output type advertised by the bridge. */
	HRESULT CreateAacOutputType(IMFMediaType** result);

	/** Validates NVIDIA's PCM input media type. */
	HRESULT ValidatePcmInputType(IMFMediaType* mediaType);

	/** Validates NVIDIA's AAC output media type. */
	HRESULT ValidateAacOutputType(IMFMediaType* mediaType);

	/** Copies NVIDIA's AAC metadata and applies bridge bitrate settings. */
	HRESULT CreateConfiguredAacOutputType(
		IMFMediaType* requestedType,
		IMFMediaType** result);
}