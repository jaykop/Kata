#pragma once

#include "Delegates/IDelegateInstance.h"
#include "Modules/ModuleInterface.h"

/**
 * 월드 상태 계층. 현재는 시간대 상태를 제공한다.
 *
 * 모듈이 시작되면 시간 치트 명령(UKataWorldCheatExtension)을 모든 CheatManager에 붙이도록 등록하고, 종료되면 해제한다.
 */
class KATAWORLD_API FKataWorldModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    FDelegateHandle CheatManagerCreatedHandle;
};
