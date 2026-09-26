# 입력 계층 계획

작성: 2026-09-26  
갱신: 2026-09-26  
연결 이슈: [#19 입력 계층 (KataFramework)](https://github.com/jaykop/Kata/issues/19) · 로드맵 [#24](https://github.com/jaykop/Kata/issues/24) 1단계  
현재 상태 근거: [현재 구현 상태](../devlog/Implementation-Status.md) · [액션 게임 기반 시스템 계획](Action-Game-Systems-Plan.md#2-인풋) · [플러그인 분리 모듈화 계획](Plugin-Modularization-Plan.md) · [타게팅 시스템 설계](Targeting-Plan.md) · [그래프 노드 타입 계획](Graph-Node-Types-Plan.md)  
대체 관계: 없음

## 목적과 현재 상태

플레이어 입력으로 KataGraph 콤보 전이, 이동·시점, 락온을 조작하는 입력 계층을 `KataFramework`에 만든다.
캐릭터의 상태 태그에 따라 Input Mapping Context(IMC)를 넣고 빼는 방식으로 입력 가능한 행동을 제어한다.

현재 구현된 부분은 다음과 같다.

- 그래프는 `UKataGraphComponent::SendTrigger(FGameplayTag)`로만 전이 요청을 받는다. 트리거는 도착한 순간에 한 번만 평가하며 버퍼가 없다.
- 그래프는 마지막 액션이 정상 완료되고 자동 전이가 없으면 `Completed`로 끝난다. 실행 중이 아니면 `SendTrigger`는 false를 반환한다.
- 진입 대기 상태에서만 Entry 엣지를 평가한다. 액션 실행 중에는 현재 노드의 엣지를 먼저 평가하고, 이어서 현재 노드를 포함하는
  `UKataAliasNode`의 엣지를 평가한다([#25](https://github.com/jaykop/Kata/issues/25), 983e3af).
  - Alias의 `bAnyState`를 켜면 실행 중일 수 있는 모든 노드가 출발지가 된다. 캔슬 전이를 노드마다 긋지 않아도 된다.
  - Alias 엣지는 Required Window Tag와 Timing을 그대로 적용한다. Window 태그가 비어 있으면 액션 중 언제든 전이한다.
  - 우선순위가 같으면 노드에 직접 그은 엣지가 이긴다. Alias 엣지가 현재 노드 자신을 가리키면 건너뛴다.
  - Alias는 진입 대기 상태에서는 평가하지 않는다. 그래프가 비어 있을 때의 시작은 여전히 Entry 엣지가 담당한다.
  - Alias는 사용자 빌드·UI 확인까지 마쳤고 런타임 확인은 아직이다.
- `UKataAction`의 `ActiveGrantedTags`는 액션이 실행되는 동안 ASC에 loose 태그로 부여된다.
- `UKataPlayerTargetingComponent`는 소프트 타겟·락온 획득·좌우 전환·해제를, 기반 컴포넌트는 `ResolveActionTarget`을 제공한다.
- `AKataCharacter`는 ASC, `UKataActionComponent`, `UKataGraphComponent`, 타게팅 컴포넌트, HitBox 컴포넌트를 소유한다.
  `AKataPlayerCharacter`는 타게팅 컴포넌트를 `UKataPlayerTargetingComponent`로 만든다([#17](https://github.com/jaykop/Kata/issues/17), ab33690).
  캐릭터에 기본 그래프 지정 프로퍼티는 없다.
- `KataFramework`는 KataGraph·KataTargeting에 이미 의존한다. EnhancedInput 의존만 없다.

## 범위

- 포함: `AKataPlayerController`, 입력 설정 데이터 에셋, 상태 태그 기반 IMC 전환, 그래프 트리거 전달,
  이동·시점 입력, 락온 입력 연결, 샘플 IMC·InputAction 에셋.
- 제외: 입력 버퍼([#8](https://github.com/jaykop/Kata/issues/8)), AI 입력, 방향 조합·저스트 입력, 차지 시간 전달,
  입력 키 리바인딩 UI, 카메라 모드([#20](https://github.com/jaykop/Kata/issues/20)).

## 구성

### `AKataPlayerController` (KataFramework)

- `OnPossess`에서 폰의 ASC, `UKataGraphComponent`, `UKataPlayerTargetingComponent`를 찾아 캐시한다.
  `AKataPlayerCharacter`가 기본 대상이지만 캐릭터 클래스가 아니라 컴포넌트로 찾으므로 같은 컴포넌트를 가진 다른 폰에도 붙일 수 있다.
- `SetupInputComponent`에서 입력 설정의 InputAction을 `UEnhancedInputComponent`에 바인딩한다.
- `OnUnPossess`·`EndPlay`에서 태그 이벤트 구독을 해제하고 자신이 넣은 IMC를 모두 제거한다.

### `UKataInputConfig` (UPrimaryDataAsset)

- **Context 규칙 목록**: IMC, 우선순위, 활성 조건(`FGameplayTagQuery`, 비어 있으면 항상 활성).
- **트리거 바인딩 목록**: InputAction, `ETriggerEvent`, 보낼 트리거 태그. 같은 InputAction에 이벤트별로 다른 태그를 줄 수 있다.
- **고정 기능 InputAction**: 이동, 시점, 락온 획득·해제, 좌·우 전환. 이 입력은 그래프를 거치지 않고 해당 기능을 직접 호출한다.

누름·홀드·뗌·연타 같은 입력 형태는 Enhanced Input의 Trigger(Pressed, Hold, Released, Tap 등)로 표현한다. 자체 입력 형태 시스템은 만들지 않는다.

### 상태 태그 기반 IMC 전환

- 상태의 출처는 폰 ASC의 소유 태그다. 액션의 `ActiveGrantedTags`, Gameplay Effect 부여 태그, 게임이 추가한 loose 태그가 모두 해당한다.
- ASC의 태그 변경 이벤트를 구독하고, 변경이 생기면 모든 규칙을 다시 평가한다. 활성·비활성 차이만 IMC 추가·제거로 반영한다.
- 컨트롤러가 넣은 IMC만 관리하고 다른 시스템이 넣은 IMC는 건드리지 않는다.
- 같은 키를 여러 IMC가 쓰면 우선순위가 높은 IMC가 입력을 소비한다(`UInputAction::bConsumeInput`). 규칙 제거 방식과 우선순위 덮어쓰기 방식을 모두 쓸 수 있다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| 입력 계층 위치 | 확정 | `KataFramework`의 PlayerController와 입력 매핑. [플러그인 분리 모듈화 계획](Plugin-Modularization-Plan.md) |
| `AKataPlayerController` 추가 | 확정 | 2026-09-26 사용자 결정 |
| 기본 IMC·InputAction 에셋 | 확정 | 2026-09-26 사용자 결정. 샘플 에셋은 프로젝트가 소유한다(플러그인 `CanContainContent` false) |
| 상태·태그에 따른 IMC 추가·제거로 행동 제어 | 확정 | 2026-09-26 사용자 결정 |
| 락온 입력 | 확정 | 좌·우 전환은 각각의 InputAction으로 호출하고, 스틱 방향은 타게팅에 쓰지 않는다. [#19](https://github.com/jaykop/Kata/issues/19) |
| 입력 버퍼 | 확정 | 이번 범위에서 제외. 실제 조작 후 [#8](https://github.com/jaykop/Kata/issues/8)에서 결정 |
| 상태의 출처 | 제안 | 폰 ASC 소유 태그로 통일한다. UI·연출처럼 폰이 아닌 상태도 loose 태그로 표현할지 결정 필요 |
| 입력 설정 형태 | 제안 | 위 `UKataInputConfig` 구조 |
| 트리거 태그 이름 | 결정 필요 | 예: `Input.Attack.Light`. 프로젝트 `Config/Tags`가 소유 |
| 샘플 에셋 위치 | 결정 필요 | 샘플용 `Content` 하위 폴더. 테스트 전용이면 `Content/KataTest` |
| KataGraph 발동 방식 | 결정 필요 | 아래 "KataGraph 발동 방식" 항목 참고 |

## KataGraph 발동 방식 (논의 대상)

| 쟁점 | 선택지 | 제안 |
|---|---|---|
| 그래프 지정 | 캐릭터 기본 그래프 하나 / 트리거 태그별 그래프 | 캐릭터 기본 그래프 하나. 입력별 시작 액션은 Entry 엣지의 트리거 태그로 구분한다 |
| 기본 그래프 보관 위치 | 확정(2026-09-26 사용자 결정) | 기본 그래프는 입력이 없던 상태에서 첫 트리거가 왔을 때 시작할 캐릭터의 콤보 그래프다. `KataFramework` 캐릭터가 런타임 슬롯을 가지고, 컨트롤러는 슬롯만 읽는다. 슬롯은 당분간 BP 기본값으로 채우고, 이후 캐릭터 정의 데이터([#26](https://github.com/jaykop/Kata/issues/26))가 생성 시 채운다. 입력 작업을 캐릭터 정의 구조보다 먼저 진행한다 |
| 그래프 수명 | 입력 시 필요하면 시작 / 상시 유지 | 입력 시 시작. 그래프가 없으면 `StartGraphOnSelf` 후 같은 트리거를 보낸다. 진입에 실패하면 그래프를 멈춰 대기 상태를 남기지 않는다 |
| 시작 대상 | 없음 / 타게팅 컴포넌트 | `ResolveActionTarget` 결과를 시작 Context의 대상으로 사용한다 |
| 액션 중 캔슬(회피 등) | 기존 기능으로 처리 | 캔슬 시점은 각 액션의 Transition Window 태스크가 열고, 엣지의 Required Window Tag와 Immediate Timing으로 전이한다. 여러 액션에서 같은 곳으로 캔슬하는 엣지는 Alias(Any State 포함)로 한 번만 긋는다. 입력 계층은 트리거만 보내며 캔슬을 따로 처리하지 않는다 |
| 캔슬 대상 액션의 진입 | Entry 엣지와 Alias 엣지를 함께 긋기 | 그래프가 비어 있을 때는 Entry, 액션 중에는 Alias가 평가되므로 회피처럼 양쪽에서 시작하는 액션은 두 엣지를 모두 긋는다. 회피 중 연속 회피는 Alias가 자기 자신을 건너뛰므로 회피 노드에 직접 엣지를 긋는다 |
| 그래프 밖 직접 실행 | 허용 / 모든 액션을 그래프로 | 모든 액션을 그래프로. 단일 액션도 Entry→Action 그래프로 만든다 |

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
단계마다 사용자 빌드·실행 확인을 거친 뒤 다음 단계로 넘어간다. 각 단계 끝에 구현 상태 문서와 설명서를 해당 범위만 갱신한다.

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| IN-0 | 높음 | 결정 확정: 기본 그래프 보관 위치, 캔슬 방식(Any State Alias 제안), 트리거 태그 이름, 샘플 에셋 위치, 상태의 출처 | 이 문서 검토 | 확정·미확정 표의 결정 필요 항목이 확정된다 |
| IN-1 | 높음 | 기본 조작: `KataFramework`에 EnhancedInput 의존 추가, `UKataInputConfig` 뼈대, `AKataPlayerController`의 빙의·정리, 기본 IMC 적용, 이동·시점 바인딩, 샘플 IMC·InputAction·GameMode 지정 | IN-0 | 샘플 맵에서 `AKataPlayerCharacter` 기반 캐릭터가 이동하고 시점을 돌린다 |
| IN-2 | 높음 | 그래프 발동: 기본 그래프 프로퍼티, 트리거 바인딩, 그래프가 없을 때 시작 후 재전송, 진입 실패 시 정지, `ResolveActionTarget` 대상, 샘플 태그와 콤보 그래프(Entry 콤보와 Any State Alias 회피 캔슬 포함) | IN-1 | 키 입력으로 그래프 진입과 콤보 전이, 공격 중 회피 캔슬이 동작한다. [#25](https://github.com/jaykop/Kata/issues/25) Alias의 런타임 확인과 [#8](https://github.com/jaykop/Kata/issues/8)의 버퍼 없는 조작감 확인을 겸한다 |
| IN-3 | 높음 | 상태 태그 기반 IMC 전환: IMC 규칙 쿼리, ASC 태그 이벤트 구독, 차이만 추가·제거, 눌린 키 처리 | IN-2 | 액션의 `ActiveGrantedTags`로 이동 IMC가 빠졌다가 액션 종료 후 돌아온다 |
| IN-4 | 보통 | 락온 입력: 획득·해제, 좌·우 전환 InputAction 연결 | IN-1, [#13](https://github.com/jaykop/Kata/issues/13) | 락온 획득·좌우 전환·해제가 입력으로 동작한다 |
| IN-5 | 보통 | 마무리: #19 완료 조건 점검, 설명서·구현 상태 정리 | IN-2~IN-4 | 사용자 실행 확인 후 #19를 닫고 결정 이유를 devlog로 옮긴다 |

## 영향과 제한

- 모듈 경계: 코어 `Kata`는 변경하지 않는다. 캔슬은 이미 구현된 Alias로 처리한다.
- 의존: `KataFramework`에 EnhancedInput만 추가된다. KataGraph·KataTargeting 의존은 [#17](https://github.com/jaykop/Kata/issues/17)에서 추가됐다.
- SubGraph 평탄화([#25](https://github.com/jaykop/Kata/issues/25) G3)는 입력 계층에 영향이 없다. 샘플 그래프가 커지면 나중에 나눌 수 있다.
- 자원 수명: 태그 이벤트 구독과 컨트롤러가 넣은 IMC는 빙의 해제와 파괴 시 정리한다.
- IMC를 제거했다가 다시 넣을 때 눌린 키가 재발동하지 않도록 `FModifyContextOptions`의 눌린 키 무시 옵션을 확인한다.
