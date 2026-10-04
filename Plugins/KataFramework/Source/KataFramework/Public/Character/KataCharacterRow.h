#pragma once

#include "CoreMinimal.h"
#include "Data/KataRowBase.h"
#include "Equipment/KataEquipmentRow.h"
#include "KataCharacterRow.generated.h"

class AKataCharacter;
class UAnimInstance;
class UKataGraph;
class UKataEquipmentSetup;
class UKataInputConfig;
class USkeletalMesh;

/**
 * 캐릭터 데이터 테이블의 공통 행. 행 하나가 캐릭터 하나의 조립 정보이며 행 이름이 캐릭터 ID다.
 *
 * 캡슐, 이동, 팩션 같은 캐릭터 기본값은 Character Class가 정한다. 선택 항목을 비워 두면 Character Class의 기본값을 그대로 쓰므로
 * 같은 Blueprint로 외형만 다른 캐릭터를 행 추가만으로 만들 수 있다.
 * 모든 에셋 참조는 소프트 참조라서 테이블을 로드해도 에셋은 로드되지 않는다. UKataCharacterSpawnSubsystem이 생성할 때 비동기로 로드한다.
 * 이 구조체를 직접 행 구조로 쓰지 않고 FKataPlayerCharacterRow나 FKataNPCCharacterRow를 쓴다.
 */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataCharacterRow : public FKataRowBase
{
    GENERATED_BODY()

    /** 생성할 캐릭터 Blueprint. 비어 있으면 생성에 실패한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Character|Class")
    TSoftClassPtr<AKataCharacter> CharacterClass;

    /** 캐릭터 Mesh에 지정할 스켈레탈 메시. 비워 두면 Character Class의 기본값을 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Character|Appearance")
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    /** 캐릭터 Mesh에 지정할 Anim Blueprint. 비워 두면 Character Class의 기본값을 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Character|Appearance")
    TSoftClassPtr<UAnimInstance> AnimClass;

    /** 장착 컴포넌트에 지정할 장비 설정(슬롯→소켓 매핑, 기본 슬롯). 스켈레톤마다 하나를 공유한다. 비워 두면 컴포넌트의 Blueprint 기본값을 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Character|Equipment")
    TSoftObjectPtr<UKataEquipmentSetup> EquipmentSetup;

    /** 캐릭터가 BeginPlay에서 장착할 장비. 장비 에셋은 생성 때가 아니라 장착할 때 비동기로 로드한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Character|Equipment")
    TArray<FKataStartingEquipment> StartingEquipment;

    /**
     * 생성 전에 비동기로 로드할 에셋 경로를 모은다. 비어 있는 참조는 넣지 않는다.
     * 파생 행은 Super를 호출한 뒤 자기 항목을 덧붙인다.
     */
    virtual void GatherAssetsToLoad(TArray<FSoftObjectPath>& OutPaths) const;
};

/**
 * PC 캐릭터 테이블의 행.
 *
 * Character Class는 AKataPlayerCharacter 계열이어야 하며, 아니면 생성에 실패한다.
 * 입력 설정과 콤보 그래프는 캐릭터의 UKataInputHandlerComponent에 적용하며, 비워 두면 그 컴포넌트의 Blueprint 기본값을 쓴다.
 */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataPlayerCharacterRow : public FKataCharacterRow
{
    GENERATED_BODY()

    /** 입력 처리 컴포넌트에 지정할 입력 설정. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Character|Input")
    TSoftObjectPtr<UKataInputConfig> InputConfig;

    /** 입력 트리거로 구동할 콤보 그래프. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Character|Combo")
    TSoftObjectPtr<UKataGraph> Graph;

    virtual void GatherAssetsToLoad(TArray<FSoftObjectPath>& OutPaths) const override;
};

/**
 * NPC·AI 캐릭터 테이블의 행.
 *
 * 지금은 공통 항목만 가진다. AIController, StateTree, 콤보 그래프 같은 AI 항목은 KataAI가 더한다.
 */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataNPCCharacterRow : public FKataCharacterRow
{
    GENERATED_BODY()
};
