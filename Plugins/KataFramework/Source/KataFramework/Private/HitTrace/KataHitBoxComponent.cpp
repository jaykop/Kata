#include "HitTrace/KataHitBoxComponent.h"

#include "Animation/AnimMontage.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HitTrace/KataHitPoseSampler.h"
#include "HitTrace/KataHitSubsystem.h"
#include "KataFrameworkLog.h"

UKataHitBoxComponent::UKataHitBoxComponent()
{
    // 기록은 UKataHitSubsystem이 포즈 확정 뒤에 호출한다. 컴포넌트 Tick은 포즈보다 먼저 돌 수 있어 쓰지 않는다.
    PrimaryComponentTick.bCanEverTick = false;
}

void UKataHitBoxComponent::OnRegister()
{
    Super::OnRegister();
    if (UWorld* World = GetWorld())
    {
        if (UKataHitSubsystem* Subsystem = World->GetSubsystem<UKataHitSubsystem>())
        {
            Subsystem->RegisterHitBoxComponent(this);
        }
    }
}

void UKataHitBoxComponent::OnUnregister()
{
    if (UWorld* World = GetWorld())
    {
        if (UKataHitSubsystem* Subsystem = World->GetSubsystem<UKataHitSubsystem>())
        {
            Subsystem->UnregisterHitBoxComponent(this);
        }
    }
    Super::OnUnregister();
}

UMeshComponent* UKataHitBoxComponent::GetHitBoxMesh(EKataHitBoxMeshSource Source) const
{
    if (Source == EKataHitBoxMeshSource::Weapon)
    {
        return WeaponMesh.Get();
    }
    if (UMeshComponent* Mesh = CharacterMesh.Get())
    {
        return Mesh;
    }
    const ACharacter* Character = Cast<ACharacter>(GetOwner());
    return Character != nullptr ? Character->GetMesh() : nullptr;
}

void UKataHitBoxComponent::SetCharacterMesh(UMeshComponent* InMesh)
{
    CharacterMesh = InMesh;
}

void UKataHitBoxComponent::SetWeaponMesh(UMeshComponent* InMesh)
{
    WeaponMesh = InMesh;
}

void UKataHitBoxComponent::RecordPoses(uint64 TickIndex)
{
    RecordPose(CharacterRecord, GetHitBoxMesh(EKataHitBoxMeshSource::Character), TickIndex);
    RecordPose(WeaponRecord, GetHitBoxMesh(EKataHitBoxMeshSource::Weapon), TickIndex);
}

void UKataHitBoxComponent::RecordPose(FPoseRecord& Record, const UMeshComponent* Mesh, uint64 TickIndex) const
{
    Record.Mesh = Mesh;
    Record.bValid = Mesh != nullptr;
    if (!Record.bValid)
    {
        return;
    }
    Record.ComponentToWorld = Mesh->GetComponentTransform();
    Record.TickIndex = TickIndex;
    const USkinnedMeshComponent* SkinnedMesh = Cast<USkinnedMeshComponent>(Mesh);
    Record.BoneTransformRevision = SkinnedMesh != nullptr ? SkinnedMesh->GetBoneTransformRevisionNumber() : 0;

    Record.Montage.Reset();
    Record.MontagePosition = 0.0f;
    KataHitPoseSampler::FAnimationState AnimationState;
    const USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(Mesh);
    if (SkeletalMesh != nullptr && KataHitPoseSampler::GetAnimationState(*SkeletalMesh, AnimationState))
    {
        Record.Montage = AnimationState.Montage;
        Record.MontagePosition = AnimationState.Position;
    }
}

bool UKataHitBoxComponent::GetPreviousAnimationState(const USkeletalMeshComponent* Mesh, uint64 CurrentTick,
    const UAnimMontage*& OutMontage, float& OutPosition, FTransform& OutComponentToWorld) const
{
    for (const FPoseRecord* Record : { &CharacterRecord, &WeaponRecord })
    {
        if (Record->bValid && Record->Mesh.Get() == Mesh && Record->TickIndex + 1 == CurrentTick && Record->Montage.IsValid())
        {
            OutMontage = Record->Montage.Get();
            OutPosition = Record->MontagePosition;
            OutComponentToWorld = Record->ComponentToWorld;
            return true;
        }
    }
    return false;
}

bool UKataHitBoxComponent::GetPreviousSocketTransform(const UMeshComponent* Mesh, FName SocketName, uint64 CurrentTick, FTransform& OutTransform) const
{
    if (Mesh == nullptr)
    {
        return false;
    }

    const FPoseRecord* Record = nullptr;
    for (const FPoseRecord* Candidate : { &CharacterRecord, &WeaponRecord })
    {
        if (Candidate->bValid && Candidate->Mesh.Get() == Mesh)
        {
            Record = Candidate;
            break;
        }
    }
    // 바로 앞 Tick의 기록만 직전 프레임 포즈다. 메시를 방금 바꿨거나 기록이 끊겼으면 쓰지 않는다.
    if (Record == nullptr || Record->TickIndex + 1 != CurrentTick)
    {
        UE_LOG(LogKataFramework, VeryVerbose, TEXT("Kata hit box previous pose unavailable for '%s': %s (recorded tick %llu, current tick %llu)"),
            *GetNameSafe(Mesh), Record == nullptr ? TEXT("no record for this mesh") : TEXT("record is not from the previous tick"),
            Record != nullptr ? Record->TickIndex : 0ull, CurrentTick);
        return false;
    }

    const USkinnedMeshComponent* SkinnedMesh = Cast<USkinnedMeshComponent>(Mesh);
    if (SkinnedMesh == nullptr)
    {
        // 스태틱 메시의 소켓은 컴포넌트 기준으로 고정이므로 기록한 컴포넌트 트랜스폼만으로 계산된다.
        OutTransform = Mesh->GetSocketTransform(SocketName, RTS_Component) * Record->ComponentToWorld;
        return true;
    }

    // 엔진은 애니메이션 갱신마다 리비전을 1 올리고, 텔레포트처럼 모션 벡터를 지울 때는 2 올린다.
    // 정확히 1만큼 늘었을 때만 직전 본 버퍼가 기록 시점의 포즈와 같다.
    if (SkinnedMesh->GetBoneTransformRevisionNumber() != Record->BoneTransformRevision + 1)
    {
        UE_LOG(LogKataFramework, VeryVerbose, TEXT("Kata hit box previous pose unavailable for '%s': bone transform revision %u, recorded %u"),
            *GetNameSafe(Mesh), SkinnedMesh->GetBoneTransformRevisionNumber(), Record->BoneTransformRevision);
        return false;
    }

    FName BoneName = SocketName;
    FTransform SocketLocal = FTransform::Identity;
    if (const USkeletalMeshSocket* Socket = SkinnedMesh->GetSocketByName(SocketName))
    {
        BoneName = Socket->BoneName;
        SocketLocal = Socket->GetSocketLocalTransform();
    }

    // Leader Pose를 따르는 메시는 자체 본 버퍼가 비어 있어 여기서 실패한다.
    const int32 BoneIndex = SkinnedMesh->GetBoneIndex(BoneName);
    const TArray<FTransform>& PreviousBones = SkinnedMesh->GetPreviousComponentTransformsArray();
    if (BoneIndex == INDEX_NONE || !PreviousBones.IsValidIndex(BoneIndex))
    {
        return false;
    }

    OutTransform = SocketLocal * PreviousBones[BoneIndex] * Record->ComponentToWorld;
    return true;
}
