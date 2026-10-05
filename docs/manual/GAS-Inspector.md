# GAS Inspector 사용법

갱신: 2026-10-05  
대상: KataGASInspector / KataGASInspectorEditor  
적용 기준: UE 5.8, 플러그인 0.1.0  
확인 상태: 2026-10-05 사용자 빌드 성공 및 테스트 완료 보고.

## 목적과 준비

에디터에서 GAS의 상태를 읽고 Ability 에셋의 Trigger 설정을 검색한다. ASC 파생 클래스를 교체하거나 게임에 별도 디버그 컴포넌트를 붙일 필요가 없다. GameplayAbilities와 KataGASInspector가 활성화돼 있어야 한다. 샘플 프로젝트는 Editor 타깃에서 도구를 활성화한다.

Editor 배치 객체와 PIE/SIE 월드를 관찰한다. ActorInfo 초기화 전에는 읽을 수 있는 컴포넌트 데이터만 표시하며 미초기화 상태를 안내한다. Game 타깃이나 다른 프로세스에서 실행 중인 게임에는 접속하지 않는다.

## 사용 순서

1. 에디터의 **Window → GAS Inspector**를 열거나 콘솔에 `Kata.GASInspector.Open`을 입력한다.
2. `Auto world`는 PIE를 우선하고 없으면 Editor를 선택한다. 월드 메뉴에서 직접 고르면 자동 선택을 끈다.
3. ASC 메뉴에서 액터·컴포넌트·클래스 이름을 검색해 대상을 고른다. 첫 월드의 첫 ASC는 초기 편의 선택이며, 대상 파괴 후 다른 ASC로 자동 교체하지 않는다.
4. `Use selection`은 **현재 선택 월드**의 ASC 가운데 에디터 선택 액터와 Owner/Avatar가 연결된 항목을 고른다. 여러 ASC가 연결돼 있으면 첫 항목을 쓰므로 ASC 메뉴로 구분한다.
5. 페이지 메뉴에서 `Tags`, `Attributes`, `Abilities`, `Active Effects`를 고른다. 행을 선택하면 아래에 세부 정보가 표시된다.
6. `Freeze`로 시점을 고정하고 `Refresh`로 한 번 갱신하거나 `Resume`으로 자동 갱신을 재개한다.

## 주요 설정과 실행 계약

| UI 항목 | 의미·입력 | 기본값·실패 시 동작 |
|---|---|---|
| Auto world | PIE 우선 자동 선택 | 기본 켜짐. 수동 선택 월드가 사라지면 대상 재선택 필요 |
| ASC 메뉴 | 현재 월드의 ASC 목록과 검색 | 동명 액터도 컴포넌트별 별도 항목. tooltip에서 객체 경로 확인 |
| Interval (s) | live 갱신 주기 | 0.2초, 0.1~1초 |
| Refresh | 목록을 다시 찾고 snapshot 한 번 수집 | Freeze 유지. 대상 없으면 안내, frozen 과거 데이터는 유지 |
| Freeze / Resume | 데이터·tooltip·remaining을 고정/재개 | 고정 화면에 수집 시각·당시 대상 표시 |
| 공통 Filter | 이름·state·value·tag·source·detail 부분 문자열 검색 | 대소문자 구분 없음. 자식이 일치하면 부모 표시 |
| Active only | 활성 Spec만 표시 | Abilities에서 사용 |
| Observed block only | input/tag 차단을 관측한 Spec만 표시 | Active 필터와 함께 켜면 두 조건 모두 만족해야 함 |
| 열 제목 | 클릭해 표시 문자열 정렬 | 동일 값은 고유 key로 정렬. 숫자의 크기 정렬이 아닌 문자열 정렬 |
| 열 메뉴 | 열 표시/숨김 | 개인 Editor ini에 저장 |
| Expand all / Collapse all | 현재 검색 결과의 계층 펼침/접기 | 개별 선택·확장은 live 행의 key가 유지되는 동안 보존 |
| Open asset / 더블클릭 | Blueprint Ability/GE 원본 에셋 열기 | native class에는 source 에셋 링크 없음. 삭제·로드 실패 안내 |
| Copy name | 선택 행의 이름 그대로 복사 | Tags에서 정확한 태그 이름 복사 |

주기·페이지·정렬·필터 토글·숨긴 열·Trigger 범위는 개인 `GEditorPerProjectIni`에 저장한다. 대상 UObject·freeze snapshot은 저장하지 않는다. 탭 도킹 위치는 에디터의 탭 관리가 담당한다.

탭이 foreground가 아니면 수집을 쉬며, `Ability Triggers` 페이지에서는 GAS live 수집을 쉬고 검색 작업만 진행한다. 페이지를 돌아오면 현재 월드·대상을 다시 확인한다.

## 표시 데이터의 의미

| 페이지 | 표시하는 값 | 해석상의 제한 |
|---|---|---|
| Tags | Owned tag의 count, Ability blocked tags, 정의 출처·주석 | blocked는 count 대신 차단 여부. owned count는 계층 합산을 포함. 모든 제공자 추적은 아님 |
| Attributes | Set 클래스·속성, base/current | 지원 속성 API로 열거. 읽기 실패와 음수 값을 구분 |
| Abilities | Spec active/count/level/input, asset tags, input/tag block, cooldown 세부, 기본·동적 triggers, 활성 인스턴스와 Task | Idle은 활성화 가능을 뜻하지 않음. CanActivateAbility를 자동 호출하지 않음 |
| Active Effects | 적용/억제, duration/remaining/period, level/stack, instigator/source/causer, granted/asset tags, modifier operation/magnitude | Instant·종료 effect 이력 없음. evaluated modifier는 최종 Attribute 기여량과 다를 수 있음 |

Ability 인스턴스를 펼치면 활성 Task의 이름·state·instance name·debug string을 볼 수 있다. non-instanced Ability에는 실행별 인스턴스/Task 목록이 없다. 보호 metadata의 스키마를 읽을 수 없으면 `unsupported schema`로 표시한다.

## Ability Trigger 검색

1. 페이지에서 **Ability Triggers**를 고른다.
2. 태그 입력에 정확한 TriggerTag 이름을 쉼표 또는 공백으로 나눈다. 비우면 전체 Trigger 설정을 보여 준다.
3. `Include child tags`를 켜면 입력 태그 아래의 **자식 TriggerTag**도 포함한다. 예를 들어 입력 `Event.Attack`은 `Event.Attack.Heavy`를 포함한다. 역방향 부모 매칭이나 GAS 이벤트 전달 전체를 재현하는 기능은 아니다.
4. 콘텐츠 루트는 기본 `/Game`이다. 플러그인 콘텐츠를 보려면 해당 mount root를 입력하거나 비워 전체 콘텐츠를 대상으로 한다.
5. **Scan**으로 인덱스를 구축한다. native는 현재 로드된 클래스만 별도 포함한다. 현재 로드된 Blueprint CDO는 미저장 변경을 포함할 수 있으며 Value 열에 출처가 표시된다.
6. 진행 중에는 partial 결과, 취소 후에는 Cancelled를 표시한다. 실패 로드/지원 불가 수치를 확인한다.
7. 에셋 추가·삭제·갱신·rename·Blueprint compile 후 `Stale`이 표시되면 다시 Scan한다.

태그 query와 자식 포함 옵션을 바꾸면 이미 수집한 인덱스에서 다시 필터링한다. 콘텐츠 루트 변경은 다음 Scan에 적용된다. row는 TriggerTag·Source·Ability class 기준으로 중복을 제거한다.

이 화면은 Trigger **설정 검색**이며 Gameplay Event 발생 기록이나 Ability 활성화 성공 표시가 아니다. 레지스트리가 아직 준비되지 않으면 기다리고, 검색 완료 여부와 실패 수를 함께 표시한다. 미로드 native 모듈이나 레지스트리에 없는 에셋까지 완전하게 수집한다고 보장하지 않는다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| Target unavailable | 대상 소멸·월드 종료·수동 월드 소멸 | 월드/ASC 재선택. frozen 화면은 당시 값으로 남음 |
| ActorInfo not initialized | ASC의 Owner/Avatar 실행 준비 전 | 실행 시 InitAbilityActorInfo 경로 확인 |
| 선택 액터에서 ASC를 못 찾음 | 에디터 선택과 선택 월드의 actor가 다름 | 월드와 ASC를 직접 선택 |
| 일부 metadata 지원 불가 | Trigger/Task 프로퍼티 형식이 예상 schema와 다름 | 나머지 데이터는 조회 가능. 엔진 버전·오류 보고 |
| Scan 중 입력 지연 | 개별 CDO 에셋이 동기로 로드됨 | 콘텐츠 루트를 좁힌다. 틱당 배치는 나뉘지만 단일 로드 시간 제한은 없음 |
| Stale 검색 결과 | 검색 후 에셋/Blueprint 변경 | Scan으로 재구축 |
| Idle·차단 상태가 실제 활성화 판단과 다름 | 비용·대상·사용자 CanActivate 조건은 자동 평가 안 함 | 게임의 활성화 실패 진단과 함께 해석 |

게임 상태 변경·멀티플레이·원격 연결·실행 이력 기능은 제공하지 않는다. 읽기 과정에서 cooldown getter와 Task GetDebugString은 호출하며, 해당 확장 함수는 읽기 API의 계약을 지켜야 한다.

## 확인 상태와 근거

2026-10-05 사용자가 빌드 성공 및 테스트 완료를 보고했다. 개별 테스트 항목과 성능 측정 결과는 별도로 제공되지 않았다. 상세 확인 항목은 [설계의 확인 항목](../plan/GAS-Inspector-Plan.md)을 따른다.

- [플러그인 설정](../../Plugins/KataGASInspector/KataGASInspector.uplugin)
- [세션](../../Plugins/KataGASInspector/Source/KataGASInspectorEditor/Private/KataGASInspectionSession.cpp)
- [수집기](../../Plugins/KataGASInspector/Source/KataGASInspectorEditor/Private/KataGASSnapshotCollector.cpp)
- [Trigger 인덱스](../../Plugins/KataGASInspector/Source/KataGASInspectorEditor/Private/KataGASAbilityTriggerIndex.cpp)
- [구현 기록](../devlog/2026-10-05-GAS-Inspector.md)

사용자 요청으로 관련 이슈는 게시하지 않았다.
