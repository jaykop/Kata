#include "KataEmbeddedSubGraphReferenceCustomization.h"

#include "DetailWidgetRow.h"
#include "KataGraph.h"
#include "KataSubGraphPortNode.h"
#include "PropertyHandle.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "KataEmbeddedSubGraphReference"

TSharedRef<IPropertyTypeCustomization> FKataEmbeddedSubGraphReferenceCustomization::MakeInstance()
{
    return MakeShared<FKataEmbeddedSubGraphReferenceCustomization>();
}

void FKataEmbeddedSubGraphReferenceCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle,
    FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    StructHandle = StructPropertyHandle;
    GraphHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKataEmbeddedSubGraphReference, Graph));

    TArray<UObject*> OuterObjects;
    StructPropertyHandle->GetOuterObjects(OuterObjects);
    for (UObject* Object : OuterObjects)
    {
        if (UKataSubGraphPortNode* Port = Cast<UKataSubGraphPortNode>(Object))
        {
            EditedPorts.Add(Port);
        }
    }

    RefreshOptions();
    HeaderRow.NameContent()
    [
        StructPropertyHandle->CreatePropertyNameWidget()
    ]
    .ValueContent().MinDesiredWidth(180.0f)
    [
        SAssignNew(ComboBox, SComboBox<TSharedPtr<FKataEmbeddedSubGraphOption>>)
        .OptionsSource(&Options)
        .IsEnabled(this, &FKataEmbeddedSubGraphReferenceCustomization::CanSelectGraph)
        .OnComboBoxOpening(this, &FKataEmbeddedSubGraphReferenceCustomization::RefreshOptions)
        .OnGenerateWidget(this, &FKataEmbeddedSubGraphReferenceCustomization::GenerateOption)
        .OnSelectionChanged(this, &FKataEmbeddedSubGraphReferenceCustomization::SelectOption)
        [
            SNew(STextBlock)
            .Text(this, &FKataEmbeddedSubGraphReferenceCustomization::GetSelectedText)
            .Font(IPropertyTypeCustomizationUtils::GetRegularFont())
        ]
    ];
}

UKataGraph* FKataEmbeddedSubGraphReferenceCustomization::GetCommonOwningGraph() const
{
    UKataGraph* CommonGraph = nullptr;
    for (const TWeakObjectPtr<UKataSubGraphPortNode>& WeakPort : EditedPorts)
    {
        const UKataSubGraphPortNode* Port = WeakPort.Get();
        UKataGraph* Owner = Port != nullptr ? Port->GetTypedOuter<UKataGraph>() : nullptr;
        if (Owner == nullptr || (CommonGraph != nullptr && CommonGraph != Owner))
        {
            return nullptr;
        }
        CommonGraph = Owner;
    }
    return CommonGraph;
}

void FKataEmbeddedSubGraphReferenceCustomization::RefreshOptions()
{
    Options.Reset();
    Options.Add(MakeShared<FKataEmbeddedSubGraphOption>());
    if (const UKataGraph* Owner = GetCommonOwningGraph())
    {
        // 콤보를 열 때 소유 목록을 다시 읽어 생성·삭제·Undo 이후의 후보를 반영한다.
        for (UKataGraph* Graph : Owner->EmbeddedSubGraphs)
        {
            if (Owner->OwnsEmbeddedSubGraph(Graph))
            {
                TSharedPtr<FKataEmbeddedSubGraphOption> Option = MakeShared<FKataEmbeddedSubGraphOption>();
                Option->Graph = Graph;
                Options.Add(Option);
            }
        }
    }
    if (ComboBox.IsValid())
    {
        ComboBox->RefreshOptions();
    }
}

FText FKataEmbeddedSubGraphReferenceCustomization::GetSelectedText() const
{
    UObject* Value = nullptr;
    if (GraphHandle.IsValid())
    {
        const FPropertyAccess::Result Result = GraphHandle->GetValue(Value);
        if (Result == FPropertyAccess::MultipleValues)
        {
            return LOCTEXT("MultipleValues", "Multiple Values");
        }
        const UKataGraph* Owner = GetCommonOwningGraph();
        const UKataGraph* Graph = Cast<UKataGraph>(Value);
        if (Result == FPropertyAccess::Success && Owner != nullptr && Owner->OwnsEmbeddedSubGraph(Graph))
        {
            return Graph->GetGraphDisplayName();
        }
    }
    return LOCTEXT("None", "(None)");
}

TSharedRef<SWidget> FKataEmbeddedSubGraphReferenceCustomization::GenerateOption(TSharedPtr<FKataEmbeddedSubGraphOption> Option)
{
    return SNew(STextBlock)
        .Text_Lambda([Option]()
            {
                return Option.IsValid() && Option->Graph.IsValid()
                    ? Option->Graph->GetGraphDisplayName()
                    : LOCTEXT("None", "(None)");
            });
}

void FKataEmbeddedSubGraphReferenceCustomization::SelectOption(TSharedPtr<FKataEmbeddedSubGraphOption> Option,
    ESelectInfo::Type SelectInfo)
{
    if (!Option.IsValid() || !CanSelectGraph())
    {
        return;
    }
    UKataGraph* Graph = Option->Graph.Get();
    const UKataGraph* Owner = GetCommonOwningGraph();
    if (Graph != nullptr && (Owner == nullptr || !Owner->OwnsEmbeddedSubGraph(Graph)))
    {
        return;
    }

    // 참조 변경 알림이 Details를 다시 만들더라도 현재 콜백의 커스터마이제이션은 끝까지 유지한다.
    const TSharedRef<IPropertyTypeCustomization> KeepAlive = AsShared();
    const TSharedPtr<IPropertyHandle> SelectedGraphHandle = GraphHandle;
    // PropertyHandle이 트랜잭션과 부모 UObject의 변경 알림을 처리한다.
    UObject* Value = Graph;
    if (SelectedGraphHandle->SetValue(Value) == FPropertyAccess::Success)
    {
        SelectedGraphHandle->NotifyFinishedChangingProperties();
    }
}

bool FKataEmbeddedSubGraphReferenceCustomization::CanSelectGraph() const
{
    if (!GraphHandle.IsValid() || !StructHandle.IsValid() || !StructHandle->IsEditable() || GetCommonOwningGraph() == nullptr)
    {
        return false;
    }
    for (const TWeakObjectPtr<UKataSubGraphPortNode>& Port : EditedPorts)
    {
        if (!Port.IsValid() || !Port->bUseEmbeddedSubGraph)
        {
            return false;
        }
    }
    return EditedPorts.Num() > 0;
}

#undef LOCTEXT_NAMESPACE
