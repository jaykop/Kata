#include "Attributes/KataStanceExecution.h"

#include "AbilitySystemComponent.h"
#include "Attributes/KataAttributeSet_Stance.h"
#include "Attributes/KataCombatSettings.h"
#include "GameplayEffect.h"

namespace KataStanceExecution
{
    /** 캡처 정의는 모든 Execution 인스턴스가 공유하므로 한 번만 만든다. */
    struct FCaptureDefinitions
    {
        FGameplayEffectAttributeCaptureDefinition PoiseDamageTakenMultiplier;

        FCaptureDefinitions()
            : PoiseDamageTakenMultiplier(UKataAttributeSet_Stance::GetPoiseDamageTakenMultiplierAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
        {
        }
    };

    static const FCaptureDefinitions& GetCaptureDefinitions()
    {
        static const FCaptureDefinitions Definitions;
        return Definitions;
    }

    /** 태그가 비어 있거나 스펙에 값이 없으면 0을 돌려준다. Poise·Groggy 피해는 공격마다 선택이므로 값이 없어도 경고하지 않는다. */
    static float GetOptionalSetByCaller(const FGameplayEffectSpec& Spec, const FGameplayTag& Tag)
    {
        return Tag.IsValid() ? Spec.GetSetByCallerMagnitude(Tag, false, 0.0f) : 0.0f;
    }
}

UKataStanceExecution::UKataStanceExecution()
{
    RelevantAttributesToCapture.Add(KataStanceExecution::GetCaptureDefinitions().PoiseDamageTakenMultiplier);
}

void UKataStanceExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
    FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    // 세트가 없는 ASC도 캡처 자체는 0으로 성공할 수 있으므로 세트 보유 여부를 따로 확인한다.
    const UAbilitySystemComponent* TargetAbilitySystem = ExecutionParams.GetTargetAbilitySystemComponent();
    if (TargetAbilitySystem == nullptr || !TargetAbilitySystem->HasAttributeSetForAttribute(UKataAttributeSet_Stance::GetPoiseDamageAttribute()))
    {
        return;
    }

    const UKataCombatSettings* Settings = UKataCombatSettings::Get();
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

    const float PoiseDamage = KataStanceExecution::GetOptionalSetByCaller(Spec, Settings->PoiseDamageSetByCallerTag);
    if (PoiseDamage > 0.0f)
    {
        FAggregatorEvaluateParameters EvaluateParameters;
        EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
        EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

        float Multiplier = 1.0f;
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
            KataStanceExecution::GetCaptureDefinitions().PoiseDamageTakenMultiplier, EvaluateParameters, Multiplier);

        const float FinalPoiseDamage = PoiseDamage * FMath::Max(Multiplier, 0.0f);
        if (FinalPoiseDamage > 0.0f)
        {
            OutExecutionOutput.AddOutputModifier(
                FGameplayModifierEvaluatedData(UKataAttributeSet_Stance::GetPoiseDamageAttribute(), EGameplayModOp::AddBase, FinalPoiseDamage));
        }
    }

    const float GroggyDamage = KataStanceExecution::GetOptionalSetByCaller(Spec, Settings->GroggyDamageSetByCallerTag);
    if (GroggyDamage > 0.0f)
    {
        OutExecutionOutput.AddOutputModifier(
            FGameplayModifierEvaluatedData(UKataAttributeSet_Stance::GetGroggyDamageAttribute(), EGameplayModOp::AddBase, GroggyDamage));
    }
}
