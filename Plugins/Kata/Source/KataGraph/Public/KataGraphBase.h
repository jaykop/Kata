#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataGraphEdgeBase.h"
#include "KataGraphNodeBase.h"
#include "UObject/Object.h"
#include "KataGraphBase.generated.h"

class UEdGraph;

/**
 * 노드와 엣지를 담는 그래프 에셋의 공용 기반.
 *
 * 순회 편의와 편집기 연결만 담당하며 Kata 고유 설정은 파생 클래스가 더한다.
 * 실행 상태는 이 객체에 저장하지 않는다.
 */
UCLASS(Blueprintable)
class KATAGRAPH_API UKataGraphBase : public UObject
{
    GENERATED_BODY()

public:
    UKataGraphBase();

    /** 이 그래프에서 새로 만들 노드의 기본 클래스. */
    UPROPERTY(EditDefaultsOnly, Category = "KataGraph")
    TSubclassOf<UKataGraphNodeBase> NodeType;

    /** 이 그래프에서 새로 만들 엣지의 기본 클래스. */
    UPROPERTY(EditDefaultsOnly, Category = "KataGraph")
    TSubclassOf<UKataGraphEdgeBase> EdgeType;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KataGraph")
    FGameplayTagContainer GraphTags;

    /** 부모가 없는 시작 노드들. */
    UPROPERTY(BlueprintReadOnly, Category = "KataGraph")
    TArray<TObjectPtr<UKataGraphNodeBase>> RootNodes;

    UPROPERTY(BlueprintReadOnly, Category = "KataGraph")
    TArray<TObjectPtr<UKataGraphNodeBase>> AllNodes;

    /** 끄면 연결선이 엣지 객체 없이 그려진다. Kata 그래프는 엣지를 사용한다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KataGraph")
    bool bEdgeEnabled;

    /** 루트부터 단계별로 노드를 로그와 화면에 출력한다. 진단용이다. */
    UFUNCTION(BlueprintCallable, Category = "KataGraph")
    void Print(bool bToConsole = true, bool bToScreen = true) const;

    /**
     * 루트에서 가장 먼 노드까지의 단계 수.
     * 순환 그래프에서도 끝나도록 이미 지난 노드는 다시 내려가지 않는다.
     */
    UFUNCTION(BlueprintCallable, Category = "KataGraph")
    int32 GetLevelNum() const;

    /** 지정한 단계에 있는 노드를 모은다. 순환 그래프에서도 각 노드는 한 번만 방문한다. */
    UFUNCTION(BlueprintCallable, Category = "KataGraph")
    void GetNodesByLevel(int32 Level, TArray<UKataGraphNodeBase*>& OutNodes) const;

    /** 노드의 연결 정보와 목록을 모두 비운다. */
    void ClearGraph();

#if WITH_EDITORONLY_DATA
    /** 편집기 그래프 표현. 실행에는 사용하지 않는다. */
    UPROPERTY()
    TObjectPtr<UEdGraph> EdGraph;

    UPROPERTY(EditDefaultsOnly, Category = "KataGraph|Editor")
    bool bCanRenameNode;

    /** 콤보가 중립 상태로 돌아오려면 순환이 필요하므로 기본값을 켠다. */
    UPROPERTY(EditDefaultsOnly, Category = "KataGraph|Editor")
    bool bCanBeCyclical;
#endif
};
