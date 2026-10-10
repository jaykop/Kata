#include "Animation/KataAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/KataAnimLayerSetup.h"
#include "Animation/KataTiltComponent.h"
#include "Animation/KataTiltDebug.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Equipment/KataEquipmentComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
    // 이 속력(cm/s) 미만이면 이동 방향 각도를 0으로 둔다. 정지 직전의 미세 속도로 각도가 튀는 것을 막는다.
    constexpr float KataAnimDirectionMinSpeed = 1.0f;
}

void UKataAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    CacheOwner();

#if WITH_EDITOR
    // Anim Blueprint 편집기 프리뷰에는 장착 컴포넌트를 가진 캐릭터가 없어 레이어를 링크할 주체가 없다.
    // 초기화 도중에는 링크하지 않고, 메시가 초기화를 마쳤다고 알릴 때 링크한다.
    USkeletalMeshComponent* OwningMesh = GetOwningComponent();
    const UWorld* World = GetWorld();
    const AActor* OwningActor = GetOwningActor();
    const bool bAnimEditorPreview = World != nullptr && World->WorldType == EWorldType::EditorPreview
        && (OwningActor == nullptr || OwningActor->FindComponentByClass<UKataEquipmentComponent>() == nullptr);
    if (PreviewAnimLayerSetup != nullptr && OwningMesh != nullptr && bAnimEditorPreview)
    {
        OwningMesh->OnAnimInitialized.AddUniqueDynamic(this, &UKataAnimInstance::HandlePreviewAnimInitialized);
    }
#endif
}

void UKataAnimInstance::HandlePreviewAnimInitialized()
{
#if WITH_EDITOR
    USkeletalMeshComponent* OwningMesh = GetOwningComponent();
    if (OwningMesh == nullptr)
    {
        return;
    }

    // 편집기 프리뷰는 컴파일할 때마다 Anim Instance를 새로 만든다. 바인딩을 남겨 두면 이전 인스턴스가 메시에 계속 쌓이므로
    // 한 번 불리면 해제한다. 같은 인스턴스가 다시 초기화되면 NativeInitializeAnimation이 다시 바인딩한다.
    OwningMesh->OnAnimInitialized.RemoveDynamic(this, &UKataAnimInstance::HandlePreviewAnimInitialized);

    // 이미 교체된 이전 인스턴스라면 링크하지 않는다.
    if (PreviewAnimLayerSetup != nullptr && OwningMesh->GetAnimInstance() == this)
    {
        PreviewAnimLayerSetup->LinkLayers(OwningMesh, FGameplayTag(), nullptr);
    }
#endif
}

void UKataAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    // 초기화 시점에 소유자가 아직 없었거나 메시가 다른 캐릭터로 옮겨진 경우를 다시 잡는다.
    if (!OwnerCharacter.IsValid() || OwnerCharacter.Get() != TryGetPawnOwner())
    {
        CacheOwner();
    }

    // Tilt는 이동 컴포넌트와 무관하므로 이동 값이 없어 아래에서 돌아가는 경우에도 복사한다.
    const UKataTiltComponent* Tilt = OwnerTilt.Get();
    SnapshotTiltPitch = Tilt != nullptr ? Tilt->GetTiltPitch() : 0.0f;
    SnapshotTiltAlpha = Tilt != nullptr ? Tilt->GetTiltAlpha() : 0.0f;
#if ENABLE_DRAW_DEBUG
    // 소유자와 태스크가 없는 Anim Blueprint 편집기 프리뷰에서도 노드와 체인을 확인할 수 있도록 여기서 덮어쓴다.
    float ForcedTiltPitch = 0.0f;
    if (KataTiltDebug::GetForcedPitch(ForcedTiltPitch))
    {
        SnapshotTiltPitch = ForcedTiltPitch;
        SnapshotTiltAlpha = 1.0f;
    }
#endif

    const ACharacter* Character = OwnerCharacter.Get();
    const UCharacterMovementComponent* Movement = OwnerMovement.Get();
    if (Character == nullptr || Movement == nullptr)
    {
        SnapshotVelocity = FVector::ZeroVector;
        SnapshotAcceleration = FVector::ZeroVector;
        SnapshotRotation = FRotator::ZeroRotator;
        SnapshotMovementMode = MOVE_None;
        return;
    }

    // 컴포넌트는 워커 스레드에서 안전하게 읽을 수 없으므로 게임 스레드에서 값만 복사한다.
    SnapshotVelocity = Character->GetVelocity();
    SnapshotAcceleration = Movement->GetCurrentAcceleration();
    SnapshotRotation = Character->GetActorRotation();
    SnapshotMovementMode = Movement->MovementMode;
}

void UKataAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

    TiltPitch = SnapshotTiltPitch;
    TiltAlpha = SnapshotTiltAlpha;

    Velocity = SnapshotVelocity;
    Acceleration = SnapshotAcceleration;
    MovementMode = SnapshotMovementMode;

    const FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0);
    GroundSpeed = HorizontalVelocity.Size();

    // 로컬 속도는 피치·롤의 영향을 받지 않도록 Yaw 회전만으로 되돌린다.
    const FRotator YawRotation(0.0, SnapshotRotation.Yaw, 0.0);
    const FVector LocalVelocity = YawRotation.UnrotateVector(HorizontalVelocity);
    LocalForwardSpeed = LocalVelocity.X;
    LocalRightSpeed = LocalVelocity.Y;

    VelocityDirectionAngle = GroundSpeed >= KataAnimDirectionMinSpeed
        ? FRotator::NormalizeAxis(HorizontalVelocity.Rotation().Yaw - SnapshotRotation.Yaw)
        : 0.0f;

    bHasAcceleration = !Acceleration.IsNearlyZero();
    bIsFalling = SnapshotMovementMode == MOVE_Falling;
    bIsOnGround = SnapshotMovementMode == MOVE_Walking || SnapshotMovementMode == MOVE_NavWalking;
}

void UKataAnimInstance::CacheOwner()
{
    ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
    OwnerCharacter = Character;
    OwnerMovement = Character != nullptr ? Character->GetCharacterMovement() : nullptr;
    OwnerTilt = Character != nullptr ? Character->FindComponentByClass<UKataTiltComponent>() : nullptr;

    // AKataCharacter에 묶이지 않도록 IAbilitySystemInterface 또는 컴포넌트 검색으로 ASC를 찾는다.
    UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Character);
    if (AbilitySystem != nullptr && AbilitySystem != OwnerAbilitySystem.Get())
    {
        // 같은 ASC에 다시 연결하면 태그 이벤트가 중복 등록되므로 ASC가 바뀐 경우에만 초기화한다.
        OwnerAbilitySystem = AbilitySystem;
        GameplayTagPropertyMap.Initialize(this, AbilitySystem);
    }
}

#if WITH_EDITOR
#define LOCTEXT_NAMESPACE "KataAnimInstance"

EDataValidationResult UKataAnimInstance::IsDataValid(FDataValidationContext& Context) const
{
    Super::IsDataValid(Context);

    GameplayTagPropertyMap.IsDataValid(this, Context);

    // 본 이름은 스켈레톤이 있는 자식 ABP에서만 확인할 수 있으므로 여기서는 이름과 가중치 형식만 본다.
    float TotalTiltWeight = 0.0f;
    TSet<FName> TiltBoneNames;
    for (const FKataTiltBone& Bone : TiltBoneChain)
    {
        if (Bone.BoneName.IsNone())
        {
            Context.AddError(LOCTEXT("TiltBoneWithoutName", "Tilt Bone Chain has an entry without a bone name."));
        }
        else if (TiltBoneNames.Contains(Bone.BoneName))
        {
            Context.AddError(FText::Format(LOCTEXT("TiltBoneDuplicated", "Tilt Bone Chain lists bone {0} more than once."), FText::FromName(Bone.BoneName)));
        }
        TiltBoneNames.Add(Bone.BoneName);
        TotalTiltWeight += FMath::Max(Bone.Weight, 0.0f);
    }
    if (!TiltBoneChain.IsEmpty() && !(TotalTiltWeight > 0.0f))
    {
        Context.AddError(LOCTEXT("TiltWeightsZero", "Tilt Bone Chain weights must add up to more than zero."));
    }

    return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE
#endif
