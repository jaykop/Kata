#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "KataAIPawnInterface.generated.h"

class UKataAIData;

/** 조합 캐릭터를 참조하지 않고 Pawn의 AI 설정을 읽기 위한 인터페이스다. */
UINTERFACE(MinimalAPI)
class UKataAIPawnInterface : public UInterface
{
    GENERATED_BODY()
};

class KATAAI_API IKataAIPawnInterface
{
    GENERATED_BODY()

public:
    /** 빙의 전에 로드한 AI 설정을 반환한다. null이면 인지·행동 로직을 시작하지 않는다. */
    virtual UKataAIData* GetKataAIData() const = 0;

    /** Pawn의 BeginPlay 준비가 끝났는지 반환한다. 프리뷰와 종료 중에는 false다. */
    virtual bool IsKataAIReady() const = 0;
};
