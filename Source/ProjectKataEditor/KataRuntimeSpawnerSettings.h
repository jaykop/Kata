#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "KataRuntimeSpawnerSettings.generated.h"

class UDataTable;

/**
 * 런타임 스포너 도구가 행을 고를 NPC 테이블 목록이다. 팀이 공유하도록 DefaultEditor.ini에 저장한다.
 * 행 구조가 FKataNPCCharacterRow 계열이 아닌 테이블은 도구가 목록에서 제외한다.
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Kata Runtime Spawner"))
class UKataRuntimeSpawnerSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UKataRuntimeSpawnerSettings();

    /** 도구의 테이블 드롭다운에 보일 NPC 테이블. 데이터 컬렉션에 등록되지 않은 테이블도 쓸 수 있다. */
    UPROPERTY(config, EditAnywhere, Category = "Runtime Spawner")
    TArray<TSoftObjectPtr<UDataTable>> NPCTables;
};
