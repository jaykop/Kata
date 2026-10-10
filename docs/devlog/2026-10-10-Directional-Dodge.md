# 방향별 회피와 그래프 편집 스크립팅

작성: 2026-10-10  
갱신: 2026-10-10  
유형: 구현 기록 / 결정 기록  
대상: KataTargeting 이동 방향 조건·회전, KataGraphEditor 스크립팅 함수, 흑기사 회피 샘플  
기준: [#45](https://github.com/jaykop/Kata/issues/45) 2026-10-10 작업 트리. 사용자 빌드·PIE 확인 완료.

## 배경과 결론

#19 IN-5 확인에 필요한 회피 액션이 없었다. 흑기사에는 회피 애니메이션이 없어 DS3 플레이어 구르기를 리타게팅해 쓰고,
입력 방향(락온 중 포함)에 맞는 구르기를 고르는 회피를 만들었다. 그래프 노드·엣지는 에이전트가 에디터 Python으로 만들 수 있게
그래프 편집기와 같은 경로를 쓰는 스크립팅 함수를 추가했다. 사용법은 [타게팅 사용법](../manual/Targeting.md#입력-방향별-회피)과
[에디터 사용법](../manual/Editor-Usage.md#스크립트로-그래프-편집하기)에 있다.

## 변경 내용

| 파일 | 내용 | 신규/수정 |
|---|---|---|
| `KataTargeting/Public/Targeting/KataTargetingComponent.h` | BlueprintNativeEvent `ResolveMoveDirection`, `IsLockOnActive`(기본 false) | 수정 |
| `KataTargeting/Public/Targeting/KataPlayerTargetingComponent.h` | 위 둘 재정의(이동 입력 방향, 락온 지점 유무) | 수정 |
| `KataTargeting/Public/Conditions/KataCondition_MoveDirection.h` | 이동 입력을 정면 기준 4방향으로 나눠 판정하는 조건 | 신규 |
| `KataTargeting/Public/Commands/KataCommand_FaceMoveDirection.h` | 이동 입력 방향으로 즉시 도는 Command | 신규 |
| `KataTargeting/Public/Tasks/KataTask_RotateToFacing.h` | Direction Source(Facing / Move Input), Skip When Locked On | 수정 |
| `KataTargeting/KataTargeting.Build.cs` | `KataConditions` Public 의존 | 수정 |
| `KataGraphEditor/Private/KataGraphEditorScriptLibrary.h` | 저작 노드 조회·추가·연결·삭제, 내장 원본 조회·이름, 재구성 | 신규 |

샘플 에셋: 몽타주 `AM_BlackKnight_Roll_F/B/L/R`·`AM_BlackKnight_Backstep`, 액션 `KA_BlackKnight_Dodge_F/B/L/R`·`KA_BlackKnight_Backstep`,
공격 4종의 `Window.Transition.Dodge` 창, `KG_BlackKnight_GreatSword`의 내장 SubGraph "Dodge".

## 주요 결정과 이유

| 결정 | 대안 | 채택 이유 |
|---|---|---|
| 방향마다 별도 액션 | 액션 하나에서 방향별 몽타주를 고르는 태스크 | 방향마다 전이·캔슬 창, 길이, 이후 무적 구간을 따로 맞춘다. 단일 액션은 타임라인을 공유해 백스텝(1.0초)이 구르기(1.77초)에 맞춘 창보다 먼저 끝났다 |
| 선택은 그래프의 Conduit + 엣지 조건 | 이전 프로젝트 방식의 회피 GA가 액션을 골라 실행 GA를 호출 | 기존 그래프 구조에 바로 맞고 분기가 그래프에 보인다. 범용 실행 GA 경유 여부는 [#46](https://github.com/jaykop/Kata/issues/46)에서 따로 정한다 |
| 락온 중 몸 기준 4방향, 비락온은 돌아선 뒤 앞 회피 | 항상 방향별 구르기 | 몸 방향을 유지해야 하는 락온 중에만 뒤·좌·우가 필요하다(DS3와 같은 방식) |
| 무입력은 백스텝 | 앞 회피 | 사용자 결정 |
| 연속 회피는 회피 노드 → Conduit 엣지 | 회피 노드 두 개 교대, 노드 자기 연결 허용 | 스키마가 같은 노드 연결을 막는다. Alias는 해석 결과가 현재 노드면 건너뛰지만(`bSkipSelfTarget`), 현재 노드에서 나가는 엣지는 그 검사를 하지 않아 같은 방향 재진입이 된다 |
| 비락온 회전은 0.15초 Rotate To Facing | Face Move Direction 즉시 회전 | 연속 회피 중 방향이 바뀌면 즉시 회전은 블렌드 중이던 포즈째 한 프레임에 돌아 방향·위치가 튀었다(사용자 확인: 방향 전환 때만 발생) |
| 회피 분기를 내장 SubGraph로 | 외장 그래프 에셋 | 지금 재사용할 그래프가 없다. 내장은 다른 그래프에서 참조할 수 없으므로 무기 세트 그래프가 생기면 외장으로 다시 만든다 |
| 그래프 편집은 스키마 경로를 감싼 스크립팅 함수 | 런타임 노드를 직접 수정 | 실행 데이터는 저작 그래프에서 다시 만들어지므로 직접 수정은 다음 저장 때 사라진다 |

## 진단 기록

- 빌드 LNK2019(`UKataCondition`): KataTargeting이 KataConditions를 직접 의존하지 않았다. KataRuntime의 Public 의존으로 헤더는 찾았지만 링크 대상에는 들어가지 않았다.
- DS3 플레이어 구르기를 리타게팅하자 상체가 꼿꼿한 채 하체만 굴렀다. 원본 구르기 회전이 `Spine`의 부모 `RootRotXZ`(와 `RootRotY`)에 실려 있고,
  IK Retargeter FK 체인 연산은 원본 체인을 대상 체인 부모의 현재 방향에 맞춰 다시 놓은 뒤 회전을 옮겨 대응 뼈가 없는 흑기사에서 그 회전이 버려졌다.
  변환 단계에서 두 뼈의 회전을 `Spine` 로컬로 옮겨(월드 자세 유지) 해결했다. 변환 도구는 로컬 전용이다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 코드 | 사용자 Editor 빌드(2026-10-10) | 성공 | Game 타깃 |
| 샘플 그래프·액션 | 에이전트가 저장된 실행 데이터를 에디터 도구로 대조 | 노드·엣지·조건 매핑 일치 | — |
| 회피 동작 | 사용자 PIE(2026-10-10) | 락온 4방향, 비락온 회전 후 앞 회피, 무입력 백스텝, 공격 중 캔슬, 연속 회피, 방향 전환 시 튐 없음 | — |

## 남은 제한과 후속 작업

- 대각선 회피(원본 방향 불확실), 무적 구간, 회피 비용은 범위 밖이다.
- AI의 기반 타게팅 컴포넌트는 이동 방향이 없어 이 구성으로는 항상 백스텝이다.
- 스크립트로 새로 만든 그래프 에셋은 그래프 에디터를 한 번 열어야 저작 그래프가 생긴다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#45](https://github.com/jaykop/Kata/issues/45) | 결과 댓글과 닫기는 사용자 확인 후 게시 |
| [타게팅 사용법](../manual/Targeting.md) | 새 API·조건·Command·회전 옵션, 입력 방향별 회피 항목 |
| [에디터 사용법](../manual/Editor-Usage.md) | 스크립트로 그래프 편집하기 항목 |
| [입력 사용법](../manual/Input.md) | 회피 캔슬 안내와 확인 상태 |
