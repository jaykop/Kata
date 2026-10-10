#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KataGraphEditorScriptLibrary.generated.h"

class UKataGraphBase;
class UKataGraphEdgeBase;
class UKataGraphNodeBase;

/**
 * 에디터 스크립트(Python, Editor Utility)에서 Kata 그래프를 편집하는 함수 모음.
 *
 * 그래프 편집기와 같은 스키마 경로로 저작 노드와 엣지를 만든다. 런타임 노드 배열은 저작 그래프에서 다시 만들어지므로
 * 런타임 노드를 직접 고치지 말고 이 함수들로 편집한 뒤 RebuildGraph를 호출한다. 저장할 때는 저장 훅이 저장용 재구성을 한다.
 * 노드·엣지의 세부 값(액션, 트리거 태그, 조건 등)은 반환된 런타임 객체의 프로퍼티로 지정한다.
 * 각 편집은 실행 취소 트랜잭션으로 기록된다.
 */
UCLASS()
class UKataGraphEditorScriptLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * 그래프의 현재 페이지에서 저작한 노드를 돌려준다. 내장 서브그래프를 펼친 사본은 저작 노드가 아니므로 포함하지 않는다.
     * 그래프에 저작 그래프가 없으면 빈 배열이다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Graph Editing")
    static TArray<UKataGraphNodeBase*> GetAuthoredGraphNodes(UKataGraphBase* Graph);

    /**
     * NodeClass의 노드를 Position에 추가하고 그 런타임 노드를 돌려준다.
     * 추상 클래스, 그래프의 NodeType이나 CompatibleGraphType과 맞지 않는 클래스, 저작 그래프가 없는 그래프는 실패하고 nullptr을 돌려준다.
     * 런타임 노드 배열에는 RebuildGraph를 호출해야 들어간다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Graph Editing")
    static UKataGraphNodeBase* AddGraphNode(UKataGraphBase* Graph, TSubclassOf<UKataGraphNodeBase> NodeClass, FVector2D Position);

    /**
     * FromNode에서 ToNode로 연결하고, 엣지를 쓰는 그래프면 새 엣지를 돌려준다.
     * 두 노드는 이 그래프의 저작 노드여야 한다. 같은 노드끼리의 연결처럼 스키마가 허용하지 않는 연결은 경고를 남기고 nullptr을 돌려준다.
     * 엣지를 쓰지 않는 그래프는 연결에 성공해도 nullptr이다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Graph Editing")
    static UKataGraphEdgeBase* ConnectGraphNodes(UKataGraphBase* Graph, UKataGraphNodeBase* FromNode, UKataGraphNodeBase* ToNode);

    /**
     * 저작 노드를 지운다. 이 노드에 이어진 엣지도 함께 사라진다. 그래프 편집기의 삭제와 같은 순서로 처리한다.
     * 삭제할 수 없는 노드(루트 진입 노드 등)나 이 그래프의 저작 노드가 아니면 false다.
     * 내장 SubGraph 노드를 지워도 원본 정리는 하지 않는다. 원본 정리는 그래프 편집기의 삭제를 쓴다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Graph Editing")
    static bool RemoveGraphNode(UKataGraphBase* Graph, UKataGraphNodeBase* Node);

    /**
     * SubGraph·Port 노드가 내장 모드로 가리키는 원본 그래프를 돌려준다. 이 그래프로 다른 함수를 호출하면 내부 페이지를 편집한다.
     * 내장 모드가 아니거나 원본이 없으면 nullptr이다.
     */
    UFUNCTION(BlueprintCallable, Category = "Kata|Graph Editing")
    static UKataGraphBase* GetEmbeddedSubGraph(UKataGraphNodeBase* Node);

    /** 내장 원본의 표시 이름을 바꾼다. 공백뿐인 이름이나 내장 원본이 아니면 false다. 같은 이름 검사는 하지 않는다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Graph Editing")
    static bool SetEmbeddedSubGraphDisplayName(UKataGraphBase* EmbeddedGraph, const FText& DisplayName);

    /** 저작 노드의 편집기 위치. 찾지 못하면 (0, 0)이다. 새 노드를 기존 노드 옆에 놓을 때 쓴다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Graph Editing")
    static FVector2D GetGraphNodePosition(UKataGraphBase* Graph, UKataGraphNodeBase* Node);

    /** 저작 그래프에서 런타임 노드·엣지를 다시 만들고 편집기 화면과 패키지 변경 표시를 갱신한다. 저작 그래프가 없으면 false다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Graph Editing")
    static bool RebuildGraph(UKataGraphBase* Graph);
};
