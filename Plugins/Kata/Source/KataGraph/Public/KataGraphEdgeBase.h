#pragma once

#include "CoreMinimal.h"
#include "KataGraphNodeBase.h"
#include "UObject/Object.h"
#include "KataGraphEdgeBase.generated.h"

class UKataGraphBase;

/**
 * 두 노드를 잇는 전이의 공용 기반.
 *
 * 연결 정보만 담당하며 트리거·조건·타이밍 같은 Kata 고유 의미는 파생 클래스가 더한다.
 * 실행 상태는 이 객체에 저장하지 않는다.
 */
UCLASS(Blueprintable)
class KATAGRAPH_API UKataGraphEdgeBase : public UObject
{
    GENERATED_BODY()

public:
    UKataGraphEdgeBase();

    UPROPERTY(VisibleAnywhere, Category = "KataGraph|Edge")
    TObjectPtr<UKataGraphBase> Graph;

    UPROPERTY(BlueprintReadOnly, Category = "KataGraph|Edge")
    TObjectPtr<UKataGraphNodeBase> StartNode;

    UPROPERTY(BlueprintReadOnly, Category = "KataGraph|Edge")
    TObjectPtr<UKataGraphNodeBase> EndNode;

    UFUNCTION(BlueprintPure, Category = "KataGraph|Edge")
    UKataGraphBase* GetGraph() const;

#if WITH_EDITORONLY_DATA
    /** 그래프 화면에서 연결선 위에 제목을 그릴지. */
    UPROPERTY(EditDefaultsOnly, Category = "KataGraph|Edge|Editor")
    bool bShouldDrawTitle = false;

    UPROPERTY(EditDefaultsOnly, Category = "KataGraph|Edge|Editor")
    FText NodeTitle;

    UPROPERTY(EditDefaultsOnly, Category = "KataGraph|Edge|Editor")
    FLinearColor EdgeColour = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
#endif

#if WITH_EDITOR
    virtual FText GetNodeTitle() const { return NodeTitle; }
    FLinearColor GetEdgeColour() const { return EdgeColour; }

    virtual void SetNodeTitle(const FText& NewTitle);
#endif
};
