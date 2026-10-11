#pragma once

#include "CoreMinimal.h"
#include "Delegates/IDelegateInstance.h"
#include "GameFramework/CheatManager.h"
#include "KataFrameworkCheatExtension.generated.h"

class APawn;
class UAbilitySystemComponent;
struct FGameplayTag;
struct FOnAttributeChangeData;

/**
 * 캐릭터 생존·전투 상태를 바꾸는 치트 명령이다. 모듈이 시작될 때 모든 CheatManager에 붙는다.
 *
 * 대상 인자는 Self·Target·Nearest·All이며 비우면 Target이다. 토글 인자는 비우면 반전, 0·1이면 그 값으로 정한다.
 * PC 토글(무적, 대미지 무시, 무한 스태미나)은 이 확장이 PlayerController별로 기억하고, Pawn이 바뀌면 새 Pawn에 다시 건다.
 * CheatManager는 Shipping 구성에서 만들어지지 않으므로 이 명령도 Shipping에서는 쓸 수 없다.
 */
UCLASS(NotBlueprintable)
class KATAFRAMEWORK_API UKataFrameworkCheatExtension : public UCheatManagerExtension
{
    GENERATED_BODY()

public:
    /**
     * 이미 있는 CheatManager와 이후 생성되는 CheatManager에 이 확장을 붙이도록 등록한다. 모듈 시작 시 호출한다.
     * @return UnregisterFromCheatManagers에 넘길 핸들. Shipping 구성에서는 등록하지 않고 빈 핸들을 돌려준다.
     */
    static FDelegateHandle RegisterForCheatManagers();

    /** RegisterForCheatManagers의 등록을 해제한다. 이미 붙은 확장은 CheatManager와 함께 사라진다. */
    static void UnregisterFromCheatManagers(FDelegateHandle Handle);

    /** 대상을 사망 처리한다. 사망 컴포넌트의 Kill을 쓰므로 Health 0과 같은 사망 흐름을 탄다. 예: KataKill Self */
    UFUNCTION(Exec)
    void KataKill(const FString& Target);

    /** 대상의 Groggy를 가득 채우고 그로기 반응 이벤트를 보낸다. Stance 세트가 없거나 Max Groggy가 0인 대상은 건너뛴다. */
    UFUNCTION(Exec)
    void KataGroggy(const FString& Target);

    /** PC 무적. 켜는 동안 Kata 히트 판정이 PC의 히트 처리기를 실행하지 않아 피해와 피격 반응이 모두 없다. */
    UFUNCTION(Exec)
    void KataGod(const FString& Enable);

    /** PC 대미지 무시. 피해 GE는 적용되어 피격 반응과 Poise·Groggy 누적은 그대로이고 Health만 줄지 않는다. */
    UFUNCTION(Exec)
    void KataNoDamage(const FString& Enable);

    /** PC 무한 스태미나. Stamina가 줄어들 때마다 Max Stamina로 되돌린다. 비용 지불 판정은 그대로 지나간다. */
    UFUNCTION(Exec)
    void KataInfiniteStamina(const FString& Enable);

    /** PC를 지연 없이 Player Start에서 다시 만든다. 살아 있으면 먼저 사망 처리한다. AKataGameMode가 필요하다. */
    UFUNCTION(Exec)
    void KataRespawn();

    bool IsGodEnabled() const { return bGod; }
    bool IsNoDamageEnabled() const { return bNoDamage; }
    bool IsInfiniteStaminaEnabled() const { return bInfiniteStamina; }

    //~ Begin UCheatManagerExtension Interface
    virtual void AddedToCheatManager_Implementation() override;
    virtual void RemovedFromCheatManager_Implementation() override;
    //~ End UCheatManagerExtension Interface

private:
    UFUNCTION()
    void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

    /** 현재 토글 상태를 Pawn의 ASC에 맞춘다. bApply가 false면 이 확장이 건 효과를 모두 걷어 낸다. */
    void SyncPawn(APawn* Pawn, bool bApply);

    /** Loose Tag를 토글 상태에 맞춰 0 또는 1개로 둔다. 태그가 비어 있으면 설정 누락을 경고한다. */
    void SyncLooseTag(UAbilitySystemComponent& AbilitySystem, const FGameplayTag& Tag, bool bEnabled, const TCHAR* SettingName) const;

    void BindStamina(UAbilitySystemComponent& AbilitySystem);
    void UnbindStamina();
    void HandleStaminaChanged(const FOnAttributeChangeData& Data);

    /** 콘솔과 로그에 결과를 한 줄 남긴다. */
    void Report(const FString& Message) const;

    bool bGod = false;
    bool bNoDamage = false;
    bool bInfiniteStamina = false;

    /** 무한 스태미나가 구독 중인 ASC. Pawn이 바뀌거나 사라지면 구독을 푼다. */
    TWeakObjectPtr<UAbilitySystemComponent> StaminaAbilitySystem;
    FDelegateHandle StaminaChangedHandle;
};
