#pragma once

#include "CoreMinimal.h"
#include "KataDebugPanel.h"
#include "KataRuntimeSpawnerPicker.h"
#include "UObject/SoftObjectPath.h"
#include "KataDebugPanel_Spawner.generated.h"

class AKataCharacter;

/**
 * PIE 중 플레이어 정면에 NPC를 생성하는 런타임 스포너 패널이다. 콘솔 명령 Kata.RuntimeSpawner로 Kata Debug 탭을 이 패널로 연다.
 * 입력값은 UKataRuntimeSpawnerUserSettings에 저장한다. 반복과 보호는 생성한 NPC에 붙이는 UKataDummyComponent가 맡는다.
 * 이 패널로 만든 NPC 목록은 PIE가 끝나면 비운다.
 */
UCLASS()
class UKataDebugPanel_Spawner : public UKataDebugPanel
{
    GENERATED_BODY()

public:
    UKataDebugPanel_Spawner();

    virtual FString GetDisplayName() const override { return TEXT("Spawner"); }
    virtual int32 GetSortOrder() const override { return 10; }
    virtual void Draw(const FKataDebugPanelContext& Context) override;
    virtual void OnHidden() override;
    virtual void OnPlaySessionEnded() override;

private:
    struct FDummyRequest;

    void DrawCharacterSection();
    void DrawPlacementSection();
    void DrawAISection();
    /** 반복 모드에서만 보이는 줄. 항상 같은 묶음 안에서 그려 다른 섹션의 위젯 순서를 밀지 않는다. */
    void DrawRepeatRows(bool bRepeatAction, bool bRepeatGraph);
    void DrawActionsSection(const FKataDebugPanelContext& Context);

    /** 설정의 테이블 목록이 바뀌었으면 이름 목록을 다시 만들고 저장된 선택을 복원한다. */
    void RefreshTables(bool bForce);
    /** 선택한 테이블의 행 이름을 다시 읽고 저장된 행을 복원한다. */
    void RefreshRows();
    /** AssetRegistry에서 Action·Graph 목록과 Trigger 태그 목록을 다시 모은다. */
    void RefreshCatalog();
    /** 행 선택 목록의 항목을 다시 만들고 캐릭터 폴더와 에셋 목록을 갱신한다. */
    void ApplyRowFilter();
    /** 선택한 행의 Character Class 폴더를 다시 구하고 에셋 목록 필터를 적용한다. */
    void UpdateCharacterFolder();
    /** 캐릭터 폴더 필터를 Action·Graph 선택 목록에 적용한다. 현재 선택이 걸러지면 첫 항목으로 바꾼다. */
    void ApplyAssetFilters();

    void SpawnCharacters(UWorld* World, APlayerController* PlayerController);
    void DespawnCharacters();
    void HandleSpawned(AKataCharacter* Character, uint32 RequestSession, const TSharedRef<FDummyRequest>& Request);
    /** 범위를 벗어난 저장값을 기본값으로 되돌린다. 패널을 처음 그릴 때 한 번 한다. */
    void SanitizeUserSettings();
    void SaveUserSettings();

    /** 원본 목록이다. 선택 목록에는 1차 필터를 통과한 항목만 넣는다. */
    TArray<FSoftObjectPath> TablePaths;
    TArray<FString> TableNames;
    TArray<FString> RowNames;
    TArray<FSoftObjectPath> ActionPaths;
    TArray<FString> ActionFolders;
    TArray<FSoftObjectPath> GraphPaths;
    TArray<FString> GraphFolders;
    TArray<FName> TriggerTags;
    TArray<FString> TriggerNames;

    /**
     * 검색창과 묶음 트리가 있는 선택 메뉴다. 메뉴 콜백이 약한 참조로 돌아오므로 공유 포인터로 만들고 패널 수명 동안 같은 인스턴스를 쓴다.
     * UObject 멤버는 기본 생성이 가능해야 하므로 TSharedRef 대신 TSharedPtr로 두고 생성자에서 만든다.
     */
    TSharedPtr<FKataRuntimeSpawnerPicker> RowPicker;
    TSharedPtr<FKataRuntimeSpawnerPicker> ActionPicker;
    TSharedPtr<FKataRuntimeSpawnerPicker> GraphPicker;

    /** 폴더 필터를 통과한 항목 수. 0이면 안내 문구를 보인다. */
    int32 VisibleActionCount = 0;
    int32 VisibleGraphCount = 0;

    int32 TableIndex = INDEX_NONE;
    int32 RowIndex = INDEX_NONE;
    int32 ActionIndex = INDEX_NONE;
    int32 GraphIndex = INDEX_NONE;
    int32 TriggerIndex = 0;

    /** 선택한 행의 Character Class가 있는 패키지 경로다. 비어 있으면 폴더 필터를 적용하지 않는다. */
    FString CharacterFolder;

    /** 목록을 바꾼 프레임에만 콤보박스를 다시 만들게 한다. */
    bool bRefreshTableCombo = true;
    bool bRefreshTriggerCombo = true;
    bool bTablesBuilt = false;
    bool bCatalogBuilt = false;

    /** SpinBox를 끄는 동안 매 프레임 파일에 쓰지 않도록 저장을 생성·패널 전환·탭 닫기·PIE 종료로 미룬다. */
    bool bUserSettingsDirty = false;
    bool bUserSettingsSanitized = false;
    uint32 TableListHash = 0;

    TArray<TWeakObjectPtr<AKataCharacter>> SpawnedCharacters;

    /** PIE가 끝난 뒤 도착한 이전 세션의 생성 결과를 버리기 위한 번호다. */
    uint32 Session = 1;
    int32 PendingCount = 0;
    FString StatusText;
};
