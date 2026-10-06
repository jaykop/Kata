# GAS Inspector 탭별 화면과 조회 범위 정리

작성: 2026-10-06  
갱신: 2026-10-06  
유형: 변경·결정 기록  
대상: KataGASInspector 0.2.0  
기준: 현재 작업 트리. 2026-10-06 사용자 최종 화면 동작 확인.

## 배경과 결론

사용자가 공통 열과 상태 안내가 과하다고 지적했다. 한 창에서 같은 ASC와 Freeze를 공유하면서 탭별 필요한 열·버튼만 제공하기로 했다. 추가 논의에서 별도 Ability Trigger 에셋 검색은 기본 상태 조회에 필요하지 않아 제거하기로 확정했다.

## 변경 내용과 이유

- 네 탭 전용 열 구성, Attributes Current 단독 표시. Source/Class는 상세로 이동했다.
- Abilities·Effects에만 계층 버튼·화살표와 에셋 열기를 표시한다. 기본·동적 Trigger는 Ability 선택 상세에 유지한다.
- 세 줄의 정상 상태 요약을 Live/Frozen과 개수로 줄였다. 수집 시각·대상·Owner/Avatar는 tooltip에 보존하고 미준비·소멸·Frozen 과거 대상은 표시한다.
- 탭별 검색·정렬·선택·펼침 상태를 분리했다. 세션과 snapshot은 공유하므로 대상과 Freeze를 다시 맞출 필요가 없다.
- Trigger 검색 서비스·레지스트리 이벤트 구독·AssetRegistry 모듈 의존성을 제거했다. 기존 Trigger 검색 개인 설정은 더 이상 읽지 않는다.

## 영향과 근거

게임 에셋이나 런타임 플러그인은 변경하지 않는다. 초기 구현과 당시 확인 기록은 [이전 기록](2026-10-05-GAS-Inspector.md)에 보존한다. 현재 사용법은 [manual](../manual/GAS-Inspector.md), 선택 이유와 화면 계약은 [설계](../plan/GAS-Inspector-Plan.md)를 따른다.

- [화면](../../Plugins/KataGASInspector/Source/KataGASInspectorEditor/Private/SKataGASInspector.cpp): 탭별 화면 구성·상태 복원.
- [수집기](../../Plugins/KataGASInspector/Source/KataGASInspectorEditor/Private/KataGASSnapshotCollector.cpp): Current·Trigger 상세·snapshot 상태 값.

## 확인 범위와 결과

소스와 사용 문서를 변경했다. 에이전트는 빌드·테스트·lint·별도 코드 검사를 실행하지 않았다. 2026-10-05 사용자 확인은 초기 버전의 결과이며 이번 개편에는 적용하지 않는다. 탭 전환 상태 보존, Frozen·대상 소멸, Ability Task·GE modifier 계층과 에셋 열기는 사용자가 확인한다.

## 연관 문서 반영

manual·plan·README와 AGENTS의 도구 설명을 갱신했다. 이슈는 기존 사용자 결정에 따라 게시하지 않았다.

## 화면 선택 후속 변경

화면 선택은 Tags / Attributes / Abilities / Effects 네 항목의 드롭다운으로 제공한다. 태그와 Attribute는 각각 독립 화면이며 검색·정렬·선택 상태를 따로 유지한다. 빌드·실행은 아직 확인하지 않았다.

## 상단 도구 모음 간소화

사용자 요청으로 화면 드롭다운을 140 Slate 단위 폭으로 줄여 Refresh·Freeze와 같은 줄에 배치했다. Settings UI와 Interval 개인 설정 읽기·쓰기를 제거하고 자동 갱신을 0.2초로 고정했다. 빌드·실행은 확인하지 않았다.

## 갱신 간격 입력 복원

사용자 후속 요청으로 상태 안내를 도구 모음 아래 줄로 옮기고 Freeze 오른쪽에 Interval 숫자 입력을 복원했다. 별도 Settings 메뉴는 두지 않는다. 기본 0.2초·범위 0.1~1초이며 개인 설정에 저장한다. 앞선 고정 간격 결정은 이 요청으로 대체한다. 빌드·실행은 직접 확인하지 않았다.

## 사용자 확인

2026-10-06 사용자가 최종 배치와 Interval 입력 변경 후 정상 동작을 확인하고 커밋을 요청했다. 개별 회귀 항목·성능 측정의 상세 결과는 보고되지 않았다. 에이전트는 빌드·테스트·별도 검사를 직접 실행하지 않았다.
