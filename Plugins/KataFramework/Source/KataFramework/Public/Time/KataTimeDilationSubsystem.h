#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "KataTimeDilationSubsystem.generated.h"

class AActor;

/**
 * 액터와 월드의 시간 배율 요청을 모아 엔진 값에 반영하는 월드 Subsystem.
 *
 * 히트스톱·슬로모션·연출처럼 여러 주체가 같은 배율을 바꿀 때 서로 덮어쓰지 않도록, 배율을 직접 쓰지 않고 이 Subsystem에 요청한다.
 * 층은 액터마다 하나(AActor::CustomTimeDilation)와 월드 하나(AWorldSettings의 Time Dilation)다.
 * 같은 층에서는 Priority가 가장 높은 요청 하나를 쓰고, Priority가 같으면 나중에 들어온 요청을 쓴다. 두 층은 엔진이 곱한다.
 * 층에 첫 요청이 들어올 때 원래 값을 기억하고, 마지막 요청이 끝나면 그 값으로 되돌린다.
 * 요청이 있는 동안 다른 코드가 같은 엔진 값을 직접 바꾸면 그 층의 다음 갱신 때 덮어쓴다.
 *
 * 만료 시간은 시간 배율의 영향을 받지 않고 일시정지 중에는 멈추는 시간으로 잰다. 월드 Tick이 받은 시간을 월드 유효 배율로 나눠 누적한다.
 * 엔진 일시정지 중에는 이 Subsystem이 Tick하지 않으므로 만료도 멈춘다. 일시정지 자체는 요청으로 다루지 않는다.
 * 게임·PIE 월드와 액션 에디터 프리뷰 월드에서 만들어진다. 싱글플레이 전용이며 복제하지 않는다.
 */
UCLASS()
class KATAFRAMEWORK_API UKataTimeDilationSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    /**
     * 액터의 시간 배율을 요청한다.
     *
     * @param TargetActor 이 Subsystem과 같은 월드에 있는 액터. 액터가 파괴되면 다음 Tick에서 요청을 정리한다.
     * @param TimeDilation 0 이상의 유한한 값. 0이면 액터 Tick에 전달되는 시간이 0이 되고 이 액터를 기준으로 실행하는 Kata 시각도 멈춘다. Tick 호출 자체를 끄지는 않으며, Duration을 주거나 다른 경로로 반드시 해제한다.
     * @param Priority 값이 클수록 우선한다.
     * @param Duration 0보다 크면 이 시간이 지난 뒤 스스로 풀린다. 0 이하이면 ReleaseRequest를 부를 때까지 유지한다.
     * @return 요청 핸들. 대상이 없거나 값이 잘못되면 경고를 남기고 INDEX_NONE을 반환한다.
     */
    int32 RequestActorTimeDilation(AActor* TargetActor, float TimeDilation, int32 Priority, float Duration = 0.0f);

    /**
     * 월드 시간 배율을 요청한다. 실제 값은 AWorldSettings의 Min·Max Global Time Dilation 범위로 잘린다.
     * TimeDilation은 0 이상의 유한한 값이어야 하며, 0을 요청해도 엔진 하한이 적용된다.
     * Priority와 Duration의 계약은 RequestActorTimeDilation과 같다.
     * World Settings가 없거나 배율이 잘못되면 경고를 남기고 INDEX_NONE을 반환한다.
     */
    int32 RequestWorldTimeDilation(float TimeDilation, int32 Priority, float Duration = 0.0f);

    /** 요청을 푼다. 이미 만료됐거나 INDEX_NONE이면 아무것도 하지 않는다. */
    void ReleaseRequest(int32 Handle);

    //~ Begin USubsystem Interface
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    //~ Begin FTickableGameObject Interface
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;
    //~ End FTickableGameObject Interface

protected:
    /** 엔진 기본값(Game·Editor·PIE) 대신 게임·PIE·액션 에디터 프리뷰 월드를 지원한다. 편집 중인 레벨의 배율은 바꾸지 않는다. */
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
    struct FRequest
    {
        int32 Handle = INDEX_NONE;

        /** 액터 층의 대상. 월드 층 요청이면 비어 있다. */
        TWeakObjectPtr<AActor> TargetActor;

        bool bWorldLayer = false;
        float TimeDilation = 1.0f;
        int32 Priority = 0;

        /** 같은 Priority에서 나중 요청을 고르기 위한 순번. */
        uint64 Sequence = 0;

        /** ElapsedTime 기준 만료 시각. 0보다 작으면 해제할 때까지 유지한다. */
        double ExpireTime = -1.0;
    };

    int32 AddRequest(FRequest&& Request);

    /** 요청 중 이긴 것 하나를 고른다. 없으면 nullptr. bWorldLayer가 false이면 Actor 대상 요청만 본다. */
    const FRequest* FindWinningRequest(bool bWorldLayer, const AActor* Actor) const;

    /** 액터 층의 현재 이긴 값을 엔진 값에 쓰고, 요청이 없으면 기억한 원래 값으로 되돌린다. */
    void ApplyActorLayer(AActor* Actor);

    /** 월드 층의 현재 이긴 값을 엔진 값에 쓰고, 요청이 없으면 기억한 원래 값으로 되돌린다. */
    void ApplyWorldLayer();

    TArray<FRequest> Requests;

    /** 요청 중인 액터의 원래 CustomTimeDilation. 마지막 요청이 끝나면 지운다. */
    TMap<TWeakObjectPtr<AActor>, float> OriginalActorTimeDilations;

    /** 월드 층에 요청이 있는 동안 기억하는 원래 Time Dilation. */
    TOptional<float> OriginalWorldTimeDilation;

    /** 시간 배율의 영향을 받지 않고 일시정지 중에는 멈추는 만료용 시계(초). */
    double ElapsedTime = 0.0;

    int32 NextHandle = 1;
    uint64 NextSequence = 1;
};
