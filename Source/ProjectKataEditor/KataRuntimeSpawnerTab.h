#pragma once

#include "CoreMinimal.h"
#include "KataRuntimeSpawnerLoop.h"
#include "KataRuntimeSpawnerPicker.h"
#include "SlateIMWidgetBase.h"
#include "UObject/SoftObjectPath.h"

class AKataCharacter;
class UWorld;

/**
 * PIE 중 플레이어 정면에 NPC를 생성하는 SlateIM 도킹 탭이다. Tools 메뉴나 콘솔 명령 Kata.RuntimeSpawner로 연다.
 * 입력값은 UKataRuntimeSpawnerUserSettings에 저장하고, 이 도구로 만든 NPC와 반복 기록은 PIE가 끝나면 비운다.
 */
class FKataRuntimeSpawnerTab : public FSlateIMNomadTabBase
{
public:
    FKataRuntimeSpawnerTab();
    virtual ~FKataRuntimeSpawnerTab() override;

    /** 탭을 닫을 때 바뀐 입력값을 저장한다. */
    virtual void DisableWidget() override;

protected:
    virtual void DrawContent(float DeltaTime) override;

private:
    struct FRepeatRequest;

    void DrawCharacterSection();
    void DrawPlacementSection();
    void DrawAISection();
    void DrawActionsSection();

    /** 라벨과 선택 메뉴 버튼을 한 줄에 그린다. 사용자가 고른 원본 인덱스를 돌려주며, 고르지 않았으면 INDEX_NONE이다. */
    int32 DrawPicker(const TCHAR* Label, FKataRuntimeSpawnerPicker& Picker, const FString& CurrentLabel);

    /** 설정의 테이블 목록이 바뀌었으면 이름 목록을 다시 만들고 저장된 선택을 복원한다. */
    void RefreshTables(bool bForce);
    /** 선택한 테이블의 행 이름을 다시 읽고 저장된 행을 복원한다. */
    void RefreshRows();
    /** AssetRegistry에서 Action·Graph 목록과 Trigger 태그 목록을 다시 모은다. */
    void RefreshCatalog();

    /** 행 선택 메뉴의 항목을 다시 만들고 캐릭터 폴더와 에셋 목록을 갱신한다. */
    void ApplyRowFilter();
    /** 선택한 행의 Character Class 폴더를 다시 구하고 에셋 목록 필터를 적용한다. */
    void UpdateCharacterFolder();
    /** 캐릭터 폴더 필터를 Action·Graph 선택 메뉴에 적용한다. 현재 선택이 걸러지면 첫 항목으로 바꾼다. */
    void ApplyAssetFilters();

    void SpawnCharacters();
    void DespawnCharacters();
    void HandleSpawned(AKataCharacter* Character, uint32 RequestSession, const TSharedRef<FRepeatRequest>& Repeat);
    void HandleEndPIE(bool bIsSimulating);
    void SaveUserSettings();

    static UWorld* GetPlayWorld();

    /** 원본 목록이다. 선택 메뉴에는 1차 필터를 통과한 항목만 넣는다. */
    TArray<FSoftObjectPath> TablePaths;
    TArray<FString> TableNames;
    TArray<FString> RowNames;
    TArray<FSoftObjectPath> ActionPaths;
    TArray<FString> ActionNames;
    TArray<FString> ActionFolders;
    TArray<FSoftObjectPath> GraphPaths;
    TArray<FString> GraphNames;
    TArray<FString> GraphFolders;
    TArray<FName> TriggerTags;
    TArray<FString> TriggerNames;

    /** 검색창과 카테고리 트리가 있는 선택 메뉴다. 위젯 수명 동안 같은 인스턴스를 쓴다. */
    TSharedRef<FKataRuntimeSpawnerPicker> RowPicker;
    TSharedRef<FKataRuntimeSpawnerPicker> ActionPicker;
    TSharedRef<FKataRuntimeSpawnerPicker> GraphPicker;
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
    /** SpinBox를 끄는 동안 매 프레임 파일에 쓰지 않도록 저장을 생성·탭 닫기·PIE 종료로 미룬다. */
    bool bUserSettingsDirty = false;
    uint32 TableListHash = 0;

    TArray<TWeakObjectPtr<AKataCharacter>> SpawnedCharacters;
    FKataRuntimeSpawnerLoop Loop;
    /** PIE 세션이 바뀐 뒤 도착한 이전 요청의 결과를 버리기 위한 번호다. */
    uint32 Session = 1;
    int32 PendingCount = 0;
    FString StatusText;
    FDelegateHandle EndPIEHandle;
};
