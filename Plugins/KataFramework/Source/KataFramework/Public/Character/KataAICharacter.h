#pragma once

#include "Character/KataCharacter.h"
#include "KataAIPawnInterface.h"
#include "KataAICharacter.generated.h"

class UKataAIData;

/** KataAI Controller와 공용 Kata 캐릭터를 조합한다. 인지·공격 정책은 StateTree가 맡는다. */
UCLASS(Blueprintable, meta = (DisplayName = "Kata AI Character"))
class KATAFRAMEWORK_API AKataAICharacter : public AKataCharacter, public IKataAIPawnInterface
{
    GENERATED_BODY()

public:
    AKataAICharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void ApplyCharacterRow(const FInstancedStruct& RowData) override;
    virtual UKataAIData* GetKataAIData() const override { return AIData; }
    virtual bool IsKataAIReady() const override { return bAIReady; }

private:
    /** NPC 행에서 로드한 실행용 참조다. Blueprint 기본값과 배치 인스턴스에는 저장하지 않는다. */
    UPROPERTY(Transient)
    TObjectPtr<UKataAIData> AIData;

    bool bAIReady = false;
};
