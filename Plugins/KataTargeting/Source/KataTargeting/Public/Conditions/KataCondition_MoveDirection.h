#pragma once

#include "CoreMinimal.h"
#include "KataCondition.h"
#include "KataCondition_MoveDirection.generated.h"

/** 실행 주체 기준으로 나눈 이동 입력 방향. */
UENUM(BlueprintType)
enum class EKataMoveDirection : uint8
{
    /** 이동 입력이 없다. */
    None,
    Forward,
    Backward,
    Left,
    Right
};

/**
 * 실행 주체의 이동 입력 방향이 지정한 방향인지 판정한다. 회피처럼 입력 방향마다 다른 액션으로 갈라지는 엣지에 쓴다.
 *
 * 방향은 실행 주체의 UKataTargetingComponent::ResolveMoveDirection이 정한다. 입력을 실행 주체의 정면 기준으로
 * 앞·뒤·좌·우 네 구간(각 90도)으로 나누며, 경계인 정확히 45도는 앞·뒤 쪽으로 판정한다.
 * 락온이 아닐 때는 입력 방향으로 돌아서서 앞으로 움직이는 동작을 위해, 입력이 있으면 앞으로 판정할 수 있다.
 * 타게팅 컴포넌트가 없으면 판정할 수 없어 Invalid를 돌려준다.
 */
UCLASS(meta = (DisplayName = "Kata Condition: Move Direction"))
class KATATARGETING_API UKataCondition_MoveDirection : public UKataCondition
{
    GENERATED_BODY()

public:
    /** 통과시킬 방향. None은 이동 입력이 없을 때 통과한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Direction")
    EKataMoveDirection Direction = EKataMoveDirection::Forward;

    /**
     * 켜면 락온이 아닐 때 입력이 있으면 방향과 관계없이 Forward로 판정한다.
     * Face Move Direction Command로 입력 방향으로 돌아선 뒤 앞 동작을 재생하는 구성에 맞춘다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move Direction")
    bool bUnlockedInputIsForward = true;

protected:
    virtual FKataConditionResult EvaluateCondition_Implementation(const FKataConditionContext& Context) const override;
};
