#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KataDebugPanel.generated.h"

class AActor;
class APlayerController;
class UWorld;

/** 패널이 그릴 때 메뉴가 넘기는 문맥이다. 그리는 동안만 유효하므로 보관하지 않는다. */
struct FKataDebugPanelContext
{
    UWorld* World = nullptr;
    APlayerController* PlayerController = nullptr;
    /** 메뉴의 공용 선택 대상. 고르지 않았거나 사라졌으면 nullptr. */
    AActor* SelectedActor = nullptr;
};

/**
 * 인게임 디버그 메뉴의 패널 하나다. 이 클래스를 상속한 비추상 클래스는 메뉴가 시작할 때 자동으로 찾아 목록에 넣는다.
 * 메뉴가 로컬 플레이어마다 인스턴스를 만들고 소유하므로, 패널 상태는 그 플레이어의 메뉴 수명 동안 유지된다.
 * 패널은 Kata 플러그인의 공개 API와 치트 확장만 호출하고 플러그인 쪽에 UI 코드를 두지 않는다.
 */
UCLASS(Abstract)
class UKataDebugPanel : public UObject
{
    GENERATED_BODY()

public:
    /** 패널 목록 버튼에 보일 이름. */
    virtual FString GetDisplayName() const PURE_VIRTUAL(UKataDebugPanel::GetDisplayName, return FString(););

    /** 목록 순서. 작을수록 앞에 온다. 같으면 이름순이다. */
    virtual int32 GetSortOrder() const { return 100; }

    /** true면 메뉴가 이 패널 위에 공용 대상 선택 영역을 그린다. */
    virtual bool UsesTargetSelection() const { return false; }

    /** SlateIM 루트 안에서 이 패널이 보이는 동안 매 프레임 호출된다. 세로 스택 안에서 불린다. */
    virtual void Draw(const FKataDebugPanelContext& Context) PURE_VIRTUAL(UKataDebugPanel::Draw, );

    /** 메뉴가 열리거나 이 패널로 바뀌었을 때 호출된다. */
    virtual void OnShown() {}

    /** 메뉴가 닫히거나 다른 패널로 바뀌었을 때 호출된다. */
    virtual void OnHidden() {}
};
