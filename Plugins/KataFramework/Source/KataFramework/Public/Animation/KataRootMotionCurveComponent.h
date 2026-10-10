#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "KataRootMotionCurveComponent.generated.h"

class UCharacterMovementComponent;

/**
 * 몽타주 루트 모션을 Kata 루트 모션 커브 값으로 바꾸는 캐릭터 컴포넌트.
 * bUseRootMotionCurves를 켠 캐릭터에서만 동작한다.
 *
 * 소유 캐릭터의 UCharacterMovementComponent::ProcessRootMotionPreConvertToWorld를 바인딩해, 엔진이 꺼낸 몽타주 루트 모션을
 * 같은 트랙 구간의 커브 변화량으로 바꾼다. 우선순위는 몽타주 커브 → 시퀀스 커브 → 원래 루트 모션이다.
 * 커브가 하나도 쓰이지 않은 갱신은 엔진 값을 그대로 돌려준다.
 *
 * 엔진 루트 모션 경로를 그대로 쓰므로 몽타주 재생 속도, 섹션 반복과 연결, URO 예외가 원래 루트 모션과 같게 적용된다.
 * AnimInstance의 Root Motion Mode가 Root Motion From Montages Only일 때만 동작한다.
 * Z 이동은 엔진 루트 모션과 같이 Walking에서는 바닥을, Falling에서는 중력을 따르므로 Flying 같은 이동 모드에서만 반영된다.
 * 갱신 도중 노티파이가 위치를 바꿨거나 역재생이면 그 갱신은 엔진 값을 쓴다.
 *
 * 델리게이트는 바인딩을 하나만 받는다. 다른 시스템(예: Motion Warping)이 먼저 바인딩했으면 덮어쓰지 않고 경고만 남긴다.
 */
UCLASS(ClassGroup = (Kata), meta = (BlueprintSpawnableComponent))
class KATAFRAMEWORK_API UKataRootMotionCurveComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKataRootMotionCurveComponent();

    /**
     * 켜면 커브가 있는 몽타주·시퀀스의 루트 모션을 커브 값으로 바꾼다. 기본값은 꺼짐이며, 커브를 쓸 캐릭터에서 켠다.
     * 꺼져 있으면 커브가 있어도 원래 루트 모션을 쓴다. 실행 중에 바꿔도 다음 이동 갱신부터 적용된다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Root Motion Curve")
    bool bUseRootMotionCurves = false;

protected:
    virtual void OnRegister() override;
    virtual void OnUnregister() override;

private:
    /** CharacterMovement가 월드 공간으로 바꾸기 전의 루트 모션을 받아 커브 결과로 바꾼다. */
    FTransform ProcessRootMotion(const FTransform& InRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds);

    /** 델리게이트를 바인딩한 CharacterMovement. 해제할 때 자신이 바인딩한 경우에만 푼다. */
    TWeakObjectPtr<UCharacterMovementComponent> BoundMovement;
};
