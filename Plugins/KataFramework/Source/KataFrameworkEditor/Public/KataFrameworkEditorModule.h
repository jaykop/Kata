#pragma once

#include "Modules/ModuleInterface.h"

/**
 * KataFramework 기능의 에디터 도구 계층.
 * Kata 액션 에디터 프리뷰 툴바에 Hit Trace 디버그 토글을 더하고, 데이터 행 ID의 행 이름 드롭다운을 등록한다.
 */
class FKataFrameworkEditorModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenus();
    void RegisterPropertyCustomizations();
    void UnregisterPropertyCustomizations();
};
