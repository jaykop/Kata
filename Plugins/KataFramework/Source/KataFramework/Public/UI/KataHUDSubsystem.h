#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "KataHUDSubsystem.generated.h"

class APawn;
class UAbilitySystemComponent;
class UKataVM_PlayerStatus;
struct FOnAttributeChangeData;

/**
 * 로컬 플레이어의 HUD ViewModel을 소유하고 모델을 구독해 값을 채운다.
 * ViewModel은 클래스마다 하나씩 만들어 플레이어 수명 동안 유지하므로, HUD 위젯을 다시 만들어도 같은 인스턴스에 바인딩된다.
 * 관찰할 폰은 AKataPlayerController가 빙의 폰이 바뀔 때 SetObservedPawn으로 전달한다.
 */
UCLASS()
class KATAFRAMEWORK_API UKataHUDSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * ViewModelClass의 인스턴스를 돌려주고, 없으면 만들어 보관한다.
     * 클래스가 nullptr이면 nullptr을 반환한다. 새로 만들 클래스가 INotifyFieldValueChanged를 구현하지 않거나
     * 추상·폐기·이전 버전 클래스면 nullptr을 반환하고 경고 로그를 남긴다.
     * 반환한 인스턴스는 이 서브시스템이 소유하며, 서브시스템 종료 시 보관 참조를 해제한다.
     */
    UObject* FindOrAddViewModel(const UClass* ViewModelClass);

    /**
     * 플레이어 상태 ViewModel이 따를 폰을 정한다. 이전 폰의 Attribute 구독을 해제하고 새 폰의 값을 즉시 반영한다.
     * 폰이 nullptr이거나 ASC 또는 UKataAttributeSet_Base가 없으면 값을 0으로 두고 HasStatus를 false로 둔다.
     */
    void SetObservedPawn(APawn* Pawn);

    UKataVM_PlayerStatus* GetPlayerStatus() const { return PlayerStatus; }

private:
    void ClearPlayerStatusBindings();
    void HandleAttributeChanged(const FOnAttributeChangeData& Data);

    /** 클래스별로 하나씩 보관한다. 개수가 적으므로 선형 탐색으로 찾는다. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UObject>> ViewModels;

    /** ViewModels에도 들어 있으며, 구독 처리에서 바로 쓰기 위해 따로 가리킨다. */
    UPROPERTY(Transient)
    TObjectPtr<UKataVM_PlayerStatus> PlayerStatus;

    TWeakObjectPtr<UAbilitySystemComponent> ObservedAbilitySystem;

    /** 구독한 Attribute 순서대로 보관한 핸들. 순서는 구현 파일의 Attribute 목록과 같다. */
    TArray<FDelegateHandle> AttributeChangeHandles;
};
