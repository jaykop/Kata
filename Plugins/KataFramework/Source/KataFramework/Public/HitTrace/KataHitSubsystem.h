#pragma once

#include "CoreMinimal.h"
#include "HitTrace/KataHitTraceTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "KataHitSubsystem.generated.h"

class AActor;
class UAbilitySystemComponent;
class UKataActionInstance;
class UKataHitBoxComponent;
class UKataHitBoxPreset;
class UKataTask_HitTrace;
class UKataTask_Trail;
class UKataTaskInstance;
class UMeshComponent;
class UNiagaraComponent;
class UPrimitiveComponent;

/** UKataTask_HitTrace가 판정 구간을 등록할 때 넘기는 값. */
struct FKataHitBoxRegistration
{
    UKataTask_HitTrace* Task = nullptr;
    UKataTaskInstance* TaskInstance = nullptr;
    UKataActionInstance* ActionInstance = nullptr;
    UMeshComponent* Mesh = nullptr;
    /** 직전 포즈를 제공할 컴포넌트. 없으면 판정 첫 구간 대신 시작 시점 판정만 한다. */
    UKataHitBoxComponent* HitBoxComponent = nullptr;
    AActor* InstigatorActor = nullptr;
    UAbilitySystemComponent* SourceAbilitySystem = nullptr;
    /** 판정 구간의 Kata 시각 [StartTime, EndTime]. */
    float StartTime = 0.0f;
    float EndTime = 0.0f;
};

/** 등록된 판정 구간 하나의 실행 상태. 공유 에셋에 둘 수 없는 이전 포즈와 맞은 대상 목록을 여기에 둔다. */
USTRUCT()
struct FKataActiveHitBox
{
    GENERATED_BODY()

    int32 Handle = INDEX_NONE;

    UPROPERTY()
    TObjectPtr<UKataTask_HitTrace> Task = nullptr;

    UPROPERTY()
    TObjectPtr<UKataHitBoxPreset> Preset = nullptr;

    TWeakObjectPtr<UKataTaskInstance> TaskInstance;
    TWeakObjectPtr<UKataActionInstance> ActionInstance;
    TWeakObjectPtr<UMeshComponent> Mesh;
    TWeakObjectPtr<UKataHitBoxComponent> HitBoxComponent;
    TWeakObjectPtr<AActor> InstigatorActor;
    TWeakObjectPtr<UAbilitySystemComponent> SourceAbilitySystem;

    float StartTime = 0.0f;
    float EndTime = 0.0f;

    /** 직전 판정 시각의 소켓 포즈와 몽타주 상태. 소켓 순서는 UKataHitBoxPreset::GetRequiredSockets와 같다. */
    FKataSocketPathHistory PoseHistory;

    bool bStartChecked = false;
    /** 태스크가 정상 완료로 끝났다. 이번 Tick에 EndTime까지 잘라낸 마지막 판정을 한 뒤 지운다. */
    bool bClosing = false;

    /** 이 구간에서 이미 맞은 대상. 태스크당 대상 1회 규칙을 지킨다. */
    TSet<TWeakObjectPtr<AActor>> HitActors;

    /** 디버그 표시 전용. 맞은 뒤에도 교차 영역을 계속 그리기 위해 맞은 컴포넌트를 기억한다. 판정에는 쓰지 않는다. */
    TArray<TWeakObjectPtr<UPrimitiveComponent>> DebugHitComponents;
};

/** UKataTask_Trail이 궤적 구간을 등록할 때 넘기는 값. */
struct FKataTrailRegistration
{
    UKataTask_Trail* Task = nullptr;
    UKataTaskInstance* TaskInstance = nullptr;
    UKataActionInstance* ActionInstance = nullptr;
    UMeshComponent* Mesh = nullptr;
    /** 직전 포즈를 제공할 컴포넌트. 기록이 없으면 몽타주 재생 위치를 되짚고, 재샘플링도 불가능하면 현재 포즈를 쓴다. */
    UKataHitBoxComponent* HitBoxComponent = nullptr;
    AActor* InstigatorActor = nullptr;
    /** 궤적을 그릴 Niagara 컴포넌트. 등록한 쪽이 생성해 넘기고, 등록에 성공하면 방출 정지와 시간 배율은 Subsystem이 맡는다. */
    UNiagaraComponent* NiagaraComponent = nullptr;
    /** 궤적 구간의 Kata 시각 [StartTime, EndTime]. */
    float StartTime = 0.0f;
    float EndTime = 0.0f;
};

/** 등록된 궤적 하나의 실행 상태. 직전 포즈와 Niagara 컴포넌트는 공유 태스크 에셋에 둘 수 없어 여기에 둔다. */
USTRUCT()
struct FKataActiveTrail
{
    GENERATED_BODY()

    int32 Handle = INDEX_NONE;

    UPROPERTY()
    TObjectPtr<UKataTask_Trail> Task = nullptr;

    TWeakObjectPtr<UKataTaskInstance> TaskInstance;
    TWeakObjectPtr<UKataActionInstance> ActionInstance;
    TWeakObjectPtr<UMeshComponent> Mesh;
    TWeakObjectPtr<UKataHitBoxComponent> HitBoxComponent;
    TWeakObjectPtr<AActor> InstigatorActor;
    TWeakObjectPtr<UNiagaraComponent> NiagaraComponent;

    float StartTime = 0.0f;
    float EndTime = 0.0f;

    /** 등록할 때 에셋 User 파라미터에서 읽은 샘플 간격(cm, 도). */
    float SampleDistance = 0.0f;
    float SampleAngle = 0.0f;

    /** 에셋에 있는 출력 파라미터. KataTrailParameters::GetOutputNames 순서의 비트다. 없는 배열에는 쓰지 않는다. */
    uint32 OutputMask = 0;

    /** 직전 처리 시각의 소켓 포즈와 몽타주 상태. 소켓 순서는 UKataTask_Trail::Sockets와 같다. */
    FKataSocketPathHistory PoseHistory;

    /** 구간 시작점을 이미 넘겼다. 이후 프레임은 직전 프레임의 마지막 샘플에서 이어 간다. */
    bool bStartPointWritten = false;
    /** 태스크가 정상 완료로 끝났다. 이번 Tick에 EndTime까지 넘긴 뒤 방출을 멈춘다. */
    bool bClosing = false;
    /**
     * 방출을 멈췄다. 남은 입자가 사라져 컴포넌트가 스스로 소멸할 때까지 시간 배율만 계속 맞춘다.
     * 히트스톱 중에 끝난 궤적이 혼자 사라지지 않게 하기 위해서다.
     */
    bool bEmissionStopped = false;
};

/**
 * Hit Trace 판정과 히트 처리, 궤적 샘플링을 프레임 끝에 모아 수행하는 월드 Subsystem.
 *
 * Tick은 FTickableGameObject 경로로 TG_PostPhysics 뒤, TG_PostUpdateWork 앞에 실행된다.
 * TG_PrePhysics에서 도는 Kata Tick과 애니메이션 갱신이 끝난 뒤이므로 이번 프레임의 소켓 위치를 읽는다.
 * Tick 순서: 등록된 구간마다 판정 → 필터 → 히트 기록 제출 → 궤적 샘플을 Niagara로 전달
 * → UKataHitBoxComponent 포즈 기록 → 제출 순서대로 처리기 호출.
 * 궤적은 판정과 같은 직전 포즈 기록과 프레임 사이 재샘플링을 써야 하므로 포즈 기록 전에 처리한다.
 * 판정 대상은 UKataHurtBoxComponent뿐이다. UKataHitTraceSettings의 HurtBox Object Type으로 후보를 모으고 프리셋의 HurtBoxTagQuery로 거른다.
 * SocketTrace는 직전·현재 소켓 점으로 만든 삼각형 띠를 후보 HurtBox 도형과 직접 교차 계산하고,
 * ShapeSweep은 엔진 Object Type Sweep으로 판정한다.
 *
 * EditorPreview 월드에서도 생성되어 액션 에디터 프리뷰가 같은 경로로 판정하고 처리기를 실행한다.
 */
UCLASS()
class KATAFRAMEWORK_API UKataHitSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    /**
     * 판정 구간을 등록한다. 기준 메시에 필요한 소켓이 없으면 경고를 남기고 INDEX_NONE을 반환한다.
     * 등록한 쪽은 구간이 끝날 때 반드시 CloseHitBox를 호출한다.
     */
    int32 RegisterHitBox(const FKataHitBoxRegistration& Registration);

    /**
     * 판정 구간을 닫는다.
     * @param bFinalSweep true면 이번 Tick에 EndTime까지 잘라낸 마지막 판정을 한 뒤 지운다. 정상 완료에서만 true로 호출한다.
     */
    void CloseHitBox(int32 Handle, bool bFinalSweep);

    /**
     * 궤적 구간을 등록한다. 필수 참조가 없거나 소켓이 2개 미만이면 INDEX_NONE을 반환한다.
     * 기준 메시에 필요한 소켓이 없으면 경고를 남기고 INDEX_NONE을 반환한다.
     * 실패하면 넘긴 Niagara 컴포넌트는 등록한 쪽이 정리한다. 등록한 쪽은 구간이 끝날 때 반드시 CloseTrail을 호출한다.
     */
    int32 RegisterTrail(const FKataTrailRegistration& Registration);

    /**
     * 궤적 구간을 닫는다. 남은 입자는 Niagara 컴포넌트가 사라질 때까지 수명대로 진행한다.
     * @param bFinalSegment true면 이번 Tick에 EndTime까지 샘플을 넘긴 뒤 방출을 멈춘다. 정상 완료에서만 true로 호출한다.
     *                      false면 샘플을 더 넘기지 않고 바로 방출을 멈춘다.
     */
    void CloseTrail(int32 Handle, bool bFinalSegment);

    void RegisterHitBoxComponent(UKataHitBoxComponent* Component);
    void UnregisterHitBoxComponent(UKataHitBoxComponent* Component);

    /** 이 월드의 디버그 시각화 단계를 지정한다. CVar Kata.HitTrace.Debug보다 우선한다. ENABLE_DRAW_DEBUG가 꺼진 빌드에서는 효과가 없다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Hit Trace")
    void SetDebugDrawMode(EKataHitTraceDebugMode Mode);

    /** 월드별 지정을 지우고 CVar Kata.HitTrace.Debug를 따르게 한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|Hit Trace")
    void ClearDebugDrawMode();

    /** 이 월드에 실제로 적용되는 디버그 시각화 단계. */
    UFUNCTION(BlueprintPure, Category = "Kata|Hit Trace")
    EKataHitTraceDebugMode GetDebugDrawMode() const;

    /**
     * 새로 만들어지는 EditorPreview 월드가 시작할 디버그 단계.
     * 에디터 모듈이 사용자 설정에서 불러와 지정한다. 에디터를 다시 켜도 프리뷰 토글 선택이 유지되게 하려는 값이며 게임 월드에는 영향이 없다.
     */
    static void SetPreviewDefaultDebugDrawMode(EKataHitTraceDebugMode Mode);
    static EKataHitTraceDebugMode GetPreviewDefaultDebugDrawMode();

    //~ Begin USubsystem Interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    //~ Begin FTickableGameObject Interface
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    //~ End FTickableGameObject Interface

protected:
    /** 엔진 기본값(Game·Editor·PIE) 대신 게임·PIE·액션 에디터 프리뷰 월드를 지원한다. 편집 중인 레벨에서는 판정하지 않는다. */
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
    struct FHitCandidate;

    /** 구간 하나를 이번 Tick에 판정한다. false를 반환하면 구간을 지운다. */
    bool ProcessHitBox(FKataActiveHitBox& Entry, float DeltaTime);

    void SubmitHits(FKataActiveHitBox& Entry, TArray<FHitCandidate>& Candidates);
    void DispatchPendingHits();

    /** 궤적 하나를 이번 Tick에 샘플링해 Niagara로 넘긴다. false를 반환하면 항목을 지운다. */
    bool ProcessTrail(FKataActiveTrail& Entry, float DeltaTime);

    UPROPERTY(Transient)
    TArray<FKataActiveHitBox> ActiveHitBoxes;

    UPROPERTY(Transient)
    TArray<FKataActiveTrail> ActiveTrails;

    /** 이번 Tick에 제출된 히트. 같은 Tick의 끝에서 처리하고 비운다. */
    UPROPERTY(Transient)
    TArray<FKataHitRecord> PendingHits;

    TArray<TWeakObjectPtr<UKataHitBoxComponent>> HitBoxComponents;

    /** 이 Subsystem의 Tick 번호. 컴포넌트 포즈 기록이 바로 앞 Tick의 것인지 판정하는 데 쓴다. */
    uint64 TickIndex = 0;
    int32 NextHandle = 1;
    int32 NextSequence = 0;

    TOptional<EKataHitTraceDebugMode> DebugDrawModeOverride;

    static EKataHitTraceDebugMode PreviewDefaultDebugDrawMode;
};
