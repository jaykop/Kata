#include "Definition/KataAsset.h"

#include "Algo/Reverse.h"
#include "Definition/KataPropertyOverride.h"
#include "Definition/KataResolvedDefinition.h"
#include "Definition/KataTask.h"
#include "UObject/UnrealType.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UKataAsset::CollectAssetChain(TArray<const UKataAsset*>& OutChain) const
{
    OutChain.Reset();
    TSet<const UKataAsset*> Visited;
    for (const UKataAsset* Current = this; Current; Current = Current->ParentKata)
    {
        if (Visited.Contains(Current))
        {
            OutChain.Reset();
            return false;
        }
        Visited.Add(Current);
        OutChain.Add(Current);
    }
    Algo::Reverse(OutChain);
    return true;
}

UKataAsset* UKataAsset::MakeEffectiveSettings(UObject* Outer) const
{
    UKataAsset* Effective = NewObject<UKataAsset>(Outer ? Outer : GetTransientPackage(), NAME_None, RF_Transient | RF_Transactional);
    TArray<const UKataAsset*> Chain;
    if (!CollectAssetChain(Chain))
    {
        return Effective;
    }

    // 고유 설정만 합친다. 태스크와 부모 참조는 별도의 병합 규칙을 따른다.
    const TArray<FName> Settings = {
        TEXT("KataTags"), TEXT("ActivationRequiredTags"), TEXT("ActivationBlockedTags"),
        TEXT("ActiveGrantedTags"), TEXT("StartCondition"), TEXT("BlockingPolicy"),
        TEXT("CooldownPolicy"), TEXT("LoopPolicy")
    };
    for (int32 Index = 0; Index < Chain.Num(); ++Index)
    {
        const UKataAsset* Source = Chain[Index];
        const TArray<FName>& Paths = Index == 0 ? Settings : Source->OverriddenSettings;
        for (FName Path : Paths)
        {
            FString RootName;
            FString Tail;
            if (!Path.ToString().Split(TEXT("."), &RootName, &Tail))
            {
                RootName = Path.ToString();
            }
            if (Settings.Contains(FName(*RootName)))
            {
                FString Error;
                KataPropertyOverride::CopyOverriddenProperty(Effective, Source, Path, Error);
            }
        }
    }
    Effective->ParentKata = ParentKata;
    Effective->OverriddenSettings = OverriddenSettings;
#if WITH_EDITORONLY_DATA
    Effective->PreviewActorClass = PreviewActorClass;
    Effective->PreviewTargetClass = PreviewTargetClass;
    Effective->PreviewActorTransform = PreviewActorTransform;
    Effective->PreviewTargetTransform = PreviewTargetTransform;
    Effective->PreviewLightRotation = PreviewLightRotation;
    Effective->PreviewLightBrightness = PreviewLightBrightness;
    Effective->PreviewLightColor = PreviewLightColor;
    Effective->PreviewBackgroundColor = PreviewBackgroundColor;
    Effective->PreviewEnvironmentSize = PreviewEnvironmentSize;
    Effective->bPreviewShowDebugShape = bPreviewShowDebugShape;
    Effective->PreviewDebugShape = PreviewDebugShape;
    Effective->PreviewDebugColor = PreviewDebugColor;
    Effective->PreviewDebugThickness = PreviewDebugThickness;
    Effective->PreviewGridCellSize = PreviewGridCellSize;
#endif
    return Effective;
}

UKataResolvedDefinition* UKataAsset::Resolve(UObject* Outer, bool bForEditing) const
{
    UObject* ResultOuter = Outer ? Outer : GetTransientPackage();
    TArray<const UKataAsset*> Assets;
    if (!CollectAssetChain(Assets))
    {
        UKataResolvedDefinition* Result = NewObject<UKataResolvedDefinition>(ResultOuter);
        Result->SourceAsset = const_cast<UKataAsset*>(this);
        Result->AddDiagnostic(EKataDiagnosticSeverity::Error, TEXT("ParentAssetCycle"), {},
            TEXT("Parent Kata assets form a cycle"));
        return Result;
    }
    TArray<const UKataDefinition*> Chain;
    for (const UKataAsset* Asset : Assets)
    {
        Chain.Add(Asset);
    }
    UKataAsset* Effective = MakeEffectiveSettings(GetTransientPackage());
    UKataResolvedDefinition* Result = ResolveChain(Chain, Effective, ResultOuter, true, bForEditing);
    Result->SourceAsset = const_cast<UKataAsset*>(this);
    return Result;
}

#if WITH_EDITOR
void UKataAsset::PostEditChangeChainProperty(FPropertyChangedChainEvent& Event)
{
    // 오브젝트 에셋에는 클래스 선언자 도장이 필요하지 않다.
    for (FKataTimelineEntry& Entry : TimelineTasks)
    {
        if (Entry.Task)
        {
            Entry.Task->EnsureTaskId();
        }
    }
    if (ParentKata && Event.MemberProperty)
    {
        const FName Name = Event.MemberProperty->GetFName();
        if (Name != GET_MEMBER_NAME_CHECKED(UKataAsset, ParentKata)
            && Name != GET_MEMBER_NAME_CHECKED(UKataAsset, OverriddenSettings)
            && Name != GET_MEMBER_NAME_CHECKED(UKataDefinition, TimelineTasks)
            && Name != GET_MEMBER_NAME_CHECKED(UKataDefinition, TaskOverrides)
            && !Name.ToString().StartsWith(TEXT("Preview"))
            && !Name.ToString().StartsWith(TEXT("bPreview")))
        {
            OverriddenSettings.AddUnique(KataPropertyOverride::GetPropertyPath(Event.MemberProperty, Event.Property));
        }
    }
    UObject::PostEditChangeChainProperty(Event);
}

EDataValidationResult UKataAsset::IsDataValid(FDataValidationContext& Context) const
{
    UKataResolvedDefinition* Result = Resolve(GetTransientPackage());
    bool bInvalid = false;
    for (const FKataDiagnostic& Diagnostic : Result->Diagnostics)
    {
        if (Diagnostic.Severity == EKataDiagnosticSeverity::Info)
        {
            continue;
        }
        // 저작이 끝나지 않은 태스크는 저장을 막지 않는다. 실행에서는 그대로 제외한다.
        const bool bError = Diagnostic.Severity == EKataDiagnosticSeverity::Error && !Diagnostic.bIncompleteAuthoring;
        const FText Message = FText::FromString(Diagnostic.ToDetailString());
        if (bError)
        {
            Context.AddError(Message);
            bInvalid = true;
        }
        else
        {
            Context.AddWarning(Message);
        }
    }
    return bInvalid ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
