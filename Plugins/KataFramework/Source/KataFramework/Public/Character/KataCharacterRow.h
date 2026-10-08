#pragma once

#include "CoreMinimal.h"
#include "Data/KataRowBase.h"
#include "Equipment/KataEquipmentRow.h"
#include "GameplayTagContainer.h"
#include "KataCharacterRow.generated.h"

class AKataCharacter;
class UAnimInstance;
class UKataGraph;
class UKataAnimLayerSetup;
class UKataEquipmentSetup;
class UKataGameplayData;
class UKataInputConfig;
class AKataAIController;
class UKataAIData;
class UStateTree;
class USkeletalMesh;

/**
 * 캐릭터 데이터 테이블의 공통 행. 행 하나가 캐릭터 하나의 조립 정보이며 행 이름이 캐릭터 ID다.
 *
 * 캡슐, 이동 같은 캐릭터 기본값은 Character Class가 정한다. 선택 항목을 비워 두면 Character Class의 기본값을 그대로 쓰므로
 * 같은 Blueprint로 외형만 다른 캐릭터를 행 추가만으로 만들 수 있다.
 * 모든 에셋 참조는 소프트 참조라서 테이블을 로드해도 에셋은 로드되지 않는다. UKataCharacterSpawnSubsystem이 생성할 때 비동기로 로드한다.
 * 이 구조체를 직접 행 구조로 쓰지 않고 FKataPlayerCharacterRow나 FKataNPCCharacterRow를 쓴다.
 */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataCharacterRow : public FKataRowBase
{
    GENERATED_BODY()

    /** 생성할 캐릭터 Blueprint. 비어 있으면 생성에 실패한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
    TSoftClassPtr<AKataCharacter> CharacterClass;

    /** 캐릭터 Mesh에 지정할 스켈레탈 메시. 비워 두면 Character Class의 기본값을 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    /** 캐릭터 Mesh에 지정할 Anim Blueprint. 비워 두면 Character Class의 기본값을 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    TSoftClassPtr<UAnimInstance> AnimClass;

    /** 장착 컴포넌트에 지정할 장비 설정(슬롯→소켓 매핑, 기본 슬롯). 스켈레톤마다 하나를 공유한다. 비워 두면 컴포넌트의 Blueprint 기본값을 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
    TSoftObjectPtr<UKataEquipmentSetup> EquipmentSetup;

    /**
     * 장착 컴포넌트에 지정할 Linked Anim Layer 설정(Body 레이어, 무기 종류별 레이어). 스켈레톤마다 하나를 공유한다.
     * 비워 두면 컴포넌트의 Blueprint 기본값을 쓴다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
    TSoftObjectPtr<UKataAnimLayerSetup> AnimLayerSetup;

    /** 캐릭터가 BeginPlay에서 장착할 장비. 장비 에셋은 생성 때가 아니라 장착할 때 비동기로 로드한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
    TArray<FKataStartingEquipment> StartingEquipment;

    /**
     * 캐릭터 ASC에 배열 순서대로 적용할 GAS 데이터(AttributeSet과 초기값, Ability, Effect). 여러 에셋을 지정해 조합한다.
     * 생성 전에 비동기로 로드하며, 컴포넌트 초기화 직후 UKataGameplayData::ApplyAll로 적용한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
    TArray<TSoftObjectPtr<UKataGameplayData>> GameplayData;

    /**
     * 캐릭터가 존재하는 동안 바뀌지 않는 특성 태그(예: Identity.Undead). Gameplay Data 적용 마지막에 ASC에 Loose 태그로 더한다.
     * 여러 캐릭터가 공유하는 Gameplay Data에 두면 의도하지 않은 캐릭터까지 같은 특성을 갖게 되므로 행에 둔다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay", meta = (Categories = "Identity"))
    FGameplayTagContainer IdentityTags;

    /**
     * 캐릭터의 팩션. 행을 적용할 때 타게팅 컴포넌트의 Faction에 기록하며, 비워 두면 Character Class의 컴포넌트 기본값을 유지한다.
     * 스포너의 Faction Override가 있으면 스포너가 행 사본의 이 값을 덮어쓴다. Kata Factions 설정에 등록한 태그여야 팀 번호를 얻는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay", meta = (Categories = "Faction"))
    FGameplayTag Faction;

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
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TSoftObjectPtr<UKataInputConfig> InputConfig;

    /** 입력 트리거로 구동할 콤보 그래프. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    TSoftObjectPtr<UKataGraph> Graph;

    virtual void GatherAssetsToLoad(TArray<FSoftObjectPath>& OutPaths) const override;
};

/**
 * NPC·AI 캐릭터 테이블의 행.
 *
 * AI 설정은 KataFramework에서 조합한다. AIData가 공유 인지·행동 설정을 제공한다.
 * AIControllerClass를 비워 두면 기존 NPC 클래스의 기본값을 유지한다.
 * AI 항목을 지정한 행은 AKataAICharacter 계열이어야 한다.
 */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataNPCCharacterRow : public FKataCharacterRow
{
    GENERATED_BODY()

    /** 비어 있지 않으면 생성 전에 AI 캐릭터의 Controller 클래스를 교체한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
    TSoftClassPtr<AKataAIController> AIControllerClass;

    /** 생성 전에 비동기로 로드할 공유 AI 설정이다. 비우면 인지·행동 로직을 실행하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
    TSoftObjectPtr<UKataAIData> AIData;

    /** 이전 행 값을 보존하는 이관용 필드다. 런타임에서는 사용하지 않으며 AIData로 수동 이관한다. */
    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Move StateTree into a Kata AI Data asset and assign AIData."))
    TSoftObjectPtr<UStateTree> StateTree;

    virtual void GatherAssetsToLoad(TArray<FSoftObjectPath>& OutPaths) const override;
};
