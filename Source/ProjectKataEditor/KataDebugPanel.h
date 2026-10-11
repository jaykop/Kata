#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KataDebugPanel.generated.h"

class AActor;
class APlayerController;
class UWorld;

/** 패널이 그릴 때 디버그 탭이 넘기는 문맥이다. 그리는 동안만 유효하므로 보관하지 않는다. */
struct FKataDebugPanelContext
{
    /** PIE 월드. PIE가 아니면 nullptr. */
    UWorld* World = nullptr;
    /** PIE 월드의 첫 PlayerController. PIE가 아니거나 아직 없으면 nullptr. */
    APlayerController* PlayerController = nullptr;
    /** 탭의 공용 선택 대상들. 고른 순서를 따르며 사라진 대상은 빠져 있다. 고르지 않았으면 비어 있다. */
    TArray<AActor*> SelectedActors;
};

/**
 * 디버그 탭의 섹션 배치 도우미다. 패널의 Draw는 세로 스택 안에서 불린다.
 * SlateIM은 같은 부모 안의 순서와 위젯 종류 이름으로 위젯을 재사용한다. SSpinBox<float>와 SSpinBox<int32>처럼 종류 이름이 같은 위젯이
 * 순서가 밀려 서로의 자리에 오면 값을 잘못 읽고 쓴다. 섹션이나 조건부 줄을 BeginSection·EndSection으로 감싸면 묶음 안의 줄 수가
 * 바뀌어도 뒤의 묶음 순서가 밀리지 않는다. 조건이 거짓이어도 빈 묶음을 그려 순서를 지킨다.
 */
namespace KataDebugLayout
{
    /** 가로를 채우는 독립 세로 묶음을 연다. 반드시 EndSection과 짝을 맞춘다. */
    void BeginSection();
    void EndSection();

    /**
     * 섹션 첫 줄의 제목.
     * 한 줄 문구를 단독으로 그릴 때는 SlateIM::HAlign(HAlign_Fill)을 먼저 지정한다. SlateIM 텍스트는 자동 줄바꿈이라
     * 칸을 채우지 않으면 가장 긴 단어 폭까지 줄어들어 단어마다 줄이 바뀐다.
     */
    void SectionHeader(const TCHAR* Title);
}

/**
 * Kata Debug 탭의 패널 하나다. 이 클래스를 상속한 비추상 클래스는 탭이 처음 열릴 때 자동으로 찾아 목록에 넣는다.
 * 탭이 에디터 수명 동안 인스턴스 하나를 소유하므로 패널 상태는 PIE 세션을 넘어 유지된다. 세션별 기록은 OnPlaySessionEnded에서 비운다.
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

    /** true면 탭이 PIE 중에 이 패널 위에 공용 대상 선택 영역을 그린다. */
    virtual bool UsesTargetSelection() const { return false; }

    /**
     * SlateIM 루트 안에서 이 패널이 보이는 동안 매 프레임 호출된다. 세로 스택 안에서 불리며,
     * 섹션과 조건부 줄은 KataDebugLayout::BeginSection·EndSection으로 감싼다. PIE가 아니면 문맥의 월드가 비어 있다.
     */
    virtual void Draw(const FKataDebugPanelContext& Context) PURE_VIRTUAL(UKataDebugPanel::Draw, );

    /** 탭이 열리거나 이 패널로 바뀌었을 때 호출된다. */
    virtual void OnShown() {}

    /** 탭이 닫히거나 다른 패널로 바뀌었을 때, 에디터가 닫힐 때 호출된다. */
    virtual void OnHidden() {}

    /** PIE가 끝날 때 호출된다. 반복 실행·생성 기록처럼 그 세션의 월드에 묶인 상태를 여기서 비운다. */
    virtual void OnPlaySessionEnded() {}
};
