# 스포너 더미 설정: 보호, 사망 뒤 재생성, 반복 행동

작성: 2026-10-15  
갱신: 2026-10-15  
유형: 구현 기록  
대상: KataFramework 스포너(`AKataCharacterSpawner`), 더미 설정 `UKataSpawnerComponent_Dummy`, NPC 컴포넌트 `UKataDummyComponent`, Kata Combat 설정, 샘플 런타임 스포너 패널  
기준: [#112](https://github.com/jaykop/Kata/issues/112) 구현 작업 트리(커밋 71e02cd), [#113](https://github.com/jaykop/Kata/issues/113) 구현 작업 트리. 같은 트리에 있던 다른 작업의 미커밋 변경은 포함하지 않는다.

## 배경과 결론

샘플 확인 레벨(#75)의 베이가 영역만 나뉘고 실제로는 차이가 없었다. 더미 역할(맞아도 죽지 않음, 죽으면 다시 생김)은 Kata Debug 탭에서 PIE마다 손으로 걸어야 했고, 다시 생성되면 풀렸다.
배치 스포너에 더미 역할을 저장하는 `Dummy` 설정을 추가했다. #112는 공통 기반, 보호(None·Invincible·Ignore Damage), 사망 뒤 재생성을 맡았다.
#113은 같은 컴포넌트에 Action·Graph 반복과 시작 위치 복귀를 더하고, 런타임 스포너 패널의 반복·보호를 이 컴포넌트로 합쳤다. 머리 위 피격 표시(#114)는 후속 이슈다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 더미 역할 저장 | Debug 탭 치트 패널·런타임 스포너 패널에서 PIE마다 설정. 다시 생성되면 풀림 | 스포너 `Spawner Components`에 `Dummy`를 추가해 레벨에 저장 |
| 보호 | 치트 태그(`Cheat.Invincible`·`Cheat.IgnoreDamage`)만 있음 | 게임플레이 태그로 거는 Invincible·Ignore Damage. Kata Combat에 `Invincible Tag` 설정 추가 |
| 죽은 NPC | 다시 생성하지 않음(거리 관리 스포너도 제외) | `Respawn On Death`를 켠 더미만 정한 시간 뒤 처음 생성된 위치에 새로 생성 |
| 스포너 배치 | 모든 개체가 Spawn Area 선택과 위치 보정을 거침 | 앞쪽 개체에 고정 Transform을 쓰는 배치 경로(`FixedTransforms`) 추가 |
| 스포너 API | 없음 | C++ `ScheduleRespawn(Transform, Delay)` |
| 반복 행동 (#113) | 런타임 스포너 패널만 Action·Graph를 반복(에디터 모듈의 `FKataRuntimeSpawnerLoop`) | 더미 설정의 Repeat Mode·Action·Graph·Entry Trigger·간격. 배치 스포너와 패널이 같은 컴포넌트를 쓴다 |
| 시작 위치 복귀 (#113) | 없음. 루트 모션 공격을 반복하면 자리가 계속 바뀜 | Return To Start(기본 켬). 한 번의 실행이 끝나면 시작 위치·회전으로 순간이동 |
| 패널 보호 (#113) | 치트 태그. 치트 패널에서 생성 뒤에 끄고 켤 수 있음 | 더미 보호(게임플레이 태그). 치트 패널 토글로는 풀리지 않음 |

## 주요 결정과 이유

- **설정과 실행 상태를 나눴다.** 설정 객체는 배치마다 복사되고 배치가 끝나면 사라지므로, 캐릭터별 상태(시작 Transform, 붙인 보호 태그)는 NPC에 붙이는 `UKataDummyComponent`가 갖는다. 컴포넌트는 설정을 값으로 복사해 배치 사본을 참조하지 않는다.
- **스포너는 더미를 모른다.** 재생성은 범용 `ScheduleRespawn`으로 열고 더미 컴포넌트가 사망 알림에서 호출한다. 스포너가 더미 타입을 아는 곳은 중복 설정 검사 하나뿐이다(AI Override와 같은 방식).
- **재생성을 일반 생성 배치로 처리했다.** 단건 생성 요청을 따로 만들면 행 사본(AI Override·Faction Override), 설정 훅, 생성 이벤트, 거리 관리의 분산 경로가 모두 빠진다. 그래서 배치에 고정 Transform 목록을 더해 그 개체만 위치 선택·보정을 건너뛰게 했다. 생성 작업은 동시에 하나뿐이라 예약은 준비 목록에 쌓았다가 진행 중인 작업·수동 제거가 끝나면 시작한다.
- **위치는 생성 직후 캐릭터 Transform이다.** 배치의 개체별 요청 Transform은 제출 직후 비워지거나(일반 모드) 다음 후보가 덮어쓴다(분산 모드). 생성 훅은 생성과 같은 프레임에 오므로 그때 액터 Transform을 기록했다. 충돌 보정 뒤의 실제 위치라 다시 생성할 때도 같은 자리에 놓인다.
- **거리 관리 스포너는 원점이 범위 밖이면 다음 진입 배치에 합친다.** 범위 밖에서 생성하면 바로 거리 제거 대상이 되기 때문이다. 원점 이탈로 취소한 고정 위치 개체는 수가 아니라 Transform으로 돌려줘 다음 진입 때 같은 자리에 생성한다.
- **지연이 0이어도 다음 틱에 생성한다.** 사망 알림 안에서 생성을 시작하지 않기 위해서다.
- **Invincible 태그는 Kata Combat 설정에 두었다.** 플러그인은 태그를 정의하지 않으므로 프로젝트가 고른 태그를 설정에서 읽는다. 피해 GE의 Target Tag Requirements에 같은 태그가 있어야 막히므로, GE를 쓰지 않는 처리와 `KataKill`은 막지 않는다. 히트 판정 자체를 건너뛰는 치트 무적보다 약하다는 점을 manual에 적었다.
- **Loose Tag는 개수를 더하고 뺀다.** 치트 확장처럼 개수를 1로 맞추면 같은 태그를 다른 경로가 붙였을 때 그 기여를 지운다.
- 상태 보존 재생성과 슬롯 ID([휴면 계약 계획](../plan/Spawner-Dormancy-Contract-Plan.md))와는 별개다. 더미 재생성은 명시적으로 켠 개체만 새 캐릭터로 만들며 체력·상태를 이어받지 않는다.
- **반복은 더미 컴포넌트의 Tick으로 옮겼다(#113).** 패널의 반복은 에디터 모듈의 `FTSTicker` 루프였기 때문에 배치 스포너가 쓸 수 없었다. 반복 상태(대기 시간, 실행 중 표시)는 캐릭터마다 다르므로 컴포넌트에 두고, 반복 설정이 유효할 때만 Tick을 켠다. 판정 로직(ASC 준비 대기, 다른 Kata 실행 중 대기, 대상은 첫 플레이어 Pawn, Graph 진입 대기 처리)은 기존 루프를 그대로 옮겼다.
- **실행 종료는 시작한 실행이 끝난 시점으로 본다.** 시작에 성공했을 때만 실행 중으로 표시하고, Action·Graph가 모두 멈추면 끝난 것으로 처리한다. 피격 반응으로 중단돼도 끝난 것으로 보고 복귀한다. 복귀를 켜면 복귀한 뒤부터 간격을 센다.
- **복귀는 순간이동만 한다(사용자 결정).** 걸어서 돌아오는 방식은 AI Controller 이동과 실패 처리가 필요하고 확인 시간이 길어진다. 이동을 멈춘 뒤 `TeleportTo`로 시작 위치·회전에 놓고 Controller 회전도 맞춘다. 막히면 경고 후 제자리에서 계속한다.
- **패널 보호도 더미 보호로 바꿨다(#113 범위).** 구현을 하나로 합치는 대신 치트 패널에서 생성 뒤에 보호를 끄고 켜는 기능은 잃었다. 패널 사용자 설정은 값 이름이 같은 `EKataDummyProtection`으로 바꿔 저장값을 그대로 읽는다.
- **반복 에셋은 하드 참조다.** 레벨에 놓은 스포너 설정이 Action·Graph를 함께 로드해도 확인용 더미라 부담이 작다. 패널은 생성 콜백까지 에셋을 강한 참조로 잡은 뒤 설정에 넣는다.

## 근거

- [스포너 액터](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataCharacterSpawner.h): `ScheduleRespawn`, 재생성 대기·준비 목록, `StartSpawnBatch`의 고정 Transform 인자.
- [배치 실행 데이터](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataSpawnBatchTypes.h): `UKataSpawnBatchState::FixedTransforms`.
- [더미 컴포넌트](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataDummyComponent.h)와 [더미 설정](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataSpawnerComponent_Dummy.h). 반복 설정, `TickComponent`, `ReturnToStart`.
- [런타임 스포너 패널](../../Source/ProjectKataEditor/KataDebugPanel_Spawner.h): 생성 완료 때 더미 컴포넌트를 붙이는 경로.
- [Kata Combat 설정](../../Plugins/KataFramework/Source/KataFramework/Public/Attributes/KataCombatSettings.h): `InvincibleTag`.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| Editor 빌드 | 사용자 빌드 보고(2026-10-15) | 통과 | Game 타깃 |
| 더미 보호·재생성 | 사용자가 샘플 확인 레벨에서 테스트 가이드로 PIE 확인(2026-10-15) | 테스트 통과 보고 | 항목별 개별 결과는 보고되지 않음 |
| #113 Editor 빌드 | 사용자 빌드 보고(2026-10-15) | 통과 | Game 타깃 |
| 반복·복귀·패널 통합 | 사용자가 샘플 확인 레벨의 공격자 더미와 패널로 테스트 가이드 PIE 확인(2026-10-15) | 테스트 통과 보고 | 항목별 개별 결과는 보고되지 않음 |

## 남은 제한과 후속 작업

- 머리 위 피격 정보 표시: [#114](https://github.com/jaykop/Kata/issues/114).
- 거리 관리 스포너는 `Cancel Spawning`·`Despawn Characters`를 거절하므로 재생성 예약은 스포너 종료 때만 버려진다.
- 재생성 지연이 시체 제거보다 짧으면 시체와 겹쳐 생성된다.
- 반복은 AI를 끈 더미를 전제로 한다. AI가 켜진 캐릭터에 반복을 걸면 StateTree의 행동과 겹친다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [연결 이슈](https://github.com/jaykop/Kata/issues/112) | 구현과 사용자 확인 결과. 댓글·닫기는 사용자 확인 후 |
| [#113](https://github.com/jaykop/Kata/issues/113) | 반복·복귀·패널 통합 구현과 사용자 확인 결과. 댓글·닫기는 사용자 확인 후 |
| [스포너 사용법](../manual/Spawner.md) | "더미로 쓰기" 단락, 재생성 예약과 거리 관리 규칙, 반복 설정과 반복 규칙 |
| [런타임 스포너 사용법](../manual/Runtime-Spawner.md) | 보호 방식 변경, Return To Start, 반복을 더미 컴포넌트가 맡는다는 설명 |
| [Attribute 사용법](../manual/Attributes.md) | Invincible Tag 설정과 더미 보호의 관계 |
| [게임플레이 태그 사용법](../manual/Gameplay-Tags.md) | `Status` 루트를 쓰는 설정에 Invincible Tag 추가 |
| [사망 사용법](../manual/Death.md) | 부활 없음 행에 더미 재생성 안내 |
