#pragma once

#include "CoreMinimal.h"
#include "KataRowId.generated.h"

class UDataTable;
struct FKataCharacterRow;
struct FKataEquipmentRow;

/**
 * 데이터 컬렉션의 한 영역에서 행 하나를 가리키는 ID의 공용 기반. 영역마다 이 구조체를 상속한 ID 타입을 둔다.
 *
 * 영역 ID끼리 섞이지 않도록 Blueprint에는 영역 ID만 노출한다. Blueprint는 구조체 상속을 자동 변환하지 않으므로
 * 캐릭터 ID 자리에 다른 영역의 ID를 넣을 수 없다. 에디터는 영역 테이블의 행 이름 드롭다운으로 값을 고르게 한다.
 */
USTRUCT(BlueprintInternalUseOnly)
struct KATAFRAMEWORK_API FKataRowId
{
    GENERATED_BODY()

    FKataRowId() = default;
    explicit FKataRowId(FName InRowName)
        : RowName(InRowName)
    {
    }

    /** 가리키는 행 이름. None이면 아무 행도 가리키지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Data")
    FName RowName;

    /** 행 이름이 지정되어 있으면 참이다. 행이 실제로 있는지는 확인하지 않는다. */
    bool IsValid() const
    {
        return !RowName.IsNone();
    }

    FString ToString() const
    {
        return RowName.ToString();
    }
};

/**
 * 캐릭터 영역(데이터 컬렉션의 PC·NPC 테이블)의 행을 가리키는 ID.
 */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataCharacterId : public FKataRowId
{
    GENERATED_BODY()

    FKataCharacterId() = default;
    explicit FKataCharacterId(FName InRowName)
        : FKataRowId(InRowName)
    {
    }

    /**
     * 프로젝트 설정의 데이터 컬렉션에서 이 ID의 행을 찾는다. 컬렉션이 아직 로드되지 않았으면 동기로 로드한다.
     *
     * @param OutTable 행을 찾은 테이블. PC 행인지 판별해야 하는 호출자는 이 테이블의 행 구조를 확인한다.
     * @return 찾은 행. ID가 비었거나 컬렉션이 없거나 행이 없으면 nullptr이다. 컬렉션이 없으면 경고 로그를 남긴다.
     */
    const FKataCharacterRow* Find(const UDataTable** OutTable = nullptr) const;

    bool operator==(const FKataCharacterId& Other) const
    {
        return RowName == Other.RowName;
    }

    friend uint32 GetTypeHash(const FKataCharacterId& Id)
    {
        return GetTypeHash(Id.RowName);
    }
};

/**
 * 장비 영역(데이터 컬렉션의 장비 테이블 목록)의 행을 가리키는 ID. 무기처럼 FKataEquipmentRow를 상속한 행도 가리킨다.
 */
USTRUCT(BlueprintType)
struct KATAFRAMEWORK_API FKataEquipmentId : public FKataRowId
{
    GENERATED_BODY()

    FKataEquipmentId() = default;
    explicit FKataEquipmentId(FName InRowName)
        : FKataRowId(InRowName)
    {
    }

    /**
     * 프로젝트 설정의 데이터 컬렉션에서 이 ID의 행을 찾는다. 컬렉션이 아직 로드되지 않았으면 동기로 로드한다.
     *
     * @param OutTable 행을 찾은 테이블. 파생 행 구조인지 판별해야 하는 호출자는 이 테이블의 행 구조를 확인한다.
     * @return 찾은 행. ID가 비었거나 컬렉션이 없거나 행이 없으면 nullptr이다. 컬렉션이 없으면 경고 로그를 남긴다.
     */
    const FKataEquipmentRow* Find(const UDataTable** OutTable = nullptr) const;

    bool operator==(const FKataEquipmentId& Other) const
    {
        return RowName == Other.RowName;
    }

    friend uint32 GetTypeHash(const FKataEquipmentId& Id)
    {
        return GetTypeHash(Id.RowName);
    }
};
