#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "KataVM_EnemyHealthList.generated.h"

class UKataVM_EnemyHealth;

/**
 * 지금 표시할 적 체력바 항목 목록. UKataHUDSubsystem이 소유하고 항목을 더하고 지운다.
 * 레이어 위젯은 이 목록에 맞춰 항목 위젯을 만들거나 숨긴다. 항목 ViewModel의 수명은 이 목록이 가진다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Kata Enemy Health List Viewmodel"))
class KATAFRAMEWORK_API UKataVM_EnemyHealthList : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    const TArray<TObjectPtr<UKataVM_EnemyHealth>>& GetEntries() const { return Entries; }

    /** 이미 있으면 무시한다. */
    void AddEntry(UKataVM_EnemyHealth* Entry);

    /** 없으면 무시한다. */
    void RemoveEntry(UKataVM_EnemyHealth* Entry);

    void ClearEntries();

private:
    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, Category = "Kata|HUD", meta = (AllowPrivateAccess = true))
    TArray<TObjectPtr<UKataVM_EnemyHealth>> Entries;
};
