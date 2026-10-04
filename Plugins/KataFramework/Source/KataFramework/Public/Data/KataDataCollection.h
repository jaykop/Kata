#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KataDataCollection.generated.h"

class UDataTable;
struct FKataCharacterRow;
struct FKataEquipmentRow;

/**
 * 게임 전체가 참조하는 데이터 테이블 묶음. 프로젝트 설정의 Kata Data에 하나를 지정해 쓴다.
 * 컬렉션을 바꿔 지정하면 게임이 쓰는 데이터 묶음 전체가 바뀐다.
 *
 * 데이터는 영역 단위로 묶는다. 캐릭터 영역은 PC 테이블 하나와 NPC 테이블 목록으로 이루어지며, 행은 영역 안에서 행 이름 하나로 찾는다.
 * 그래서 같은 영역의 테이블끼리는 행 이름이 겹치면 안 된다. 겹친 행 이름은 데이터 검증과 테이블 편집 때 경고한다.
 * NPC 테이블 목록에는 Creature처럼 종류별로 나눈 테이블을 더할 수 있다. 행 구조는 FKataNPCCharacterRow 계열이어야 한다.
 * 장비 영역은 장비 테이블 목록으로 이루어지며, 무기처럼 FKataEquipmentRow를 상속한 행 구조의 테이블도 같은 목록에 넣는다.
 * 같은 행 구조의 Composite Data Table도 넣을 수 있다.
 *
 * 테이블은 하드 참조라서 컬렉션을 로드하면 함께 로드되고, 컬렉션이 유지되는 동안 유지된다.
 * 행 안의 에셋은 소프트 참조라서 함께 로드되지 않는다.
 */
UCLASS(BlueprintType)
class KATAFRAMEWORK_API UKataDataCollection : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** PC 캐릭터 테이블. 행 구조는 FKataPlayerCharacterRow다. */
    UPROPERTY(EditAnywhere, Category = "Character",
        meta = (RequiredAssetDataTags = "RowStructure=/Script/KataFramework.KataPlayerCharacterRow"))
    TObjectPtr<UDataTable> PlayerCharacterTable;

    /**
     * NPC·AI 캐릭터 테이블 목록. 행 구조는 FKataNPCCharacterRow 계열이어야 하며, 맞지 않는 테이블은 오류 로그를 남기고 무시한다.
     * 파생 행 구조의 테이블도 넣을 수 있도록 선택기는 행 구조로 거르지 않는다.
     */
    UPROPERTY(EditAnywhere, Category = "Character")
    TArray<TObjectPtr<UDataTable>> NPCCharacterTables;

    /**
     * 캐릭터 영역에서 행 이름으로 행을 찾는다.
     *
     * @param RowName 찾을 행 이름.
     * @param OutTable 행을 찾은 테이블. PC 행인지 판별해야 하는 호출자는 이 테이블의 행 구조를 확인한다.
     * @return 찾은 행. 테이블이 지정되지 않았거나 행이 없으면 nullptr. 반환한 포인터는 테이블이 다시 로드되거나 편집되기 전까지만 유효하다.
     */
    const FKataCharacterRow* FindCharacterRow(FName RowName, const UDataTable** OutTable = nullptr) const;

    /** 캐릭터 영역의 테이블(PC 테이블, NPC 테이블 목록 순서)을 돌려준다. 테이블이 지정되지 않았거나 행 구조가 맞지 않는 항목은 제외한다. */
    void GetCharacterTables(TArray<const UDataTable*>& OutTables) const;

    /** Table이 이 컬렉션의 NPC 테이블 목록에 있고 행 구조가 맞으면 참이다. 스포너의 Source Table 검사에 쓴다. */
    bool IsNPCCharacterTable(const UDataTable* Table) const;

    /**
     * 장비 테이블 목록. 행 구조는 FKataEquipmentRow 계열이어야 하며, 맞지 않는 테이블은 오류 로그를 남기고 무시한다.
     * 무기 같은 파생 행 구조의 테이블도 넣을 수 있도록 선택기는 행 구조로 거르지 않는다.
     */
    UPROPERTY(EditAnywhere, Category = "Equipment")
    TArray<TObjectPtr<UDataTable>> EquipmentTables;

    /**
     * 장비 영역에서 행 이름으로 행을 찾는다.
     *
     * @param OutTable 행을 찾은 테이블. 파생 행 구조(무기 등)인지 판별해야 하는 호출자는 이 테이블의 행 구조를 확인한다.
     * @return 찾은 행. 행이 없으면 nullptr. 반환한 포인터는 테이블이 다시 로드되거나 편집되기 전까지만 유효하다.
     */
    const FKataEquipmentRow* FindEquipmentRow(FName RowName, const UDataTable** OutTable = nullptr) const;

    /** 장비 영역의 테이블을 목록 순서로 돌려준다. 테이블이 지정되지 않았거나 행 구조가 맞지 않는 항목은 제외한다. */
    void GetEquipmentTables(TArray<const UDataTable*>& OutTables) const;

#if WITH_EDITOR
    /**
     * 테이블의 행 이름이 같은 영역의 다른 테이블과 겹치면 경고 로그를 남긴다. FKataRowBase가 테이블 편집 때 호출한다.
     * 이 컬렉션에 없는 테이블이면 아무 일도 하지 않는다.
     */
    void WarnDuplicateRowName(const UDataTable* ChangedTable, FName RowName) const;

    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    /** 전달된 테이블이 요구하는 행 구조를 사용하는지 확인한다. 맞지 않으면 오류 로그를 남기고 nullptr을 반환한다. */
    const UDataTable* GetCheckedTable(const UDataTable* Table, const UScriptStruct* RequiredRowStruct) const;

    /** 테이블 목록에서 행을 찾는다. 둘 이상에서 찾히면 오류 로그를 남기고 앞쪽 테이블의 행을 쓴다. */
    static const uint8* FindRowInTables(TConstArrayView<const UDataTable*> Tables, FName RowName, const UDataTable** OutTable);

#if WITH_EDITOR
    /** 영역 안 모든 테이블 쌍에서 겹치는 행 이름을 찾아 OutMessages에 담는다. */
    static void CollectDuplicateRowNames(TConstArrayView<const UDataTable*> Tables, TArray<FText>& OutMessages);

    /** 캐릭터 영역과 장비 영역의 겹치는 행 이름을 모두 모은다. 영역이 다르면 같은 이름을 써도 된다. */
    void CollectAllDuplicateRowNames(TArray<FText>& OutMessages) const;
#endif
};
