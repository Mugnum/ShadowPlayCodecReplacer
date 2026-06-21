#include <windows.h>
#include <unknwn.h>

#include "AacEncoderTransform.h"
#include "CodecSettings.h"

using ShadowPlayAudioBridge::AacEncoderTransform;
using ShadowPlayAudioBridge::Settings::kEnableReplacement;

extern "C" __declspec(dllexport)
HRESULT WINAPI ConfigureReplacementAudioEncoder(
	UINT32 bitrateKbps)
{
	return ShadowPlayAudioBridge::Settings::
		ConfigureBitrateKbps(bitrateKbps);
}

extern "C" __declspec(dllexport)
HRESULT WINAPI CreateReplacementAudioEncoder(
	IUnknown* originalEncoder,
	REFIID requestedInterface,
	void** result)
{
	if (!result)
	{
		return E_POINTER;
	}

	*result = nullptr;

	if (!originalEncoder)
	{
		return E_POINTER;
	}

	if (kEnableReplacement)
	{
		HRESULT replacementResult =
			AacEncoderTransform::Create(
				requestedInterface,
				result);

		if (SUCCEEDED(replacementResult))
		{
			return S_OK;
		}
	}

	return originalEncoder->QueryInterface(
		requestedInterface,
		result);
}

BOOL APIENTRY DllMain(
	HMODULE module,
	DWORD reason,
	LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(module);
	}

	return TRUE;
}