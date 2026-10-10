#include "Spawning/KataSpawnerComponent_AIOverride.h"

#include "Character/KataCharacterRow.h"
#include "StructUtils/InstancedStruct.h"

void UKataSpawnerComponent_AIOverride::ModifySpawnRow(FInstancedStruct& RowData) const
{
    // 스포너는 NPC 행만 받으므로 다른 행 구조는 무시한다.
    FKataNPCCharacterRow* Row = RowData.GetMutablePtr<FKataNPCCharacterRow>();
    if (Row == nullptr)
    {
        return;
    }
    if (bDisableAI)
    {
        // 기준과 덮어쓰기를 함께 비워 합성 단계가 경고 없이 AI Data 없음으로 처리하게 한다.
        Row->AIData.Reset();
        Row->AIDataOverride = FKataAIDataOverride();
        return;
    }
    // 교체한 참조도 행의 AIData 필드라 기존 비동기 로드 목록에 포함된다.
    if (!AIData.IsNull())
    {
        Row->AIData = AIData;
    }
    Row->AIDataOverride = Overrides;
}
