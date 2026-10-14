#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataConditionTypes.h"
#include "Templates/SubclassOf.h"
#include "UObject/ObjectPtr.h"
#include "KataRuntimeTypes.generated.h"

class AActor;
class UAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;
class UKataCommand;
class UKataTask;

/**
 * 부모 Template과 자식 Action에서 같은 타임라인 태스크를 가리키는 고유 ID.
 * 부모에서 태스크를 추가하거나 삭제해도 오버라이드 대상을 식별할 수 있도록 배열 인덱스 대신 사용한다.
 */
USTRUCT(BlueprintType)
struct KATARUNTIME_API FKataTaskId
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Timeline")
    FGuid Value;

    FKataTaskId() = default;
    explicit FKataTaskId(const FGuid& InValue)
        : Value(InValue)
    {
    }

    bool IsValid() const { return Value.IsValid(); }
    void Invalidate() { Value.Invalidate(); }
    FString ToString() const { return Value.ToString(EGuidFormats::DigitsWithHyphens); }

    static FKataTaskId NewId() { return FKataTaskId(FGuid::NewGuid()); }

    bool operator==(const FKataTaskId& Other) const { return Value == Other.Value; }
    bool operator!=(const FKataTaskId& Other) const { return Value != Other.Value; }

    friend uint32 GetTypeHash(const FKataTaskId& TaskId) { return GetTypeHash(TaskId.Value); }
};

/**
 * 같은 시각에 시작하는 태스크의 종류별 선후 관계를 나누는 실행 단계.
 * 숫자 우선순위만으로 의존성을 대신하지 않도록 단계, OrderHint, 명시적 의존성을 분리한다.
 */
UENUM(BlueprintType)
enum class EKataTaskPhase : uint8
{
    /** 입력·의도 해석처럼 다른 단계보다 먼저 확정해야 하는 처리. */
    Input,
    /** 이동 요청과 루트 모션 제어. */
    Movement,
    /** 게임플레이 판정, Gameplay Effect 적용, 이벤트 발송. */
    Gameplay,
    /** 애니메이션·몽타주 재생 요청. */
    Animation,
    /** 이펙트·사운드 등 표현 계층. */
    Presentation
};

/**
 * 태스크를 처리할 엔진 업데이트 시점. 현재는 KataTick만 지원한다.
 */
UENUM(BlueprintType)
enum class EKataTaskUpdateHook : uint8
{
    /**
     * 월드의 액터 Tick 전에 UKataExecutionWorldSubsystem이 액션 인스턴스를 갱신할 때 처리한다.
     * Subsystem을 사용할 수 없는 프리뷰 월드 등에서는 실행 컴포넌트의 PrePhysics Tick으로 처리한다.
     */
    KataTick,
    /** 스켈레탈 메시 포즈 확정 이후. 현재 미지원이며 해석 단계에서 오류로 보고한다. */
    AfterMeshPose UMETA(DisplayName = "After Mesh Pose (Unsupported)")
};

/** 선행 태스크에 무엇을 요구하는지 구분한다. 시간 의존과 완료 대기는 같지 않다. */
UENUM(BlueprintType)
enum class EKataTaskDependencyRequirement : uint8
{
    /** 같은 시각에 시작할 때 선행 태스크보다 뒤에 실행한다. 완료를 기다리지 않는다. */
    AfterStart,
    /** 선행 태스크가 끝날 때까지 시작을 미룬다. */
    AfterCompletion
};

/** 선행 태스크 참조. 대상은 같은 Kata의 해석된 타임라인 안에 있어야 한다. */
USTRUCT(BlueprintType)
struct KATARUNTIME_API FKataTaskDependency
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Ordering")
    FKataTaskId TaskId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Ordering")
    EKataTaskDependencyRequirement Requirement = EKataTaskDependencyRequirement::AfterStart;
};

/** 자식 Action이 부모 Template의 타임라인 태스크에 적용할 변경 방식. */
UENUM(BlueprintType)
enum class EKataTimelineChangeMode : uint8
{
    /** 지정한 프로퍼티만 덮어쓴다. 나머지는 부모 값을 따른다. */
    Modify,
    /** 항목을 남기되 실행하지 않는다. */
    Disable,
    /** 항목을 타임라인에서 제거한다. */
    Remove
};

/** 액션 에셋을 해석하면서 발견한 문제의 심각도. Error가 있으면 실행할 수 없다. */
UENUM(BlueprintType)
enum class EKataDiagnosticSeverity : uint8
{
    Info,
    Warning,
    Error
};

/** 해석 결과의 진단 항목. Code는 확장 가능한 식별자이고 Detail은 영어 진단 문자열이다. */
USTRUCT(BlueprintType)
struct KATARUNTIME_API FKataDiagnostic
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Diagnostics")
    EKataDiagnosticSeverity Severity = EKataDiagnosticSeverity::Info;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Diagnostics")
    FName Code;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Diagnostics")
    FKataTaskId TaskId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Diagnostics")
    FString Detail;

    /** 메시지에 표시할 태스크 이름. 비어 있으면 TaskId를 대신 쓴다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Diagnostics")
    FString TaskLabel;

    /**
     * 아직 설정하지 않은 필수 항목 때문에 발생한 오류인지 나타낸다.
     * 실행에서는 다른 오류와 같게 다루고, 에셋 저장 검사에서만 경고로 낮춘다.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kata|Diagnostics")
    bool bIncompleteAuthoring = false;

    FKataDiagnostic() = default;
    FKataDiagnostic(EKataDiagnosticSeverity InSeverity, FName InCode, const FKataTaskId& InTaskId, FString InDetail,
        FString InTaskLabel = FString(), bool bInIncompleteAuthoring = false)
        : Severity(InSeverity)
        , Code(InCode)
        , TaskId(InTaskId)
        , Detail(MoveTemp(InDetail))
        , TaskLabel(MoveTemp(InTaskLabel))
        , bIncompleteAuthoring(bInIncompleteAuthoring)
    {
    }

    /** 심각도를 앞에 붙인 메시지. 로그와 타임라인 진단에 사용한다. */
    FString ToDisplayString() const;

    /** 심각도를 빼고 코드·대상·설명만 담은 메시지. 심각도를 따로 전달하는 경로에서 사용한다. */
    FString ToDetailString() const;
};

/** 에셋이 직접 소유하는 타임라인 항목. */
USTRUCT(BlueprintType)
struct KATARUNTIME_API FKataTimelineEntry
{
    GENERATED_BODY()

    /** 공유 가능한 태스크 설정. 실행 상태를 저장하지 않는다. */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Kata|Timeline")
    TObjectPtr<UKataTask> Task;
};

/** 상속받은 타임라인 항목에 대한 자식의 변경분. 프로퍼티 단위로 오버라이드를 기록한다. */
USTRUCT(BlueprintType)
struct KATARUNTIME_API FKataTaskOverride
{
    GENERATED_BODY()

    /** 부모 타임라인에서 찾을 대상 항목. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Timeline")
    FKataTaskId TargetTaskId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Timeline")
    EKataTimelineChangeMode Mode = EKataTimelineChangeMode::Modify;

    /**
     * 오버라이드한 프로퍼티 이름 목록. 여기에 없는 값은 부모 변경을 그대로 따른다.
     * 값이 부모와 같아졌다는 이유만으로 항목을 자동 제거하지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Timeline", meta = (EditCondition = "Mode == EKataTimelineChangeMode::Modify", EditConditionHides))
    TArray<FName> OverriddenProperties;

    /** 오버라이드 값을 담는 편집용 사본. OverriddenProperties에 등록된 프로퍼티만 읽는다. */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Kata|Timeline", meta = (EditCondition = "Mode == EKataTimelineChangeMode::Modify", EditConditionHides))
    TObjectPtr<UKataTask> OverrideValues;
};

/** 이 액션이 실행되는 동안 다른 액션·Ability의 시작을 차단하는 설정. */
USTRUCT(BlueprintType)
struct KATARUNTIME_API FKataBlockingPolicy
{
    GENERATED_BODY()

    /** 이 액션이 실행되는 동안 시작을 차단할 다른 액션의 KataTags. UKataActionComponent가 판정한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Blocking")
    FGameplayTagContainer BlockedKataTags;

    /** 실행 중 ASC에서 차단할 Ability 태그. GAS의 BlockAbilitiesWithTags로 연결한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Blocking")
    FGameplayTagContainer BlockedAbilityTags;

    /** 켜면 이 액션을 시작할 때 현재 실행 중인 액션을 중단할 수 있다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Blocking")
    bool bCanInterruptActiveKata = false;
};

/** 쿨다운 시간을 시작할 시점. */
UENUM(BlueprintType)
enum class EKataCooldownApplyTime : uint8
{
    OnActivation UMETA(DisplayName = "On Activation"),
    OnEnd UMETA(DisplayName = "On End")
};

/**
 * 액션의 쿨다운 설정. 남은 시간은 ASC에 적용한 내부 Duration Gameplay Effect로 관리한다.
 * GroupTags가 비어 있으면 원본 Kata 에셋별로 독립된 쿨다운을 사용한다.
 */
USTRUCT(BlueprintType)
struct KATARUNTIME_API FKataCooldownPolicy
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Cooldown")
    bool bEnabled = false;

    /** 쿨다운 지속 시간(초). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Cooldown",
        meta = (DisplayName = "Duration", ClampMin = "0.0", Units = "s", EditCondition = "bEnabled", EditConditionHides))
    float Duration = 0.0f;

    /** 쿨다운을 적용할 시점. OnEnd는 정상 완료뿐 아니라 중단과 취소도 포함한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Cooldown",
        meta = (DisplayName = "Start Time", EditCondition = "bEnabled", EditConditionHides))
    EKataCooldownApplyTime ApplyTime = EKataCooldownApplyTime::OnActivation;

    /** 쿨다운을 공유할 그룹 태그. 비워 두면 이 액션 에셋에만 쿨다운을 적용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Kata|Cooldown",
        meta = (DisplayName = "Shared Group Tags", EditCondition = "bEnabled", EditConditionHides))
    FGameplayTagContainer GroupTags;
};

/**
 * 루프 정책. 한 번 활성화된 인스턴스 안에서 타임라인을 반복한다.
 * 반복할 때마다 Ability를 재활성화하거나 비용을 지불하거나 쿨다운을 다시 적용하지 않는다.
 * 한 회차가 끝나면 다음 프레임에 재시작하며, 끝을 넘긴 시간은 다음 회차로 넘기지 않는다.
 */
USTRUCT(BlueprintType)
struct KATARUNTIME_API FKataLoopPolicy
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Loop")
    bool bLoop = false;

    /** 최초 실행을 포함한 총 실행 횟수. 0은 외부에서 멈출 때까지 반복한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Loop", meta = (ClampMin = "0", EditCondition = "bLoop", EditConditionHides))
    int32 MaxLoopCount = 0;
};

/** Kata 인스턴스의 수명 단계. */
UENUM(BlueprintType)
enum class EKataInstanceState : uint8
{
    Created,
    Running,
    Ended
};

/** 인스턴스 종료 사유. */
UENUM(BlueprintType)
enum class EKataEndReason : uint8
{
    /** 타임라인이 끝까지 진행되어 정상 종료했다. */
    Completed,
    /** 다른 Kata나 게임플레이 요청으로 중단됐다. */
    Interrupted,
    /** 호출자가 명시적으로 취소했다. */
    Cancelled,
    /** 소유 액터나 ASC가 유효하지 않게 됐다. */
    OwnerInvalid,
    /** 실행 계약을 만족하지 못해 강제 종료했다. */
    ContractError,
    /**
     * 그래프 전이로 다음 액션에 실행을 넘기며 끝났다. 외부 요청으로 끊긴 Interrupted와 구분한다.
     * 직렬화된 값이 바뀌지 않도록 새 값은 항상 끝에 추가한다.
     */
    Branched
};

/** 액션이 종료될 때 실행할 명령과 실행할 종료 사유. */
USTRUCT(BlueprintType)
struct KATARUNTIME_API FKataPostCommandEntry
{
    GENERATED_BODY()

    /** 실행할 명령. 설정 객체이며 실행 상태를 저장하지 않는다. */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Kata|Command")
    TObjectPtr<UKataCommand> Command;

    /** 이 명령을 실행할 종료 사유. 비워 두면 모든 사유에서 실행한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Command")
    TArray<EKataEndReason> EndReasons;

    bool ShouldRunFor(EKataEndReason Reason) const
    {
        return EndReasons.IsEmpty() || EndReasons.Contains(Reason);
    }
};

/** 개별 태스크의 실행 상태. */
UENUM(BlueprintType)
enum class EKataTaskState : uint8
{
    /** 아직 시작 시각에 도달하지 않았다. */
    Pending,
    /** 시작 시각을 지났지만 완료 의존성 때문에 대기 중이다. */
    WaitingForDependency,
    Running,
    Finished,
    /** 의존성 미충족이나 Restart On Loop 해제 때문에 이번 회차에서 시작하지 않았다. */
    Skipped
};

/** 태스크 종료 사유. */
UENUM(BlueprintType)
enum class EKataTaskEndReason : uint8
{
    /** 지속 시간이 끝났거나 태스크가 스스로 완료를 알렸다. */
    Completed,
    /** 루프 경계나 외부 요청으로 중단됐다. */
    Interrupted,
    Cancelled,
    /** 소유 Kata 인스턴스가 끝났다. */
    KataEnded
};

/**
 * 연출 태스크가 정상 완료(Completed)로 끝났을 때 재생 중인 효과를 어떻게 할지 정한다.
 * 실행 중인 태스크가 중단·취소·Kata 종료로 끝나면 이 값과 관계없이 효과를 정리한다.
 * 이미 정상 완료한 태스크가 남긴 효과는 이후 Kata 종료로 다시 정리하지 않는다.
 */
UENUM(BlueprintType)
enum class EKataEffectEndPolicy : uint8
{
    /** 효과가 스스로 끝날 때까지 둔다. 루프 에셋은 끝나지 않으므로 Stop으로 처리한다. */
    KeepPlaying,
    /** 태스크가 끝나면 효과를 부드럽게 멈춘다. 멈추는 방식은 태스크마다 다르다. */
    Stop
};

/** Kata 시작 요청의 결과. 거절 사유를 묵시적으로 감추지 않는다. */
UENUM(BlueprintType)
enum class EKataStartResult : uint8
{
    Started,
    InvalidDefinition,
    InvalidContext,
    MissingAbilitySystem,
    ResolveFailed,
    MissingRequiredTags,
    BlockedByTags,
    BlockedByActiveKata,
    ConditionFailed,
    OnCooldown,
    /**
     * ASC의 차단 Ability 태그(GA·GE의 Block Abilities with Tag, 다른 Kata의 BlockedAbilityTags)가 이 Kata의 KataTags와 맞는다.
     * 보유 태그로 막힌 BlockedByTags와 구분한다. 직렬화된 값이 바뀌지 않도록 새 값은 항상 끝에 추가한다.
     */
    BlockedByAbilityTags
};

/**
 * 태스크가 대상을 고르는 방법.
 *
 * 대상 지정 코드를 태스크마다 따로 만들지 않도록 최소 선택지를 공용으로 둔다.
 * 선행 태스크의 출력까지 받는 공용 FKataTargetSpec은 아직 도입하지 않았다.
 */
UENUM(BlueprintType)
enum class EKataTaskTargetSource : uint8
{
    /** 메시와 애니메이션을 가진 실제 액터. 비어 있으면 OwnerActor를 사용한다. */
    Avatar,
    /** Kata를 실행하는 논리적 주체. */
    Owner,
    /** Context가 지정한 대상 액터. 비어 있으면 대상을 찾지 못한 것으로 본다. */
    ContextTarget
};

/**
 * Kata 실행 한 번에 전달하는 외부 참조.
 * ASC가 PlayerState 등 별도 액터에 있으면 AbilitySystem을 직접 전달한다.
 */
USTRUCT(BlueprintType)
struct KATARUNTIME_API FKataContext
{
    GENERATED_BODY()

    /** Kata를 실행하는 논리적 주체. 보통 컴포넌트를 가진 액터다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Context")
    TWeakObjectPtr<AActor> OwnerActor;

    /** 메시와 애니메이션을 가진 실제 액터. 비어 있으면 OwnerActor를 사용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Context")
    TWeakObjectPtr<AActor> AvatarActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Context")
    TWeakObjectPtr<AActor> TargetActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kata|Context")
    TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;

    /** GAS에서 시작한 경우의 호출 Ability. 몽타주 재생과 비용·쿨다운 책임 판단에 사용한다. */
    UPROPERTY(BlueprintReadWrite, Category = "Kata|Context")
    TWeakObjectPtr<UGameplayAbility> OwningAbility;

    AActor* GetOwnerActor() const { return OwnerActor.Get(); }

    /** AvatarActor가 없으면 OwnerActor를 반환한다. */
    AActor* GetAvatarActor() const;

    AActor* GetTargetActor() const { return TargetActor.Get(); }

    /** 명시적 ASC를 우선 사용하고, 없으면 Avatar와 Owner에서 조회한다. */
    UAbilitySystemComponent* ResolveAbilitySystem() const;

    /** 지정한 방식으로 대상 액터를 고른다. 대상이 없으면 nullptr를 돌려준다. */
    AActor* ResolveActor(EKataTaskTargetSource Source) const;

    /**
     * 지정한 대상의 ASC를 돌려준다. 없으면 nullptr를 돌려준다.
     * Avatar와 Owner는 Context에 명시된 ASC를 우선 사용하고, ContextTarget은 대상 액터에서 조회한다.
     */
    UAbilitySystemComponent* ResolveAbilitySystemFor(EKataTaskTargetSource Source) const;

    /** 기존 KataConditions 평가용 Context로 변환한다. */
    FKataConditionContext ToConditionContext() const;

    bool HasValidOwner() const;
};
