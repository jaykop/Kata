// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "Algorithms/MadgwickAhrs.h"
#include "CoreMinimal.h"
#include "GCore/Interfaces/ISonyGamepad.h"
#include "GCore/Types/Structs/Context/DeviceContext.h"
#include "GenericPlatform/IInputInterface.h"
#include "IHapticDevice.h"
#include "IInputDevice.h"
#include "InputCoreTypes.h"

/**
 * Represents the primary interface for managing DualSense controllers, extending functionality
 * as both an input and haptic device. The class handles input events, haptic feedback, light color,
 * and other controller properties.
 *
 * This class encapsulates the logic specific to DualSense devices, providing mechanisms to
 * configure device properties, manage controller state updates, and deliver force feedback.
 */
class WINDOWSDUALSENSE_DS5W_API DeviceManager : public IInputDevice, public IHapticDevice
{
public:
	virtual ~DeviceManager() override;

	/**
	 * Constructs a DeviceManager instance, initializing it with a message handler for input events
	 * and an audio haptics listener for processing audio-based haptic feedback.
	 *
	 * @param InMessageHandler The message handler responsible for capturing and processing input events.
	 */
	explicit DeviceManager(
	    const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler);
	/**
	 * Called every frame to update Controller-State
	 *
	 * @param DeltaTime Time
	 */
	virtual void Tick(float DeltaTime) override;
	/**
	 * Determines whether the specified controller supports force feedback functionality.
	 *
	 * @param ControllerId The unique identifier of the controller to check.
	 * @return true if the controller supports force feedback; false otherwise.
	 */
	virtual bool SupportsForceFeedback(int32 ControllerId) override
	{
		return true;
	}

	/**
	 * Sets the force feedback values for a specific controller.
	 * Updates the intensity of vibration or other force effects on the specified DualSense controller.
	 *
	 * @param ControllerId The identifier of the controller for which force feedback is being set.
	 * @param Values The force feedback values, including intensity and other parameters.
	 */
	virtual void SetChannelValues(int32 ControllerId, const FForceFeedbackValues& Values) override;
	/**
	 * Sets a specific property of the DualSense input device for the given controller.
	 * Allows configuration of properties such as light color and trigger resistance.
	 *
	 * @param ControllerId ID of the controller to apply the property to.
	 * @param Property A pointer to the input device property to set. This includes details like property name and value.
	 */
	virtual void SetDeviceProperty(int32 ControllerId, const FInputDeviceProperty* Property) override;
	/**
	 * Sets the light bar color on the specified DualSense controller.
	 * This method updates the LED output to match the given color.
	 *
	 * @param ControllerId The unique identifier for the controller to update.
	 * @param Color The desired color to be applied to the controller's light bar.
	 */
	virtual void SetLightColor(int32 ControllerId, FColor Color) override;
	/**
	 * Executes a command in the context of the provided world.
	 * This function is typically used for handling console commands.
	 *
	 * @param InWorld The world context in which the command is executed.
	 * @param Cmd The command string to be executed.
	 * @param Ar The output device to log execution results or messages.
	 * @return True if the command was successfully executed, otherwise false.
	 */
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override { return false; }
	/**
	 * Sets the haptic feedback values for a specific controller and hand.
	 * This function updates the haptic feedback values for the specified controller using the supplied values structure.
	 * The function does nothing if lazy loading is enabled or the library instance for the controller cannot be retrieved.
	 *
	 * @param ControllerId The unique identifier of the controller.
	 * @param Hand The hand for which to set the haptic feedback (e.g., left or right hand).
	 * @param Values The structure containing the haptic feedback values to apply.
	 */
	virtual void SetHapticFeedbackValues(int32 ControllerId, int32 Hand, const FHapticFeedbackValues& Values) override;
	/**
	 * Retrieves the range of haptic frequency supported by the DualSense controller.
	 *
	 * @param MinFrequency Reference to a float where the minimum frequency will be stored.
	 * @param MaxFrequency Reference to a float where the maximum frequency will be stored.
	 */
	virtual void GetHapticFrequencyRange(float& MinFrequency, float& MaxFrequency) const override
	{
	}

	/**
	 * Retrieves the scale factor applied to haptic amplitude for the device.
	 * Used to control the intensity of haptic feedback.
	 *
	 * @return The scale factor for haptic amplitude, with a default value of 1.0f.
	 */
	virtual float GetHapticAmplitudeScale() const override { return 1.0f; }
	/**
	 * Retrieves the haptic device interface implementation.
	 * This method allows access to the haptic device functionality provided by the DualSense controller.
	 *
	 * @return Pointer to the current instance as an IHapticDevice interface.
	 */
	virtual IHapticDevice* GetHapticDevice() override { return this; }
	/**
	 * Determines whether a gamepad is currently attached.
	 * This method always indicates that a DualSense gamepad is attached.
	 *
	 * @return True if a gamepad is attached; false otherwise.
	 */
	virtual bool IsGamepadAttached() const override;
	/**
	 * Handles user login state changes
	 * @param bLoggedIn Whether a User is logged in
	 * @param UserId Platform-specific user identifier
	 * @param UserIndex Index of the user
	 */
	void OnUserLoginChangedEvent(bool bLoggedIn, int32 UserId, int32 UserIndex);

	/**
	 * 이 플러그인의 장치가 Disconnected로 바뀌면 중립 입력(스틱·트리거 0, 눌린 버튼 해제)을 한 번 보낸다.
	 * 레지스트리는 라이브러리를 정리하기 직전에 장치를 Disconnected로 바꾸므로, 호출 시점에는 라이브러리가 아직 있다.
	 */
	void HandleInputDeviceConnectionChange(EInputDeviceConnectionState NewConnectionState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId);
	/**
	 * Sends controller input events to the appropriate systems for processing.
	 * This method is overridden to handle specific input events from the DualSense controller,
	 * ensuring they are relayed correctly within the application.
	 */
	virtual void SendControllerEvents(float DeltaTime);
	virtual void SendControllerEvents() override {}
	/**
	 * Sets the message handler for the application to process input events.
	 *
	 * @param InMessageHandler Reference to the shared generic application message handler used to handle input events.
	 */
	virtual void SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) override
	{
		// 원본은 비어 있었고, 생성 시 받은 참조로 애플리케이션의 멤버를 계속 가리켜 우연히 동작했다.
		MessageHandler = InMessageHandler;
	}

	/**
	 * Sets the force feedback intensity for a specific channel on a particular controller.
	 * Used to deliver haptic feedback through the given channel.
	 *
	 * @param ControllerId The unique identifier of the controller to set the channel value for.
	 * @param ChannelType The type of the feedback channel to update (e.g., left motor, right motor).
	 * @param Value The intensity of the feedback typically ranging from 0.0 (no feedback) to 1.0 (maximum feedback).
	 */
	virtual void SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value) override
	{
	}

	/**
	 * Processes sensor input data including gyroscope and accelerometer readings,
	 * updates the sensor filters, and delivers motion detection events.
	 *
	 * This method calculates sensor values, applies transformations, and updates
	 * the Madgwick AHRS filter to compute orientation and rotation. It also
	 * communicates with the message handler to notify about motion events.
	 *
	 * @param Context The device context containing configuration flags and state information.
	 * @param FrameInput The input context with raw gyroscope and accelerometer data for the current frame.
	 * @param UserId The platform-specific user identifier associated with the controller.
	 * @param InputDeviceId The unique identifier of the input device for which the data is being processed.
	 * @param DeltaTime The elapsed time since the last update, used for temporal calculations in the filter.
	 */
	virtual void SensorsImpl(FDeviceContext* Context, FInputContext& FrameInput, const FPlatformUserId UserId, const FInputDeviceId InputDeviceId, float DeltaTime) const;

	/**
	 * Processes input data from the touchpad, updating the input context with the latest state
	 * and interactions for the specified user and input device.
	 *
	 * This function is responsible for handling and interpreting touchpad events, applying
	 * them to the input context for the associated platform user and input device. It operates
	 * over a single frame, taking into account the elapsed time to ensure accurate updates.
	 *
	 * @param Context       The device context providing access to device-specific state and operations.
	 * @param FrameInput    The input context to be updated with touchpad interactions.
	 * @param UserId        The unique identifier of the platform user associated with the input.
	 * @param InputDeviceId The unique identifier of the input device generating the touchpad data.
	 * @param DeltaTime     The elapsed time since the last frame, used for time-sensitive calculations.
	 */
	virtual void TouchpadImpl(FDeviceContext* Context, FInputContext& FrameInput, const FPlatformUserId UserId, const FInputDeviceId InputDeviceId, float DeltaTime) const {}

	/**
	 * Stores and manages sensor filters for input devices, using a mapping between
	 * input device identifiers and Madgwick AHRS instances.
	 * This variable helps maintain orientation and motion-filtering data for each
	 * connected input device.
	 */
	mutable TMap<FInputDeviceId, TSharedPtr<FMadgwickAhrs>> FilterSensors;

	/**
	 * Represents a mapping between touch IDs and their respective states. The key is an integer
	 * identifier for a specific touch event, and the value is a boolean indicating whether the
	 * touch was previously registered.
	 *
	 * This structure is used to track the touch state of multiple input sources, ensuring accurate
	 * detection of touch interactions.
	 */
	TSet<FInputDeviceId> ActiveTouches;
	TSet<FInputDeviceId> ActiveTouchesRelative;

	virtual void CheckEvents(FDeviceContext* Context, FInputContext& FrameInput, const FPlatformUserId UserId, const FInputDeviceId InputDeviceId, float DeltaTime) const;
	/**
	 * @brief Handles button input events for a DualSense controller.
	 *
	 * This function checks the current state of a button on a DualSense controller and triggers
	 * appropriate input events (button press or release) based on the change in its state.
	 *
	 * @param UserId The platform user ID associated with the controller.
	 * @param InputDeviceId The unique identifier for the DualSense input device.
	 * @param ButtonName The name of the button being checked.
	 * @param IsButtonPressed A boolean indicating the current pressed state of the button (true if pressed, false otherwise).
	 */
	virtual void CheckButtonInput(FDeviceContext* Context, const FPlatformUserId UserId, const FInputDeviceId InputDeviceId, const FName ButtonName, const bool IsButtonPressed) const;

	/**
	 * Handles application-level messages and events, facilitating communication
	 * between the application framework and platform-specific input systems.
	 * This variable is used to process input events, updates, and interactions
	 * within the application.
	 */
	// 원본은 const 참조 멤버였다. 엔진이 넘긴 TSharedRef의 수명에 의존하지 않도록 값으로 보관한다.
	TSharedRef<FGenericApplicationMessageHandler> MessageHandler;

	/** IPlatformInputDeviceMapper의 연결 변경 구독 핸들. 소멸자에서 해제한다. */
	FDelegateHandle ConnectionChangeHandle;
};
