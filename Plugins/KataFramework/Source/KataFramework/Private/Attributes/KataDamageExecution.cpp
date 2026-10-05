#include "Attributes/KataDamageExecution.h"

#include "AbilitySystemComponent.h"
#include "Attributes/KataAttributeSet_Base.h"
#include "Attributes/KataAttributeSet_Combat.h"
#include "Attributes/KataCombatSettings.h"
#include "GameplayEffect.h"
#include "KataFrameworkLog.h"

namespace KataDamageExecution
{
    /** 캡처 정의는 모든 Execution 인스턴스가 공유하므로 한 번만 만든다. */
    struct FCaptureDefinitions
    {
        FGameplayEffectAttributeCaptureDefinition AttackPower;
        FGameplayEffectAttributeCaptureDefinition Defense;

        FCaptureDefinitions()
            : AttackPower(UKataAttributeSet_Combat::GetAttackPowerAttribute(), EGameplayEffectAttributeCaptureSource::Source, true)
            , Defense(UKataAttributeSet_Combat::GetDefenseAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
        {
        }
    };

    static const FCaptureDefinitions& GetCaptureDefinitions()
    {
        static const FCaptureDefinitions Definitions;
        return Definitions;
    }
}

UKataDamageExecution::UKataDamageExecution()
{
    const KataDamageExecution::FCaptureDefinitions& Definitions = KataDamageExecution::GetCaptureDefinitions();
    RelevantAttributesToCapture.Add(Definitions.AttackPower);
    RelevantAttributesToCapture.Add(Definitions.Defense);
}

void UKataDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
    FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    const KataDamageExecution::FCaptureDefinitions& Definitions = KataDamageExecution::GetCaptureDefinitions();
    const UKataCombatSettings* Settings = UKataCombatSettings::Get();
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

    if (!Settings->DamageSetByCallerTag.IsValid())
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Kata damage execution in '%s' has no Damage Set By Caller Tag in the Kata Combat settings"),
            *GetNameSafe(Spec.Def));
        return;
    }

    // 계수가 빠진 스펙은 설정 실수이므로 엔진 경고를 그대로 남긴다.
    const float Coefficient = Spec.GetSetByCallerMagnitude(Settings->DamageSetByCallerTag, true, 0.0f);

    FAggregatorEvaluateParameters EvaluateParameters;
    EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    // 세트가 없는 ASC도 캡처 자체는 0으로 성공할 수 있으므로 세트 보유 여부를 따로 확인한다.
    const UAbilitySystemComponent* SourceAbilitySystem = ExecutionParams.GetSourceAbilitySystemComponent();
    float RawDamage = Coefficient;
    if (SourceAbilitySystem != nullptr && SourceAbilitySystem->HasAttributeSetForAttribute(Definitions.AttackPower.AttributeToCapture))
    {
        float AttackPower = 0.0f;
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Definitions.AttackPower, EvaluateParameters, AttackPower);
        RawDamage = Coefficient * FMath::Max(AttackPower, 0.0f);
    }

    const UAbilitySystemComponent* TargetAbilitySystem = ExecutionParams.GetTargetAbilitySystemComponent();
    float Defense = 0.0f;
    if (TargetAbilitySystem != nullptr && TargetAbilitySystem->HasAttributeSetForAttribute(Definitions.Defense.AttributeToCapture))
    {
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Definitions.Defense, EvaluateParameters, Defense);
        Defense = FMath::Max(Defense, 0.0f);
    }

    const float DefenseConstant = FMath::Max(Settings->DefenseConstant, 1.0f);
    const float FinalDamage = RawDamage * DefenseConstant / (DefenseConstant + Defense);
    if (FinalDamage <= 0.0f)
    {
        return;
    }

    OutExecutionOutput.AddOutputModifier(
        FGameplayModifierEvaluatedData(UKataAttributeSet_Base::GetDamageAttribute(), EGameplayModOp::AddBase, FinalDamage));
}
