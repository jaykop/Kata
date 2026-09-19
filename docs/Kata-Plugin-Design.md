# Kata — UE5 Action Timeline Plugin 설계 문서

> 캐릭터 액션 1개를 단위로, 여러 태스크를 타임라인 기반으로 배치하고
> 에디터 프리뷰에서 시뮬레이션·세팅하며, 인게임에서 재생하는 플러그인.

**상태**: 설계 논의 단계 (구현 전)
**다음 단계**: 이름 확정 → 프로젝트 생성 → Claude Code에서 구현

---

## 0. 요약

| 항목 | 결정 |
|---|---|
| 액션 단위 이름 | `Kata` (型) |
| 플러그인 이름 | `Kata` |
| 콤보 설계 | Kata 내부에 넣지 않고 **Kata 간 전이(Transition)** 로 표현 |
| 조건 시스템 | `KataConditions` 독립 모듈 — Kata 런타임에 의존하지 않음 |
| 시간 소유권 | **Kata가 시간을 소유**, 몽타주는 그 위의 태스크 하나 |
| GAS 관계 | Kata가 Ability를 대체하지 않음. Ability **안에** 들어감 |

**최대 리스크 2가지**
1. 타임라인 에디터 Slate 위젯 — 플러그인 전체에서 가장 비싼 항목
2. 프리뷰 월드의 틱/물리 — 야심 수준에 따라 일정이 배수로 갈림

---

## 1. 네이밍

### 왜 `Action`을 피하는가

UE에는 이미 `InputAction`, `GameplayTask`, `GameplayAbility`가 있어 의미가 겹친다.
`UActionTask`, `UActionCondition` 같은 타입명은 코드를 읽는 사람을 헷갈리게 만들고,
타입 50개에 접두사로 붙었을 때 심볼 검색이 사실상 불가능해진다.

### 후보 비교

| 후보 | 평가 |
|---|---|
| **Kata** (型) | 엔진 심볼 충돌 없음. 4글자. `UKataAsset` / `FKataTimeline` / `SKataTimelineView` 모두 자연스러움. 무협·동양풍과 맞으면서도 추상적이라 범용성 유지 |
| Arte | 테일즈 시리즈 용어. 짧고 충돌 없음. "Art/Arts"와 오타 혼동 가능 |
| Gambit | FF12 연상. AI 조건 시스템 뉘앙스가 강해 액션 단위엔 다소 부적합 |
| Chosik (招式) | 동양풍엔 최적이나 비한국어권 팀원에게 발음·의미 전달이 어려움 |
| Move / Act / Score | 전부 충돌. 특히 `Move*`는 엔진 함수명 지뢰밭 |

### `Kata` 기준 네이밍

```
UKataAsset            // 액션 1개 = 에셋 1개
UKataTask             // 타임라인 위의 요소
UKataCondition        // 공용 조건 베이스
UKataComponent        // 런타임 재생기
FKataInstance         // 재생 중 상태
FKataHandle           // 재생 핸들
FKataTransition       // 콤보 전이
UKataSequenceAsset    // 콤보 그래프 뷰 (2차)
SKataTimelineView     // 에디터 타임라인 위젯
```

---

## 2. 모듈 구조

```
Kata/
├─ KataConditions   (Runtime)   조건만. 의존: Core, GameplayTags
├─ KataCore         (Runtime)   에셋/태스크/타임라인. 의존: KataConditions
├─ KataTasks        (Runtime)   기본 태스크 구현. 의존: KataCore, Niagara, AnimGraphRuntime
├─ KataGAS          (Runtime)   Tag/Attribute 조건, PlayKata Ability. 의존: GameplayAbilities
├─ KataAI           (Runtime)   BT Decorator / StateTree Condition 어댑터. 의존: KataConditions
├─ KataEditor       (Editor)    툴킷, 타임라인 위젯, 프리뷰 씬
└─ KataDeveloper    (Developer) Gameplay Debugger, CVar, 에셋 검증
```

### 분리 원칙 2가지

**`KataGAS`는 반드시 분리한다.**
`KataCore`가 `GameplayAbilities`에 하드 의존하면 GAS를 쓰지 않는 프로젝트에서 사용 불가능해진다.

**`KataAI`는 `KataConditions`에만 의존한다.**
BT 데코레이터 하나 쓰겠다고 타임라인 런타임 전체가 딸려오면 안 된다.
"조건 공용화"라는 요구사항의 실질적 성립 조건이 이것이다.

---

## 3. 데이터 모델

### 3.1 정의(Definition)와 인스턴스(Instance) 분리 — 1일차부터

이 분리를 하지 않으면 같은 Kata를 두 캐릭터가 동시에 재생할 때 상태가 서로 덮어쓴다.
나중에 고치려면 전면 리팩터링이 된다.

```cpp
// 에셋에 저장되는 불변 정의
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, Blueprintable)
class KATACORE_API UKataTask : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Timeline")
    float StartTime = 0.f;

    UPROPERTY(EditAnywhere, Category="Timeline")
    float Duration = 0.f;          // 0 = 순간 실행

    UPROPERTY(EditAnywhere, Category="Timeline")
    int32 TrackIndex = 0;

    UPROPERTY(EditAnywhere, Instanced, Category="Condition")
    TObjectPtr<UKataCondition> EnterCondition;

    UPROPERTY(EditAnywhere, Category="Network")
    EKataNetScope NetScope = EKataNetScope::Both;

    // 상태는 여기 두지 않는다. 전부 Runtime 쪽으로.
    virtual TSharedPtr<FKataTaskRuntime> CreateRuntime() const;
    virtual void OnEnter (FKataTaskRuntime&, FKataExecContext&) const {}
    virtual void OnTick  (FKataTaskRuntime&, FKataExecContext&, float Dt) const {}
    virtual void OnExit  (FKataTaskRuntime&, FKataExecContext&, EKataExitReason) const {}
};
```

### 3.2 필수 열거형

```cpp
UENUM()
enum class EKataNetScope : uint8
{
    ServerOnly,      // 히트박스, 데미지
    AutonomousOnly,  // 로컬 예측 연출
    SimulatedOnly,   // 타 클라이언트용
    Both
};

UENUM()
enum class EKataExitReason : uint8
{
    Completed,
    Interrupted,
    Cancelled,
    ActorDestroyed
};
```

`NetScope`를 태스크마다 선언하게 하지 않으면 구현체가 전부 `if (HasAuthority())` 떡칠이 된다.
`ExitReason`으로 중단 사유를 구분하지 않으면 정리 로직이 반드시 깨진다.

### 3.3 시간은 Kata가 소유한다

**Kata가 타임라인 시간을 소유하고, 애님 몽타주는 그 위에 얹히는 태스크 하나로 둔다.**

이유:
- 애니메이션 없는 Dedicated Server, AI 시뮬레이션, 애님 미완성 프로토타입에서도 동작
- PlayRate / 타임 스케일 제어 지점이 한 곳으로 모임
- 노티파이에 의존하지 않으므로 애니메이터와 프로그래머의 작업이 분리됨

단, 노티파이 기반 감각이 필요한 경우를 위해
`UKataTask_PlayMontage`에 `bDriveTimeline` 옵션을 두어 몽타주 포지션으로 타임라인을 리싱크할 수 있게 한다.

### 3.4 태스크 저장 방식: UObject vs InstancedStruct

| | Instanced UObject | FInstancedStruct |
|---|---|---|
| 성능/메모리 | 보통 | 우수 (StateTree/Mass 방식) |
| 디테일 패널 UX | 우수 | 보통 |
| BP 서브클래싱 | 우수 | 불가에 가까움 |

**요구사항이 "게임 프로젝트가 상속해서 확장"이므로 v1은 UObject로 간다.**
정의/런타임 분리만 지켜두면 이후 struct로 교체하는 것이 국소 변경으로 끝난다.

---

## 4. 콤보 설계

### 원칙

Kata 하나에 콤보 3타를 전부 넣지 않는다. 재사용이 불가능해지고,
AI가 "지금 상황에 쓸 수 있는 다음 수"를 검색할 수 없게 된다.

> **콤보 = Kata + 수락 윈도우 + 조건 + 다음 Kata**

### 4.1 전이

```cpp
USTRUCT(BlueprintType)
struct FKataTransition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere) TSoftObjectPtr<UKataAsset> NextKata;

    // 이 구간에 입력이 들어오면 예약 (선입력 버퍼)
    UPROPERTY(EditAnywhere) FFloatInterval AcceptWindow;

    // 예약된 전이가 실제로 발동하는 시점
    UPROPERTY(EditAnywhere) float CommitTime = -1.f;   // <0 = 즉시

    UPROPERTY(EditAnywhere) FGameplayTag InputTag;     // Attack.Light / Dodge

    UPROPERTY(EditAnywhere, Instanced) TObjectPtr<UKataCondition> Condition;

    UPROPERTY(EditAnywhere) float BlendTime = 0.1f;
    UPROPERTY(EditAnywhere) int32 Priority = 0;
};
```

### 4.2 캔슬 윈도우

캔슬은 1급 개념으로 별도 트랙에 둔다.

```cpp
USTRUCT()
struct FKataCancelWindow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere) FFloatInterval Range;
    UPROPERTY(EditAnywhere) FGameplayTagQuery AllowedCancelTags;  // Dodge, Block...
};
```

태스크로 만들 수도 있지만, 매 프레임 조회되는 데이터라 배열 스캔이 싸야 하고
디자이너가 타임라인에서 한눈에 봐야 하므로 별도 트랙이 낫다.

### 4.3 콤보 그래프 뷰 (2차 작업)

"콤보 전체를 한 화면에서 보고 싶다"는 요구는 반드시 나온다.
Kata를 노드, 전이를 엣지로 그리는 `UKataSequenceAsset`을 읽기 전용 그래프 뷰로 추가한다.

> **데이터의 주인은 반드시 Kata 쪽 전이 배열이어야 한다.**
> 두 곳에 저장하면 동기화 지옥이 된다.

### 4.4 체감을 결정하는 3개 값

- 선입력 버퍼 길이 (Input Buffer Duration)
- `AcceptWindow`
- `CommitTime`

`UKataDeveloperSettings`에 프로젝트 기본값을 두고 에셋에서 오버라이드하게 한다.

---

## 5. 조건 시스템 (핵심)

공용화의 성패는 **컨텍스트 추상화**에 달려 있다.
조건이 `UBlackboardComponent`나 `FStateTreeExecutionContext`나 `FKataInstance`를
직접 알고 있으면 그것은 공용 조건이 아니다.

### 5.1 컨텍스트

```cpp
USTRUCT(BlueprintType)
struct KATACONDITIONS_API FKataConditionContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) TWeakObjectPtr<AActor> Source;
    UPROPERTY(BlueprintReadOnly) TWeakObjectPtr<AActor> Target;
    UPROPERTY(BlueprintReadOnly) FVector TargetLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) float   NormalizedTime = 0.f;
    UPROPERTY(BlueprintReadOnly) FGameplayTagContainer TransientTags;

    // 프로젝트 확장용 슬롯
    UPROPERTY(BlueprintReadOnly) TScriptInterface<IKataContextProvider> Extra;
};
```

### 5.2 베이스 클래스

```cpp
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, Blueprintable, CollapseCategories)
class KATACONDITIONS_API UKataCondition : public UObject
{
    GENERATED_BODY()
public:
    // const & 부작용 없음이 계약. BT 데코레이터가 매 프레임 호출한다.
    virtual bool Evaluate(const FKataConditionContext& Ctx) const;

    virtual bool IsStatic() const { return false; }   // 캐싱 가능 여부
    virtual FText GetRichDescription() const;          // 타임라인/그래프 라벨

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext&) override;
#endif

protected:
    UFUNCTION(BlueprintImplementableEvent, DisplayName="Evaluate")
    bool K2_Evaluate(const FKataConditionContext& Ctx) const;
};
```

### 5.3 기본 제공 조건

| 분류 | 조건 |
|---|---|
| 복합 | `All`, `Any`, `Not`, `Xor` |
| GAS | `HasTag`, `TagQuery`, `AttributeCompare`, `AbilityCooldownReady`, `EffectStackCount` |
| 공간 | `Distance(2D/3D, Min/Max)`, `Angle(전방 FOV / 상대 각도)`, `Height`, `LineOfSight` |
| 상태 | `IsOnGround`, `IsFalling`, `HasMovementInput`, `VelocityCompare` |
| 기타 | `RandomChance(Seed)`, `Cooldown`, `TimeWindow(NormalizedTime 구간)` |

### 5.4 호스트 어댑터

어댑터는 각 호스트가 컨텍스트 슬롯을 채우는 얇은 껍데기다.

```cpp
// Behavior Tree
UCLASS()
class KATAAI_API UBTDecorator_KataCondition : public UBTDecorator
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Instanced) TObjectPtr<UKataCondition> Condition;
    UPROPERTY(EditAnywhere) FBlackboardKeySelector TargetKey;   // Target 슬롯 매핑

    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent&, uint8*) const override;
};

// StateTree — 조건이 InstancedStruct이므로 래핑
USTRUCT()
struct KATAAI_API FStateTreeKataCondition : public FStateTreeConditionCommonBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Instanced) TObjectPtr<UKataCondition> Condition;
    // InstanceData에 Source/Target 액터를 두고 스키마 바인딩으로 채운다
};
```

### 5.5 주의점

- **BT의 `Observer aborts`는 값 변화 감지가 필요하다.** 순수 폴링만 지원한다고 문서에 명시하고,
  필요하면 `UKataCondition_TagObserver`처럼 델리게이트 기반 서브클래스를 별도 제공한다.
- **BP 서브클래싱은 허용하되 `K2_Evaluate`는 매 프레임 호출 시 비용이 크다.**
  `IsStatic()`이 true인 조건은 재생 시작 시 1회 평가 후 캐시한다.
- **`Evaluate`에서 절대 할당하지 않는다.** 반환은 bool만.
  실패 이유는 `#if !UE_BUILD_SHIPPING` out param으로 처리한다.

---

## 6. 에디터 / 프리뷰 월드

### 6.1 프리뷰 씬의 함정

- `FPreviewScene`의 월드는 **액터가 자동으로 틱하지 않는다.**
  툴킷에서 직접 `World->Tick(LEVELTICK_All, Dt)`를 호출해야 한다.
- 물리가 필요하면 `FPreviewScene::ConstructionValues().SetCreatePhysicsScene(true)`.
- 몽타주만 보여주는 수준이면 `FAdvancedPreviewScene` + `USkeletalMeshComponent`로 충분하다.
- **이동/충돌/GAS까지 시뮬레이션하려면 실제 Pawn 스폰이 필요하고, 그 순간 사실상 미니 PIE가 된다.**

### 6.2 현실적인 절충안

| 용도 | 수단 |
|---|---|
| 스크러빙 / 타이밍 조정 | 프리뷰 씬 (히트박스 도형, 워핑 궤적, 이펙트 미리보기) |
| 실제 동작 검증 | PIE |

양쪽이 동일한 `UKataDebugSubsystem`을 사용해 디버그 드로잉 코드를 공유한다.
**PIE 중인 캐릭터의 Kata 재생을 에디터 타임라인에 실시간 반영(읽기 전용 플레이헤드)** 하면
작업 체감이 크게 좋아진다.

### 6.3 프리뷰 설정

에셋별 `UKataPreviewSettings`로 저장한다.
- 프리뷰 메시 / 애님 BP
- 더미 타겟의 거리·각도 (거리/각도 조건 테스트에 필수)
- 카메라 프리셋

### 6.4 타임라인 위젯: Sequencer vs 커스텀 Slate

Sequencer(`UMovieSceneSequence`) 위에 올리면 UI를 거의 공짜로 얻지만,
데이터 모델이 MovieScene에 묶이고 런타임 평가 방식이 게임플레이/네트워크와 맞지 않는다.

**커스텀 Slate를 권장한다.** 트랙 행, 드래그 가능한 섹션, 플레이헤드, 스냅.
다만 이것이 플러그인 전체에서 가장 비싼 항목이라는 점은 감안해야 한다.

---

## 7. 런타임

### 7.1 컴포넌트

```cpp
UCLASS(ClassGroup=Kata, meta=(BlueprintSpawnableComponent))
class KATACORE_API UKataComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    FKataHandle Play(UKataAsset* Kata, const FKataPlayParams& Params);
    void        Stop(FKataHandle Handle, EKataExitReason Reason);
    bool        TryQueueInput(FGameplayTag InputTag);   // 선입력 버퍼

    const FKataInstance* FindInstance(FKataHandle) const;
};
```

### 7.2 네트워크

상태 전체가 아니라 **재생 지시만 복제**한다.

```cpp
USTRUCT()
struct FKataPlaybackRep
{
    GENERATED_BODY()

    UPROPERTY() TObjectPtr<UKataAsset> Kata;
    UPROPERTY() float ServerStartTime;
    UPROPERTY() float PlayRate;
    UPROPERTY() int32 RandomSeed;
    UPROPERTY() uint8 SequenceId;   // 같은 Kata 연속 재생 구분
};
```

클라이언트는 이를 받아 자신의 시간축에서 재시뮬레이션한다.
오차가 임계를 넘으면 시간만 스냅한다.

### 7.3 GAS와의 관계

**Kata는 Ability를 대체하지 않고 Ability 안에 들어간다.**
쿨다운 / 코스트 / 예측은 GAS가 이미 잘 처리하는 영역이므로 다시 만들 이유가 없다.

```cpp
UCLASS()
class KATAGAS_API UGameplayAbility_PlayKata : public UGameplayAbility
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly) TSoftObjectPtr<UKataAsset> Kata;
};
```

---

## 8. 구현 단계

| 단계 | 내용 | 비고 |
|---|---|---|
| 1 | `KataConditions` 단독 — 조건 + BT/StateTree 어댑터 | **이것만으로도 실전 투입 가능**하고 설계가 검증된다 |
| 2 | `KataCore` + 태스크 3종(Montage, Trace, Niagara) + 컴포넌트 | 에디터는 디테일 패널만 |
| 3 | 타임라인 Slate 위젯 + 프리뷰 씬 | 가장 비싼 구간 |
| 4 | 전이 / 콤보 + 입력 버퍼 | |
| 5 | 네트워크 복제, GAS 통합 | |
| 6 | Gameplay Debugger, 에셋 검증기 | |

1단계를 먼저 끝내고 실제로 써보는 것이 중요하다.
조건 컨텍스트 추상화가 틀렸다면 그 시점에 드러나고, 그때 고치는 비용이 가장 싸다.

---

## 9. 미결정 사항

### 9.1 이름
`Kata`로 확정할지, 다른 후보를 더 볼지.
프로젝트명이 여기에 묶이므로 가장 먼저 확정해야 한다.

### 9.2 GAS 전제 여부
- **GAS 전용으로 간다면**: Attribute를 조건에 직접 넣어 설계를 크게 단순화할 수 있다.
- **범용으로 간다면**: 현재의 `KataGAS` 분리 구조를 유지한다.

### 9.3 프리뷰의 야심 수준
| 수준 | 예상 규모 |
|---|---|
| 몽타주 + 히트박스 스크러빙 | 2~3주 |
| 실제로 걷고 때리는 시뮬레이션 | 배수로 증가 |

**이 결정이 전체 일정을 가장 크게 좌우한다.**
