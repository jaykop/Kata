#pragma once

#include "CoreMinimal.h"
#include "KataGraphBase.h"
#include "KataGraph.generated.h"

class UKataGraph;

/** 저장 때 사용한 외장 원본의 세대. 강한 소유 참조를 만들지 않는다. */
USTRUCT()
struct KATAGRAPH_API FKataGraphDependencyGeneration
{
    GENERATED_BODY()

    UPROPERTY()
    TSoftObjectPtr<UKataGraph> Graph;

    UPROPERTY()
    FGuid Generation;
};

/**
 * 콤보 전이를 담는 그래프 에셋.
 *
 * 노드는 실행할 액션을, 엣지는 트리거 태그와 조건으로 전이를 정의한다.
 * 내장 서브그래프의 저작 데이터는 에디터 전용으로 소유한다.
 * 실행 데이터는 저장 시 부모 그래프로 펼쳐진 노드·엣지를 사용한다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Kata Graph"))
class KATAGRAPH_API UKataGraph : public UKataGraphBase
{
    GENERATED_BODY()

public:
    UKataGraph();

#if WITH_EDITORONLY_DATA
    /**
     * 이 그래프가 직접 소유한 내장 서브그래프의 저작 원본.
     *
     * 각 원본의 Outer는 이 그래프다. 마지막 저작 참조를 삭제하면 편집기가 원본도 제거한다.
     * 목록 편집은 그래프 에디터가 담당하며, 실행에 필요한 노드는 저장 시 부모에 복제한다.
     */
    UPROPERTY(Instanced)
    TArray<TObjectPtr<UKataGraph>> EmbeddedSubGraphs;

    /** 내장 서브그래프의 표시 이름. 내부 UObject 이름과 별개이며 그래프 에디터가 관리한다. */
    UPROPERTY(VisibleAnywhere, Category = "KataGraph|Editor")
    FText GraphDisplayName;

    /** 성공한 저장용 재구성의 세대. 화면 갱신과 내장 사본 복제로는 새 세대를 발급하지 않는다. */
    UPROPERTY()
    FGuid CompiledGeneration;

    /** 루트 에셋이 마지막 저장용 재구성에 사용한 외장 원본과 전이 의존의 세대. */
    UPROPERTY()
    TArray<FKataGraphDependencyGeneration> CompiledDependencies;
#endif

#if WITH_EDITOR
    /** 내장은 표시 이름을, 외장 에셋이나 표시 이름이 빈 내장은 오브젝트 이름을 반환한다. */
    FText GetGraphDisplayName() const;

    /**
     * 후보가 이 그래프의 직접 소유 목록에 있는 내장 원본인지 판정한다.
     *
     * 목록과 Outer가 모두 일치해야 한다. 다른 페이지의 내장을 선택할 수는 없다.
     * 원본을 수정하거나 소유를 변경하지 않는다.
     */
    bool OwnsEmbeddedSubGraph(const UKataGraph* Candidate) const;

    /** 소유 목록과 Outer가 일치하는 내장 경로를 따라 후보가 이 트리에 속하는지 판정한다. */
    bool ContainsGraph(const UKataGraph* Candidate) const;

    /** 소유 경로가 유효한 루트 에셋을 반환한다. 분리된 Undo 객체와 임시 사본은 nullptr이다. */
    const UKataGraph* GetRootGraph() const;
#endif
};
