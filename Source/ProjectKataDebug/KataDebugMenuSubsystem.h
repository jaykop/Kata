#pragma once

#include "CoreMinimal.h"
#include "Delegates/IDelegateInstance.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "KataDebugMenuSubsystem.generated.h"

class AActor;
class APlayerController;
class FKataDebugMenuInputProcessor;
class UKataDebugPanel;
struct FKataDebugPanelContext;

/**
 * 게임 화면 위에 그리는 인게임 디버그 메뉴다. 로컬 플레이어마다 하나씩 생기며 PIE와 Standalone Game에서 동작한다.
 *
 * Home 키나 콘솔 명령 Kata.DebugMenu로 연다. 열면 마우스 커서를 보이고 Game and UI 입력으로 바꾸며, 닫으면 Game Only 입력과
 * 열기 전 커서 표시로 되돌린다. 메뉴는 열려 있는 동안만 Slate PreTick에서 SlateIM으로 그리고, 닫으면 그리기를 멈춰
 * SlateIM이 다음 Tick에 화면 요소를 제거한다.
 */
UCLASS()
class UKataDebugMenuSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    //~ Begin USubsystem Interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    void ToggleMenu();
    void SetMenuOpen(bool bOpen);
    bool IsMenuOpen() const { return bMenuOpen; }

    /** 이 로컬 플레이어의 게임 뷰포트나 그 안의 위젯이 키보드 포커스를 가졌으면 true. 단축키를 받을 메뉴를 고를 때 쓴다. */
    bool IsViewportFocused() const;

    /** 메뉴의 공용 선택 대상. 고르지 않았거나 사라졌으면 nullptr. */
    AActor* GetSelectedActor() const { return SelectedActor.Get(); }

private:
    void HandleSlatePreTick(float DeltaTime);
    void DrawHeader();
    void DrawPanelList();
    void DrawTargetSelection(const FKataDebugPanelContext& Context);
    void RefreshTargetCandidates(APlayerController& PlayerController);

    /** 메뉴를 열고 닫을 때 입력 모드·커서·일시정지를 바꾸거나 되돌린다. */
    void ApplyOpenState(bool bOpen);
    void ApplyPause(bool bPause);

    APlayerController* GetPlayerController() const;
    UKataDebugPanel* GetActivePanel() const;

    /** 시작할 때 찾은 패널 인스턴스. 서브시스템이 소유해 GC가 추적한다. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UKataDebugPanel>> Panels;

    int32 ActivePanelIndex = 0;
    bool bMenuOpen = false;

    /** 메뉴를 연 동안 게임을 멈출지. 메뉴의 체크박스로 바꾼다. */
    bool bPauseWhileOpen = false;

    /** 이 메뉴가 일시정지를 걸었으면 true. 다른 경로가 건 일시정지는 닫을 때 풀지 않는다. */
    bool bPausedByMenu = false;

    /** 메뉴를 열기 전의 커서 표시 상태. 닫을 때 되돌린다. */
    bool bSavedShowMouseCursor = false;

    TWeakObjectPtr<AActor> SelectedActor;

    /** 대상 선택 목록. 거리순이며 TargetRefreshInterval마다 다시 모은다. */
    TArray<TWeakObjectPtr<AActor>> TargetCandidates;
    TArray<FString> TargetCandidateNames;
    int32 TargetListIndex = INDEX_NONE;
    double NextTargetRefreshTime = 0.0;
    bool bRefreshTargetList = true;

    /** SlateIM 루트 이름. 로컬 플레이어마다 달라야 하므로 시작할 때 정한다. */
    FName RootName;

    FDelegateHandle PreTickHandle;
    TSharedPtr<FKataDebugMenuInputProcessor> InputProcessor;
};
