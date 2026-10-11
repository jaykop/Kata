#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManagerDefines.h"

class AActor;
class APlayerController;
struct FGameplayTagContainer;

/** 치트 명령이 고르는 대상 범위. 콘솔 인자는 이름을 대소문자 구분 없이 받는다. */
enum class EKataCheatTarget : uint8
{
    /** 플레이어가 조종하는 Pawn. */
    Self,
    /** 플레이어 Pawn의 타게팅 컴포넌트가 지금 고른 대상(락온 대상 또는 소프트 타겟). */
    Target,
    /** 플레이어 Pawn에서 가장 가까운, ASC를 가진 다른 Pawn. */
    Nearest,
    /** 플레이어 Pawn을 뺀, ASC를 가진 모든 Pawn. */
    All
};

#if UE_WITH_CHEAT_MANAGER
/**
 * 치트 확장들이 공유하는 대상 해석 함수다. KataFramework와 KataAI가 모두 의존하는 KataTargeting에 둔다.
 * Shipping 구성에서는 컴파일되지 않는다.
 */
namespace KataFL
{
    /**
     * 콘솔 인자를 대상 범위로 바꾼다. Self·Target·Nearest·All을 대소문자 구분 없이 받는다.
     *
     * @param Text 콘솔 인자. 비어 있으면 DefaultTarget을 쓴다.
     * @return 알 수 없는 이름이면 false이며 OutTarget은 바뀌지 않는다.
     */
    KATATARGETING_API bool ParseCheatTarget(const FString& Text, EKataCheatTarget DefaultTarget, EKataCheatTarget& OutTarget);

    /**
     * 플레이어 기준으로 대상 액터를 모은다.
     * Nearest·All은 ASC를 가진 Pawn 중 플레이어 Pawn과 ExcludedTags 중 하나라도 가진 Pawn을 뺀다(예: 사망 상태 태그).
     * Self·Target은 ExcludedTags를 보지 않는다.
     *
     * @return 대상이 없으면 빈 배열. 플레이어가 Pawn을 조종하지 않으면 Self·Target·Nearest는 비고 All만 채워질 수 있다.
     */
    KATATARGETING_API TArray<AActor*> ResolveCheatTargets(const APlayerController* PlayerController, EKataCheatTarget Target,
        const FGameplayTagContainer& ExcludedTags);

    /**
     * 토글 치트의 인자를 해석한다. 비어 있으면 현재 값을 반전하고, 1·true·on과 0·false·off는 그 값으로 정한다.
     * 대상 해석 함수와 같은 치트 확장들이 공유하므로 여기에 둔다.
     *
     * @return 알 수 없는 값이면 false이며 bOutEnabled는 바뀌지 않는다.
     */
    KATATARGETING_API bool ParseCheatToggle(const FString& Text, bool bCurrent, bool& bOutEnabled);

    /** 대상 범위의 표시 이름. 로그와 콘솔 응답에 쓴다. */
    KATATARGETING_API const TCHAR* GetCheatTargetName(EKataCheatTarget Target);
}
#endif
