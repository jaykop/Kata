# GAS Inspector 화면 개편 설계

작성: 2026-10-05  
갱신: 2026-10-06  
연결 이슈: 없음. 사용자 요청으로 이슈를 게시하지 않는다.  
현재 상태 근거: [사용법](../manual/GAS-Inspector.md) · [구현 기록](../devlog/2026-10-05-GAS-Inspector.md)  
대체 관계: 같은 문서의 초기 도구 설계를 계승하며, 화면 구성은 아래 개편안으로 대체한다.

## 목적과 현재 상태

초기 도구는 다섯 데이터 영역에 공통 열과 버튼을 사용했다. Attributes의 Base/Current 동시 표시, 모든 화면의 State/Kind·Source/Class, 중복된 대상·월드·상태 안내가 일반 조회에 비해 과하다. 초기 구현은 2026-10-05 사용자가 빌드 성공과 테스트 완료를 보고했다. 아래는 2026-10-06 사용자 피드백으로 정한 화면 개편 계약이다. 실제 사용법은 manual을 따른다.

하나의 GAS Inspector 도킹 창 안에서 Tags·Attributes·Abilities·Effects를 드롭다운으로 전환한다. 각 항목은 독립 화면을 유지한다. 같은 ASC를 관찰하는 네 화면은 대상과 수집 시점을 공유하되, 표·도구 모음·상세 내용은 각 데이터에 맞게 구성한다.

## 범위

- 포함: 탭별 전용 화면, Attribute Current 단독 표시, 필요한 열과 버튼만 노출, 상태 안내 간소화, 개인 화면 설정 분리.
- 유지: Editor 전용 단일 모듈, 월드·ASC 선택, 읽기 전용 값 snapshot, Freeze/Refresh, 선택 Ability의 Trigger 설정 조회, 에셋 탐색.
- 제외: 여러 독립 도킹 창, 게임 상태 변경, Ability 강제 실행, 이벤트 이력, 원격 연결·멀티플레이, GAS 계산 재구현.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 |
|---|---|---|
| 창 구성 | 확정 | 사용자 승인: 한 창 안의 전용 화면. 후속 요청으로 탭 버튼을 네 항목 드롭다운으로 변경. 독립 창마다 대상을 맞추는 부담을 피함 |
| Attribute | 확정 | 사용자 요청: Current만 표시. 표·상세·tooltip에서 Base를 반복 노출하지 않음 |
| 열과 버튼 | 확정 | 공통 다목적 열 대신 화면별 의미가 있는 열. 계층 버튼은 Abilities·Active Effects에만 표시 |
| 대상과 Freeze | 확정 | 네 화면이 같은 세션과 snapshot 공유 |
| 상태 요약 | 확정 | Live/Frozen을 간단히 표시. 전체 경로·시각·Owner/Avatar는 tooltip, 정상 준비 안내는 숨김 |
| Trigger 검색 제거 | 확정 | 사용자 승인: 별도 검색 탭·인덱스 제거. 선택 Ability의 기본·동적 Trigger는 상세에 유지 |
| 세부 레이아웃 | 구현 제안 | 화면 드롭다운과 아래 표·선택 상세 영역. 간격·열 폭은 실제 사용 확인 후 조정 |

## 공통 화면과 세션 계약

창 상단에는 네 항목의 화면 드롭다운을 표시한다. 각 화면에서는 월드·ASC 선택과 Use selection, Refresh, Freeze/Resume을 표시한다. 항목 드롭다운은 약 140 Slate 단위의 짧은 폭으로 Refresh·Freeze와 같은 줄에 둔다. Freeze 오른쪽에 Interval 숫자 입력을 둔다. 기본 0.2초, 범위 0.1~1초. 별도 Settings 메뉴는 없으며 상태 안내는 아래 줄로 분리한다.

- Use selection: 에디터에서 선택한 Actor에 연결된 ASC를 현재 월드에서 찾는다. 계속 추적하는 토글이 아니라 한 번 선택하는 버튼이다.
- Refresh: 목록과 현재 snapshot을 즉시 갱신한다. Frozen 상태는 유지한다.
- Freeze/Resume: 화면 데이터의 자동 갱신만 멈추거나 재개한다. 게임 실행은 멈추지 않는다.
- 탭 전환: 선택 월드·ASC·Freeze·수집 시점은 보존한다. 검색어·정렬·행 선택·펼침 상태는 탭별로 보존한다.

각 화면은 자체 검색 입력과 필요한 필터만 둔다. 정렬은 기존 표시 문자열 정렬 계약을 유지하며 숫자 정렬은 이번 개편으로 추가하지 않는다.

## 탭별 표와 도구

| 탭 | 기본 열 | 전용 도구 | 선택 상세 |
|---|---|---|---|
| Tags | Tag, Kind, Count | 검색, Copy tag | 정의 출처·주석 |
| Attributes | Attribute, Current | 검색, Copy name | Attribute Set·속성 경로 |
| Abilities | Ability, State, Execution | 검색, Active only, Observed block only, Expand all/Collapse all, Open asset, Copy name | level/input/count, 차단 근거, 클래스·에셋 경로, 기본·동적 Trigger, 인스턴스·Task |
| Effects | Effect, State, Timing/Stacks | 검색, Expand all/Collapse all, Open asset, Copy name | level/period, instigator/source/causer, tags, evaluated modifier |

State는 Abilities의 Active/Idle·관측 차단과 Effects의 Applied/Inhibited처럼 의미가 있는 화면에서만 표시한다. Source/Class는 기본 표에서 제거하고 상세·tooltip에 둔다. Attributes의 unavailable 상태는 Current 셀과 진단 안내로 표현한다.

Tags의 차단 행은 Count에 owned count를 재사용하지 않고 해당 없음으로 표시한다. Active Effects의 Instant·종료 효과는 조회 대상이 아니다. Ability의 Idle은 실행 가능의 보증이 아니며 CanActivateAbility를 자동 호출하지 않는다.

Expand/Collapse와 expander는 실제 하위 행을 가진 Abilities·Effects에서만 제공한다. 나머지 화면은 평면 목록을 사용한다. Open asset는 에셋이 있는 화면에서만 제공하고, native 항목처럼 링크가 없으면 비활성화한다.

## 상태·상세 정보의 노출

| 정보 | 노출 방식 |
|---|---|
| Live/Frozen | 런타임 탭의 짧은 상태 표시 |
| 월드·현재 대상 | 상단 선택기에 표시하고 요약에 반복하지 않음 |
| snapshot 시각·당시 대상·Owner/Avatar 경로 | 상태 tooltip. Frozen은 현재 선택과 다른 당시 대상임을 짧게 표시 |
| 행 수 | 검색 결과 주변의 작은 개수 표시 |
| ActorInfo ready | 정상 상태에서 숨김 |
| ActorInfo 미준비·대상 소멸·조회 불가·에셋 열기 실패 | 필요한 시점에 짧은 진단 표시 |
| 활성화 가능 여부 미평가 | Abilities 도움말 또는 tooltip에 한 번 설명 |

대상이 사라진 Live 화면은 비우고 재선택을 안내한다. Frozen의 과거 값은 보존하되 현재 대상의 Live 값으로 오인하지 않도록 표시한다. 상세 영역은 선택한 행의 관련 정보만 보여 주며 빈 State·Tags·Source 줄을 반복하지 않는다.

## 구조와 수명

SKataGASInspector는 탭 전환과 공통 세션을 소유하는 호스트로 둔다. 탭 전환 시 전용 Slate 화면과 열 구성을 재구성하고 탭별 상태를 복원한다. 세션·수집기·metadata reader는 기존 책임을 유지하고, 화면 분리를 위해 객체 수명이나 엔진 GAS 조회 계약을 바꾸지 않는다.

UI는 값 snapshot과 soft asset path만 사용한다. World/ASC는 weak 참조이며 탭별로 별도 ASC 수집기를 만들지 않는다. Freeze 값은 태그 count·remaining·tooltip까지 같은 수집 시점을 사용한다. 창이 foreground가 아니면 세션 갱신을 쉰다. Trigger 검색 이벤트 구독과 CDO 인덱스 서비스는 제거한다.

기존 전역 열 숨김·정렬 설정은 새 화면에 그대로 적용하지 않는다. 표별 설정 키를 분리하고 기존 데이터와 맞지 않는 설정은 새 기본값을 사용한다. Interval은 개인 설정에서 복원하고 입력값을 0.1~1초로 제한한다. 게임 에셋·리플렉션 타입·모듈 경계는 변경하지 않는다.

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| UI-1 | 높음 | 호스트와 탭별 화면 분리 | 창 구성 확정 | 한 창에서 전용 화면 전환, 공통 대상·Freeze 유지 |
| UI-2 | 높음 | 데이터별 열·평면/계층 목록 구성 | UI-1 | Current 단독 표시, State/Source 불필요 노출 제거 |
| UI-3 | 보통 | 전용 버튼과 상세 정리 | UI-2 | 계층·에셋·런타임 버튼이 필요한 화면에만 표시 |
| UI-4 | 보통 | 상태 안내와 개인 설정 분리 | UI-1 | 정상 요약 간소화, Frozen 과거 대상 식별, 탭별 설정 보존 |
| UI-5 | 보통 | 사용법·변경 이유 반영 | 구현 계약 결정 | manual에 실제 구현, devlog에 변경 이유와 실제 확인 근거 |

## 사용자 확인 항목

구현과 실행 확인은 구분한다. 빌드·테스트·별도 검사는 사용자가 담당한다.

- 탭을 바꿔도 월드·ASC·Freeze가 유지되고 각 탭 검색·정렬이 섞이지 않는지.
- Attributes에 Current만 보이고 동일 이름의 Attribute Set는 상세에서 구분되는지.
- 평면 목록에 펼침 버튼이 없고 Ability·Effect 하위 행은 정상 동작하는지.
- Frozen·PIE 종료·대상 파괴 후 당시 데이터와 현재 선택을 구분할 수 있는지.
- 기존 Tags·Task·GE modifier·선택 Ability의 Trigger 상세와 에셋 열기가 유지되는지.

## 완료 시 갱신할 문서

- [사용법](../manual/GAS-Inspector.md): 구현 후 탭별 UI와 상태 안내를 반영. 현재 화면 동작과 사용자 확인 근거를 구분한다.
- [구현 기록](../devlog/2026-10-05-GAS-Inspector.md): 기존 구현·사용자 확인 기록을 보존하고 개편 결정과 실제 결과를 연결한다.
- [문서 목록](../README.md): 개편 설계 설명 반영.
- 이슈 미게시 결정은 유지한다. 이번 개편은 사용자 구현 요청에 따라 소스에 반영하며 빌드·테스트·커밋은 별도 요청을 따른다.
