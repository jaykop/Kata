#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "KataRuntimeTypes.h"
#include "SKataTimeline.h"
#include "TickableEditorObject.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/GCObject.h"

class UKataAction;
class UKataResolvedAction;
class UKataTask;
class UKataTimelineGroupDetails;
class FToolBarBuilder;
class FUICommandList;
class IDetailsView;
class SKataPreviewViewport;
class SScrollBox;
enum class EKataPreviewActorSlot : uint8;

/** 원본 에셋과 편집용 사본을 분리하고, 사용자 편집만 원본 변경분으로 기록한다. */
class FKataActionEditor : public FAssetEditorToolkit, public FGCObject, public FEditorUndoClient, public FTickableEditorObject
{
public:
    virtual ~FKataActionEditor() override;
    void Init(UKataAction* InAsset);
    virtual FName GetToolkitFName() const override { return TEXT("KataAssetEditor"); }
    virtual FText GetBaseToolkitName() const override { return NSLOCTEXT("Kata", "EditorName", "Kata Editor"); }
    virtual FString GetWorldCentricTabPrefix() const override { return TEXT("Kata"); }
    virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor(0.15f, 0.65f, 0.85f); }
    virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
    virtual FString GetReferencerName() const override { return TEXT("FKataActionEditor"); }
    virtual void PostUndo(bool bSuccess) override;
    virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return Asset != nullptr && Preview.IsValid(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FKataActionEditor, STATGROUP_Tickables); }

private:
    TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args);
    TSharedRef<SWidget> MakeTimelinePanel();
    TSharedRef<SWidget> MakePreviewPanel();
    TSharedRef<SWidget> MakeSettingsPanel();
    TSharedRef<SWidget> MakePreviewSettingsPanel();
    TSharedRef<SWidget> MakeTaskPanel();
    TSharedRef<SWidget> MakeTaskClassMenu();
    TSharedRef<SWidget> MakeResetMenu(bool bTask);
    /** 프리뷰 재생 아이콘 버튼 묶음. 타임라인 탭 상단에 둔다. */
    TSharedRef<SWidget> MakeTransportControls();
    /** 타임라인 우클릭 팝업. 클릭한 시각을 삽입 위치로 사용한다. */
    TSharedPtr<SWidget> MakeTimelineContextMenu(float Time, FGuid GroupId);
    /** 타임라인 스냅 설정 위젯. */
    TSharedRef<SWidget> MakeSnapControls();
    /** 주석 표시 방식을 고르는 팝업 메뉴. */
    TSharedRef<SWidget> MakeCommentDisplayMenu();
    /** 스냅 대상을 중복 선택하는 팝업 메뉴. */
    TSharedRef<SWidget> MakeSnapTargetMenu();
    /** 스냅 대상 버튼에 표시할 요약. */
    FText DescribeSnapTargets() const;
    void BindCommands();
    /** 저장·브라우저 버튼 오른쪽에 Select Self, Select Target, Resize와 Auto Resize를 추가한다. */
    void ExtendToolbar();
    void FillToolbar(FToolBarBuilder& Builder);
    /** 지정한 자리를 조작 대상으로 삼는다. 이미 그 자리면 해제해 카메라 조작으로 돌아간다. */
    void TogglePreviewSlot(EKataPreviewActorSlot Slot);
    bool IsPreviewSlotActive(EKataPreviewActorSlot Slot) const;
    /** 가장 늦게 끝나는 태스크에 타임라인 표시 범위를 맞춘다. */
    void ResizeViewToTasks();
    /** Auto Resize 토글. 켜는 즉시 한 번 맞추고 설정을 저장한다. */
    void ToggleAutoResizeView();
    void GroupSelectedTasks();
    void AddSelectedTasksToGroup(FGuid GroupId);
    void UngroupSelectedTasks();
    void RenameTimelineGroup(FGuid GroupId);
    void DeleteTimelineGroup(FGuid GroupId);
    void ToggleTimelineGroup(FGuid GroupId);
    void SelectTimelineGroup(FGuid GroupId);
    void OnGroupDetailsEdited(const FPropertyChangedEvent& Event);
    bool CanGroupSelectedTasks() const;
    bool CanUngroupSelectedTasks() const;
    /** 에디터 사용자 설정에서 타임라인 표시 범위를 불러온다. */
    void LoadEditorSettings();
    /** 타임라인 표시 범위를 에디터 사용자 설정에 저장한다. */
    void SaveEditorSettings() const;
    /** 프리뷰에서 옮긴 액터의 트랜스폼을 자리에 맞는 에셋 프로퍼티에 기록한다. */
    void ApplyPreviewActorTransform(EKataPreviewActorSlot Slot, const FTransform& Transform);
    void Refresh();
    void RefreshRows();
    /** 선택된 태스크가 같은 클래스일 때 공통 Details 편집 대상을 갱신한다. */
    void RefreshTaskDetails();
    void SelectTask(FKataTaskId Id, bool bToggle);
    void AddTask(UClass* Class);
    void MoveTask(FKataTaskId Id, float Start, float Duration);
    /** 선택한 태스크 하나의 TaskName을 대화 상자로 바꾼다. F2와 타임라인 우클릭 메뉴에 연결된다. */
    void RenameSelectedTask();
    bool CanRenameSelectedTask() const;
    /**
     * 태스크를 TargetGroupId 그룹(유효하지 않으면 최상위)의 BeforeEntry 항목 앞으로 옮긴다.
     * BeforeEntry는 태스크의 TaskId 값이나(최상위면) 그룹의 GroupId이며, 유효하지 않으면 대상 구역의 끝이다.
     * 다른 그룹으로 옮기면 원래 그룹에서 빠지고, 비게 된 그룹은 삭제한다.
     */
    void ReorderTask(FKataTaskId Id, FGuid TargetGroupId, FGuid BeforeEntry);
    /** 그룹을 최상위 항목 BeforeEntry 앞으로 옮긴다. BeforeEntry가 유효하지 않으면 맨 뒤로 옮긴다. */
    void ReorderTimelineGroup(FGuid GroupId, FGuid BeforeEntry);
    void OnSettingsEdited(const FPropertyChangedEvent& Event);
    void OnTaskEdited(const FPropertyChangedEvent& Event);
    void OnObjectChanged(UObject* Object, FPropertyChangedEvent& Event);
    void ApplyTaskProperty(UKataTask* Edited, FName Path);
    void ResetSetting(FName Path);
    void ResetTaskProperty(FName Path);
    void DeleteSelectedTask();
    void CopySelectedTask();
    void PasteTask();
    bool CanDeleteTask() const;
    bool CanCopyTask() const;
    bool CanPasteTask() const;
    FReply CreateChild();
    UKataTask* GetSelectedTask() const;
    TArray<UKataTask*> GetSelectedTasks() const;
    bool IsLocalTask(FKataTaskId Id) const;
    /** 해석된 액션에 있는 태스크마다 화면에 표시할 그룹을 찾는다. 여러 그룹에 들어 있으면 앞 그룹을 쓴다. */
    TMap<FKataTaskId, FGuid> BuildTaskGroupMap() const;
    /** 타임라인 최상위 행(그룹의 GroupId, 그룹 없는 태스크의 TaskId 값)을 화면 순서대로 반환한다. */
    TArray<FGuid> GetTimelineTopLevelOrder() const;
    void Changed();
    /** 현재 시각 입력칸의 값이 실제로 바뀌었을 때만 프리뷰 탐색을 요청한다. */
    void SeekFromTimeInput(float Value);

    TObjectPtr<UKataAction> Asset;
    TObjectPtr<UKataAction> Settings;
    TObjectPtr<UKataResolvedAction> EditingAction;
    /** Timeline Details에서 그룹 구조체를 안전하게 편집하기 위한 임시 객체. */
    TObjectPtr<UKataTimelineGroupDetails> GroupDetails;
    TSharedPtr<IDetailsView> SettingsDetails;
    TSharedPtr<IDetailsView> TaskDetails;
    /** Preview Details 탭에서 편집하는 프리뷰 배치·조명 설정. */
    TSharedPtr<IDetailsView> PreviewDetails;
    TSharedPtr<SKataTimeline> Timeline;
    /** 타임라인을 담는 스크롤 영역. 타임라인의 최소 높이를 이 영역 높이에 맞추는 데 사용한다. */
    TSharedPtr<SScrollBox> TimelineScrollBox;
    TSharedPtr<SKataPreviewViewport> Preview;
    TSharedPtr<FUICommandList> TimelineCommands;
    /** 마지막으로 선택한 태스크. 단일 항목 작업의 기준으로 사용한다. */
    FKataTaskId SelectedId;
    TSet<FKataTaskId> SelectedIds;
    FDelegateHandle PropertyChangedHandle;
    /** 태스크를 추가하거나 붙여넣을 타임라인 시각. */
    float InsertTime = 0.0f;
    /** 타임라인에 표시할 전체 길이(초). 실행 에셋의 실제 길이와 독립적인 편집 화면 범위다. */
    float TimelineLength = 5.0f;
    /** 타임라인 눈금과 드래그 스냅 간격(초). */
    float SnapInterval = 0.5f;
    bool bSnapEnabled = true;
    /** 드래그가 붙을 대상. 에디터 사용자 설정에 저장한다. */
    EKataTimelineSnapTarget SnapTargets = EKataTimelineSnapTarget::Default;
    /**
     * 프리뷰가 끝나면 자동으로 다시 실행할지 여부.
     * 저작 편의를 위한 편집기 설정이며 에셋의 FKataLoopPolicy와는 무관하다.
     */
    bool bPreviewRepeat = false;
    /**
     * 태스크 Duration이 에셋 길이로 자동 설정될 때(예: Play Montage에 몽타주 지정) 타임라인 표시 범위도 맞출지 여부.
     * 에디터 사용자 설정에 저장한다.
     */
    bool bAutoResizeView = true;
    /** 태스크와 그룹의 Editor Comment를 타임라인에 보여줄 방식. */
    EKataTimelineCommentDisplay CommentDisplay = EKataTimelineCommentDisplay::Tooltip;
    /** 사용자 설정에 저장하는 접힌 그룹 ID. */
    TSet<FGuid> CollapsedTimelineGroups;
    /** Timeline Details에 현재 표시 중인 그룹. 유효하지 않으면 태스크 선택을 표시한다. */
    FGuid SelectedGroupId;
    /** 다음 Add Task가 들어갈 그룹. 일반 타임라인 메뉴에서는 유효하지 않다. */
    FGuid InsertGroupId;
    bool bRefreshQueued = false;
    /**
     * 편집으로 장면을 다시 만든 뒤 되돌릴 재생 헤드 시각. 음수면 기록이 없다.
     * Changed()가 프리뷰를 멈추면 재생 헤드가 0으로 돌아가므로 멈추기 전에 기록해 둔다.
     */
    float PendingPlayheadRestore = -1.0f;
    FString Diagnostics;
};
