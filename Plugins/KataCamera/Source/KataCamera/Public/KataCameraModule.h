#pragma once

#include "Modules/ModuleInterface.h"

/**
 * 플레이어 카메라 계층.
 *
 * 모듈이 시작되면 GameplayDebugger에 "KataCamera" 카테고리를 등록하고, 종료되면 해제한다.
 * GameplayDebugger를 쓰지 않는 빌드 대상에서는 아무것도 등록하지 않는다.
 */
class KATACAMERA_API FKataCameraModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
