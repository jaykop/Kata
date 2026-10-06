#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Widgets/Input/SComboBox.h"

class IPropertyHandle;
class UKataGraph;
class UKataSubGraphPortNode;

/** 콤보 항목은 소유 목록의 원본을 약한 참조로만 가리킨다. */
struct FKataEmbeddedSubGraphOption
{
    TWeakObjectPtr<UKataGraph> Graph;
};

/** 포트가 속한 그래프의 내장 원본을 단일 선택 콤보로 편집한다. */
class FKataEmbeddedSubGraphReferenceCustomization
    : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle,
        FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle,
        IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override {}

private:
    UKataGraph* GetCommonOwningGraph() const;
    void RefreshOptions();
    FText GetSelectedText() const;
    TSharedRef<SWidget> GenerateOption(TSharedPtr<FKataEmbeddedSubGraphOption> Option);
    void SelectOption(TSharedPtr<FKataEmbeddedSubGraphOption> Option, ESelectInfo::Type SelectInfo);
    bool CanSelectGraph() const;

    TSharedPtr<IPropertyHandle> StructHandle;
    TSharedPtr<IPropertyHandle> GraphHandle;
    TArray<TWeakObjectPtr<UKataSubGraphPortNode>> EditedPorts;
    TArray<TSharedPtr<FKataEmbeddedSubGraphOption>> Options;
    TSharedPtr<SComboBox<TSharedPtr<FKataEmbeddedSubGraphOption>>> ComboBox;
};
