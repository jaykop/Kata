#include "KataMVVMAuthoringToolset.h"

#include "Editor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "MVVMBlueprintView.h"
#include "MVVMBlueprintViewModelContext.h"
#include "MVVMDeveloperProjectSettings.h"
#include "MVVMEditorSubsystem.h"
#include "ScopedTransaction.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WidgetBlueprint.h"

#define LOCTEXT_NAMESPACE "KataMVVMAuthoringToolset"

namespace
{
    UMVVMBlueprintView* RequestWidgetView(UWidgetBlueprint* WidgetBlueprint)
    {
        if (!IsValid(WidgetBlueprint))
        {
            UKismetSystemLibrary::RaiseScriptError(TEXT("Invalid widget blueprint."));
            return nullptr;
        }

        UMVVMEditorSubsystem* MVVMEditorSubsystem = GEditor ? GEditor->GetEditorSubsystem<UMVVMEditorSubsystem>() : nullptr;
        if (!IsValid(MVVMEditorSubsystem))
        {
            UKismetSystemLibrary::RaiseScriptError(TEXT("UMVVMEditorSubsystem is not loaded."));
            return nullptr;
        }

        UMVVMBlueprintView* View = MVVMEditorSubsystem->RequestView(WidgetBlueprint);
        if (!IsValid(View))
        {
            UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Failed to get the MVVM view of '%s'."), *WidgetBlueprint->GetPathName()));
        }
        return View;
    }

    FString GetCreationTypeName(EMVVMBlueprintViewModelContextCreationType CreationType)
    {
        return StaticEnum<EMVVMBlueprintViewModelContextCreationType>()->GetNameStringByValue(static_cast<int64>(CreationType));
    }
}

TArray<FKataMVVMViewModelContextInfo> UKataMVVMAuthoringToolset::ListViewModelContexts(UWidgetBlueprint* WidgetBlueprint)
{
    TArray<FKataMVVMViewModelContextInfo> Result;
    const UMVVMBlueprintView* View = RequestWidgetView(WidgetBlueprint);
    if (!View)
    {
        return Result;
    }

    for (const FMVVMBlueprintViewModelContext& Context : View->GetViewModels())
    {
        FKataMVVMViewModelContextInfo& Info = Result.AddDefaulted_GetRef();
        Info.ViewModelName = Context.GetViewModelName();
        Info.ViewModelClassPath = Context.GetViewModelClass() ? Context.GetViewModelClass()->GetPathName() : FString();
        Info.CreationType = GetCreationTypeName(Context.CreationType);
        Info.ResolverClassPath = Context.Resolver ? Context.Resolver->GetClass()->GetPathName() : FString();
    }
    return Result;
}

bool UKataMVVMAuthoringToolset::SetViewModelCreationType(UWidgetBlueprint* WidgetBlueprint, FName ViewModelName, const FString& CreationType, UClass* ResolverClass)
{
    UMVVMBlueprintView* View = RequestWidgetView(WidgetBlueprint);
    if (!View)
    {
        return false;
    }

    // 이름으로 찾는 FindViewModel은 const만 제공하므로 Id를 얻은 뒤 수정 가능한 컨텍스트를 다시 찾는다.
    const FMVVMBlueprintViewModelContext* FoundContext = View->FindViewModel(ViewModelName);
    FMVVMBlueprintViewModelContext* Context = FoundContext ? View->FindViewModel(FoundContext->GetViewModelId()) : nullptr;
    if (!Context)
    {
        UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Viewmodel '%s' was not found in '%s'."), *ViewModelName.ToString(), *WidgetBlueprint->GetPathName()));
        return false;
    }

    const int64 EnumValue = StaticEnum<EMVVMBlueprintViewModelContextCreationType>()->GetValueByNameString(CreationType);
    if (EnumValue == INDEX_NONE)
    {
        UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Unknown creation type '%s'."), *CreationType));
        return false;
    }

    const EMVVMBlueprintViewModelContextCreationType NewCreationType = static_cast<EMVVMBlueprintViewModelContextCreationType>(EnumValue);
    const UClass* ViewModelClass = Context->GetViewModelClass();

    // ViewModel 클래스 메타데이터가 허용 생성 방식을 제한할 수 있다. 디테일 패널의 선택지와 같은 기준이다.
    if (!UE::MVVM::GetAllowedContextCreationType(ViewModelClass).Contains(NewCreationType))
    {
        UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Creation type '%s' is not allowed for '%s'."), *CreationType, *GetNameSafe(ViewModelClass)));
        return false;
    }

    // Resolver는 컨텍스트를 바꾸기 전에 만든다. 만들 수 없으면 아무것도 바꾸지 않고 실패한다.
    // Resolver 프로퍼티는 Instanced이므로 컨텍스트를 소유한 View 아래에 인스턴스를 만든다.
    UMVVMViewModelContextResolver* NewResolver = nullptr;
    if (NewCreationType == EMVVMBlueprintViewModelContextCreationType::Resolver)
    {
        if (ResolverClass)
        {
            const UMVVMViewModelContextResolver* ResolverCDO = ResolverClass->IsChildOf<UMVVMViewModelContextResolver>()
                && !ResolverClass->HasAnyClassFlags(CLASS_Abstract)
                ? GetDefault<UMVVMViewModelContextResolver>(ResolverClass)
                : nullptr;
            if (!ResolverCDO)
            {
                UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("'%s' is not a concrete viewmodel resolver class."), *ResolverClass->GetPathName()));
                return false;
            }
            if (!ResolverCDO->DoesSupportViewModelClass(ViewModelClass))
            {
                UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Resolver '%s' does not support '%s'."), *ResolverClass->GetName(), *GetNameSafe(ViewModelClass)));
                return false;
            }
            NewResolver = NewObject<UMVVMViewModelContextResolver>(View, ResolverClass, NAME_None, RF_Public | RF_Transactional);
        }
        else
        {
            // 클래스를 지정하지 않으면 엔진처럼 프로젝트 설정의 기본 Resolver를 쓴다.
            // CreateDefaultResolver는 컨텍스트의 생성 방식이 Resolver일 때만 동작하므로 바꾼 사본으로 호출한다.
            FMVVMBlueprintViewModelContext ResolverContext = *Context;
            ResolverContext.CreationType = NewCreationType;
            NewResolver = ResolverContext.CreateDefaultResolver(View);
            if (!NewResolver)
            {
                UKismetSystemLibrary::RaiseScriptError(TEXT("No resolver class was given and the project has no default resolver for this viewmodel."));
                return false;
            }
        }
    }

    const FScopedTransaction Transaction(LOCTEXT("SetViewModelCreationType", "Set viewmodel creation type"));
    View->Modify();

    // 디테일 패널(FBlueprintViewModelContextDetailCustomization)이 생성 방식을 바꿀 때 맞추는 값과 같게 둔다.
    // Manual과 Context는 값이 없을 수 있으므로 항상 Optional이고, Manual만 외부에서 넣을 Setter를 만든다.
    Context->CreationType = NewCreationType;
    Context->bOptional = NewCreationType == EMVVMBlueprintViewModelContextCreationType::Manual
        || NewCreationType == EMVVMBlueprintViewModelContextCreationType::Context;
    Context->bCreateSetterFunction = NewCreationType == EMVVMBlueprintViewModelContextCreationType::Manual;
    if (!Context->bCreateSetterFunction && Context->bOverrideForceExecuteBindingsOnSetSource)
    {
        Context->bOverrideForceExecuteBindingsOnSetSource = false;
        Context->bForceExecuteBindingsOnSetSource = GetDefault<UMVVMDeveloperProjectSettings>()->bForceExecuteBindingsOnSetSource;
    }
    if (NewResolver)
    {
        Context->Resolver = NewResolver;
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
    return true;
}

#undef LOCTEXT_NAMESPACE
