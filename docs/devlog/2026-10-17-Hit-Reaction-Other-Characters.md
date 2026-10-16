# 피격 반응 은기사·사냥개 샘플

작성: 2026-10-17  
갱신: 2026-10-17  
유형: 구현 기록 / 결정 기록  
대상: 샘플 은기사(SilverKnight)·사냥개(StarvedHound) 피격 반응 Ability·반응 Kata·몽타주, 두 캐릭터의 Gameplay Data 부여  
기준: 이 기록을 담은 커밋. 샘플 에셋은 Diversion에서만 추적한다. C++·Config 변경은 없다

## 배경과 결론

흑기사 샘플에만 있던 등급별 피격 반응을 은기사와 사냥개에도 만들었다([#122](https://github.com/jaykop/Kata/issues/122)).
작업 전에는 사냥개에 `Light`만 있었고, 은기사 세트에는 흑기사용 `Light`·`Flinch` Ability가 들어 있어 은기사 Skeleton에 흑기사 몽타주를 재생하려 했다.
범위는 원본 클립이 있는 등급 전부로 정했다(2026-10-17 사용자 결정). 은기사는 `Light`·`Medium`·`Heavy`·`Knock.Back`·`Knock.Down`·`Flinch`, 사냥개는 기존 `Light`에 `Medium`·`Heavy`·`Knock.Back`·`Knock.Down`을 더했다.
사용자 PIE에서 두 캐릭터의 등급·방향별 반응, 은기사 `Flinch`, `Knock.Down`의 재반응 차단과 기상 무적을 확인했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| 은기사 반응 | 흑기사 `Light`·`Flinch` Ability를 그대로 부여 | 은기사 몽타주 22개, 가산 클립 4개, 반응 Kata 18개, Ability 6개. 세트의 흑기사 `Light`·`Flinch`를 은기사 것으로 바꾸고 나머지 넷을 더했다. `GA_BlackKnight_Die`는 범위 밖이라 그대로 두었다 |
| 사냥개 반응 | `Light`만 있음 | 몽타주 14개, 반응 Kata 14개, Ability 4개를 만들고 세트 끝에 더했다 |
| 원본 클립 루트 모션 | 사냥개 008000 외에는 꺼져 있음 | 반응에 쓰는 클립 32개를 모두 켰다(은기사 Sword 18, 사냥개 14) |
| 은기사 `Flinch` 좌우 | 흑기사와 같게 009003을 L, 009002를 R에 넣었음 | 사용자 프리뷰에서 009002가 오른쪽, 009003이 왼쪽으로 휘청였다. 몽타주를 다시 만들어 L에 009002, R에 009003을 넣었다. 흑기사는 사용자 확인 결과 방향이 맞아 바꾸지 않았다 |
| 가산 몽타주 프리뷰 | 은기사·흑기사 `Flinch` 몽타주가 편집기에서 T자세로 보임 | Preview Base Pose가 비어 Reference Pose 위에 더해 보이는 표시 문제였다. 은기사 `Flinch` 몽타주 4개에 `AS_SilverKnight_Idle_Normal_Sword`를 지정했다. 흑기사 몽타주는 그대로 두었다 |

## 주요 결정과 이유

- **에셋 구조**: 같은 등급의 흑기사 Ability·반응 Kata를 복제해 몽타주, 재생 길이, 캔슬·무적 구간, 프리뷰 캐릭터만 바꿨다. 태그와 Activation Blocked Tags가 흑기사와 같아 재피격 규칙도 같다. Ability Blueprint는 복제해서 만든다는 기존 규칙을 따랐다.
- **은기사 클립과 시각**: 은기사 Sword(Bank 0) 원본 TAE는 8000번대의 이벤트 구성과 끝 캔슬 시작이 흑기사와 같다(`Light` 0.33, `Medium` 0.80, `Heavy` 1.17, `Knock.Back` 1.33, `Knock.Down` F 2.83·B 2.17초). `Light`·`Knock.Back` 방향은 루트 이동 방향으로 확인했고, 이동량이 없는 `Medium`·`Heavy`는 원본 번호 순서로 배정한 뒤 사용자 프리뷰로 확인했다.
- **사냥개 `Knock.Back`**: 사냥개에는 흑기사가 쓴 008140번대가 없다. 008120~008123은 맞고 뒤로 넉백되는 동작이고 `Heavy`(008020번대)는 앞발을 들며 살짝 밀리는 동작이어서, 008120번대를 `Knock.Back`에 썼다(사용자 프리뷰).
- **사냥개 `Knock.Down`**: F는 뒤로 날아갔다 일어나는 008100, B는 제자리에서 쓰러졌다 일어나는 008110이다(사용자 결정). 좌·우 피격은 F를 쓴다.
- **사냥개 `Groggy` 제외**: 후보 008600·008601은 바닥에서 뒹굴다 일어나는 동작으로, 불 지속 피해용으로 보였다. 사냥개에는 그로기가 없어도 된다는 사용자 판단으로 넣지 않았다. 은기사에는 강인도 붕괴 원본이 없다.
- **기상 무적 구간**: TAE에는 일어나기 시작하는 시각에 맞는 이벤트가 없어, 원본 머리 높이를 쟀다(사용자 선택). 규칙은 누운 자세의 최저점(+2cm 안) 마지막 프레임 뒤에 머리가 최저점보다 10cm 넘게 올라오는 첫 시각이다. 이 규칙은 흑기사의 기존 값(1.6·1.1초)을 다시 만들어 낸다. 은기사 F 1.57·B 1.03초, 사냥개 F 1.30·B 0.77초부터 이동 캔슬 시작까지 `Status.Invincible`을 붙인다.
- **사냥개 이동 캔슬**: 사냥개 TAE는 끝 캔슬 이벤트가 종류별로 조금씩 엇갈려 있어, 이동 캔슬과 같은 뜻인 Movement 이벤트(78)의 시작을 썼다. 기존 사냥개 `Light`의 0.45초는 흑기사 값을 복사한 값이지만 이번 범위에서 바꾸지 않았다.
- **에셋 생성 방식**: 몽타주와 가산 클립 복제는 에디터 Python(`AnimMontageFactory`, `duplicate_asset`)으로 만들고, 슬롯·Kata·Ability·세트 값은 MCP로 넣은 뒤 다시 읽어 확인했다. Gameplay Data의 부여 배열은 크기와 요소를 한 번에 바꾸면 MCP가 거절해, 기존 요소 교체와 추가를 두 번에 나눴다.

## 근거

- [피격 반응 사용법](../manual/Hit-Reaction.md#은기사사냥개-샘플): 두 캐릭터의 원본 배정, 캔슬·무적 시각, 부여 목록.
- [KataHitReactionAbility.h](../../Plugins/KataFramework/Source/KataFramework/Public/HitReaction/KataHitReactionAbility.h): 방향별 반응 표와 비어 있는 방향의 Front 대체.
- 원본 TAE와 머리 높이 측정은 Git에서 제외한 로컬 변환 도구로 읽기만 했다. 값은 로컬 인계 메모에 있다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 몽타주·Kata·Ability·세트 설정 | MCP로 값 읽기(에이전트) | 길이·Blend·슬롯·연결 클립·캔슬·무적 구간·방향 표·태그·부여 목록이 계획과 일치. Ability 값은 컴파일·저장 뒤에도 유지 | - |
| 클립 방향·용도 | 사용자 몽타주 프리뷰(2026-10-17) | 은기사 `Medium`·`Heavy` 정상, `Flinch` 좌우 반대(바로잡음), 사냥개 008120번대·008100·008110·008600번대 판정 | - |
| 반응 재생 | 사용자 PIE(2026-10-17) | 두 캐릭터 `Light`·`Medium`·`Heavy`·`Knock.Back`·`Knock.Down` 등급·방향, 은기사 `Flinch`, 다운 중 재반응 없음, 기상 무적 | AI가 반응 뒤 행동으로 돌아오는 경우(Idle NPC로 확인), PC가 은기사·사냥개에게 맞는 경우 |

PIE 확인 때는 런타임 스포너의 Idle·Ignore Damage NPC를 쓰고, 흑기사 공격 4개의 Impact를 `Light`·`Medium`·`Heavy`·`Knock.Back`(2회차 `Knock.Down`)으로, Poise 피해를 999로, Groggy 피해를 0으로 임시로 바꿨다. 확인 뒤 원래 값(`Knock.Back`·`Heavy`·없음·없음, Poise 20, Groggy 15)으로 되돌렸다.

## 남은 제한과 후속 작업

- 은기사에는 `Groggy` 반응이 없어, Groggy가 가득 차면 반응하지 않는다. 흑기사처럼 다른 캐릭터 클립을 리타게팅할지는 정하지 않았다.
- 사냥개 불 지속 피해 반응(008600·008601)은 지속 피해 기능이 생기면 다시 본다.
- `Medium`·`Knock.Down` 샘플 공격은 여전히 없다([#74](https://github.com/jaykop/Kata/issues/74)).

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#122](https://github.com/jaykop/Kata/issues/122) | 구현·사용자 확인 완료. 결과 댓글과 닫기는 사용자 확인 뒤 |
| [피격 반응 사용법](../manual/Hit-Reaction.md) | 확인 상태, "은기사·사냥개 샘플" 단락, `Medium`·`Knock.Back` 부여 문장 |
| [피격 반응 계획](../plan/Hit-Reaction-Plan.md) | 영향 없음(샘플 캐릭터 추가이며 설계 변경 없음) |
