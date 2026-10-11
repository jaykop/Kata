#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KataConditionTypes.h"
#include "KataRuntimeTypes.h"

/**
 * 액션 실행 이력 코드를 빌드에 넣을지 정한다.
 *
 * Shipping·Test 구성에서는 0이라 기록 구조체, 조회 API, 콘솔 명령·변수와 GameplayDebugger 카테고리가 모두 빠진다.
 * 이 헤더의 다른 선언은 모두 이 매크로 안에 있으므로 기록하거나 읽는 쪽도 같은 조건으로 감싼다.
 * UHT는 이 조건 안의 UFUNCTION·UPROPERTY를 받지 않으므로 이력 API는 리플렉션 없는 C++ 전용이다.
 */
#ifndef KATA_WITH_ACTION_TRACE
#define KATA_WITH_ACTION_TRACE (!(UE_BUILD_SHIPPING || UE_BUILD_TEST))
#endif

#if KATA_WITH_ACTION_TRACE

class AActor;

/** 실행 이력을 글로 꺼낸 결과를 남기는 로그 카테고리. 한 줄이 기록 하나다. */
KATARUNTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogKataActionTrace, Log, All);

/**
 * 실행 이력 기록의 종류. 저장하지 않는 값이므로 순서를 바꿔도 된다.
 * 출력 이름은 KataActionTrace::GetEventName이 정한다.
 */
enum class EKataActionTraceEvent : uint8
{
    /** 액션이 시작됐다. 같은 실행의 종료와 InstanceId로 짝지어진다. */
    ActionStarted,
    /** 시작 요청이 거절됐다. StartResult가 사유이고 Tag·Detail·조건 결과가 상세다. */
    ActionRejected,
    /** 액션이 끝났다. EndReason이 사유다. */
    ActionEnded,
    /** UKataActionComponent::CancelAbilitiesAndKata가 호출됐다. Detail은 필터 유무, EndReason은 요청한 종료 사유다. */
    CancelRequested,
    TransitionWindowOpened,
    TransitionWindowClosed,
    CancelWindowOpened,
    CancelWindowClosed,
    /** 그래프 실행이 시작됐다. Asset이 그래프다. */
    GraphStarted,
    /** 그래프 실행이 끝났다. EndReason이 사유다. */
    GraphEnded,
    /** 트리거로 엣지를 골라 지금 전이를 시도했다. 결과는 이어지는 ActionStarted·ActionRejected가 남긴다. */
    TriggerTransition,
    /** 트리거로 엣지를 골라 현재 액션이 끝난 뒤의 전이로 예약했다. */
    TriggerReserved,
    /** 요구하는 전이 창이 닫혀 있어 트리거를 보관했다. 창이 열리면 다시 평가한다. */
    TriggerBuffered,
    /** 트리거를 받을 엣지가 없거나 조건이 맞지 않았다. Detail이 이유다. */
    TriggerUnmatched,
    /** 그래프가 실행 중이 아니거나 액션을 바꾸는 중이라 트리거를 평가하지 않았다. Detail이 이유다. */
    TriggerIgnored
};

/** 액션 시작 요청이 들어온 경로. */
enum class EKataActionTraceSource : uint8
{
    /** 시작 요청이 아닌 기록. */
    None,
    /** PlayKataAction. 그래프의 첫 액션과 예약 전이의 실행도 실행 중인 액션이 없으므로 여기에 해당한다. */
    Direct,
    /** Context에 OwningAbility가 있다. */
    Ability,
    /** PlayKataActionTransition으로 실행 중인 액션을 넘겨받았다. */
    Transition
};

/**
 * 실행 이력 기록 하나.
 *
 * 문자열과 배열이 없는 고정 크기 구조체다. 기록할 때 문자열을 만들지 않고 출력할 때만 글로 바꾼다.
 * 에셋은 이름과 약한 참조로만 남기므로 기록이 GC나 PIE 중 저장을 막지 않는다.
 * 이벤트마다 쓰지 않는 필드는 기본값으로 둔다.
 */
struct KATARUNTIME_API FKataActionTraceEntry
{
    /** 컴포넌트 안의 일련번호. 1부터 시작하며 AddTraceEntry가 채운다. */
    uint32 Sequence = 0;

    /** 같은 실행의 시작·창·종료를 짝짓는 번호. 액션 인스턴스의 UObject 고유 번호이며 0이면 실행과 무관하다. */
    uint32 InstanceId = 0;

    /** 기록한 엔진 프레임(GFrameCounter). AddTraceEntry가 채운다. */
    uint64 FrameNumber = 0;

    /** 기록한 월드 시각(초). AddTraceEntry가 채운다. */
    double WorldSeconds = 0.0;

    /** 액션 에셋 이름. 그래프 이벤트는 그래프 에셋 이름이다. */
    FName AssetName;

    /** AssetName의 약한 참조. 패널이 에셋을 찾아가는 데 쓴다. */
    TWeakObjectPtr<const UObject> Asset;

    /** 그래프 이벤트의 대상 노드. 액션 노드는 액션 에셋 이름, 그 밖의 노드는 객체 이름이다. */
    FName NodeName;

    /** 트리거·창·캔슬 태그, 또는 태그 거절에서 처음 걸린 태그. */
    FGameplayTag Tag;

    /** 이벤트별 보충 ID. 트리거 무시 이유, 공용 중단 필터, 차단 종류 같은 값이다. */
    FName Detail;

    /** 조건 결과의 Reason. bHasConditionResult가 true일 때만 의미가 있다. */
    FName ConditionReason;

    /** 같은 내용이 연달아 기록되어 합친 횟수. 1이면 합친 적이 없다. */
    uint16 RepeatCount = 1;

    EKataActionTraceEvent Event = EKataActionTraceEvent::ActionStarted;
    EKataActionTraceSource Source = EKataActionTraceSource::None;

    /** ActionRejected의 사유. */
    EKataStartResult StartResult = EKataStartResult::Started;

    /** ActionEnded·GraphEnded의 사유, CancelRequested가 요청한 사유. */
    EKataEndReason EndReason = EKataEndReason::Completed;

    EKataConditionStatus ConditionStatus = EKataConditionStatus::Pass;
    bool bHasConditionResult = false;

    /** 에셋 이름과 약한 참조를 함께 채운다. nullptr이면 비운다. */
    void SetAsset(const UObject* InAsset);

    /** 조건 평가 결과를 남긴다. */
    void SetConditionResult(const FKataConditionResult& Result);

    /** 시각·일련번호·반복 수를 빼고 내용이 같은지. 같은 기록을 합칠 때 쓴다. */
    bool HasSameContent(const FKataActionTraceEntry& Other) const;
};

/**
 * 정해진 개수만 보관하고 넘치면 가장 오래된 기록부터 덮어쓰는 순환 보관소.
 *
 * 첫 기록 때 저장 공간을 잡으므로 기록이 없는 소유자는 메모리를 쓰지 않는다.
 * 보관 개수가 바뀌면 다음 기록 때 최근 기록부터 남기고 크기를 다시 잡는다.
 */
class KATARUNTIME_API FKataActionTraceBuffer
{
public:
    /**
     * 기록을 더한다. 직전 기록과 내용이 같으면 새 항목 대신 직전 항목의 반복 수와 시각을 갱신한다.
     * 새 항목이면 InOutEntry의 Sequence와 RepeatCount를 채운다.
     *
     * @return 새 항목을 만들었으면 true, 직전 기록에 합쳤으면 false.
     */
    bool Add(FKataActionTraceEntry& InOutEntry, int32 InCapacity);

    /** 보관 중인 기록을 오래된 것부터 OutEntries에 복사한다. OutEntries의 기존 내용은 지운다. */
    void CopyEntries(TArray<FKataActionTraceEntry>& OutEntries) const;

    /** 보관 중인 기록 수. */
    int32 Num() const { return Entries.Num(); }

    /** 기록을 더하거나 합칠 때마다 1씩 늘어난다. 읽는 쪽이 다시 복사할지 판단하는 데 쓴다. */
    uint32 GetSerial() const { return Serial; }

    /** 기록과 저장 공간을 모두 비운다. 일련번호는 이어서 센다. */
    void Reset();

private:
    void Resize(int32 NewCapacity);

    TArray<FKataActionTraceEntry> Entries;

    /** 현재 저장 공간의 크기. 0이면 아직 기록하지 않았다. */
    int32 Capacity = 0;

    /** 가득 찬 뒤 가장 오래된 기록의 위치. 가득 차기 전에는 0이다. */
    int32 Head = 0;

    uint32 NextSequence = 1;
    uint32 Serial = 0;
};

namespace KataActionTrace
{
    /** Kata.ActionTrace.Enable. 0이면 새 기록을 남기지 않는다. 보관 중인 기록은 지우지 않는다. */
    KATARUNTIME_API bool IsEnabled();

    /** Kata.ActionTrace.Capacity를 1~1024로 제한한 값. 소유자 하나가 보관하는 기록 수다. */
    KATARUNTIME_API int32 GetCapacity();

    /** Kata.ActionTrace.LogLive. 켜져 있으면 새 기록을 남길 때마다 LogKataActionTrace에 한 줄씩 쓴다. */
    KATARUNTIME_API bool IsLiveLogEnabled();

    KATARUNTIME_API const TCHAR* GetEventName(EKataActionTraceEvent Event);
    KATARUNTIME_API const TCHAR* GetSourceName(EKataActionTraceSource Source);

    /**
     * 기록 하나를 `key=value` 한 줄로 바꾼다. 쓰지 않는 필드는 뺀다.
     * 로그를 정규식으로 거르는 도구(unreal-mcp LogsToolset 등)가 읽을 수 있도록 키 이름과 순서를 고정한다.
     * Owner는 actor= 값에 쓰며 nullptr이면 None이다.
     */
    KATARUNTIME_API FString FormatEntry(const FKataActionTraceEntry& Entry, const AActor* Owner);
}

#endif // KATA_WITH_ACTION_TRACE
