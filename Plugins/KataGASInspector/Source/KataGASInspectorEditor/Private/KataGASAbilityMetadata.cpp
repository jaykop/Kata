#include "KataGASAbilityMetadata.h"

#include "Abilities/GameplayAbility.h"
#include "GameplayTask.h"
#include "UObject/UnrealType.h"

bool FKataGASAbilityMetadata::ReadTriggers(const UGameplayAbility* Ability, TArray<FAbilityTriggerData>& OutTriggers)
{
    OutTriggers.Reset();
    if (!IsValid(Ability))
    {
        return false;
    }
    const FArrayProperty* Array = FindFProperty<FArrayProperty>(Ability->GetClass(), TEXT("AbilityTriggers"));
    const FStructProperty* Inner = Array ? CastField<FStructProperty>(Array->Inner) : nullptr;
    if (!Inner || Inner->Struct != FAbilityTriggerData::StaticStruct())
    {
        return false;
    }
    FScriptArrayHelper Values(Array, Array->ContainerPtrToValuePtr<void>(Ability));
    for (int32 Index = 0; Index < Values.Num(); ++Index)
    {
        FAbilityTriggerData Trigger;
        Inner->CopyCompleteValue(&Trigger, Values.GetRawPtr(Index));
        OutTriggers.Add(Trigger);
    }
    return true;
}

bool FKataGASAbilityMetadata::ReadTasks(UGameplayAbility* Ability, const FString& ParentKey,
    TArray<TSharedPtr<FKataGASInspectionRow>>& OutRows)
{
    if (!IsValid(Ability))
    {
        return false;
    }
    const FArrayProperty* Array = FindFProperty<FArrayProperty>(Ability->GetClass(), TEXT("ActiveTasks"));
    const FObjectPropertyBase* Inner = Array ? CastField<FObjectPropertyBase>(Array->Inner) : nullptr;
    if (!Inner || !Inner->PropertyClass->IsChildOf(UGameplayTask::StaticClass()))
    {
        return false;
    }
    FScriptArrayHelper Values(Array, Array->ContainerPtrToValuePtr<void>(Ability));
    for (int32 Index = 0; Index < Values.Num(); ++Index)
    {
        // 오브젝트 프로퍼티 접근자는 TObjectPtr의 저장 표현과 해석을 엔진에 맡긴다.
        UGameplayTask* Task = Cast<UGameplayTask>(Inner->GetObjectPropertyValue(Values.GetRawPtr(Index)));
        if (!IsValid(Task))
        {
            continue;
        }
        TSharedPtr<FKataGASInspectionRow> Row = MakeShared<FKataGASInspectionRow>();
        Row->Page = EKataGASInspectionPage::Abilities;
        Row->Key = ParentKey + TEXT("/task/") + FString::FromInt(Task->GetUniqueID());
        Row->Name = Task->GetName();
        Row->State = Task->GetTaskStateName();
        Row->Value = Task->GetInstanceName().ToString();
        Row->Source = Task->GetClass()->GetPathName();
        Row->Detail = Task->GetDebugString();
        OutRows.Add(Row);
    }
    return true;
}

FSoftObjectPath FKataGASAbilityMetadata::GetSourceAsset(const UClass* Class)
{
    if (IsValid(Class) && IsValid(Class->ClassGeneratedBy))
    {
        return FSoftObjectPath(Class->ClassGeneratedBy);
    }
    return FSoftObjectPath();
}
