#pragma once

#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"
#include "KataAnimLayerInstance.generated.h"

class UKataAnimInstance;

/**
 * Linked Anim Layer로 링크되는 레이어 ABP의 부모 Anim Instance.
 *
 * 레이어 ABP는 BlendSpace·Sequence 같은 에셋 변수를 갖고, 이동 상태 계산은 메인 인스턴스(UKataAnimInstance)에 맡긴다.
 * 레이어 그래프는 GetMainAnimInstance로 메인 인스턴스의 이동 값을 읽는다.
 * 무기별 차이는 이 클래스를 상속한 레이어 ABP의 자식 ABP가 에셋 변수 기본값만 바꿔 표현한다.
 *
 * 메인 인스턴스가 UKataAnimInstance가 아니거나(ABP 에디터 프리뷰 등) 아직 없으면 GetMainAnimInstance는 nullptr를 돌려준다.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Kata Anim Layer Instance"))
class KATAFRAMEWORK_API UKataAnimLayerInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    //~ Begin UAnimInstance Interface
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    //~ End UAnimInstance Interface

    /**
     * 이 레이어가 링크된 메시의 메인 Anim Instance를 돌려준다.
     * 게임 스레드에서 캐시한 값을 읽기만 하므로 Thread Safe 함수와 Property Access에서 호출할 수 있다.
     * 메인 인스턴스가 UKataAnimInstance가 아니면 nullptr다.
     */
    UFUNCTION(BlueprintPure, Category = "Kata|Animation", meta = (BlueprintThreadSafe))
    UKataAnimInstance* GetMainAnimInstance() const { return MainAnimInstance; }

private:
    /** 소유 메시의 메인 Anim Instance를 찾아 캐시한다. 게임 스레드에서만 호출한다. */
    void CacheMainAnimInstance();

    // 메인 인스턴스와 링크된 레이어 인스턴스는 같은 메시가 소유하며, 메인 인스턴스가 바뀌면 레이어 인스턴스도 다시 만들어진다.
    // 워커 스레드에서 약참조를 해석하지 않도록 강참조로 둔다.
    UPROPERTY(Transient)
    TObjectPtr<UKataAnimInstance> MainAnimInstance;
};
