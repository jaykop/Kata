#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "KataHUDSubsystem.generated.h"

/**
 * 로컬 플레이어의 HUD ViewModel을 소유한다.
 * ViewModel은 클래스마다 하나씩 만들어 플레이어 수명 동안 유지하므로, HUD 위젯을 다시 만들어도 같은 인스턴스에 바인딩된다.
 * 모델(GAS Attribute, 사망, 피해 알림) 구독은 아직 하지 않는다. 각 ViewModel을 추가하는 단계에서 이 서브시스템에 넣는다.
 */
UCLASS()
class KATAFRAMEWORK_API UKataHUDSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    /**
     * ViewModelClass의 인스턴스를 돌려주고, 없으면 만들어 보관한다.
     * 클래스가 INotifyFieldValueChanged를 구현하지 않거나 추상 클래스면 nullptr을 반환하고 경고 로그를 남긴다.
     * 반환한 인스턴스는 이 서브시스템이 소유하며 로컬 플레이어가 사라질 때 해제된다.
     */
    UObject* FindOrAddViewModel(const UClass* ViewModelClass);

private:
    /** 클래스별로 하나씩 보관한다. 개수가 적으므로 선형 탐색으로 찾는다. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UObject>> ViewModels;
};
