#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "KataRowBase.generated.h"

/**
 * Kata 게임 데이터 테이블 행의 공용 기반이다. Kata 행 구조체는 FTableRowBase 대신 이 구조체를 상속한다.
 *
 * 게임 데이터 설정은 이 구조체를 상속한 행 구조의 테이블만 등록하고, 모든 Kata 행에 공통으로 적용할 데이터 검증도 이곳에 둔다.
 * 지금은 필드를 두지 않는다. 필드를 더하면 기존 테이블 에셋의 행은 기본값으로 로드되므로, 기본값에서 기존 동작이 바뀌지 않게 정한다.
 */
USTRUCT(BlueprintInternalUseOnly)
struct KATAFRAMEWORK_API FKataRowBase : public FTableRowBase
{
    GENERATED_BODY()

    /** 에디터에서 테이블을 편집하면 같은 데이터 영역의 다른 테이블과 행 이름이 겹치는지 검사해 경고 로그를 남긴다. */
    virtual void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName) override;
};
