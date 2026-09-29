#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KataCameraPlacement.generated.h"

struct FKataCameraPipelineContext;

/**
 * 카메라 데이터가 정하는 배치 방식의 기반 클래스.
 *
 * 카메라 데이터 에셋의 인스턴스 서브오브젝트로 존재하며 여러 플레이어가 같은 객체를 공유한다.
 * 그래서 Evaluate()는 const이고 실행 상태를 저장하지 않는다. 지연 같은 프레임 간 상태가 필요하면 카메라 매니저 쪽에 둔다.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class KATACAMERA_API UKataCameraPlacement : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Context의 PivotLocation과 ViewRotation을 읽어 CameraLocation과 CameraRotation을 채운다.
     * 다른 필드는 바꾸지 않는다.
     */
    virtual void Evaluate(FKataCameraPipelineContext& Context) const PURE_VIRTUAL(UKataCameraPlacement::Evaluate, );
};

/**
 * 피벗에서 시선 반대 방향으로 고정 거리만큼 떨어진 곳에 카메라를 둔다.
 *
 * SpringArm과 같은 배치이며 충돌 처리는 하지 않는다. 장애물 회피는 Constraint 단계의 Feature가 맡는다.
 */
UCLASS(meta = (DisplayName = "Boom Arm"))
class KATACAMERA_API UKataCameraPlacement_BoomArm : public UKataCameraPlacement
{
    GENERATED_BODY()

public:
    virtual void Evaluate(FKataCameraPipelineContext& Context) const override;

    /** 피벗에서 카메라까지의 거리. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boom Arm", meta = (ClampMin = "0.0", Units = "Centimeters"))
    float Distance = 400.0f;
};
