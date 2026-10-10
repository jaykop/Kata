#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "KataRootMotionCurveComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class USkeletalMeshComponent;
struct FAnimMontageInstance;

/**
 * 루트 모션 거리 보정 요청. 오토 대시 태스크처럼 구간 동안 대상 앞까지 전진 거리를 맞추려는 쪽이 만든다.
 * 요청을 받은 UKataRootMotionCurveComponent가 실행 상태와 함께 보관한다.
 */
struct FKataRootMotionDistanceRequest
{
    /**
     * 대상 기준 지점과 그 지점에서 추가로 뺄 거리(cm, 대상 몸통 반지름 등)를 구한다.
     * false를 돌려주면 그 갱신은 보정하지 않는다. 대상이 파괴됐을 수 있으므로 약한 참조로 캡처한다.
     */
    TFunction<bool(FVector& OutLocation, float& OutTargetRadius)> ResolveTarget;

    /** 보정 구간 길이(초). 처음 만난 루트 모션 몽타주의 재생 속도로 트랙 길이로 바꾼다. */
    float Duration = 0.0f;

    /** 자신의 캡슐 표면과 대상 기준 사이에 남길 간격(cm). */
    float StopDistance = 0.0f;

    /**
     * 보정 구간 동안 대상 쪽으로 전진할 총 거리의 하한(cm). 대상이 더 가까워도 이만큼은 나아간다.
     * 0이면 이미 가까울 때 전진을 멈춘다. 어떤 값이어도 뒤로 끌려가지는 않는다.
     */
    float MinDistance = 0.0f;

    /** 보정 구간 동안 대상 쪽으로 전진할 총 거리의 상한(cm). 대상이 더 멀면 이 거리까지만 가고 멈춘다. */
    float MaxDistance = 0.0f;

    /** 켜면 매 갱신 대상 지점을 다시 구한다. 끄면 처음 구한 지점을 쓴다. */
    bool bTrackTarget = true;
};

/**
 * 몽타주 루트 모션을 Kata 처리 단계로 바꾸는 캐릭터 컴포넌트.
 *
 * 소유 캐릭터의 UCharacterMovementComponent::ProcessRootMotionPreConvertToWorld를 바인딩하고, 엔진이 꺼낸 몽타주 루트 모션을
 * 다음 순서로 처리한다. 각 단계는 켜는 조건이 따로 있으며, 아무 단계도 켜져 있지 않으면 엔진 값을 그대로 돌려준다.
 *  1. 커브 대체: bUseRootMotionCurves가 켜져 있으면 같은 트랙 구간의 커브 변화량으로 바꾼다.
 *     우선순위는 몽타주 커브 → 시퀀스 커브 → 원래 루트 모션이다.
 *  2. 거리 보정: BeginDistanceCorrection 요청이 있으면 남은 구간에서 가야 할 거리를 원래 남은 이동량으로 나눈 배율을 수평 이동에 곱한다.
 *     가야 할 거리는 대상까지의 거리를 요청의 최소·최대 전진 거리로 제한한 값이다.
 *
 * 엔진 루트 모션 경로를 그대로 쓰므로 몽타주 재생 속도, 섹션 반복과 연결, URO 예외가 원래 루트 모션과 같게 적용된다.
 * AnimInstance의 Root Motion Mode가 Root Motion From Montages Only일 때만 동작한다.
 * Z 이동은 엔진 루트 모션과 같이 Walking에서는 바닥을, Falling에서는 중력을 따르므로 Flying 같은 이동 모드에서만 반영된다.
 * 갱신 도중 노티파이가 위치를 바꿨거나 역재생이면 그 갱신은 엔진 값을 쓰고, 진행 중인 거리 보정은 멈춘다.
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
     * 꺼져 있으면 커브가 있어도 원래 루트 모션을 쓴다. 거리 보정은 이 설정과 관계없이 동작한다.
     * 실행 중에 바꿔도 다음 이동 갱신부터 적용된다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Root Motion Curve")
    bool bUseRootMotionCurves = false;

    /**
     * 거리 보정을 시작하고 해제에 쓸 핸들을 돌려준다.
     * 요청은 한 번에 하나만 유지하며 새 요청이 이전 요청을 대체한다. 보정 구간을 다 쓰거나, 고정한 몽타주 재생이 바뀌거나,
     * 역재생·노티파이 위치 변경을 만나면 컴포넌트가 스스로 요청을 끝낸다.
     */
    int32 BeginDistanceCorrection(FKataRootMotionDistanceRequest Request);

    /** Handle이 현재 요청이면 거리 보정을 끝낸다. 이미 끝났거나 다른 요청으로 바뀐 핸들은 무시한다. */
    void EndDistanceCorrection(int32 Handle);

protected:
    virtual void OnRegister() override;
    virtual void OnUnregister() override;

private:
    /** 거리 보정 요청과 실행 상태. */
    struct FActiveDistanceCorrection
    {
        FKataRootMotionDistanceRequest Request;
        int32 Handle = INDEX_NONE;

        /** 창을 고정한 루트 모션 몽타주 인스턴스. 고정 전에는 INDEX_NONE이다. */
        int32 MontageInstanceId = INDEX_NONE;

        /** 아직 지나지 않은 보정 구간의 트랙 길이(초). 섹션 반복에서도 같은 위치를 두 번 셀 수 있게 위치 대신 길이로 둔다. */
        float RemainingWindowLength = 0.0f;

        /** 보정 구간에서 지금까지 보정해 이동한 수평 거리(cm). 최소·최대 전진 거리에서 빼 남은 허용량을 구한다. */
        float TraveledDistance = 0.0f;

        /** bTrackTarget이 꺼져 있을 때 처음 구한 대상 지점. */
        bool bHasFixedTarget = false;
        FVector FixedTargetLocation = FVector::ZeroVector;
        float FixedTargetRadius = 0.0f;
    };

    /** CharacterMovement가 월드 공간으로 바꾸기 전의 루트 모션을 받아 처리 단계를 차례로 적용한다. */
    FTransform ProcessRootMotion(const FTransform& InRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds);

    /**
     * 이번 갱신의 이동(FrameMotion, 메시 공간)에 거리 보정을 적용한다. FrameRanges는 이번 갱신에 몽타주가 지나간 트랙 구간이다.
     * 보정할 수 없는 갱신은 FrameMotion을 그대로 돌려준다.
     */
    FTransform ApplyDistanceCorrection(const FTransform& FrameMotion, const ACharacter& Character, const USkeletalMeshComponent& Mesh,
        const FAnimMontageInstance& Instance, TConstArrayView<TPair<float, float>> FrameRanges);

    /**
     * 남은 구간의 월드 수평 이동(RemainingWorld)과 대상까지의 거리로 배율을 구한다.
     * 경로 방향은 바꾸지 않고, 경로 위에서 대상에 가장 가까워지는 지점까지 가도록 정한다.
     * 대상 지점을 구하지 못했거나 남은 이동이 대상 쪽으로 거의 전진하지 않으면 false다.
     */
    bool ComputeDistanceScale(FActiveDistanceCorrection& Correction, const ACharacter& Character, const FVector& RemainingWorld,
        float& OutScale) const;

    /** 델리게이트를 바인딩한 CharacterMovement. 해제할 때 자신이 바인딩한 경우에만 푼다. */
    TWeakObjectPtr<UCharacterMovementComponent> BoundMovement;

    /** 진행 중인 거리 보정. 없으면 2단계를 건너뛴다. */
    TOptional<FActiveDistanceCorrection> DistanceCorrection;

    /** 다음 요청에 줄 핸들. */
    int32 NextDistanceCorrectionHandle = 0;
};
