// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "CoreMinimal.h"
#include "DualShockFeatureReport.generated.h"

/**
 * @brief Represents the feature report for DualShock gamepad devices using the Sony gamepad interface.
 *
 * This structure is derived from the IFeatureReport interface and is designed to encapsulate
 * device-specific feature reports (input status, configurations, or other data) that can be
 * utilized in the context of DualShock controllers. The functionality is exposed for use
 * in both C++ and Blueprint contexts.
 *
 * @note The struct leverages the Unreal Engine macros and type system, providing seamless
 * integration with Blueprint scripting environments.
 */
USTRUCT(BlueprintType)
struct FDualShockFeatureReport
{
	GENERATED_BODY()
};
