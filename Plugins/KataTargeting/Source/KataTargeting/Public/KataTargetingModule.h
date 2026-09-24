#pragma once

#include "Modules/ModuleInterface.h"

/**
 * PC와 AI가 함께 쓰는 타게팅과 팩션 계층.
 *
 * 모듈이 시작되면 엔진 전역 팀 관계 판정 함수를 Kata 팩션 설정으로 바꾸고, 종료되면 엔진 기본값으로 되돌린다.
 * 프로젝트가 따로 관계 판정 함수를 등록하면 나중에 등록한 쪽이 적용된다.
 */
class KATATARGETING_API FKataTargetingModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
