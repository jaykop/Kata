#include "KataGraphEditorSettings.h"

UKataGraphEditorSettings::UKataGraphEditorSettings()
{
	AutoLayoutStrategy = EKataGraphLayoutStrategy::Tree;

	bFirstPassOnly = false;

	bRandomInit = false;

	OptimalDistance = 100.f;

	MaxIteration = 50;

	InitTemperature = 10.f;

	CoolDownRate = 10.f;
}

UKataGraphEditorSettings::~UKataGraphEditorSettings()
{

}

