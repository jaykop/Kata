#pragma once

#include "Delegates/IDelegateInstance.h"
#include "Modules/ModuleInterface.h"

/**
 * KataAI 런타임 모듈의 진입점이다.
 *
 * 모듈이 시작되면 GameplayDebugger에 "KataAI" 카테고리를 등록하고, 종료되면 해제한다.
 * GameplayDebugger를 쓰지 않는 빌드 대상에서는 카테고리를 등록하지 않는다.
 * 치트 확장 UKataAICheatExtension도 CheatManager에 붙도록 등록하고, 종료되면 해제한다.
 */
class FKataAIModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    FDelegateHandle CheatManagerCreatedHandle;
};
