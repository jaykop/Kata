#include "Tasks/KataTask_SpawnVFX.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "HitTrace/KataHitBoxComponent.h"
#include "KataFrameworkLog.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UKataTask_SpawnVFX::UKataTask_SpawnVFX()
{
    Phase = EKataTaskPhase::Presentation;
}

TSubclassOf<UKataTaskInstance> UKataTask_SpawnVFX::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_SpawnVFX::StaticClass();
}

FName UKataTask_SpawnVFX::GetConfigurationError() const
{
    const FName SuperError = Super::GetConfigurationError();
    if (!SuperError.IsNone())
    {
        return SuperError;
    }
    if (System == nullptr)
    {
        return TEXT("MissingSystem");
    }
    return NAME_None;
}

FString UKataTask_SpawnVFX::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("MissingSystem"))
    {
        return TEXT("'System' is not set");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

#if WITH_EDITOR
EDataValidationResult UKataTask_SpawnVFX::IsDataValid(FDataValidationContext& Context) const
{
    const EDataValidationResult Result = Super::IsDataValid(Context);
    if (System != nullptr && System->IsLooping() && CompletedPolicy == EKataVFXEndPolicy::KeepPlaying)
    {
        Context.AddWarning(FText::FromString(FString::Printf(
            TEXT("Niagara system '%s' loops, so Keep Playing never lets it finish. Emission will stop when the task completes."),
            *GetNameSafe(System))));
    }
    return Result;
}
#endif

UMeshComponent* UKataTaskInstance_SpawnVFX::ResolveMesh() const
{
    const UKataTask_SpawnVFX* Definition = Cast<UKataTask_SpawnVFX>(GetTaskDefinition());
    AActor* Avatar = GetKataContext().GetAvatarActor();
    if (Definition == nullptr || Avatar == nullptr)
    {
        return nullptr;
    }

    // 무기 메시를 고르는 규칙은 Hit Trace와 같게 둔다. 궤적과 판정이 같은 메시를 따라가야 하기 때문이다.
    UMeshComponent* Mesh = nullptr;
    if (const UKataHitBoxComponent* HitBoxComponent = Avatar->FindComponentByClass<UKataHitBoxComponent>())
    {
        Mesh = HitBoxComponent->GetHitBoxMesh(Definition->MeshSource, Definition->WeaponSlot);
    }
    else if (Definition->MeshSource == EKataHitBoxMeshSource::Character)
    {
        const ACharacter* Character = Cast<ACharacter>(Avatar);
        Mesh = Character != nullptr ? Character->GetMesh() : Avatar->FindComponentByClass<USkeletalMeshComponent>();
    }

    if (Mesh == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Kata VFX task '%s' found no %s mesh on '%s' (slot '%s'). Add a Kata Hit Box component or equip a weapon in that slot."),
            *GetDisplayName(),
            Definition->MeshSource == EKataHitBoxMeshSource::Weapon ? TEXT("weapon") : TEXT("character"),
            *GetNameSafe(Avatar), *Definition->WeaponSlot.ToString());
    }
    return Mesh;
}

void UKataTaskInstance_SpawnVFX::OnTaskStarted_Implementation()
{
    // 생성하지 못해도 태스크를 일찍 완료하지 않는다. 연출 실패가 의존 태스크의 시작 시각을 바꾸지 않게 한다.
    const UKataTask_SpawnVFX* Definition = Cast<UKataTask_SpawnVFX>(GetTaskDefinition());
    if (Definition == nullptr || Definition->System == nullptr)
    {
        return;
    }
    UMeshComponent* Mesh = ResolveMesh();
    if (Mesh == nullptr)
    {
        return;
    }

    // 풀링을 쓰면 끝난 컴포넌트가 다른 생성에 재사용돼 약한 참조가 남의 이펙트를 가리킬 수 있으므로 풀링하지 않는다.
    UNiagaraComponent* Spawned = nullptr;
    if (Definition->bAttachToMesh)
    {
        Spawned = UNiagaraFunctionLibrary::SpawnSystemAttached(
            Definition->System, Mesh, Definition->AttachSocket,
            Definition->LocationOffset, Definition->RotationOffset, Definition->Scale,
            EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::None);
    }
    else
    {
        const FTransform Offset(Definition->RotationOffset, Definition->LocationOffset, Definition->Scale);
        const FTransform SpawnTransform = Offset * Mesh->GetSocketTransform(Definition->AttachSocket);
        Spawned = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            Mesh, Definition->System, SpawnTransform.GetLocation(), SpawnTransform.Rotator(), SpawnTransform.GetScale3D(),
            true, true, ENCPoolMethod::None);
    }
    NiagaraComponent = Spawned;
}

void UKataTaskInstance_SpawnVFX::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    UNiagaraComponent* Component = NiagaraComponent.Get();
    NiagaraComponent.Reset();
    if (Component == nullptr)
    {
        return;
    }

    const UKataTask_SpawnVFX* Definition = Cast<UKataTask_SpawnVFX>(GetTaskDefinition());
    EKataVFXEndPolicy Policy = Reason == EKataTaskEndReason::Completed && Definition != nullptr
        ? Definition->CompletedPolicy
        : EKataVFXEndPolicy::StopEmitting;
    // 루프 시스템은 스스로 끝나지 않으므로 Keep Playing이어도 방출을 멈춘다.
    if (Policy == EKataVFXEndPolicy::KeepPlaying && Component->GetAsset() != nullptr && Component->GetAsset()->IsLooping())
    {
        Policy = EKataVFXEndPolicy::StopEmitting;
    }

    switch (Policy)
    {
    case EKataVFXEndPolicy::StopEmitting:
        // 자동 소멸 컴포넌트이므로 남은 파티클이 사라지면 스스로 정리된다.
        Component->Deactivate();
        break;

    case EKataVFXEndPolicy::DestroyImmediately:
        Component->DestroyComponent();
        break;

    case EKataVFXEndPolicy::KeepPlaying:
    default:
        break;
    }
}
