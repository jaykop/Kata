#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "KataTiltComponent.generated.h"

/** Tilt 요청 하나의 블렌드 설정. 요청을 등록할 때 정하고 해제될 때까지 바꾸지 않는다. */
struct FKataTiltBlendSettings
{
    /** 요청이 시작된 뒤 Alpha가 1에 이르는 시간(초). 0이면 바로 1이다. */
    float BlendInTime = 0.0f;

    /** 요청이 해제된 뒤 Alpha가 0에 이르는 시간(초). 0이면 바로 0이다. */
    float BlendOutTime = 0.0f;

    /** 출력 Pitch가 목표 Pitch로 다가가는 초당 최대 각도(도). 0이면 바로 목표로 맞춘다. */
    float MaxPitchSpeed = 0.0f;
};

/**
 * 대상 방향 Tilt의 요청을 모아 Anim Instance가 읽을 Pitch와 Alpha를 만드는 컴포넌트.
 *
 * 액션 태스크가 요청을 등록하고 매 Tick 목표 Pitch를 갱신하며, 끝날 때 해제한다. 요청이 여러 개면 가장 최근 요청을 따르고,
 * 그 요청이 해제되면 남은 요청 중 가장 최근 것을 따른다. 모든 요청이 해제되면 마지막 Pitch를 유지한 채
 * 마지막으로 해제된 요청의 Blend Out Time 동안 Alpha를 0으로 내린다. 그래서 콤보 전이로 액션이 취소돼도 기울기가 한 프레임에 풀리지 않는다.
 *
 * Pitch는 도 단위이며 위쪽이 양수다. 요청이 있거나 Alpha가 0보다 클 때만 Tick한다.
 * 실행 상태만 가지며 캐릭터별 설정은 갖지 않는다. 체인과 높이 설정은 UKataAnimInstance의 Class Defaults에 있다.
 */
UCLASS(ClassGroup = (Kata), meta = (BlueprintSpawnableComponent))
class KATAFRAMEWORK_API UKataTiltComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKataTiltComponent();

    /**
     * 요청을 등록하고 해제에 쓸 핸들을 돌려준다. 등록한 요청이 가장 최근 요청이 된다.
     * Alpha가 0인 상태에서 등록하면 출력 Pitch를 TargetPitch로 바로 맞춰, 이전 값에서 쓸고 지나가지 않게 한다.
     */
    int32 BeginTilt(const FKataTiltBlendSettings& Settings, float TargetPitch);

    /** Handle 요청의 목표 Pitch를 바꾼다. 이미 해제된 핸들은 무시한다. */
    void SetTiltTarget(int32 Handle, float TargetPitch);

    /** Handle 요청을 해제한다. 이미 해제된 핸들은 무시한다. */
    void EndTilt(int32 Handle);

    /** 지금 적용할 Pitch(도, 위쪽 양수). */
    UFUNCTION(BlueprintPure, Category = "Kata|Tilt")
    float GetTiltPitch() const { return CurrentPitch; }

    /** 지금 적용할 비율(0~1). 0이면 Tilt가 없다. */
    UFUNCTION(BlueprintPure, Category = "Kata|Tilt")
    float GetTiltAlpha() const { return CurrentAlpha; }

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnUnregister() override;

private:
    struct FActiveTilt
    {
        int32 Handle = INDEX_NONE;
        FKataTiltBlendSettings Settings;
        float TargetPitch = 0.0f;
    };

    /** 요청과 출력 값을 비우고 Tick을 끈다. */
    void ResetTilt();

    /**
     * 이 컴포넌트의 Tick을 Kata 액션 컴포넌트 뒤, 소유 캐릭터 메시 앞에 둔다.
     * 태스크가 이번 프레임에 넘긴 목표를 같은 프레임의 포즈에 반영하기 위해서다.
     * 루트 모션 몽타주 재생 중에는 CharacterMovement가 포즈를 갱신하므로 그때는 한 프레임 늦을 수 있다.
     */
    void SetTickPrerequisites(bool bAdd);

    /** 등록 순서대로 쌓는다. 마지막 항목이 가장 최근 요청이다. */
    TArray<FActiveTilt> ActiveTilts;

    int32 NextHandle = 1;

    float CurrentPitch = 0.0f;

    float CurrentAlpha = 0.0f;

    /** 요청이 모두 해제됐을 때 Alpha를 내리는 시간. 마지막으로 해제된 요청의 Blend Out Time이다. */
    float FadeOutTime = 0.0f;
};
