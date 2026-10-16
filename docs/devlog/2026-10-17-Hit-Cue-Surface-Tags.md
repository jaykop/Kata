# 피격 Cue의 재질별 이펙트: HurtBox 태그와 Kata GCN Hit Burst

작성: 2026-10-17  
갱신: 2026-10-17  
유형: 구현 기록 / 결정 기록 / 진단  
대상: KataFramework `UKataHitHandler_ApplyGameplayEffect`, 신규 `UKataGameplayCueNotify_HitBurst`, 샘플 맞은 쪽 Cue  
기준: `5310bb7` 기준 작업 트리. 같은 파일(`Hit-Reaction.md`, `Hit-Trace.md`, `docs/README.md`)에 다른 작업의 미커밋 변경이 있으며 이 기록에 포함하지 않는다.

## 배경과 결론

[#104](https://github.com/jaykop/Kata/issues/104)에서 맞는 순간의 이펙트를 히트 Cue에 넣으면서, 같은 등급의 Cue가 피격자에 따라 다른 이펙트(갑옷 입은 흑기사는 불꽃, 플레이어는 피)를 내야 했다.
엔진 Burst Cue의 Surface Type 조건은 Kata 히트 결과에 물리 재질이 없어 동작하지 않는다. 사용자 제안에 따라 HurtBox의 태그로 재질을 표시하고, 처리기가 그 태그를 Cue로 넘기며, 새 Cue 클래스가 태그로 이펙트를 고르게 했다.
사용자가 2026-10-17에 Editor 빌드와 PIE로 흑기사를 때릴 때 불꽃, 흑기사에게 맞을 때 피가 나오는 것을 확인했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| 처리기 → Cue 정보 | Cue 파라미터에 맞은 부위 정보가 없었다 | 히트 컴포넌트가 HurtBox면 Hurt Box Tags를 `AggregatedTargetTags`에 더한다. 맞은 쪽·때린 쪽 Cue가 같은 태그를 받는다 |
| Cue 클래스 | 엔진 Burst는 태그로 효과를 고를 수 없다 | `UKataGameplayCueNotify_HitBurst`(표시 이름 Kata GCN Hit Burst): 기본 Burst Effects 뒤에 Tag Burst Effects에서 처음 맞는 항목 하나를 같은 Spawn Context로 실행한다 |
| 샘플 태그 | 재질 태그가 없었다 | `HurtBox.Surface.Metal`·`HurtBox.Surface.Flesh`. 흑기사 HurtBox는 Metal, 플레이어는 Flesh |
| 샘플 Cue | Light·Heavy만 있고 흔들림·진동만 담았다 | 맞은 쪽 Cue 5개(Light·Medium·Heavy·Knock.Back·Knock.Down)를 Kata GCN Hit Burst로 두고 재질별 불꽃·피 Niagara를 넣었다 |
| 샘플 Niagara 가시성 | 첫 PIE에서 보이지 않았다. 이펙트가 HurtBox 표면(몸 메시 안쪽)에서 생기고, 불꽃 선이 1픽셀보다 가늘었다 | 사용자 파라미터 `User.CameraOffset`(60 cm)으로 카메라 쪽에 당겨 그리고, `User.SparkWidthScale`(4)로 불꽃 선을 굵게 했다 |
| 새 Cue가 재생되지 않음 | 에디터를 켠 채 기존 Cue를 복제하고 Cue 태그를 바꾼 3개(Medium·Knock.Back·Knock.Down)는 실행 로그만 남고 재생되지 않았다. Cue 매니저가 복제 시점의 태그로 등록한 것으로 판단했다 | 에디터 재시작 뒤 재생됐다. 엔진 내부 원인은 확인하지 않았다 |

## 주요 결정과 이유

- **물리 재질 대신 HurtBox 태그.** Hit Trace는 HurtBox 도형으로 히트를 직접 만들어 `PhysMaterial`이 비어 있다. 물리 재질을 채우려면 판정 코드에서 재질을 찾아 넣고 프로젝트에 Surface Type을 정의해야 한다. 이미 있는 `Hurt Box Tags`(루트 `HurtBox`)를 쓰면 판정 코드를 건드리지 않고, 약점 같은 다른 부위 속성과 같은 방식으로 다룰 수 있다. 사용자가 태그 방식을 제안했다(2026-10-17).
- **태그 전달 위치는 `AggregatedTargetTags`.** Cue 파라미터에 이미 있는 칸이고, 싱글플레이·비복제 경로라 직렬화 제약이 없다.
- **첫 일치 항목 하나만 실행.** 목록 순서를 우선순위로 써서 약점처럼 재질보다 앞서야 하는 태그를 표현한다. 맞는 항목이 없으면 기본 효과만 낸다.
- **엔진 Burst 실행 경로 재사용.** `FGameplayCueNotify_BurstEffects::ExecuteEffects`와 `FGameplayCueNotify_SpawnContext` 생성자가 `GAMEPLAYABILITIES_API`로 공개돼 있어 엔진을 고치지 않고 같은 Spawn Condition·Placement를 따른다.
- 채택하지 않은 안: 샘플 Cue Blueprint에서 피격 액터를 보고 분기하는 방식. 빌드는 필요 없지만 부위별로 나눌 수 없고 엔진 조건 대신 BP 로직이 생긴다.

## 근거

- [KataGameplayCueNotify_HitBurst.h](../../Plugins/KataFramework/Source/KataFramework/Public/HitReaction/KataGameplayCueNotify_HitBurst.h): Tag Burst Effects 항목과 실행 규칙.
- [KataHitHandler.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/HitTrace/KataHitHandler.cpp): `ExecuteHitCues`의 HurtBox 태그 전달.
- [KataHitSubsystem.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/HitTrace/KataHitSubsystem.cpp): 히트 결과를 HurtBox 컴포넌트로 직접 만드는 경로.
- 엔진 `GameplayCueNotifyTypes.cpp`의 `FindSpawnTransform`: 이펙트 배치는 히트의 충돌 지점과 법선을 쓴다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| C++ 변경 | 사용자 Editor 빌드 보고(2026-10-17) | 통과 | Game 타깃 빌드 |
| 태그 → Cue → Niagara 경로 | 에이전트가 PIE에서 Python으로 `GCN_Hit_Light`를 태그와 함께 실행, 화면 캡처(2026-10-17) | 불꽃·피 생성 확인 | - |
| 실제 공격 | 사용자 PIE(2026-10-17, 에디터 재시작 뒤) | 흑기사를 때리면 불꽃, 흑기사에게 맞으면 피 | 등급별 불꽃 크기 차이, 원본과의 나란한 비교 |
| 진단 | `log LogKataFramework VeryVerbose`, `AbilitySystem.DisplayGameplayCues 1` 로그 | 실제 공격이 `GameplayCue.Hit.Knock.Back`을 실행함을 확인 | 복제 Cue가 재생되지 않던 엔진 내부 원인 |

## 남은 제한과 후속 작업

- [#104](https://github.com/jaykop/Kata/issues/104)에 SFX(MetaSound 임시음), 히트스톱 0.05초 비교, 공격 타임라인의 [#7](https://github.com/jaykop/Kata/issues/7) 임시 연출 정리가 남았다.
- 엔진 Spawn Condition의 Surface Type 조건은 여전히 동작하지 않는다. 재질 구분이 필요한 다른 Cue도 Kata GCN Hit Burst를 쓴다.
- 샘플 Niagara의 카메라 쪽 오프셋과 불꽃 선 굵기 배율은 가시성을 위한 조정값이다.
- 에디터 실행 중 Cue 에셋을 새로 만들거나 태그를 바꾸면 PIE 확인 전에 에디터를 재시작한다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#104](https://github.com/jaykop/Kata/issues/104) | 진행 중. 이번 범위는 피격 VFX와 Cue 구조이며 SFX 등은 남음. 댓글 게시는 사용자 확인 후 |
| [피격 반응 사용법](../manual/Hit-Reaction.md) | 피격 연출 단락: Kata GCN Hit Burst 사용법, 태그 전달·우선순위, 샘플 Cue·Niagara, 확인 상태 |
| [Hit Trace 사용법](../manual/Hit-Trace.md) | Hurt Box Tags가 피격 연출 Cue로도 넘어간다는 점 |
| [게임플레이 태그 사용법](../manual/Gameplay-Tags.md) | `HurtBox` 루트 사용처와 샘플 재질 태그 |
