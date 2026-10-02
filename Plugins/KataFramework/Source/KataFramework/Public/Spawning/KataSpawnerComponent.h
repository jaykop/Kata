#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/Object.h"
#include "KataSpawnerComponent.generated.h"

class AKataCharacter;
class AKataCharacterSpawner;

/**
 * 스포너 Details에 인라인으로 추가하는 설정 컴포넌트의 기반 클래스.
 * GEComponent와 같은 Instanced UObject이며 스포너의 SpawnerComponents 배열이 소유한다.
 * 설정은 요청 시 복사하며, 대기 핸들·개체 목록 같은 실행 상태는 스포너가 관리한다.
 * 현재 완료 훅은 캐릭터 BeginPlay 이후에 호출되므로 초기 AI 설정용으로 사용하지 않는다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories,
    meta = (DisplayName = "Kata Spawner Component"))
class KATAFRAMEWORK_API UKataSpawnerComponent : public UObject
{
    GENERATED_BODY()

public:
    /** 이 설정을 다음 생성 작업에 사용할지 여부. 진행 중인 작업의 설정 사본은 바뀌지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    bool bEnabled = true;

    /** 생성 완료를 알린다. 설정을 변경하지 않으며, 실행 상태는 전달된 Spawner에 둔다. */
    UFUNCTION(BlueprintNativeEvent, Category = "Kata|Spawning")
    void OnCharacterSpawned(AKataCharacterSpawner* Spawner, AKataCharacter* Character, const FDataTableRowHandle& Row) const;
    virtual void OnCharacterSpawned_Implementation(AKataCharacterSpawner* Spawner, AKataCharacter* Character,
        const FDataTableRowHandle& Row) const;
};