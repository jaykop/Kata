#pragma once

#include "CoreMinimal.h"
#include "Data/KataAIData.h"
#include "Spawning/KataSpawnerComponent.h"
#include "KataSpawnerComponent_AIOverride.generated.h"

/**
 * 이 스포너가 만드는 NPC의 AI 설정을 배치 단위로 덮어쓰는 인라인 설정 객체.
 *
 * 배치를 시작할 때 행 사본에 기록하므로 캐릭터 BeginPlay 전에 적용되고, 다음 배치부터 반영된다. 원본 테이블과 AI Data 에셋은 바뀌지 않는다.
 * AI Data를 지정하면 행의 AI Data 대신 기준으로 쓴다. 슬롯·LeashDistance 덮어쓰기는 기준 AI Data의 캐릭터별 사본에 적용한다.
 * 기준 AI Data가 없으면 덮어쓰기를 무시하고 경고 후 AI를 시작하지 않는다. 의도적으로 AI 없이 생성하려면 bDisableAI를 쓴다.
 * 한 스포너에 하나만 활성화할 수 있으며, 둘 이상이면 생성 요청을 거절한다.
 */
UCLASS(EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "AI Override"))
class KATAFRAMEWORK_API UKataSpawnerComponent_AIOverride : public UKataSpawnerComponent
{
    GENERATED_BODY()

public:
    /**
     * 행의 AI Data를 비워 인지·행동 로직 없이 생성한다. Controller는 행·캐릭터 설정대로 빙의하지만 StateTree와 Perception을 시작하지 않는다.
     * 켜면 AIData와 덮어쓰기 값은 무시한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
    bool bDisableAI = false;

    /** 행의 AI Data 대신 쓸 AI Data. 비우면 행의 값을 유지한다. 생성 전에 비동기로 로드한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (EditCondition = "!bDisableAI"))
    TSoftObjectPtr<UKataAIData> AIData;

    /** 기준 AI Data 위에 덮어쓸 배치 단위 값. 모두 비우면 기준 AI Data를 복제하지 않고 그대로 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (EditCondition = "!bDisableAI"))
    FKataAIDataOverride Overrides;

    virtual void ModifySpawnRow(FInstancedStruct& RowData) const override;
};
