# 레벨 이벤트 계획

작성: 2026-10-12  
갱신: 2026-10-12  
연결 이슈: [#63 레벨 이벤트 시스템 (KataWorld)](https://github.com/jaykop/Kata/issues/63)  
현재 상태 근거: [스포너 사용법](../manual/Spawner.md), [사망 사용법](../manual/Death.md), [카메라 사용법](../manual/Camera.md), [기본 조건](../manual/Conditions.md), [진행도·퀘스트 사전 검토](../devlog/2026-10-11-Progress-Quest-System-Review.md), [전역 메시지 라우터 도입 검토](../devlog/2026-09-24-Gameplay-Message-Router-Review.md)  
대체 관계: [액션 게임 기반 시스템 계획](Action-Game-Systems-Plan.md)의 스포너 항목에 있던 조우(Encounter) 볼륨 요구를 이 계획에서 구체화한다

## 목적과 현재 상태

지역 입장, 보스 조우, 트리거 발동 같은 레벨 이벤트를 레벨마다 Level Blueprint로 따로 짜지 않도록 공용 기반을 둔다.
레벨 이벤트를 네 부분으로 나눈다. 감지는 무슨 일이 일어났는지 알아내고, 채널은 감지와 반응을 GameplayTag로 잇는다.
반응은 한 프레임에 끝나는 단발 반응과 여러 단계로 진행되는 조우 흐름으로 나눈다. 남겨야 하는 사실은 월드 플래그에 기록한다.

현재 레벨 이벤트 기반은 없다. 연결할 기존 기능은 다음과 같다.

| 기존 기능 | 이 계획에서 쓰는 지점 |
|---|---|
| 스포너 | Activation `Manual`, `Spawn Characters`·`Despawn Characters`, 생성·사망·배치 완료 델리게이트 |
| 사망 | `UKataDeathComponent`의 사망 델리게이트와 `Kill`. PC 사망 시 `AKataGameMode`의 재시작. Kill Volume은 [사망 기록](../devlog/2026-10-11-Character-Death.md)에서 미뤄 두었다 |
| 카메라 | 카메라 StateTree의 Status Tag Watcher가 플레이어 ASC 태그 변화로 카메라 데이터를 다시 고른다. 커스텀 StateTree 스키마 선례 `UKataCameraStateTreeSchema` |
| 조건 | `UKataCondition`과 `FKataConditionContext`. 트리거 필터와 플래그 판정에 그대로 쓴다 |
| 진행도 검토 | GameplayTag 카운터 월드 플래그를 퀘스트보다 먼저 도입할 수 있다고 정리했다. 이 계획이 그 계층을 만들고 [#59](https://github.com/jaykop/Kata/issues/59)가 재사용한다 |

## 범위

- 포함: 새 위성 플러그인 `KataWorld`의 레벨 이벤트 채널, 트리거 볼륨과 단발 반응, 월드 플래그와 조건, 지역 볼륨과 현재 지역 추적, 조우 액터와 StateTree 스키마.
  `KataFramework`의 스포너·사망 반응과 조우 태스크, PC 사망·재시작 사실 이벤트. 샘플 흑기사 보스 아레나. 기능별 디버그 표시와 치트.
- 제외: 세이브 계층(별도 이슈), 상호작용(레버·문, [상호작용 시스템 계획](Interaction-Plan.md)·[#62](https://github.com/jaykop/Kata/issues/62)이 다룬다. 상호작용 결과를 레벨 이벤트로 보내는 연결은 두 작업이 진행된 뒤 정한다), Level Sequence 컷신 태스크, 음악 전환, 체크포인트·휴식 리스폰, 퀘스트([#59](https://github.com/jaykop/Kata/issues/59)),
  보스·지역 UI([#61](https://github.com/jaykop/Kata/issues/61)의 MVVM HUD 기반 위에서 별도로 다룬다), 보스 페이즈 전환(KataAI 범위), 복제.

## 구조

| 부분 | 구성(이름은 제안) | 위치 | 역할 |
|---|---|---|---|
| 감지 | `AKataTriggerVolume` | KataWorld | 모양, 대상 필터(플레이어·팩션·조건), 진입·이탈, 발동 정책(매번·1회·쿨다운)을 가진다. 발동하면 이벤트 태그를 방송하고 자기 단발 반응 목록을 실행한다 |
| 감지 | `AKataRegionVolume`, 현재 지역 추적 | KataWorld | 지역 태그와 우선순위를 가진다. 플레이어가 겹친 지역 중 현재 지역을 정하고, 바뀌면 진입·이탈 이벤트를 보내고 플레이어 ASC의 지역 태그를 바꾼다 |
| 감지 | PC 사망·재시작 사실 이벤트 | KataFramework | 사망과 재시작이 일어났다는 사실만 방송한다. 이벤트 태그는 설정에서 받는다 |
| 채널 | `UKataLevelEventSubsystem`(UWorldSubsystem), `FKataLevelEventPayload`, Blueprint 대기 노드 | KataWorld | 태그로 이벤트를 방송하고 구독한다. 페이로드는 Source, Instigator와 선택 `FInstancedStruct`다 |
| 반응 | `UKataLevelEventResponse` 파생 | KataWorld·KataFramework | 같은 프레임에 실행하고 끝나는 단발 반응이다. 이벤트 전송, 플래그 설정, 플레이어 태그, 스포너 생성, Kill |
| 반응 | `AKataEncounter`, 조우 StateTree 스키마와 태스크 | KataWorld·KataFramework | 여러 단계 흐름을 StateTree로 실행한다. 채널 이벤트를 StateTree 이벤트로 넘기고 Reset 진입점을 가진다 |
| 기록 | `UKataWorldFlagSubsystem`(UGameInstanceSubsystem), `UKataCondition_WorldFlag` | KataWorld | GameplayTag별 카운터와 변경 알림을 둔다. 조건은 플래그를 읽기만 한다 |

샘플 보스 조우는 다음 순서로 진행한다. 시작할 때 처치 플래그가 있으면 클리어 상태로 끝낸다. 없으면 아레나 진입을 기다리고,
진입하면 입구를 잠그고 보스를 생성한 뒤 전투에 들어간다. 보스가 죽으면 처치 플래그를 기록하고 입구 잠금을 해제한다.
PC가 죽으면 입구 잠금을 해제하고 보스를 정리해 다음 진입 때 새로 생성되게 한 뒤 준비 상태로 돌아간다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| 플러그인 배치 | 확정 | 2026-10-12 사용자 동의. 새 위성 `KataWorld`는 코어 `Kata`(KataConditions)와 GAS·StateTree에만 의존한다. 스포너·사망과 이어지는 코드는 `KataFramework`에 둔다 |
| 이벤트 채널 | 확정 | 2026-10-12 사용자 동의. 레벨 이벤트 전용 채널을 World 단위로 직접 만든다. [라우터 보류 결정](../devlog/2026-09-24-Gameplay-Message-Router-Review.md)이 정한 재검토 방식(Lyra 원본을 프로젝트 쪽에 둠)은 Kata 플러그인 안에서 쓸 수 없고 구독 수명이 GameInstance라 이 범위에 맞지 않는다. 범용 메시지 라우터는 도입하지 않는다 |
| 월드 플래그 수명 | 확정 | 2026-10-12 사용자 동의. 한 번의 플레이 동안 유지하도록 GameInstance에 둔다. 세이브 연결은 별도 이슈로 하며 그 전까지는 세션 동안만 유지한다 |
| 태그 루트 | 확정 | 2026-10-12 사용자 동의. `LevelEvent`, `Region`, `WorldFlag`. 플러그인은 태그를 정의하지 않고 루트만 정한다 |
| 조우 흐름 | 확정 | 2026-10-12 구조안 동의. 커스텀 스키마의 StateTree로 실행한다 |
| PC 사망 규칙 | 확정 | 2026-10-12 사용자 결정. 플러그인은 규칙을 두지 않고 사실 이벤트, Reset 진입점, 상태 종료 시 효과 원복만 제공한다. 리셋 여부와 시점은 조우 StateTree 에셋이 정한다. 샘플은 소울류 규칙(입구 잠금 해제, 보스 재생성 대기, 준비 상태로 복귀)을 쓴다 |
| 보스·지역 UI | 확정 | #61 계획에서 제외하고 레벨 이벤트 뒤에 다루기로 했다. 이 계획은 UI가 구독할 이벤트와 데이터만 제공한다 |
| 보스 페이즈 | 확정 | KataAI 범위다. 조우는 시작, 종료, 리셋만 맡는다 |
| 단발 반응 규칙 | 제안 | `UKataCommand`처럼 같은 프레임에 실행하고 끝나며 상태를 갖지 않는다. 시간이 걸리는 일은 조우 StateTree가 맡는다 |
| 1회 발동 기록 | 제안 | 플레이 동안 다시 발동하지 않아야 하는 트리거는 액터 변수가 아니라 플래그로 막는다. 셀 언로드·재시작 뒤에도 유지하기 위해서다 |
| 구독 매칭 | 제안 | 정확 일치가 기본이고 자식 태그 포함과 Source 필터를 선택한다. 같은 종류의 조우가 여럿일 때 Source로 구분한다 |
| 지역 중첩 | 제안 | 우선순위가 가장 높은 지역이 현재 지역이다. 같으면 나중에 들어간 지역을 쓴다 |
| 조우 태스크 원복 | 제안 | 입구 잠금, 플레이어 태그, 구독은 상태를 벗어나는 이유와 관계없이 되돌린다 |
| 샘플 리셋 시점 | 결정 필요 | PC 사망 이벤트와 재시작 이벤트 중 어느 쪽에서 리셋할지. 사망 연출 중 보스가 사라져도 되는지에 달려 있다 |
| 조우 참여 액터 참조 | 결정 필요 | 입구·스포너를 직접 액터 참조로 잡을지 태그로 찾을지. World Partition 셀 경계와 관련되므로 [#39](https://github.com/jaykop/Kata/issues/39)와 함께 정한다 |

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| LE-1 | 높음 | `KataWorld` 플러그인 생성, 이벤트 채널과 페이로드, Blueprint 대기 노드, 최근 이벤트를 보여 주는 GameplayDebugger 카테고리 | 없음 | Blueprint에서 보낸 이벤트를 다른 액터가 받고 디버거에 기록이 보인다 |
| LE-2 | 높음 | 트리거 볼륨, 발동 정책, 단발 반응 기반과 기본 반응. `KataFramework`의 스포너 생성·Kill 반응 | LE-1 | 볼륨에 들어가면 Manual 스포너가 생성한다. Kill 반응을 붙인 볼륨에 들어간 PC가 사망 흐름으로 죽는다 |
| LE-3 | 높음 | 월드 플래그와 변경 알림, `UKataCondition_WorldFlag`, 플래그 반응, 플래그 설정·조회 치트 | LE-1 | 플래그로 막은 1회 트리거가 PC 재시작 뒤 다시 발동하지 않는다 |
| LE-4 | 보통 | 지역 볼륨, 현재 지역 추적, 지역 진입·이탈 이벤트, 플레이어 지역 태그 | LE-1 | 지역 경계를 넘으면 이벤트가 오고, 샘플 카메라 StateTree가 지역 태그로 카메라 데이터를 바꾼다 |
| LE-5 | 보통 | 조우 액터와 스키마, 기본 태스크, `KataFramework`의 스포너·사망 태스크와 PC 사망·재시작 이벤트, Reset 진입점, 샘플 흑기사 아레나 | LE-2, LE-3 | 샘플 아레나에서 진입 시 입구 잠금과 보스 생성, 처치 시 플래그 기록과 잠금 해제, PC 사망 시 준비 상태 복귀 후 재도전, 처치 뒤 재시작 시 보스 미등장이 동작한다 |

항목별 진행 상태는 연결 이슈의 체크리스트와 댓글로 관리한다.

## 영향과 제한

- 모듈 경계: 의존은 `KataWorld` → 코어 `Kata`, `KataFramework` → `KataWorld` 방향이다. 코어와 다른 위성 플러그인은 `KataWorld`를 참조하지 않는다.
  `KataCamera`는 바꾸지 않고 플레이어 ASC 태그로만 이어진다. LE-1에서 `AGENTS.md`의 플러그인 구성을 갱신한다.
- 자원 수명: 채널 구독은 World Subsystem에 있어 월드가 끝나면 함께 정리된다. 월드 플래그는 GameInstance 수명이며 세이브 전까지 게임을 끄면 사라진다.
  조우 태스크와 지역 추적이 붙인 플레이어 태그는 종료·리셋·폰 교체 때 제거해야 한다.
- World Partition: 볼륨이나 조우 액터가 든 셀이 언로드되면 구독과 진행 중 흐름이 사라진다. 지켜야 할 결과는 플래그에 두며, 진행 중 조우의 언로드 처리는 #39와 함께 정한다.
- 샘플 에셋은 `Content/KataSample`에 둔다. 태그 루트는 구현하면서 [게임플레이 태그 사용법](../manual/Gameplay-Tags.md)의 루트 목록에 추가한다.
- 싱글플레이 전용이므로 복제를 넣지 않는다.

## 사용자 확인 항목

- 각 단계의 완료 조건은 사용자가 Editor 빌드와 PIE로 확인한다. 구현 완료와 실행 확인은 이슈에서 구분해 기록한다.
- 결정 필요 항목 두 개(샘플 리셋 시점, 조우 참여 액터 참조)는 LE-5 착수 전에 정한다.
- 이 목록만으로 에이전트가 빌드·테스트·별도 검사를 수행하지 않는다.

## 완료 시 갱신할 문서

- [연결 이슈](https://github.com/jaykop/Kata/issues/63): 단계별 구현·확인 상태.
- 새 manual `docs/manual/Level-Events.md`: 채널, 볼륨, 반응, 플래그, 조우 사용법과 제한.
- [게임플레이 태그 사용법](../manual/Gameplay-Tags.md): `LevelEvent`·`Region`·`WorldFlag` 루트.
- [AGENTS.md](../../AGENTS.md): 플러그인 구성과 의존 방향.
- devlog: 이슈 종료 시 이 계획의 결정과 이유를 옮기고 이 문서를 삭제한다.
- [문서 목록](../README.md): manual·devlog 추가, 이 계획 항목 제거.
