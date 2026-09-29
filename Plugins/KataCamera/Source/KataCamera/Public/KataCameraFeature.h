#pragma once

#include "CoreMinimal.h"
#include "KataCameraTypes.h"
#include "UObject/Object.h"
#include "KataCameraFeature.generated.h"

class AKataPlayerCameraManager;

/**
 * 카메라 파이프라인의 한 단계에 끼는 기능의 기반 클래스.
 *
 * 카메라 매니저가 인스턴스를 소유하므로 플레이어마다 하나씩 존재하고, 프레임 간 실행 상태를 멤버로 가져도 된다.
 * 파생 클래스는 생성자에서 Stage를 정한다. 같은 단계 안에서는 Priority가 작은 Feature부터 실행한다.
 * 다른 Feature를 직접 참조하지 않고 FKataCameraPipelineContext로만 값을 주고받는다.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class KATACAMERA_API UKataCameraFeature : public UObject
{
    GENERATED_BODY()

public:
    EKataCameraStage GetStage() const { return Stage; }
    int32 GetPriority() const { return Priority; }
    bool IsEnabled() const { return bEnabled; }

    /** 카메라 매니저가 컨트롤러에 연결될 때 한 번 호출한다. */
    virtual void Initialize(AKataPlayerCameraManager* InCameraManager);

    /**
     * 카메라 매니저가 종료될 때 한 번 호출한다.
     * 월드에 남긴 변경(예: 머티리얼 값)과 구독을 여기서 되돌린다.
     */
    virtual void Deinitialize();

    /** 자기 단계에서 Context를 읽고 고친다. bEnabled가 false이면 호출되지 않는다. */
    virtual void Evaluate(FKataCameraPipelineContext& Context) {}

protected:
    AKataPlayerCameraManager* GetCameraManager() const { return CameraManager.Get(); }

    /** 이 Feature가 끼는 단계. 기능의 성격이 정하므로 파생 클래스 생성자에서만 바꾼다. */
    EKataCameraStage Stage = EKataCameraStage::Framing;

    /** 같은 단계 안의 실행 순서. 작은 값이 먼저 실행된다. */
    UPROPERTY(EditAnywhere, Category = "Feature")
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, Category = "Feature")
    bool bEnabled = true;

private:
    TWeakObjectPtr<AKataPlayerCameraManager> CameraManager;
};
