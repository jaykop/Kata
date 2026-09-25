#pragma once

#include "CoreMinimal.h"
#include "Editor/UnrealEdTypes.h"
#include "KataPreviewViewportClient.h"
#include "SEditorViewport.h"
#include "PreviewScene.h"
#include "UObject/GCObject.h"

class FPreviewScene;
class UKataAction;
class UKataActionComponent;
class UKataActionInstance;
class UAbilitySystemComponent;

/**
 * 별도의 프리뷰 월드에서 게임과 같은 컴포넌트·인스턴스 경로를 실행한다.
 *
 * 뷰포트 위에는 엔진 뷰포트 툴바를 둔다. 트랜스폼은 Select·Move·Rotate만, 카메라는 엔진의 뷰 종류와
 * 이동·렌즈 설정에 Reset Camera를 더해 제공한다. 스냅은 레벨 에디터와 공유하는 전역 설정이므로 넣지 않는다.
 */
class SKataPreviewViewport : public SEditorViewport, public FGCObject
{
public:
    SLATE_BEGIN_ARGS(SKataPreviewViewport) {}
        /** 프리뷰 액터를 위젯으로 옮긴 결과를 에셋에 기록할 때 호출한다. */
        SLATE_EVENT(FKataPreviewTransformChanged, OnActorMoved)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);
    virtual ~SKataPreviewViewport() override;
    void ResetScene(UKataAction* Asset);
    void Play(UKataAction* Asset);
    void Pause();
    void Stop();
    /**
     * 재생 헤드를 Time으로 옮기고 그 시각의 실제 실행 상태를 보여 주도록 예약한다.
     * 한 프레임에 여러 번 호출돼도 마지막 요청만 다음 TickSimulation에서 한 번 처리한다.
     */
    void Seek(UKataAction* Asset, float Time);
    void TickSimulation(float DeltaTime);
    float GetTime() const;
    FString GetStatus() const { return Status; }

    /** 마지막 실행 시도가 실패해 GetStatus가 오류 설명을 담고 있는지. 장면을 다시 만들면 해제된다. */
    bool HasStatusError() const { return bStatusError; }

    /** 프리뷰가 지금 실제로 진행 중인지. 일시 정지와 탐색 중에는 false다. */
    bool IsPlaying() const { return bPlaying; }

    /**
     * 재생 중이던 인스턴스가 타임라인 끝까지 진행해 멈춘 상태인지.
     * 재생·일시 정지·정지·탐색을 요청하면 해제되므로 반복 재생의 재시작 판단에 쓸 수 있다.
     * 시작하자마자 끝난 인스턴스는 해당하지 않으므로 길이가 0인 액션이 매 프레임 되살아나지 않는다.
     */
    bool HasCompletedPlayback() const { return bCompletedPlayback; }

    /** 지금 뷰 종류의 카메라를 기본 위치와 확대 배율로 되돌린다. */
    void ResetCamera();

    /**
     * 트랜스폼 위젯으로 옮길 자리를 고른다. None이면 카메라 조작만 남는다.
     * 자리는 하나만 유지하므로 Self와 Target이 동시에 선택되지 않는다.
     */
    void SetManipulatedSlot(EKataPreviewActorSlot Slot);
    EKataPreviewActorSlot GetManipulatedSlot() const { return ManipulatedSlot; }

    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
    virtual FString GetReferencerName() const override { return TEXT("SKataPreviewViewport"); }

protected:
    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
    virtual TSharedPtr<SWidget> BuildViewportToolbar() override;
    /** 엔진 명령을 연결한 뒤 프리뷰에서 쓰지 않는 Bottom 뷰를 메뉴와 단축키에서 뺀다. */
    virtual void BindCommands() override;
    /** 프리뷰의 선택 집합은 조작 대상 하나뿐이므로 F는 Self와 Target을 함께 담도록 맞춘다. */
    virtual void OnFocusViewportToSelection() override;

private:
    bool Start(UKataAction* Asset);
    UAbilitySystemComponent* PrepareAbilitySystem(AActor* Actor);
    /** 조작 대상과 조명 설정을 현재 에셋 값으로 맞춘다. */
    void ApplySceneSettings(UKataAction* Asset);
    /** 지정한 자리에 해당하는 프리뷰 액터를 반환한다. 없으면 nullptr다. */
    AActor* GetActorForSlot(EKataPreviewActorSlot Slot) const;
    /** Self와 Target을 모두 담는 기준 영역을 구한다. */
    FBox GetPreviewFocusBox() const;
    /**
     * 예약된 탐색을 처리한다. 앞으로 가면 살아 있는 인스턴스를 이어서 진행하고, 뒤로 가면 장면을 다시 만들어
     * 0초부터 진행한다. 결과는 일시 정지 상태의 실제 실행 상태이므로 Play가 그대로 이어 재생한다.
     */
    void ApplyPendingSeek();
    /**
     * 실행 중인 인스턴스를 TargetTime까지 한 프레임 안에서 진행한다. 액션 전체 길이를 넘지 않는다.
     * 탐색 중에는 월드 소리를 끄고, 액션이 끝나도 반복·자동 초기화 대상으로 표시하지 않는다.
     */
    void SimulateTo(float TargetTime);
    /** 월드를 한 단계 진행한다. 실행 Subsystem 콜백이 없는 월드에서는 인스턴스를 직접 한 번 진행한다. */
    void StepWorld(float Delta);
    /** 지금 뷰 종류를 기본 위치와 확대 배율로 맞춘다. */
    void ApplyDefaultPlacement();
    /** 떠나는 뷰 종류의 카메라 상태를 기록한다. */
    void StoreViewState(ELevelViewportType Type);
    /** 새 뷰 종류에 기록한 카메라를 복원한다. 처음 여는 종류는 기본 위치로 맞춘다. */
    void RestoreViewState(ELevelViewportType Type);

    /** 뷰 종류별로 사용자가 마지막에 보던 카메라 상태. */
    struct FKataPreviewViewState
    {
        FVector Location = FVector::ZeroVector;
        FRotator Rotation = FRotator::ZeroRotator;
        FVector LookAt = FVector::ZeroVector;
        float OrthoZoom = 10000.0f;
    };
    TMap<ELevelViewportType, FKataPreviewViewState> ViewStates;
    TSharedPtr<FKataPreviewViewportClient> PreviewClient;
    FKataPreviewTransformChanged ActorMovedEvent;
    /** 지금 트랜스폼 위젯으로 조작 중인 자리. */
    EKataPreviewActorSlot ManipulatedSlot = EKataPreviewActorSlot::None;
    TUniquePtr<FPreviewScene> PreviewScene;
    TObjectPtr<AActor> PreviewActor;
    TObjectPtr<AActor> TargetActor;
    TObjectPtr<UKataActionComponent> Component;
    TObjectPtr<UKataActionInstance> Instance;
    bool bPlaying = false;
    /** 재생 중이던 인스턴스가 타임라인 끝에 도달해 멈췄음을 나타낸다. */
    bool bCompletedPlayback = false;
    /**
     * 0 이상이면 다음 TickSimulation에서 이 시각으로 탐색한다.
     * 마우스 이동마다 월드를 진행하지 않고, 입력 콜백 밖에서 프레임당 한 번만 처리하려고 미룬다.
     */
    float PendingSeekTime = -1.0f;
    /** 뒤로 탐색할 때 다시 시작할 에셋. */
    TObjectPtr<UKataAction> PendingSeekAsset;
    /** 타임라인과 Current Time에 즉시 표시할 재생 헤드 시각. */
    float PlayheadTime = 0.0f;
    /** 인스턴스가 실제로 도달한 시각. 재생 헤드는 액션 길이를 넘을 수 있지만 이 값은 넘지 않는다. */
    float SimulatedTime = 0.0f;
    FString Status = TEXT("Ready");
    bool bStatusError = false;
};
