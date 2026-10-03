#include "Data/KataRowBase.h"

#include "Data/KataDataCollection.h"
#include "Data/KataDataSettings.h"

void FKataRowBase::OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName)
{
    FTableRowBase::OnDataTableChanged(InDataTable, InRowName);

#if WITH_EDITOR
    // 행 이름 중복은 사람이 테이블을 편집할 때 알리는 것이 목적이다. 게임 실행 중의 테이블 변경에는 검사하지 않는다.
    // 테이블 로드 중(Composite 재구성 등)에도 불리므로 컬렉션을 새로 로드하지 않는다. 이미 로드된 컬렉션만 검사한다.
    if (GIsEditor && !IsRunningCommandlet())
    {
        if (const UKataDataCollection* Collection = UKataDataSettings::Get()->GetLoadedDataCollection())
        {
            Collection->WarnDuplicateRowName(InDataTable, InRowName);
        }
    }
#endif
}
