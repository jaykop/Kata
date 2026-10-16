# KataFL_ 공용 함수 파일을 주제 폴더로 통일

작성: 2026-10-16  
갱신: 2026-10-16  
유형: 결정 기록  
대상: KataConditions·KataGraph·KataTargeting 의 `KataFL_*` 파일 위치, 코딩 규칙  
기준: 이 기록을 담은 커밋. 사용자 빌드 확인 전

## 배경과 결론

공용 함수 라이브러리(`KataFL_<주제>`) 파일이 두 가지 위치에 섞여 있었다. 4개 파일은 모듈의 `FunctionLibraries/` 폴더에, 6개 파일은 KataFramework·KataAI 처럼 그 주제 폴더에 있었다.

2026-10-16 사용자가 주제 폴더로 통일하기로 정했다. 기존 4개 파일도 같은 날 옮겼다. 코딩 규칙(`.claude/workflow/profile.md`)에도 이 위치를 적었다.

에셋·코드 폴더 정책 전체는 [#80](https://github.com/jaykop/Kata/issues/80)의 캐릭터 추가 절차 manual 과 함께 쓰기로 했다. 이 기록은 그 가운데 먼저 정한 함수 라이브러리 위치만 다룬다.

## 변경 내용

| 모듈 | 이전 위치 | 현재 위치 |
|---|---|---|
| KataConditions | `Public·Private/FunctionLibraries/KataFL_Condition` | `Public·Private/Conditions/KataFL_Condition` |
| KataGraph | `Public·Private/FunctionLibraries/KataFL_ActionGroup` | `Public·Private/ActionGroup/KataFL_ActionGroup` |
| KataTargeting | `Public·Private/FunctionLibraries/KataFL_Faction` | `Public·Private/Faction/KataFL_Faction` |
| KataTargeting | `Public·Private/FunctionLibraries/KataFL_CheatTarget` | `Public·Private/Cheats/KataFL_CheatTarget`(새 폴더) |

- 이 헤더를 포함하는 16개 파일의 include 경로를 바꿨다. 포함하는 모듈은 KataConditions, KataGraph, KataTargeting, KataFramework, KataAI, ProjectKataEditor 다.
- 바뀐 경로를 적은 manual 4개(Action-Group, Cheats, Conditions, Factions), devlog 3개와 plan 1개의 경로를 고쳤다.

## 주요 결정과 이유

- 이미 다수(6개)가 주제 폴더에 있었고, 함수 라이브러리가 가장 많은 KataFramework 도 주제 폴더를 썼다. `FunctionLibraries/` 를 쓰던 모듈은 라이브러리가 모듈마다 1~2개였다.
- 파일 이름의 `KataFL_` 접두사가 이미 공용 함수라는 것을 드러내므로, 폴더는 주제로 나눈다.
- 기존 4개를 남겨 두면 같은 모듈에 새 라이브러리를 만들 때 남은 파일을 따라 `FunctionLibraries/` 를 다시 쓰게 된다. 처음에는 바꿀 이유가 없다고 보고 새 파일에만 적용하는 안을 냈다. 사용자가 규칙을 정하면 남은 4개가 규칙 위반이 된다고 보고, 지금 옮기기로 했다.
- 맞는 주제 폴더가 없으면 주제 이름으로 새로 만든다. KataTargeting 의 치트 대상 함수는 `Cheats/` 를 새로 만들었다(KataFramework·KataAI 와 같은 이름).

## 근거

- 판단 근거와 위치 집계는 TD 고문 설계 검토(TD-D-007)에서 나왔다. 그 리포트는 공개 저장소에 두지 않는 로컬 기록이다.
- [코딩 규칙](../../.claude/workflow/profile.md): `KataFL` 네임스페이스와 파일 위치.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 이동과 include 경로 | 에이전트, `FunctionLibraries` 문자열 전수 검색 | 소스·manual·devlog·plan 에 옛 경로 0건 | 빌드 |
| 에셋·블루프린트 영향 | 소스 대조 | 클래스 이름과 모듈이 그대로라 리플렉션 경로(`/Script/<모듈>.<클래스>`)가 바뀌지 않는다 | 에디터 로드 |
| Editor·Game 빌드 | 미실시 | 사용자 확인 전 | 전부 |

## 남은 제한과 후속 작업

- 사용자 Editor·Game 빌드 확인이 필요하다.
- 폴더·명명 정책 전체는 #80 에서 쓴다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [조건 사용법](../manual/Conditions.md), [팩션 사용법](../manual/Factions.md), [Action Group 사용법](../manual/Action-Group.md), [치트 사용법](../manual/Cheats.md) | include 경로와 근거 링크 |
| [Action Group 기록](2026-10-05-Action-Group.md), [치트 매니저 기록](2026-10-11-Cheat-Manager.md), [함수 라이브러리 진단](Function-Library-and-Blueprint-Task-Diagnosis.md) | 근거 링크 경로만 갱신. 당시 서술은 그대로 |
| [Action Group 계획](../plan/Action-Group-Plan.md) | 파일 표의 경로 |
| [문서 목록](../README.md) | 이 기록 링크 |
