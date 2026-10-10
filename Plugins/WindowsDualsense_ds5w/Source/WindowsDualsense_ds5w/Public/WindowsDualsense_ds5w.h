// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "CoreMinimal.h"
#include "DeviceManager.h"
#include "IInputDeviceModule.h"
#include "Templates/Function.h"

/**
 * The FWindowsDualsense_ds5wModule class represents a module for handling DualSense input devices
 * on the Windows platform. It integrates with the Unreal Engine input device system, allowing
 * for seamless management and input device creation.
 */
class WINDOWSDUALSENSE_DS5W_API FWindowsDualsense_ds5wModule : public IInputDeviceModule
{
	/**
	 * Initializes the FWindowsDualsense_ds5wModule during the startup of the module.
	 *
	 * This method registers the module as a modular feature within the engine using
	 * the input device modular feature name. Additionally, it performs the registration
	 * of custom PlayStation-specific keys for use within the engine, enabling support
	 * for PlayStation controller inputs.
	 *
	 * Overrides the IInputDeviceModule::StartupModule interface method.
	 */
public:
	virtual void StartupModule() override;
	/**
	 * Cleans up and deinitializes the module during shutdown.
	 *
	 * This method is automatically called during the module's lifecycle when the module
	 * is being unloaded. It ensures that any resources allocated during the module's lifetime
	 * are properly released and that the system's state is left clean.
	 *
	 * Override of IInputDeviceModule's ShutdownModule function.
	 */
	virtual void ShutdownModule() override;
	/**
	 * Creates and initializes an input device for DualSense controllers.
	 *
	 * This method sets up the input device instance, manages library instances
	 * through the UDualSenseLibraryManager, and configures controller settings.
	 *
	 * @param InCustomMessageHandler A shared reference to the generic application message handler
	 *                               that the input device will use to handle input.
	 * @return A shared pointer to the created input device instance, or nullptr if initialization fails.
	 */
	virtual TSharedPtr<IInputDevice> CreateInputDevice(
	    const TSharedRef<FGenericApplicationMessageHandler>& InCustomMessageHandler) override;

	using FCustomInputDeviceFactory = TFunction<TSharedPtr<IInputDevice>(const TSharedRef<FGenericApplicationMessageHandler>&)>;

	/**
	 * Sets a custom factory function to create the input device.
	 * This allows the user to inject their own implementation of DeviceManager or any class
	 * implementing IInputDevice and optionally IHapticDevice.
	 *
	 * @param Factory The factory function that returns a shared pointer to an IInputDevice.
	 */
	static void SetCustomInputDeviceFactory(FCustomInputDeviceFactory Factory);

private:
	static FCustomInputDeviceFactory CustomInputDeviceFactory;

	/**
	 * A shared pointer that manages an instance of the DualSense input device.
	 *
	 * DeviceInstance is used to handle and control the lifecycle of the DualSense input device
	 * on the Windows platform, ensuring proper resource management and functionality.
	 */
private:
	TSharedPtr<DeviceManager> DeviceInstance;
	/**
	 * Registers a set of custom input keys for PlayStation-specific controls.
	 *
	 * This method adds custom keys to the `EKeys` registry, which represent
	 * various special buttons available on PlayStation devices such as the
	 * Mic button, Menu button, Share button, Touchpad button, PlayStation
	 * button, thumbstick buttons, function buttons, and paddle buttons.
	 *
	 * It is typically called during the module startup phase to ensure the
	 * keys are properly registered before the input device is utilized.
	 *
	 * The registered keys allow developers to reference these custom PlayStation
	 * inputs within their applications, enabling enhanced input handling for
	 * compatible controllers.
	 */
	static void RegisterCustomKeys();
};
