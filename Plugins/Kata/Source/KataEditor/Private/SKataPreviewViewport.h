#pragma once

#include "CoreMinimal.h"
#include "Editor/UnrealEdTypes.h"
#include "KataPreviewViewportClient.h"
#include "SEditorViewport.h"
#include "PreviewScene.h"
#include "UObject/GCObject.h"

class FPreviewScene;
class UKataAsset;
class UKataComponent;
class UKataInstance;
class UAbilitySystemComponent;

/** 별도의 프리뷰 월드에서 게임과 같은 컴포넌트·인스턴스 경로를 실행한다. */
class SKataPreviewViewport : public SEditorViewport, public FGCObject
{
public:
    SLATE_BEGIN_ARGS(SKataPreviewViewport) {}
        /** Target Actor를 위젯으로 옮긴 결과를 에셋에 기록할 때 호출한다. */
        SLATE_EVENT(FKataTargetTransformChanged, OnTargetMoved)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);
    virtual ~SKataPreviewViewport() override;
    void ResetScene(UKataAsset* Asset);
    void Play(UKataAsset* Asset);
    void Pause();
    void Stop();
    void Seek(UKataAsset* Asset, float Time);
    void TickSimulation(float DeltaTime);
    float GetTime() const;
    FString GetStatus() const { return Status; }

    /** Perspective와 직교 뷰를 전환한다. 직교 뷰는 회전을 뷰 종류가 정한다. */
    void SetPreviewViewportType(ELevelViewportType Type);
    bool IsPreviewViewportType(ELevelViewportType Type) const;

    /** Target Actor를 트랜스폼 위젯으로 옮길 수 있는 선택 모드를 켜고 끈다. */
    void SetTargetSelectionEnabled(bool bEnabled);
    bool IsTargetSelectionEnabled() const;

    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
    virtual FString GetReferencerName() const override { return TEXT("SKataPreviewViewport"); }

protected:
    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

private:
    bool Start(UKataAsset* Asset);
    UAbilitySystemComponent* PrepareAbilitySystem(AActor* Actor);
    /** 조작 대상과 조명 설정을 현재 에셋 값으로 맞춘다. */
    void ApplySceneSettings(UKataAsset* Asset);
    TSharedPtr<FKataPreviewViewportClient> PreviewClient;
    FKataTargetTransformChanged TargetMovedEvent;
    bool bTargetSelectionEnabled = false;
    TUniquePtr<FPreviewScene> PreviewScene;
    TObjectPtr<AActor> PreviewActor;
    TObjectPtr<AActor> TargetActor;
    TObjectPtr<UKataComponent> Component;
    TObjectPtr<UKataInstance> Instance;
    /** Back View 카메라를 왼쪽 벽 안쪽에 배치할 때 사용하는 현재 환경 크기. */
    FVector PreviewEnvironmentSize = FVector(2000.0, 2000.0, 1000.0);
    bool bPlaying = false;
    float SeekTarget = -1.0f;
    float SimulatedTime = 0.0f;
    FString Status = TEXT("Ready");
};
