#include "KataRuntimeSpawnerSettings.h"

UKataRuntimeSpawnerSettings::UKataRuntimeSpawnerSettings()
{
    // 에디터 도구 설정이므로 Project Settings의 Editor 범주에 둔다.
    CategoryName = TEXT("Editor");
    SectionName = TEXT("KataRuntimeSpawner");
}
