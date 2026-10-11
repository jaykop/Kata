#pragma once

#include "Delegates/IDelegateInstance.h"
#include "Modules/ModuleInterface.h"

/**
 * Kata 코어와 위성 플러그인을 조합해 바로 쓸 수 있는 게임플레이 클래스를 제공하는 통합 계층.
 * 코어 Kata는 이 모듈을 알지 못하며, 이 모듈만 여러 플러그인에 동시에 의존한다.
 *
 * 모듈이 시작되면 치트 확장 UKataFrameworkCheatExtension을 CheatManager에 붙이도록 등록하고, 종료되면 해제한다.
 */
class KATAFRAMEWORK_API FKataFrameworkModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    FDelegateHandle CheatManagerCreatedHandle;
};
