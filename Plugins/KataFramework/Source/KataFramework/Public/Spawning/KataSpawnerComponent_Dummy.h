#pragma once

#include "CoreMinimal.h"
#include "Spawning/KataDummyComponent.h"
#include "Spawning/KataSpawnerComponent.h"
#include "KataSpawnerComponent_Dummy.generated.h"

/**
 * 이 스포너가 만드는 NPC를 더미로 쓰는 인라인 설정 객체. 생성 직후 NPC에 UKataDummyComponent를 붙이고 설정을 값으로 넘긴다.
 * 보호와 사망 뒤 재생성은 그 컴포넌트가 처리하며, 재생성은 이 스포너의 ScheduleRespawn으로 처음 생성한 Transform에 한다.
 * AI는 바꾸지 않는다. AI를 끄려면 AI Override의 Disable AI를 함께 쓴다.
 * 한 스포너에 하나만 활성화할 수 있으며, 둘 이상이면 생성 요청을 거절한다.
 */
UCLASS(EditInlineNew, DefaultToInstanced, CollapseCategories, meta = (DisplayName = "Dummy"))
class KATAFRAMEWORK_API UKataSpawnerComponent_Dummy : public UKataSpawnerComponent
{
    GENERATED_BODY()

public:
    /** 생성한 NPC에 적용할 더미 설정. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dummy", meta = (ShowOnlyInnerProperties))
    FKataDummySettings Settings;

    virtual void OnCharacterSpawned_Implementation(AKataCharacterSpawner* Spawner, AKataCharacter* Character,
        const FKataCharacterId& CharacterId) const override;
};
