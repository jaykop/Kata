# 피격 반응 Medium·Heavy 샘플과 제자리 반응의 밀림

작성: 2026-10-17  
갱신: 2026-10-17  
유형: 구현 기록 / 결정 기록 / 진단  
대상: 샘플 흑기사 피격 반응(`Medium`·`Heavy`), Impact 태그·Kata Combat 설정, 넉백 기능 분리  
기준: GitHub ed62bcd(태그·설정)와 이 기록을 담은 커밋. 샘플 에셋은 Diversion에서만 추적한다

## 배경과 결론

피격 반응 계획 HR8의 앞부분([#70](https://github.com/jaykop/Kata/issues/70))으로, 흑기사 샘플에 `Medium` 등급을 추가하고 `Medium`·`Heavy` 4방향 반응을 만들었다.
사용자 PIE에서 방향별 반응과 끝부분 이동 캔슬을 확인했다.

확인 중 "Enable Root Motion을 켰는데 밀려나지 않는다"는 문제가 나왔다. 원본 클립에 루트 이동이 없는 것이 원인이었고, 참고 원본은 이런 반응의 밀림을 애니메이션이 아니라 공격 쪽 넉백으로 처리한다는 것을 확인했다.
밀어내기 기능은 반응 Kata의 새 태스크로 만들기로 하고 [#120](https://github.com/jaykop/Kata/issues/120)으로 분리했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| Impact 등급 | `Light`·`Heavy`·`Knock.Back`·`Knock.Down` | `Impact.Medium`과 `Status.HitReaction.Medium`·`GameplayCue.Hit.Medium`·`Event.HitReaction.Medium`(Native) 추가. Impact Responses에 `Light`와 `Heavy` 사이 행 추가. 때린 쪽 Cue는 정하지 않아 비워 두었다 |
| 반응 샘플 | `Light`·`Flinch`·`Groggy`만 있음 | 몽타주·반응 Kata 각 8개, 반응 GA 2개. `Flinch`가 `Medium`·`Heavy` 중에도 막히게 했다. 흑기사와 PC 세트에만 부여했다 |
| 샘플 공격 | 모두 기본 등급(`Impact.Light`) | `KA_BlackKnight_Attack01` `Impact.Medium`, `KA_BlackKnight_Attack02` `Impact.Heavy` |
| 반응 중 밀림 | Enable Root Motion을 켜도 몸만 밀렸다가 제자리로 돌아옴 | 원본 클립에 루트 이동이 없음을 확인. 변환으로 고칠 데이터가 없어 [#120](https://github.com/jaykop/Kata/issues/120)의 런타임 태스크로 처리한다 |
| 방향 | 원본 클립 이름에 `Candidate`(방향 미확정)가 남음 | 사용자가 몽타주 프리뷰에서 F·B·L·R 순서가 맞음을 확인했다 |

## 주요 결정과 이유

- **`Medium` 추가**: 계획은 "샘플 8000번대가 세 묶음으로 구분되면 `Medium`을 추가한다"고 정해 두었다. 흑기사 제자리 반응은 0.67·1.67·2.33초 세 묶음이라 조건에 맞는다. 배정은 `Light` 008000~008003, `Medium` 008010~008013, `Heavy` 008020~008023이다(2026-10-11 사용자 결정).
- **이동 캔슬 시각**: 원본 TAE의 끝 캔슬 시작을 따른다. `Medium` 0.80초, `Heavy` 1.17초다. 네 방향에 같은 값을 쓴다.
- **방향 확인 근거**: 변환 기록은 머리 흔들림으로 잰 방향을 `Medium` B/?/?/L, `Heavy` B/B/L/?로 남겼다. 사용자 프리뷰 결과는 원본 번호 순서(F·B·L·R)와 같았다. 길이가 긴 반응은 몸이 여러 번 흔들려서, 머리 흔들림의 최대·평균 방향으로는 판단하지 못한 것으로 보인다.
- **밀림을 애니메이션으로 고치지 않은 이유**: 원본 HKX를 변환기와 같은 방식으로 다시 읽어 첫 프레임 대비 수평 변위를 쟀다. 별도 이동 정보(reference frame)가 없고, 발은 최대 45~81cm, 머리는 최대 25~57cm 움직였다가 끝 프레임에서 모두 0으로 돌아온다. 마지막 프레임에서 튀지 않고 서서히 돌아오므로 읽기 오류도 아니다. 루트로 옮길 순이동이 없다.
- **참고 원본의 처리 방식**: 같은 번호의 사냥개 반응은 클립에 루트 이동(82~108cm)이 있고, TAE의 넉백 비율 이벤트(`SetKnockbackPercent`)가 0%다. 흑기사 `Light`·`Medium`·`Heavy`에는 이 이벤트가 없다. 다운·가드 계열에는 0%·20%·40%가 있다. 공격 데이터에는 넉백 거리 필드가 있고, 플레이어 공격은 중앙값 1.0, 흑기사 공격은 1.0~3.8이다(단위는 m로 추정). 그래서 흑기사 경직의 밀림은 "공격의 넉백 거리 × 반응의 비율"로 게임이 처리한다고 판단했다. 이벤트가 없을 때의 비율은 공개 자료에 없어 100%로 추정만 했다.
- **넉백을 #70에서 떼어 낸 이유**: 반응 애니메이션과 GA를 만드는 #70의 범위 밖의 새 기능이다. 캐릭터 이동 구현에 의존하므로 KataFramework 태스크로 둔다. 거리는 반응 쪽에 먼저 두고, 공격별 배율은 [#74](https://github.com/jaykop/Kata/issues/74) 플레이 메모에서 필요가 확인될 때 더한다(2026-10-17 사용자 결정).

## 근거

- [피격 반응 계획](../plan/Hit-Reaction-Plan.md): Impact 등급 행의 `Medium` 추가 조건.
- [피격 반응 사용법](../manual/Hit-Reaction.md#확인-상태와-근거): 샘플 에셋 표와 확인 상태.
- [Gameplay Tags 사용법](../manual/Gameplay-Tags.md): 샘플 Impact 태그 목록.
- 원본 클립·TAE·공격 데이터 측정은 Git에서 제외한 로컬 변환 도구와 설치본으로 읽기만 했다. 값과 절차는 로컬 인계 메모에 있다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 태그·설정 | 사용자 Editor 빌드(2026-10-16) | 빌드 통과, 생성된 KataTags에 Medium 태그 확인 | - |
| 몽타주·Kata·GA 설정 | MCP로 값 읽기(에이전트) | 길이·Blend·슬롯·연결 애니·캔슬 구간·태그·부여 목록이 계획과 일치 | - |
| 방향 | 사용자 몽타주 프리뷰(2026-10-17) | 8개 모두 F·B·L·R 순서 일치 | - |
| 반응 동작 | 사용자 PIE(2026-10-17) | `Medium`·`Heavy` 방향별 반응, 끝부분 이동 캔슬 | PC가 `Medium`·`Heavy`에 맞는 경우, `Heavy` Cue 연출 |

PIE 확인 때는 Poise를 바로 무너뜨리려고 두 공격의 Poise 피해를 임시로 올렸다가 커밋 전에 원래 값(20)으로 되돌렸다.

## 남은 제한과 후속 작업

- 은기사·사냥개에는 `Medium`·`Heavy` 반응이 없어서, `Attack01`·`Attack02`에 맞아 Poise가 무너져도 반응하지 않는다. 대체 등급 규칙은 없다.
- 반응 중 실제로 밀어내는 기능: [#120](https://github.com/jaykop/Kata/issues/120).
- 때린 쪽 `Medium` Cue(`GameplayCue.Attack.Medium`)를 둘지는 정하지 않았다.
- `Knock.Back`·`Knock.Down`과 기상 무적: [#71](https://github.com/jaykop/Kata/issues/71).

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#70](https://github.com/jaykop/Kata/issues/70) | 구현·사용자 확인 완료. 결과 댓글과 닫기는 사용자 확인 뒤 |
| [피격 반응 사용법](../manual/Hit-Reaction.md) | 샘플 표에 `Medium`·`Heavy` 행, 설정·부여·샘플 공격 설명, 문제 해결 표 갱신 |
| [Gameplay Tags 사용법](../manual/Gameplay-Tags.md) | 샘플 Impact 목록에 `Impact.Medium` 추가(ed62bcd) |
| [피격 반응 계획](../plan/Hit-Reaction-Plan.md) | Impact 등급 행에 `Medium` 추가 결과와 이 기록 링크 |
