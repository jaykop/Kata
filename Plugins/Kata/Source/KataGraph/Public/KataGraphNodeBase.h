#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"
#include "KataGraphNodeBase.generated.h"

class UKataGraphBase;
class UKataGraphEdgeBase;

/** 노드가 가질 수 있는 연결 개수 제한 방식. */
UENUM(BlueprintType)
enum class EKataGraphNodeLimit : uint8
{
    Unlimited,
    Limited
};

/**
 * 같은 두 노드 사이를 잇는 엣지 목록.
 *
 * UHT가 TMultiMap과 중첩 컨테이너를 리플렉션 대상으로 지원하지 않으므로
 * TMap의 값 타입을 구조체로 감싸 복수 엣지를 표현한다.
 */
USTRUCT(BlueprintType)
struct KATAGRAPH_API FKataGraphEdgeList
{
    GENERATED_BODY()

    /** 선언 순서를 유지한다. 평가 우선순위는 엣지 자신이 정한다. */
    UPROPERTY(BlueprintReadOnly, Category = "KataGraph")
    TArray<TObjectPtr<UKataGraphEdgeBase>> Edges;
};

/**
 * 그래프 노드의 공용 기반.
 *
 * 부모·자식 관계와 엣지 소유만 담당하며 Kata 고유 의미는 파생 클래스가 더한다.
 * 실행 상태는 이 객체에 저장하지 않는다.
 */
UCLASS(Blueprintable)
class KATAGRAPH_API UKataGraphNodeBase : public UObject
{
    GENERATED_BODY()

public:
    UKataGraphNodeBase();

    /** 이 노드를 소유한 그래프. */
    UPROPERTY(VisibleDefaultsOnly, Category = "KataGraph|Node")
    TObjectPtr<UKataGraphBase> Graph;

    UPROPERTY(BlueprintReadOnly, Category = "KataGraph|Node")
    TArray<TObjectPtr<UKataGraphNodeBase>> ParentNodes;

    UPROPERTY(BlueprintReadOnly, Category = "KataGraph|Node")
    TArray<TObjectPtr<UKataGraphNodeBase>> ChildrenNodes;

    /** 자식 노드별 엣지 목록. 같은 자식에 여러 엣지를 둘 수 있다. */
    UPROPERTY(BlueprintReadOnly, Category = "KataGraph|Node")
    TMap<TObjectPtr<UKataGraphNodeBase>, FKataGraphEdgeList> Edges;

    /** 지정한 자식으로 가는 첫 엣지. 없으면 null이다. */
    UFUNCTION(BlueprintCallable, Category = "KataGraph|Node")
    virtual UKataGraphEdgeBase* GetEdge(UKataGraphNodeBase* ChildNode) const;

    /** 지정한 자식으로 가는 모든 엣지를 선언 순서대로 모은다. */
    UFUNCTION(BlueprintCallable, Category = "KataGraph|Node")
    void GetEdgesTo(UKataGraphNodeBase* ChildNode, TArray<UKataGraphEdgeBase*>& OutEdges) const;

    /** 이 노드에서 나가는 모든 엣지를 모은다. 자식 순회 순서를 따른다. */
    UFUNCTION(BlueprintCallable, Category = "KataGraph|Node")
    void GetOutgoingEdges(TArray<UKataGraphEdgeBase*>& OutEdges) const;

    /** 자식으로 가는 엣지를 추가한다. 이미 있는 목록에 덧붙인다. */
    void AddEdge(UKataGraphNodeBase* ChildNode, UKataGraphEdgeBase* Edge);

    UFUNCTION(BlueprintCallable, Category = "KataGraph|Node")
    bool IsLeafNode() const;

    UFUNCTION(BlueprintCallable, Category = "KataGraph|Node")
    UKataGraphBase* GetGraph() const;

    /** 그래프 화면과 진단에 표시할 설명. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KataGraph|Node")
    FText GetDescription() const;
    virtual FText GetDescription_Implementation() const;

#if WITH_EDITORONLY_DATA
    UPROPERTY(EditDefaultsOnly, Category = "KataGraph|Node|Editor")
    FText NodeTitle;

    /** 이 노드를 놓을 수 있는 그래프 타입. */
    UPROPERTY(VisibleDefaultsOnly, Category = "KataGraph|Node|Editor")
    TSubclassOf<UKataGraphBase> CompatibleGraphType;

    UPROPERTY(EditDefaultsOnly, Category = "KataGraph|Node|Editor")
    FLinearColor BackgroundColor;

    /** 그래프 우클릭 메뉴에 표시할 이름. */
    UPROPERTY(EditDefaultsOnly, Category = "KataGraph|Node|Editor")
    FText ContextMenuName;

    UPROPERTY(EditDefaultsOnly, Category = "KataGraph|Node|Editor")
    EKataGraphNodeLimit ParentLimitType;

    UPROPERTY(EditDefaultsOnly, Category = "KataGraph|Node|Editor", meta = (ClampMin = "0", EditCondition = "ParentLimitType == EKataGraphNodeLimit::Limited", EditConditionHides))
    int32 ParentLimit;

    UPROPERTY(EditDefaultsOnly, Category = "KataGraph|Node|Editor")
    EKataGraphNodeLimit ChildrenLimitType;

    UPROPERTY(EditDefaultsOnly, Category = "KataGraph|Node|Editor", meta = (ClampMin = "0", EditCondition = "ChildrenLimitType == EKataGraphNodeLimit::Limited", EditConditionHides))
    int32 ChildrenLimit;
#endif

#if WITH_EDITOR
    virtual bool IsNameEditable() const;
    virtual FLinearColor GetBackgroundColor() const;
    virtual FText GetNodeTitle() const;
    virtual void SetNodeTitle(const FText& NewTitle);

    /** 연결 자체가 허용되는지. 파생 노드가 고유 규칙을 더한다. */
    virtual bool CanCreateConnection(UKataGraphNodeBase* Other, FText& ErrorMessage);

    virtual bool CanCreateConnectionTo(UKataGraphNodeBase* Other, int32 NumberOfChildrenNodes, FText& ErrorMessage);
    virtual bool CanCreateConnectionFrom(UKataGraphNodeBase* Other, int32 NumberOfParentNodes, FText& ErrorMessage);
#endif
};
