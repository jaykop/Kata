#include "Character/KataPlayerCharacter.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "Input/KataInputConfig.h"
#include "InputActionValue.h"
#include "KataFrameworkLog.h"
#include "Targeting/KataPlayerTargetingComponent.h"

namespace
{
    UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem(const APlayerController* PlayerController)
    {
        const ULocalPlayer* LocalPlayer = PlayerController != nullptr ? PlayerController->GetLocalPlayer() : nullptr;
        return LocalPlayer != nullptr ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
    }
}

AKataPlayerCharacter::AKataPlayerCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UKataPlayerTargetingComponent>(AKataCharacter::TargetingComponentName))
{
}

UKataPlayerTargetingComponent* AKataPlayerCharacter::GetPlayerTargetingComponent() const
{
    // 파생 클래스가 서브오브젝트 타입을 PC용이 아닌 타입으로 다시 바꿀 수 있으므로 Cast로 확인한다.
    return Cast<UKataPlayerTargetingComponent>(GetTargetingComponent());
}

void AKataPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // 바인딩은 PlayerInputComponent에 속하고, 빙의가 풀리면 엔진이 컴포넌트를 파괴하므로 따로 해제하지 않는다.
    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (EnhancedInput == nullptr)
    {
        UE_LOG(LogKataFramework, Warning,
            TEXT("%s: input component is not a UEnhancedInputComponent. Check Default Input Component Class in project settings."),
            *GetName());
        return;
    }

    if (!IsValid(InputConfig))
    {
        UE_LOG(LogKataFramework, Warning, TEXT("%s: Input Config is not set. Player input is not bound."), *GetName());
        return;
    }

    if (InputConfig->MoveAction != nullptr)
    {
        EnhancedInput->BindAction(InputConfig->MoveAction, ETriggerEvent::Triggered, this, &AKataPlayerCharacter::Move);
    }
    if (InputConfig->LookAction != nullptr)
    {
        EnhancedInput->BindAction(InputConfig->LookAction, ETriggerEvent::Triggered, this, &AKataPlayerCharacter::Look);
    }
}

void AKataPlayerCharacter::NotifyControllerChanged()
{
    // APawn::NotifyControllerChanged가 PreviousController를 현재 컨트롤러로 갱신하므로
    // 그 전에 이전 컨트롤러의 IMC를 제거해야 한다.
    RemoveDefaultMappingContexts(Cast<APlayerController>(PreviousController));

    Super::NotifyControllerChanged();

    AddDefaultMappingContexts(Cast<APlayerController>(GetController()));
}

void AKataPlayerCharacter::Move(const FInputActionValue& Value)
{
    if (Controller == nullptr)
    {
        return;
    }

    // 카메라가 위아래를 보더라도 이동이 지면을 따르도록 Yaw만 사용한다.
    const FVector2D Axis = Value.Get<FVector2D>();
    const FRotationMatrix YawMatrix(FRotator(0.0, Controller->GetControlRotation().Yaw, 0.0));
    AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Axis.Y);
    AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Axis.X);
}

void AKataPlayerCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>();
    AddControllerYawInput(Axis.X);
    AddControllerPitchInput(Axis.Y);
}

void AKataPlayerCharacter::AddDefaultMappingContexts(APlayerController* PlayerController) const
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

void AKataPlayerCharacter::RemoveDefaultMappingContexts(APlayerController* PlayerController) const
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
