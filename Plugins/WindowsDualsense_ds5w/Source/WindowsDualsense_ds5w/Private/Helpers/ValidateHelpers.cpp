// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#include "Helpers/ValidateHelpers.h"

bool FValidateHelpers::ValidateMaxForce(const int32 Strength)
{
	return Strength <= 8 && Strength >= 1;
}

bool FValidateHelpers::ValidateMaxPosition(const int32 Position, const int32 MaxPosition, const int32 MinPosition)
{
	return Position <= MaxPosition && Position >= MinPosition;
}

bool FValidateHelpers::ValidateMaxFrequency(const float Frequency)
{
	return Frequency <= 1.0 && Frequency >= 0.0;
}

int FValidateHelpers::To255(const float Value)
{
	if (Value <= 0)
	{
		return 0;
	}
	if (Value >= 1.0f)
	{
		return 255;
	}

	constexpr float Min = 0;
	constexpr float Max = 1.0;
	const float NormalizedPosition = (Value - Min) / (Max - Min);
	return static_cast<int>(NormalizedPosition * 255);
}

int FValidateHelpers::To255(const unsigned char Value, const unsigned char MaxInput)
{
	if (Value <= 0)
	{
		return 0;
	}
	if (Value >= MaxInput)
	{
		return 255;
	}

	return ((Value * 255.0f) / MaxInput);
}
