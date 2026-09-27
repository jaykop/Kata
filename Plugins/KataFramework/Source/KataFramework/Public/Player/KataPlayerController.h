#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "KataPlayerController.generated.h"

/**
 * Kata 플레이어 컨트롤러.
 *
 * 빙의한 캐릭터와 무관한 플레이어 단위 기능을 맡는다. 입력 바인딩과 기본 IMC 추가는
 * AKataPlayerCharacter가 담당하므로 이 클래스는 다루지 않는다.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Kata Player Controller"))
class KATAFRAMEWORK_API AKataPlayerController : public APlayerController
{
    GENERATED_BODY()
};
