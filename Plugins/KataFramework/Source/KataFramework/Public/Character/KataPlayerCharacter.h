#pragma once

#include "Character/KataCharacter.h"
#include "CoreMinimal.h"
#include "KataPlayerCharacter.generated.h"

class UKataPlayerTargetingComponent;

/**
 * 플레이어가 조작하는 Kata 캐릭터.
 *
 * 공용 AKataCharacter의 타게팅 컴포넌트를 UKataPlayerTargetingComponent로 바꿔 만들어 소프트 타겟과 락온을 제공한다.
 * 입력 연결과 카메라는 이 클래스가 다루지 않는다.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Kata Player Character"))
class KATAFRAMEWORK_API AKataPlayerCharacter : public AKataCharacter
{
    GENERATED_BODY()

public:
    AKataPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    /** PC용 타게팅 컴포넌트. 파생 클래스가 타입을 PC용이 아닌 것으로 바꾸지 않았다면 수명 동안 항상 유효하다. */
    UFUNCTION(BlueprintPure, Category = "Kata")
    UKataPlayerTargetingComponent* GetPlayerTargetingComponent() const;
};
