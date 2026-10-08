#pragma once

#include "Modules/ModuleInterface.h"

/**
 * KataAI 런타임 모듈의 진입점이다.
 *
 * 모듈이 시작되면 GameplayDebugger에 "KataAI" 카테고리를 등록하고, 종료되면 해제한다.
 * GameplayDebugger를 쓰지 않는 빌드 대상에서는 아무것도 등록하지 않는다.
 */
class FKataAIModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
