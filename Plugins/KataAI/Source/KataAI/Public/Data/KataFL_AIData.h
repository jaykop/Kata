#pragma once

#include "CoreMinimal.h"

class UKataAIData;
struct FKataAIDataOverride;

namespace KataFL
{
    /**
     * 기준 AI Data에 배치 단위 덮어쓰기를 적용한 실행용 AI Data를 반환한다. 공유 에셋은 수정하지 않는다.
     * Override가 비었으면 Base를 그대로 반환하고, Base가 null이면 경고 후 null을 반환한다.
     * 그 외에는 Base를 Outer 아래에 Transient로 복제한 사본에 슬롯 병합과 LeashDistance를 적용한다.
     * 사본의 수명은 호출자가 UPROPERTY 참조로 유지해야 한다.
     */
    KATAAI_API UKataAIData* ComposeAIData(UKataAIData* Base, const FKataAIDataOverride& Override, UObject* Outer);
}
