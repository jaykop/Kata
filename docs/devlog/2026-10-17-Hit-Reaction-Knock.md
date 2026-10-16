# 피격 반응 Knock.Back·Knock.Down 샘플과 기상 무적

작성: 2026-10-17  
갱신: 2026-10-17  
유형: 구현 기록 / 결정 기록  
대상: 샘플 흑기사 피격 반응(`Knock.Back`·`Knock.Down`), 기상 무적 구간, 확인용 샘플 공격  
기준: 이 기록을 담은 커밋. 샘플 에셋은 Diversion에서만 추적한다. C++·Config 변경은 없다

## 배경과 결론

피격 반응 계획 HR8의 뒷부분([#71](https://github.com/jaykop/Kata/issues/71))으로, 흑기사 샘플에 `Knock.Back` 4방향 반응과 `Knock.Down` 앞·뒤 반응을 만들고 기상 구간에 무적을 붙였다.
태그 4종과 Impact Responses의 두 행은 HR1에서 이미 있었으므로 코드와 설정은 바꾸지 않고 에셋만 만들었다.
사용자 PIE에서 `Knock.Back` 반응과 `Knock.Down` 반응·기상 무적을 확인했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| `Knock.Back` 반응 | 없음 | 몽타주·반응 Kata 각 4개, `GA_BlackKnight_HitReaction_KnockBack`. 원본 008140~008143(2.30초)을 쓰고 이동 캔슬은 1.33초부터 연다 |
| `Knock.Down` 반응 | 없음 | 몽타주·반응 Kata 각 2개(F 008100 3.00초, B 008110 3.17초), `GA_BlackKnight_HitReaction_KnockDown`. 좌·우 피격은 F 반응을 쓴다. 이동 캔슬은 F 2.83초, B 2.17초부터 연다 |
| 기상 무적 | 무적을 붙이는 샘플이 없음 | `Knock.Down` 반응 Kata에 `Kata Task: Apply Loose Tag`로 `Status.Invincible`을 F 1.6~2.83초, B 1.1~2.17초 동안 붙인다 |
| 원본 클립 루트 모션 | 008140~008143, 008100, 008110의 Enable Root Motion이 꺼져 있었다(다른 반응 클립은 켜져 있음) | 켰다. 꺼져 있으면 반응 중 이동 입력이 그대로 적용되고, `Knock.Back`의 루트 이동(약 1.6m)도 쓰이지 않는다 |
| `Flinch` 차단 | `Light`·`Medium`·`Heavy`·`Knock.Down`·`Groggy` 중에만 막힘 | `Status.HitReaction.Knock.Back`을 더했다 |
| 부여 | - | 두 GA를 `DA_GameplaySet_BlackKnight`와 PC `DA_GameplaySet`에 부여했다. 은기사·사냥개 세트에는 넣지 않았다 |
| 샘플 공격 | `KA_BlackKnight_Attack01` `Impact.Medium` | `Impact.Knock.Back`. `Medium` 샘플 공격은 없어졌다. `Knock.Down` 샘플 공격은 두지 않았다 |

## 주요 결정과 이유

- **클립 배정**: `Knock.Back` 008140~008143, `Knock.Down` F 008100, B 008110이다. 008101(008100의 긴 버전)과 008130(넉업)은 쓰지 않는다(2026-10-11 사용자 결정). 008140~008143은 변환한 클립 이름에 방향이 들어 있다.
- **캔슬·무적 시각**: 이동 캔슬은 원본 TAE의 끝 캔슬 시작(JumpTable 23·78·79·86)을 따른다. 008140~008143은 네 방향 모두 1.33초다. 기상 무적은 머리가 올라오기 시작할 때부터 캔슬이 열릴 때까지로 정했다(2026-10-11 사용자 결정). 계획의 "기상 구간에만 `Status.Invincible`"을 따른 것이다.
- **무적 표시 방법**: 새 기능 없이 기존 `Kata Task: Apply Loose Tag`를 썼다. 태스크가 끝나거나 반응이 끊기면 자신이 붙인 태그만 회수하므로, 다른 출처의 무적과 겹쳐도 남거나 지워지지 않는다.
- **재피격**: 계획대로 `Knock.Back`의 Activation Blocked Tags는 `Medium`과 같아 다시 무너지면 새 반응으로 시작한다. `Knock.Down` GA는 자신의 Activation Owned Tags를 Activation Blocked Tags에도 두어 다운 중에는 다시 반응하지 않는다. 다른 반응 GA는 모두 이미 `Status.HitReaction.Knock.Down`을 막고 있다.
- **넉백 태스크를 쓰지 않은 이유**: `Knock.Back` 원본에는 루트 이동이 있어 클립 그대로 밀린다. 루트 이동이 없는 반응을 밀어내는 [#120](https://github.com/jaykop/Kata/issues/120)의 태스크는 이 반응에 쓰지 않는다.
- **샘플 공격**: 처음에는 `Attack03`에 `Impact.Knock.Back`을 넣었으나, `Attack03`은 콤보로만 나와 확인하기 어렵다는 사용자 의견으로 `Attack01`로 옮겼다. `Knock.Down`은 확인하는 동안만 `Attack01`을 바꿨다가 되돌렸다.
- **에셋 생성 방식**: 몽타주와 `Knock.Down` Kata(새 태스크 포함)는 에디터 Python(`AnimMontageFactory`, `unreal.new_object`)으로 만들었다. 나머지 값은 MCP로 넣고 읽어 확인했다.

## 근거

- [피격 반응 계획](../plan/Hit-Reaction-Plan.md): `Knock.Down` 범위와 재피격 결정.
- [피격 반응 사용법](../manual/Hit-Reaction.md#확인-상태와-근거): 샘플 에셋 표와 `Knock.Back`·`Knock.Down` 설명.
- [KataTask_ApplyLooseTag.h](../../Plugins/Kata/Source/KataRuntime/Public/Tasks/KataTask_ApplyLooseTag.h): 구간 태그 부착과 회수.
- 원본 TAE 측정은 Git에서 제외한 로컬 변환 도구로 읽기만 했다. 값은 로컬 인계 메모에 있다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 몽타주·Kata·GA·세트 설정 | MCP로 값 읽기(에이전트) | 길이·Blend·슬롯·연결 클립·캔슬과 무적 구간·태그·방향 표·부여 목록이 계획과 일치 | - |
| `Knock.Back` 반응 | 사용자 PIE(2026-10-17) | 확인 완료 | `Knock.Back` Hit Cue 연출(Cue 에셋 없음) |
| `Knock.Down` 반응·기상 무적 | 사용자 PIE(2026-10-17) | 확인 완료 | 실제 샘플 공격으로 나오는 경우(샘플 공격 없음) |

PIE 확인 때는 Poise를 바로 무너뜨리려고 `Attack01`의 Poise 피해를 임시로 999로 올렸다가 커밋 전에 20으로 되돌렸다. `Knock.Down` 확인 때 바꾼 Impact도 `Impact.Knock.Back`으로 되돌렸다.

## 남은 제한과 후속 작업

- 은기사·사냥개에는 `Knock.Back`·`Knock.Down` 반응이 없어 이 공격에 Poise가 무너져도 반응하지 않는다.
- `Medium`·`Knock.Down` 등급을 쓰는 샘플 공격이 없다. 샘플 공격 등급 배정은 [#74](https://github.com/jaykop/Kata/issues/74) 플레이 메모 뒤에 정한다.
- `Knock.Back`·`Knock.Down` Hit Cue 연출이 없다. [#104](https://github.com/jaykop/Kata/issues/104)는 이 두 등급의 Cue를 #71로 넘겼지만 #71 범위에는 넣지 않았다. 어느 이슈에서 만들지 정해야 한다. 같은 날 [#104](https://github.com/jaykop/Kata/issues/104) 범위로 옮겼다(Cue 에셋이 없는 `Medium`도 함께).

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#71](https://github.com/jaykop/Kata/issues/71) | 구현·사용자 확인 완료. 결과 댓글과 닫기는 사용자 확인 뒤 |
| [피격 반응 사용법](../manual/Hit-Reaction.md) | 확인 상태, 샘플 표 `Knock.Back`·`Knock.Down` 행, 설정·재피격·샘플 공격 설명, 문제 해결 표의 `Flinch` 차단 목록 |
| [피격 반응 계획](../plan/Hit-Reaction-Plan.md) | `Knock.Down` 범위 행에 이 기록 링크 |
| [Gameplay Tags 사용법](../manual/Gameplay-Tags.md) | 영향 없음(태그 변경 없음) |
