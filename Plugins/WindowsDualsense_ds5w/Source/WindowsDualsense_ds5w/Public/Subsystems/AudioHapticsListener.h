// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "AudioResampler.h"
#include "Containers/Queue.h"
#include "CoreMinimal.h"
#include "GCore/Interfaces/Segregations/IGamepadAudioHaptics.h"
#include "ISubmixBufferListener.h"

/**
 Class responsible for handling audio submix buffers and preparing audio data for haptic feedback systems.

 FAudioHapticsListener integrates with the audio rendering pipeline using the ISubmixBufferListener interface,
 allowing real-time access to submix audio buffers. The class supports processing, conversion, and resampling
 of audio data for use in haptic feedback hardware or systems. It includes mechanisms to manage resampling state
 and an internal queue for storing processed audio data.
 */
class FAudioHapticsListener : public ISubmixBufferListener
{
	/**
	 Constructor for the FAudioHapticsListener class, initializing the listener with the given input device ID
	 and audio submix reference.

	 @param InDeviceId The identifier for the input device associated with this haptics listener.
	 @param InSubmix Pointer to the USoundSubmix object that this listener processes audio data from.
	 @return An instance of FAudioHapticsListener initialized with the provided input device ID and submix reference.
	 */
public:
	FAudioHapticsListener(int32 InDeviceId, USoundSubmix* InSubmix, bool bIsWireless);

	/**
	 Determines if the audio processing system is actively rendering audio.

	 This method indicates whether the audio data rendering pipeline is currently active.
	 It is used to verify the state of the audio system and ensure that audio buffers
	 are being processed and rendered correctly.

	 @return True if the system is rendering audio; otherwise, false.
	 */
	virtual bool IsRenderingAudio() const override
	{
		return true;
	}

	/**
	 Processes and consumes the current audio data in the haptics queue, sending it to the appropriate haptic feedback interface.

	 This method retrieves audio packets from the internal queue and forwards them to a supported haptic feedback system, such as
	 the Sony DualSense gamepad, through the relevant interface. Packets that could not be processed are discarded after the final
	 flush of the queue.

	 It integrates with device-specific haptic systems using interfaces like ISonyGamepadTriggerInterface to achieve real-time
	 audio-haptic feedback conversion.
	 */
	void ConsumeHapticsQueue(IGamepadAudioHaptics* AudioHaptics);

	/**
	 Returns the associated audio submix instance.

	 This method retrieves the `USoundSubmix` object associated with the audio processing pipeline.
	 The submix serves as a source of mixed audio data, which is utilized in various processing
	 stages, including resampling and preparation for haptic feedback systems.

	 @return The `USoundSubmix` instance associated with the haptic feedback audio processing system.
	 */
	USoundSubmix* GetSubmix() const
	{
		return Submix;
	}

	/**
	Called when a new buffer has been rendered for a given submix
	@param OwningSubmix	The submix object which has rendered a new buffer
	@param AudioData		Ptr to the audio buffer
	@param NumSamples		The number of audio samples in the audio buffer
	@param NumChannels		The number of channels of audio in the buffer (e.g. 2 for stereo, 6 for 5.1)
	@param SampleRate		The sample rate of the audio buffer
	@param AudioClock		Double audio clock value, from Start of audio rendering.
	*/
	virtual void OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData, int32 NumSamples, int32 NumChannels, const int32 SampleRate, double AudioClock) override;
	/**
	 A thread-safe queue used for storing audio packets to be processed within the audio rendering pipeline.

	 AudioPacketQueue is implemented as a multiple-producer single-consumer (MPSC) queue, allowing audio packets
	 to be enqueued by multiple threads and dequeued by a single consumer thread. This design is optimized for
	 high-performance, concurrent audio processing workflows. The queue holds arrays of int8, which represent
	 raw audio data packets. These packets can be used for real-time processing, such as resampling or formatting
	 for haptic feedback systems.
	 */
private:
	TQueue<TArray<int8>, EQueueMode::Spsc> AudioPacketQueue;
	TQueue<std::vector<std::int16_t>, EQueueMode::Spsc> AudioPacketQueueUSB;
	/**
	 A buffer used to store audio data that has been resampled for haptic feedback systems.

	 ResampledAudioBuffer is a temporary storage array that holds audio frames after being processed
	 and converted to a target sample rate using the resampling pipeline. Its size is dynamically
	 adjusted based on input and output requirements during the resampling operation. This buffer serves
	 as the intermediate location for resampled audio data, which is subsequently converted into formats
	 compatible with haptic devices.
	 */
	TArray<float> ResampledAudioBuffer;
	/**
	 A unique pointer to an FResampler instance used for processing and resampling audio data within the haptic feedback pipeline.

	 ResamplerImpl manages the audio resampling required to match the desired sample rate for haptic feedback systems.
	 The resampling process ensures that audio data is appropriately converted from its original rate to a lower haptic-compatible rate.
	 It supports initializing the resampling method, processing the mono audio data, and writing the resampled output for further use.
	 */
	TUniquePtr<Audio::FResampler> ResamplerImpl;

	/**
	 A reference to a USoundSubmix instance used within the audio processing pipeline.

	 Submix is a core component that represents an audio submix, allowing for the mixing of multiple audio sources
	 into a single stream. Within the context of the haptic feedback system, this reference is used to monitor and
	 access generated audio buffers. The submix provides the necessary audio data for subsequent processing, such as
	 resampling or conversion, enabling it to be used for tactile feedback in haptic devices.
	 */
	USoundSubmix* Submix;
	/**
	 A unique identifier representing an input device.

	 DeviceId is used to reliably reference and interact with a specific input device
	 within the system, such as game controllers or other haptics-enabled peripherals.
	 It provides a consistent and unique mechanism for identifying devices, enabling
	 their integration into various systems, including haptic feedback and input processing pipelines.
	 */
	int32 DeviceId;
	/**
	 Flag indicating whether the device or connection operates wirelessly.

	 The bIsWireless variable determines if the communication or operation mode is wireless.
	 This can be used to configure specific behaviors or optimizations for wireless systems
	 versus wired systems within the software's logic or settings.
	 */
	bool bIsWireless;
	/**
	 Variable used to maintain the state of the left channel for a low-pass filter.

	 This state is utilized in the filtering process to ensure continuity and accuracy
	 in audio signal processing, particularly when applying a low-pass effect for the
	 left audio channel. It is updated dynamically as the audio processing pipeline executes.
	 */
	float LowPassState_Left = 0.0f;

	/**
	 A floating-point variable used to maintain the internal state of the low-pass filter for the right audio channel.

	 This variable is utilized in audio signal processing to store the current state of the filter between processing iterations,
	 ensuring continuity and accuracy in the filtering process. It plays a role in smoothing out high-frequency components
	 in the right channel audio signal based on the filter's cutoff frequency.
	 */
	float LowPassState_Right = 0.0f;
};
