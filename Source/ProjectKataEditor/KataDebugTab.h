#pragma once

#include "CoreMinimal.h"
#include "SlateIMWidgetBase.h"
#include "Templates/SubclassOf.h"
#include "UObject/GCObject.h"

class AActor;
class APlayerController;
class UKataDebugPanel;
class UWorld;
struct FKataDebugPanelContext;

/**
 * 치트·스포너 같은 디버그 패널을 모아 보이는 SlateIM 도킹 탭이다. Tools 메뉴의 Kata Debug나 콘솔 명령 Kata.Debug로 연다.
 * 패널은 PIE 월드의 첫 PlayerController를 문맥으로 그린다. PIE가 아니면 문맥의 월드와 PlayerController가 비어 있다.
 * 게임 화면을 가리지 않고 게임 입력 모드도 건드리지 않는다. 마지막으로 고른 패널은 UKataDebugUserSettings에 저장해 다음에도 그 패널로 연다.
 */
class FKataDebugTab : public FSlateIMNomadTabBase, public FGCObject
{
public:
    FKataDebugTab();
    virtual ~FKataDebugTab() override;

    /** 패널을 처음 열 때 만들고 에디터 종료 이벤트를 구독한다. GEngine은 모듈 시작 시점에 없을 수 있다. */
    virtual void EnableWidget() override;

    /** 탭을 닫을 때 보이던 패널에 알린다. */
    virtual void DisableWidget() override;

    /** 탭을 열고 PanelClass 패널로 바꾼다. 탭이 이미 그 패널로 열려 있으면 닫는다. 해당 패널이 없으면 탭만 연다. */
    void TogglePanel(TSubclassOf<UKataDebugPanel> PanelClass);

    //~ Begin FGCObject Interface
    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
    virtual FString GetReferencerName() const override { return TEXT("FKataDebugTab"); }
    //~ End FGCObject Interface

protected:
    virtual void DrawContent(float DeltaTime) override;

private:
    /** UKataDebugPanel의 비추상 파생 클래스를 찾아 한 번만 만든다. */
    void EnsurePanels();
    void SetActivePanel(int32 Index);
    UKataDebugPanel* GetActivePanel() const;

    void DrawPanelList();
    void DrawTargetSelection(const FKataDebugPanelContext& Context);
    void RefreshTargetCandidates(APlayerController& PlayerController);

    /** 사라진 대상을 선택에서 빼고 남은 대상을 돌려준다. */
    TArray<AActor*> GetSelectedActors();
    bool IsSelected(const AActor* Actor) const;
    void SetSelected(AActor* Actor, bool bSelected);
    /** 선택을 Actor 하나로 바꾼다. nullptr이면 선택을 비운다. */
    void SelectOnly(AActor* Actor);

    /** PIE가 시작되면 탭이 닫혀 있어도 패널을 만들고 OnPlaySessionStarted를 부른다. */
    void HandlePostPIEStarted(bool bIsSimulating);
    void HandleEndPIE(bool bIsSimulating);
    void HandleEditorClose();

    static UWorld* GetPlayWorld();

    /** 에디터 수명 동안 유지하는 패널. FGCObject로 GC에 알린다. */
    TArray<TObjectPtr<UKataDebugPanel>> Panels;
    /** 패널 드롭다운에 보일 이름. Panels와 같은 순서다. */
    TArray<FString> PanelNames;
    int32 ActivePanelIndex = 0;
    /** 콘솔 명령처럼 바깥에서 패널을 바꾼 프레임에 드롭다운 표시를 다시 맞춘다. */
    bool bRefreshPanelCombo = true;
    bool bPanelsCreated = false;

    /** 패널 내용 영역의 구성을 가르는 값(패널, PIE 여부). 바뀐 프레임에 내용 컨테이너 종류를 바꿔 위젯을 새로 만든다. */
    uint32 LastContentLayoutKey = 0;
    bool bAlternateContentContainer = false;

    /** 공용 선택 대상. 목록 정렬이 바뀌어도 유지되도록 줄 번호가 아니라 액터로 기억한다. */
    TArray<TWeakObjectPtr<AActor>> SelectedActors;

    /** 대상 선택 목록. 거리순이며 일정 간격으로 다시 모은다. */
    TArray<TWeakObjectPtr<AActor>> TargetCandidates;
    TArray<FString> TargetCandidateNames;
    double NextTargetRefreshTime = 0.0;
    bool bRefreshTargetList = true;

    FDelegateHandle PostPIEStartedHandle;
    FDelegateHandle EndPIEHandle;
    FDelegateHandle EditorCloseHandle;
};
