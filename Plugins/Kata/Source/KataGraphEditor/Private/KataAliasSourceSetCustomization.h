#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomNodeBuilder.h"
#include "IPropertyTypeCustomization.h"
#include "UObject/WeakObjectPtr.h"

class IPropertyHandle;
class UEdGraph;
class UKataAliasNode;
class UKataNode;
struct FEdGraphEditAction;

/**
 * 별칭에 포함할 노드를 그래프에서 모아 체크 상자로 보여 준다.
 *
 * 노드는 에셋이 아니라 그래프 에셋의 하위 객체라서 기본 오브젝트 배열 UI로는 고를 수 없다.
 *
 * 그래프에 노드가 들어오거나 빠지면 목록만 다시 만든다. IDetailCustomNodeBuilder를 쓰는 이유가
 * 이것이다. 디테일 패널 전체를 다시 그리면 펼쳐 둔 상태가 풀려 목록이 계속 접힌다.
 */
class FKataAliasSourceNodeBuilder
	: public IDetailCustomNodeBuilder
	, public TSharedFromThis<FKataAliasSourceNodeBuilder>
{
public:
	FKataAliasSourceNodeBuilder(TSharedPtr<IPropertyHandle> InNodesHandle,
		TArray<TWeakObjectPtr<UKataAliasNode>> InEditedAliases);
	virtual ~FKataAliasSourceNodeBuilder() override;

	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren) override;
	/** 비워 둔다. 헤더에 내용이 없으면 이 행이 숨고 목록이 바로 이어져 단계가 하나 줄어든다. */
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override {}
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual FName GetName() const override { return TEXT("KataAliasSourceNodes"); }
	virtual bool InitiallyCollapsed() const override { return false; }
	virtual TSharedPtr<IPropertyHandle> GetPropertyHandle() const override { return NodesHandle; }

private:
	/** 목록 맨 위에 붙는 이름과 선택 개수. */
	FText GetSummaryText() const;

	ECheckBoxState GetNodeCheckState(TWeakObjectPtr<UKataNode> Node) const;
	void OnNodeCheckStateChanged(ECheckBoxState NewState, TWeakObjectPtr<UKataNode> Node);
	void OnGraphChanged(const FEdGraphEditAction& EditAction);

	/** 변경 알림을 보내 트랜잭션과 되돌리기가 걸리게 한다. */
	TSharedPtr<IPropertyHandle> NodesHandle;

	/** 편집 중인 별칭들. 다중 선택을 지원하려고 배열로 든다. */
	TArray<TWeakObjectPtr<UKataAliasNode>> EditedAliases;

	/** 목록만 다시 만들도록 디테일 패널이 준 통로. */
	FSimpleDelegate OnRegenerateChildren;

	/** 알림을 듣고 있는 그래프와 해제용 핸들. */
	TWeakObjectPtr<UEdGraph> ObservedGraph;
	FDelegateHandle GraphChangedHandle;
};

/**
 * 별칭의 출발지 집합을 노드 목록으로 바꿔 단다.
 *
 * 헤더를 비워 두면 FDetailPropertyRow::ShowOnlyChildren이 true가 되어 이 속성의 행이 숨고
 * 목록이 카테고리 바로 아래로 붙는다. 접었다 펴는 단계를 없애 갱신할 때마다 다시 접히지 않게 한다.
 */
class FKataAliasSourceSetCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle,
		class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle,
		class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	TSharedPtr<IPropertyHandle> NodesHandle;
	TArray<TWeakObjectPtr<UKataAliasNode>> EditedAliases;
};
