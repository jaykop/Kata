#include "Tasks/KataTask_Trail.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "HitTrace/KataHitBoxComponent.h"
#include "HitTrace/KataHitSubsystem.h"
#include "HitTrace/KataTrailParameters.h"
#include "KataFrameworkLog.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UKataTask_Trail::UKataTask_Trail()
{
    Phase = EKataTaskPhase::Presentation;
}

TSubclassOf<UKataTaskInstance> UKataTask_Trail::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_Trail::StaticClass();
}

FName UKataTask_Trail::GetConfigurationError() const
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
    if (Sockets.Num() < 2 || Sockets.Contains(NAME_None))
    {
        return TEXT("InvalidSockets");
    }
    // 궤적은 구간 동안 쓸고 간 자리이므로 Duration이 0보다 커야 한다.
    if (bSingleFrame || Duration <= 0.0f)
    {
        return TEXT("InvalidDuration");
    }
    return NAME_None;
}

FString UKataTask_Trail::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("MissingSystem"))
    {
        return TEXT("'System' is not set");
    }
    if (ErrorCode == TEXT("InvalidSockets"))
    {
        return TEXT("'Sockets' needs at least two socket names and no empty entries");
    }
    if (ErrorCode == TEXT("InvalidDuration"))
    {
        return TEXT("A trail needs a duration. Turn off 'Single Frame' and set 'Duration' above 0");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

#if WITH_EDITOR
EDataValidationResult UKataTask_Trail::IsDataValid(FDataValidationContext& Context) const
{
    const EDataValidationResult Result = Super::IsDataValid(Context);
    if (System == nullptr)
    {
        return Result;
    }

    const uint32 Mask = KataTrailParameters::FindOutputMask(System->GetExposedParameters());
    const TConstArrayView<FName> OutputNames = KataTrailParameters::GetOutputNames();
    TArray<FString> MissingNames;
    for (int32 Index = 0; Index < OutputNames.Num(); ++Index)
    {
        if ((Mask & (1u << Index)) == 0)
        {
            MissingNames.Add(OutputNames[Index].ToString());
        }
    }
    if (!MissingNames.IsEmpty())
    {
        Context.AddWarning(FText::FromString(FString::Printf(
            TEXT("Niagara system '%s' has no user parameter(s) %s. The trail task skips them at runtime."),
            *GetNameSafe(System), *FString::Join(MissingNames, TEXT(", ")))));
    }
    return Result;
}
#endif

UMeshComponent* UKataTaskInstance_Trail::ResolveMesh() const
{
    const UKataTask_Trail* Definition = Cast<UKataTask_Trail>(GetTaskDefinition());
    AActor* Avatar = GetKataContext().GetAvatarActor();
    if (Definition == nullptr || Avatar == nullptr)
    {
        return nullptr;
    }

    // 기준 메시를 고르는 규칙은 Spawn VFX와 같게 둔다. 같은 공격의 판정·이펙트·궤적이 같은 메시를 따라가야 하기 때문이다.
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
        UE_LOG(LogKataFramework, Warning, TEXT("Kata trail task '%s' found no %s mesh on '%s' (slot '%s'). Add a Kata Hit Box component or equip a weapon in that slot."),
            *GetDisplayName(),
            Definition->MeshSource == EKataHitBoxMeshSource::Weapon ? TEXT("weapon") : TEXT("character"),
            *GetNameSafe(Avatar), *Definition->WeaponSlot.ToString());
    }
    return Mesh;
}

void UKataTaskInstance_Trail::OnTaskStarted_Implementation()
{
    // 그리지 못해도 태스크를 일찍 완료하지 않는다. 연출 실패가 의존 태스크의 시작 시각을 바꾸지 않게 한다.
    UKataTask_Trail* Definition = Cast<UKataTask_Trail>(GetTaskDefinition());
    UWorld* World = GetWorld();
    UKataHitSubsystem* Subsystem = World != nullptr ? World->GetSubsystem<UKataHitSubsystem>() : nullptr;
    if (Definition == nullptr || Definition->System == nullptr || Subsystem == nullptr)
    {
        return;
    }
    const FKataContext Context = GetKataContext();
    AActor* Avatar = Context.GetAvatarActor();
    UMeshComponent* Mesh = ResolveMesh();
    if (Avatar == nullptr || Mesh == nullptr)
    {
        return;
    }

    // 기준 메시에 붙여 그 메시의 소유 액터가 컴포넌트도 소유하게 한다. 해당 액터가 사라지면 컴포넌트도 함께 정리된다.
    // 입자 위치는 Subsystem이 월드 좌표로 넘긴다. 이미터를 월드 공간으로 설정해야 컴포넌트가 메시를 따라가도 리본은 휘두른 자리에 남는다.
    // 풀링을 쓰면 끝난 컴포넌트가 다른 생성에 재사용돼 약한 참조가 남의 이펙트를 가리킬 수 있으므로 풀링하지 않는다.
    // 활성화 전에 실행 방식을 정한다. Solo 모드여야 컴포넌트별 시간 배율이 적용되고,
    // 마지막 틱 그룹이어야 같은 프레임 Subsystem이 넘긴 샘플을 그 프레임에 그린다.
    UNiagaraComponent* Component = UNiagaraFunctionLibrary::SpawnSystemAttached(
        Definition->System, Mesh, NAME_None, Mesh->GetComponentLocation(), FRotator::ZeroRotator,
        EAttachLocation::KeepWorldPosition, true, false, ENCPoolMethod::None);
    if (Component == nullptr)
    {
        return;
    }
    Component->SetForceSolo(true);
    Component->SetCustomTimeDilation(Avatar->CustomTimeDilation);
    Component->SetTickBehavior(ENiagaraTickBehavior::ForceTickLast);

    FKataTrailRegistration Registration;
    Registration.Task = Definition;
    Registration.TaskInstance = this;
    Registration.ActionInstance = GetActionInstance();
    Registration.Mesh = Mesh;
    Registration.HitBoxComponent = Avatar->FindComponentByClass<UKataHitBoxComponent>();
    Registration.InstigatorActor = Avatar;
    Registration.NiagaraComponent = Component;
    Registration.StartTime = StartedAtKataTime;
    Registration.EndTime = StartedAtKataTime + Definition->Duration;

    TrailHandle = Subsystem->RegisterTrail(Registration);
    if (TrailHandle == INDEX_NONE)
    {
        Component->DestroyComponent();
        return;
    }
    Component->Activate(true);
}

void UKataTaskInstance_Trail::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    if (TrailHandle != INDEX_NONE)
    {
        if (UWorld* World = GetWorld())
        {
            if (UKataHitSubsystem* Subsystem = World->GetSubsystem<UKataHitSubsystem>())
            {
                // 정상 완료만 종료 시각까지 그린다. 캔슬된 공격의 궤적이 더 이어지지 않게 한다.
                Subsystem->CloseTrail(TrailHandle, Reason == EKataTaskEndReason::Completed);
            }
        }
        TrailHandle = INDEX_NONE;
    }

    Super::OnTaskEnded_Implementation(Reason);
}
