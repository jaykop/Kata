#include "Input/KataInputHandlerComponent.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Input/KataInputConfig.h"
#include "InputActionValue.h"
#include "KataFrameworkLog.h"
#include "KataGraph.h"
#include "KataGraphComponent.h"
#include "KataGraphInstance.h"

namespace
{
    UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem(const APlayerController* PlayerController)
    {
        const ULocalPlayer* LocalPlayer = PlayerController != nullptr ? PlayerController->GetLocalPlayer() : nullptr;
        return LocalPlayer != nullptr ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
    }
}

UKataInputHandlerComponent::UKataInputHandlerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UKataInputHandlerComponent::SetupPlayerInput(UInputComponent* PlayerInputComponent)
{
    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (EnhancedInput == nullptr)
    {
        UE_LOG(LogKataFramework, Warning,
            TEXT("%s: input component is not a UEnhancedInputComponent. Check Default Input Component Class in project settings."),
            *GetNameSafe(GetOwner()));
        return;
    }

    if (!IsValid(InputConfig))
    {
        UE_LOG(LogKataFramework, Warning, TEXT("%s: Input Config is not set. Player input is not bound."), *GetNameSafe(GetOwner()));
        return;
    }

    if (InputConfig->MoveAction != nullptr)
    {
        EnhancedInput->BindAction(InputConfig->MoveAction, ETriggerEvent::Triggered, this, &UKataInputHandlerComponent::Move);
    }
    if (InputConfig->LookAction != nullptr)
    {
        EnhancedInput->BindAction(InputConfig->LookAction, ETriggerEvent::Triggered, this, &UKataInputHandlerComponent::Look);
    }

    for (const FKataInputTagBinding& Binding : InputConfig->InputBindings)
    {
        if (Binding.InputAction != nullptr && Binding.InputTag.IsValid())
        {
            EnhancedInput->BindAction(Binding.InputAction, Binding.TriggerEvent, this,
                &UKataInputHandlerComponent::HandleInputTag, Binding.InputTag);
        }
    }
}

bool UKataInputHandlerComponent::SendGraphTrigger(FGameplayTag TriggerTag)
{
    const APawn* Pawn = GetPawn();
    UKataGraphComponent* GraphComponent = Pawn != nullptr ? Pawn->FindComponentByClass<UKataGraphComponent>() : nullptr;
    if (GraphComponent == nullptr || !IsValid(Graph) || !TriggerTag.IsValid())
    {
        return false;
    }

    bool bStartedNow = false;
    if (!GraphComponent->IsRunningGraph())
    {
        UKataGraphInstance* NewInstance = nullptr;
        if (!GraphComponent->StartGraphOnSelf(Graph, nullptr, NewInstance))
        {
            return false;
        }
        bStartedNow = true;
    }

    const bool bAccepted = GraphComponent->SendTrigger(TriggerTag);

    // 새로 시작한 그래프가 진입하지 못하면 액션 없이 입력 대기 상태로 남아 IsRunningGraph가 true가 된다.
    // 액션을 실행하지 않는 그래프가 실행 중으로 보이지 않도록 멈춘다.
    // 트리거가 비어 있는 자동 진입 엣지가 시작하면서 이미 액션을 실행했다면 현재 노드가 있으므로 그대로 둔다.
    if (bStartedNow && !bAccepted)
    {
        const UKataGraphInstance* Instance = GraphComponent->GetActiveGraphInstance();
        if (Instance != nullptr && Instance->GetCurrentNode() == nullptr)
        {
            GraphComponent->StopGraph(EKataEndReason::Cancelled);
        }
    }

    return bAccepted;
}

void UKataInputHandlerComponent::OnRegister()
{
    Super::OnRegister();

    // 빙의는 BeginPlay 전에 일어날 수 있으므로 등록 시점에 구독한다.
    if (APawn* Pawn = GetPawn())
    {
        Pawn->ReceiveControllerChangedDelegate.AddUniqueDynamic(this, &UKataInputHandlerComponent::HandleControllerChanged);

        // 이미 빙의된 폰에 나중에 붙은 경우에는 변경 알림이 오지 않으므로 지금 추가한다.
        AddDefaultMappingContexts(Cast<APlayerController>(Pawn->GetController()));
    }
}

void UKataInputHandlerComponent::OnUnregister()
{
    if (APawn* Pawn = GetPawn())
    {
        Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UKataInputHandlerComponent::HandleControllerChanged);
    }

    Super::OnUnregister();
}

void UKataInputHandlerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 폰이 파괴될 때는 빙의 해제 알림으로 이미 제거되지만, 빙의된 채로 컴포넌트만 사라지는 경우를 위해 한 번 더 정리한다.
    // 없는 IMC를 제거하는 것은 아무 효과가 없다.
    if (const APawn* Pawn = GetPawn())
    {
        RemoveDefaultMappingContexts(Cast<APlayerController>(Pawn->GetController()));
    }

    Super::EndPlay(EndPlayReason);
}

void UKataInputHandlerComponent::Move(const FInputActionValue& Value)
{
    APawn* Pawn = GetPawn();
    const AController* Controller = Pawn != nullptr ? Pawn->GetController() : nullptr;
    if (Controller == nullptr)
    {
        return;
    }

    // 카메라가 위아래를 보더라도 이동이 지면을 따르도록 Yaw만 사용한다.
    const FVector2D Axis = Value.Get<FVector2D>();
    const FRotationMatrix YawMatrix(FRotator(0.0, Controller->GetControlRotation().Yaw, 0.0));
    Pawn->AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Axis.Y);
    Pawn->AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Axis.X);
}

void UKataInputHandlerComponent::Look(const FInputActionValue& Value)
{
    APawn* Pawn = GetPawn();
    if (Pawn == nullptr)
    {
        return;
    }

    const FVector2D Axis = Value.Get<FVector2D>();
    Pawn->AddControllerYawInput(Axis.X);
    Pawn->AddControllerPitchInput(Axis.Y);
}

void UKataInputHandlerComponent::HandleInputTag(FGameplayTag InputTag)
{
    if (!IsValid(InputConfig))
    {
        return;
    }

    const FGameplayTag TriggerTag = InputConfig->FindTriggerTag(InputTag);
    if (TriggerTag.IsValid())
    {
        SendGraphTrigger(TriggerTag);
    }
}

void UKataInputHandlerComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
    // 컨트롤러가 폰을 바꾸면 이전 폰의 해제가 새 폰의 빙의보다 먼저 일어나므로
    // 두 폰이 같은 IMC를 쓰더라도 제거 후 추가 순서가 유지된다.
    RemoveDefaultMappingContexts(Cast<APlayerController>(OldController));
    AddDefaultMappingContexts(Cast<APlayerController>(NewController));
}

APawn* UKataInputHandlerComponent::GetPawn() const
{
    return Cast<APawn>(GetOwner());
}

void UKataInputHandlerComponent::AddDefaultMappingContexts(const APlayerController* PlayerController) const
{
    UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem(PlayerController);
    if (Subsystem == nullptr || !IsValid(InputConfig))
    {
        return;
    }

    for (const FKataInputMappingContextEntry& Entry : InputConfig->DefaultMappingContexts)
    {
        if (Entry.MappingContext != nullptr)
        {
            Subsystem->AddMappingContext(Entry.MappingContext, Entry.Priority);
        }
    }
}

void UKataInputHandlerComponent::RemoveDefaultMappingContexts(const APlayerController* PlayerController) const
{
    UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem(PlayerController);
    if (Subsystem == nullptr || !IsValid(InputConfig))
    {
        return;
    }

    for (const FKataInputMappingContextEntry& Entry : InputConfig->DefaultMappingContexts)
    {
        if (Entry.MappingContext != nullptr)
        {
            Subsystem->RemoveMappingContext(Entry.MappingContext);
        }
    }
}
