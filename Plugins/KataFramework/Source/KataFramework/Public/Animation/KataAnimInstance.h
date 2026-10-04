#pragma once

#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameplayEffectTypes.h"
#include "KataAnimInstance.generated.h"

class ACharacter;
class UAbilitySystemComponent;
class UCharacterMovementComponent;
class UKataAnimLayerSetup;

/**
 * 모든 Kata 캐릭터의 Anim Blueprint가 부모로 쓰는 공통 Anim Instance.
 *
 * 소유 캐릭터의 이동 상태를 몸 구조와 무관한 값(속도, 로컬 속도, 이동 방향, 가속, 이동 모드)으로 정리해
 * AnimGraph와 Thread Safe 함수가 읽을 수 있게 한다. 2족·4족·뱀·비행 같은 몸 구조별 차이는
 * 이 클래스를 나누지 않고 ABP, Control Rig, Linked Anim Layer에서 처리한다.
 *
 * 게임 스레드의 NativeUpdateAnimation은 컴포넌트 값을 복사만 하고,
 * 파생값 계산은 워커 스레드에서 실행될 수 있는 NativeThreadSafeUpdateAnimation에서 한다.
 *
 * 소유자가 ACharacter가 아니면(ABP 에디터 프리뷰 등) 이동 값은 모두 기본값으로 남는다.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Kata Anim Instance"))
class KATAFRAMEWORK_API UKataAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    //~ Begin UAnimInstance Interface
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
    //~ End UAnimInstance Interface

#if WITH_EDITOR
    //~ Begin UObject Interface
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
    //~ End UObject Interface
#endif

    // 이동 값은 UKataAnimLayerInstance의 레이어 ABP가 메인 인스턴스에서 읽어야 하므로 public에 둔다.
    // Blueprint에서는 읽기 전용이며, 값은 이 클래스의 업데이트에서만 쓴다.

    /** 월드 공간 속도(cm/s). */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Locomotion")
    FVector Velocity = FVector::ZeroVector;

    /** CharacterMovement가 입력·경로 이동으로 계산한 월드 공간 가속. 플레이어와 AI가 같은 경로로 채운다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Locomotion")
    FVector Acceleration = FVector::ZeroVector;

    /** 수평면 속력(cm/s). 수직 속도는 포함하지 않는다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Locomotion")
    float GroundSpeed = 0.0f;

    /** 액터 전방 기준 수평 속도 성분(cm/s). 후진이면 음수다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Locomotion")
    float LocalForwardSpeed = 0.0f;

    /** 액터 오른쪽 기준 수평 속도 성분(cm/s). 왼쪽 이동이면 음수다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Locomotion")
    float LocalRightSpeed = 0.0f;

    /** 액터 전방에 대한 수평 이동 방향 각도(-180~180도). 오른쪽이 양수이며, 거의 멈춰 있으면 0이다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Locomotion")
    float VelocityDirectionAngle = 0.0f;

    /** 이동 가속이 있는지 여부. 이동 의도의 시작·정지 판단에 쓴다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Locomotion")
    bool bHasAcceleration = false;

    /** CharacterMovement가 Falling 모드인지 여부. 점프 상승과 낙하를 모두 포함한다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Locomotion")
    bool bIsFalling = false;

    /** Walking 또는 NavWalking 모드인지 여부. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Locomotion")
    bool bIsOnGround = false;

    /** 현재 CharacterMovement 이동 모드. 비행·수영 같은 상태 전환 판단에 쓴다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|Locomotion")
    TEnumAsByte<EMovementMode> MovementMode = MOVE_None;

protected:
    /**
     * ASC의 Gameplay Tag가 붙고 떨어질 때 이 Anim Instance의 변수를 갱신하는 매핑.
     * ABP Class Defaults에서 태그와 bool·int·float 변수를 짝지어 쓴다(예: 방어 상태 태그 → bGuard).
     * 캐릭터마다 쓰는 상태 태그가 달라 임의 태그를 받아야 하므로 Categories 루트를 지정하지 않는다.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Kata|GameplayTags")
    FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;

#if WITH_EDITORONLY_DATA
    /**
     * Anim Blueprint 편집기 프리뷰에서만 링크할 레이어 설정. Body 레이어와 기본 무기 레이어를 링크한다.
     * 게임과 BP 뷰포트, 액션 편집기 프리뷰에서는 캐릭터의 장착 컴포넌트가 레이어를 링크하므로 쓰지 않는다.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Kata|Preview")
    TObjectPtr<UKataAnimLayerSetup> PreviewAnimLayerSetup;
#endif

private:
    /** 프리뷰용 레이어를 링크한다. 메시의 Anim Instance 초기화가 끝난 뒤에 불린다. */
    UFUNCTION()
    void HandlePreviewAnimInitialized();

    /** 소유 캐릭터와 컴포넌트를 찾아 캐시하고, ASC가 바뀌었으면 태그 매핑을 다시 연결한다. */
    void CacheOwner();

    TWeakObjectPtr<ACharacter> OwnerCharacter;
    TWeakObjectPtr<UCharacterMovementComponent> OwnerMovement;
    TWeakObjectPtr<UAbilitySystemComponent> OwnerAbilitySystem;

    // 게임 스레드에서 복사한 스냅샷. NativeThreadSafeUpdateAnimation은 컴포넌트 대신 이 값만 읽는다.
    FVector SnapshotVelocity = FVector::ZeroVector;
    FVector SnapshotAcceleration = FVector::ZeroVector;
    FRotator SnapshotRotation = FRotator::ZeroRotator;
    EMovementMode SnapshotMovementMode = MOVE_None;
};
