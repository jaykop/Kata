#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"
#include "KataActionGroup.generated.h"

class UKataAction;
class UKataGraph;

/** 그룹 항목이 참조하는 실행 에셋의 종류다. */
UENUM(BlueprintType)
enum class EKataActionGroupEntryType : uint8
{
    Action,
    Graph
};

/** 항목의 추가 설정을 위한 기반이다. 소비 모듈이 파생 구조체의 의미를 정의한다. */
USTRUCT(BlueprintType)
struct KATAGRAPH_API FKataActionGroupPayload
{
    GENERATED_BODY()
};

/** 실행 대상과 선택 가중치다. 숨겨진 비활성 에셋 참조는 저장하되 실행에 사용하지 않는다. */
USTRUCT(BlueprintType)
struct KATAGRAPH_API FKataActionGroupEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Action Group")
    EKataActionGroupEntryType Type = EKataActionGroupEntryType::Action;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Action Group",
        meta = (EditCondition = "Type == EKataActionGroupEntryType::Action", EditConditionHides))
    TObjectPtr<UKataAction> Action = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Action Group",
        meta = (EditCondition = "Type == EKataActionGroupEntryType::Graph", EditConditionHides))
    TObjectPtr<UKataGraph> Graph = nullptr;

    /** 0은 선택에서 제외한다. 유한한 양수만 확률에 기여한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Action Group", meta = (ClampMin = "0.0"))
    float Weight = 1.0f;

    /** 빈 값도 허용한다. 실행 인스턴스나 예약 핸들 대신 공유 가능한 설정만 넣는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Action Group",
        meta = (BaseStruct = "/Script/KataGraph.KataActionGroupPayload", ExcludeBaseStruct))
    FInstancedStruct Payload;

    /** 선택된 Type의 에셋 참조만 검사한다. 실제 실행 가능 여부는 소비자가 판단한다. */
    bool HasValidAsset() const;

    /** 빈 값 또는 Payload 기반의 파생 구조체인지 검사한다. 필드의 의미는 판단하지 않는다. */
    bool HasValidPayload() const;
};

/** 한 번 선택한 항목의 사본이다. 그룹의 배열을 실행 중 수정하지 않는 계약을 따른다. */
USTRUCT(BlueprintType)
struct KATAGRAPH_API FKataActionGroupSelection
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Action Group")
    int32 EntryIndex = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Action Group")
    FKataActionGroupEntry Entry;
};

/** Action·Graph의 가중 선택 설정이다. 현재 대상·추첨 결과·실행 상태는 소비자가 소유한다. */
UCLASS(BlueprintType, NotBlueprintable, meta = (DisplayName = "Kata Action Group"))
class KATAGRAPH_API UKataActionGroup : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Action Group", meta = (TitleProperty = "Type"))
    TArray<FKataActionGroupEntry> Entries;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
