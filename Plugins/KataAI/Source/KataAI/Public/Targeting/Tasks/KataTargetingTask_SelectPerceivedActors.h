#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingTask.h"
#include "KataTargetingTask_SelectPerceivedActors.generated.h"

/** 요청 소유자의 AI 타게팅에서 보이는 액터를 가져온다. Preset의 첫 Selection으로 사용한다. */
UCLASS(meta = (DisplayName = "Kata Select Perceived Actors"))
class KATAAI_API UKataTargetingTask_SelectPerceivedActors : public UTargetingTask
{
    GENERATED_BODY()

public:
    virtual void Execute(const FTargetingRequestHandle& TargetingHandle) const override;
};
