#include "Animation/KataAnimLayerInstance.h"

#include "Animation/KataAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

void UKataAnimLayerInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    CacheMainAnimInstance();
}

void UKataAnimLayerInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    // 레이어가 메인 인스턴스보다 먼저 초기화된 경우를 위해 게임 스레드 업데이트에서 다시 찾는다.
    if (MainAnimInstance == nullptr)
    {
        CacheMainAnimInstance();
    }
}

void UKataAnimLayerInstance::CacheMainAnimInstance()
{
    const USkeletalMeshComponent* OwningMesh = GetOwningComponent();
    MainAnimInstance = OwningMesh != nullptr ? Cast<UKataAnimInstance>(OwningMesh->GetAnimInstance()) : nullptr;
}
