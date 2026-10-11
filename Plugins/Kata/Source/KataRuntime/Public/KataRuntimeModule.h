#pragma once

#include "Modules/ModuleInterface.h"

/** Kata 재생과 필수 GAS 연동을 담당하는 런타임 계층. */
class KATARUNTIME_API FKataRuntimeModule final : public IModuleInterface
{
public:
    /** 실행 이력 GameplayDebugger 카테고리를 등록한다. 카테고리를 빌드하지 않는 구성에서는 아무것도 하지 않는다. */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
