#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "HitTrace/KataHitTraceTypes.h"
#include "KataHitBoxPreset.generated.h"

struct FCollisionShape;

/**
 * 공격 판정 영역의 정의.
 *
 * 판정 방식, 소켓, 도형, Trace Channel과 서브스텝 보정 설정을 담는다. 같은 무기를 쓰는 여러 공격이 공유한다.
 * 공유 에셋이므로 실행 중 상태를 저장하지 않으며, 데미지 같은 수치도 갖지 않는다.
 * 소켓은 UKataTask_HitTrace가 고른 기준 메시(캐릭터 본체 또는 무기)에서 찾는다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Kata Hit Box Preset"))
class KATAFRAMEWORK_API UKataHitBoxPreset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Box")
    EKataHitBoxMode Mode = EKataHitBoxMode::SocketTrace;

    /**
     * 칼날을 따라 놓인 소켓 목록. 2개 이상이며 이웃한 두 소켓이 칼날의 한 구간이 된다.
     * 직선 칼날은 손잡이 쪽과 끝 두 개면 충분하고, 휘어진 칼날은 휘는 지점마다 소켓을 더한다.
     * 직전 프레임과 이번 프레임의 소켓 점을 이어 만든 삼각형 띠가 판정 면이 된다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Box|Socket Trace", meta = (EditCondition = "Mode == EKataHitBoxMode::SocketTrace", EditConditionHides))
    TArray<FName> Sockets;

    /** 칼날 두께의 반지름. 판정 면과 대상 도형 사이가 이 거리 안이면 맞은 것으로 본다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Box|Socket Trace", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "Mode == EKataHitBoxMode::SocketTrace", EditConditionHides))
    float Thickness = 0.0f;

    /** 도형을 붙일 소켓. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Box|Shape Sweep", meta = (EditCondition = "Mode == EKataHitBoxMode::ShapeSweep", EditConditionHides))
    FName Socket;

    /** 소켓 기준 도형의 상대 위치·회전. 크기는 쓰지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Box|Shape Sweep", meta = (EditCondition = "Mode == EKataHitBoxMode::ShapeSweep", EditConditionHides))
    FTransform RelativeTransform;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Box|Shape Sweep", meta = (EditCondition = "Mode == EKataHitBoxMode::ShapeSweep", EditConditionHides))
    EKataHitBoxShape Shape = EKataHitBoxShape::Sphere;

    /** Sphere와 Capsule의 반지름. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Box|Shape Sweep", meta = (ClampMin = "0.1", Units = "cm", EditCondition = "Mode == EKataHitBoxMode::ShapeSweep && Shape != EKataHitBoxShape::Box", EditConditionHides))
    float Radius = 20.0f;

    /** Capsule의 반높이. 반지름보다 작으면 반지름으로 올려 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Box|Shape Sweep", meta = (ClampMin = "0.1", Units = "cm", EditCondition = "Mode == EKataHitBoxMode::ShapeSweep && Shape == EKataHitBoxShape::Capsule", EditConditionHides))
    float CapsuleHalfHeight = 40.0f;

    /** Box의 반 크기. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Box|Shape Sweep", meta = (EditCondition = "Mode == EKataHitBoxMode::ShapeSweep && Shape == EKataHitBoxShape::Box", EditConditionHides))
    FVector BoxExtent = FVector(20.0f);

    /**
     * 판정에 쓰는 Trace Channel. 샘플 프로젝트처럼 전용 채널(예: KataHit)을 정의해 맞을 쪽의 Physics Asset 바디가 반응하게 두는 것을 권장한다.
     * SocketTrace는 이 채널의 Overlap 질의로 후보를 모은 뒤 도형과 직접 교차 계산하고,
     * ShapeSweep은 대상이 Block으로 반응해도 여러 대상을 찾도록 맞은 액터를 제외하고 다시 추적한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Box|Collision")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

    /** ShapeSweep에서 복잡한 콜리전(메시 삼각형)으로 추적할지 여부. SocketTrace는 단순 도형만 판정한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Box|Collision")
    bool bTraceComplex = false;

    /** 서브스텝 한 칸에서 추적 점이 움직일 수 있는 최대 거리. 이동이 크면 칸을 늘린다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Box|Substep", meta = (ClampMin = "1.0", Units = "cm"))
    float MaxStepDistance = 20.0f;

    /** 서브스텝 한 칸에서 허용하는 최대 회전각. 휘두르는 동작의 호를 직선 여러 개로 근사한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Box|Substep", meta = (ClampMin = "1.0", ClampMax = "180.0", Units = "deg"))
    float MaxStepAngle = 15.0f;

    /** 한 프레임에서 나누는 서브스텝 수의 상한. 순간이동 같은 큰 이동에서 비용이 폭증하지 않게 막는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Box|Substep", meta = (ClampMin = "1", ClampMax = "64"))
    int32 MaxSubsteps = 16;

    /** 판정에 필요한 소켓 이름 목록. SocketTrace는 Sockets, ShapeSweep은 Socket 하나다. */
    void GetRequiredSockets(TArray<FName, TInlineAllocator<8>>& OutSockets) const;

    /**
     * 소켓 트랜스폼에서 판정 도형의 월드 트랜스폼을 만든다. ShapeSweep에서만 의미가 있다.
     * 크기는 판정에 쓰지 않으므로 결과의 스케일은 1이다.
     */
    FTransform MakeShapeTransform(const FTransform& SocketTransform) const;

    /** ShapeSweep에 쓸 충돌 도형. SocketTrace는 엔진 스윕을 쓰지 않으므로 빈 도형을 반환한다. */
    FCollisionShape MakeCollisionShape() const;

    /** 에디터와 런타임이 공유하는 설정 검사. 비어 있으면 유효하다. */
    FString GetConfigurationError() const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
