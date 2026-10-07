# GAS Inspector 사용법

갱신: 2026-10-07  
대상: KataGASInspector / KataGASInspectorEditor  
적용 기준: UE 5.8, 플러그인 0.2.0  
확인 상태: 2026-10-06 사용자 화면 동작 확인. 2026-10-07 화면 배치 유지·선택 해제·Frozen 대상 변경·갱신 생략 수정과 ASC 메뉴 이름·크기 정리 후 사용자 빌드·테스트 완료.

## 목적과 준비

GAS 상태를 읽는 Editor 전용 도구다. GameplayAbilities와 KataGASInspector를 활성화한다. 별도 디버그 컴포넌트나 ASC 교체는 필요 없다. Editor와 싱글플레이 PIE/SIE 월드를 관찰하며 Game 타깃·다른 프로세스에는 연결하지 않는다.

## 사용 순서

1. **Window → GAS Inspector** 또는 콘솔 `Kata.GASInspector.Open`으로 연다.
2. Auto world는 PIE를 우선하고 없으면 Editor를 고른다. 월드를 직접 선택하면 자동 선택을 끈다.
3. ASC 메뉴는 캐릭터(소유 액터) 이름만 표시하며 이름으로 검색해 대상을 고른다. 한 액터에 ASC가 여럿이면 컴포넌트 이름을 괄호로 덧붙인다. 컴포넌트 경로는 항목 tooltip에서 확인한다. 메뉴는 드롭다운 폭에 맞춰 열리며 항목이 많으면 목록 안에서 스크롤한다. Use selection은 현재 월드에서 에디터 선택 Actor에 연결된 ASC를 한 번 선택한다. 지속 추적 토글이 아니다.
4. Refresh·Freeze 옆의 드롭다운에서 Tags·Attributes·Abilities·Effects 화면을 고른다. 대상과 Freeze 상태는 네 화면이 공유한다.
5. 행을 선택하면 아래 상세 영역에 해당 정보가 표시된다. Ctrl+클릭으로 선택을 해제하면 상세와 선택 버튼도 비워진다.
6. Freeze는 화면 자동 갱신만 멈춘다. Refresh는 Frozen 상태를 유지하면서 한 번 다시 읽고, Resume은 자동 갱신을 재개한다.

## 화면별 구성

| 탭 | 기본 열 | 전용 기능·상세 |
|---|---|---|
| Tags | Tag, Kind, Count | 태그의 정의 출처·주석은 선택 상세에서 확인 |
| Attributes | Attribute, Current | Current만 표시. Set 클래스·속성 경로는 선택 상세에서 확인 |
| Abilities | Ability, State, Execution | Active only·Observed block only 필터, 인스턴스·Task 계층, 기본·동적 Trigger 상세 |
| Effects | Effect, State, Timing / Stacks | modifier 계층, level/period·source·tags 상세 |

Expand all/Collapse all과 펼침 화살표는 Abilities·Effects에서만 사용한다. Open asset도 이 두 탭에만 표시하며 native·링크 없는 행은 비활성화한다. 에셋 행을 더블클릭하면 원본 에셋을 연다. Copy name은 선택한 행 이름을 복사한다.

별도 Ability Trigger 검색 탭과 Scan/Cancel은 제거했다. 부여된 Ability의 TriggerTag·TriggerSource는 Abilities에서 해당 행을 선택해 확인한다. 이 값은 설정이며 실제 Gameplay Event 발생이나 활성화 성공 이력이 아니다.

## 검색·정렬·화면 상태

각 화면의 검색은 이름·상태·값·태그·출처·상세 문자열을 필터링한다. 자식이 일치하면 부모도 표시한다. 열 제목을 클릭하면 표시 문자열 기준으로 정렬하며 숫자 크기 정렬은 아니다.

화면별 검색·선택·펼침 상태는 창이 열린 동안 유지한다. 정렬, 표/상세 영역 비율, 열 너비는 표별로 개인 Editor ini에 저장하므로 화면을 오가거나 Editor를 다시 열어도 유지된다. 검색·정렬은 각 화면에서 독립적으로 유지한다. 활성/차단 필터·마지막 화면도 저장하며 대상 UObject와 snapshot은 저장하지 않는다. 기존 공통 정렬·열 숨김과 Trigger 검색 설정은 사용하지 않는다.

Freeze 오른쪽의 Interval (s) 숫자 입력에서 갱신 간격을 지정한다. 기본 0.2초, 범위 0.1~1초이며 개인 Editor ini에 저장한다. 별도 Settings 메뉴는 없다. 상태 안내는 도구 모음 아래 줄에 표시한다. 창이 foreground가 아니면 수집을 쉰다.

정상 요약은 Live/Frozen과 행 수만 표시한다. 월드·수집 시각·당시 대상·Owner/Avatar는 상태 tooltip에 둔다. Frozen 중 ASC를 바꾸면 당시 값을 그대로 두고 Previous target으로 안내한다. Refresh나 Resume을 누르면 새 대상의 값을 읽는다. 값이 바뀌지 않은 갱신 주기에는 수집 시각만 바꾸고 표를 다시 정렬하지 않는다. 대상 소멸·ActorInfo 미준비 등의 문제는 필요한 시점에 표시한다.

## 데이터 의미와 제한

- Tags: Owned count는 계층 합산을 포함한다. Ability blocked는 owned count나 차단 횟수가 아니다. 전체 태그 제공자 추적은 제공하지 않는다.
- Attributes: 엔진 API로 열거한 Current 값이며 Base는 표시하지 않는다. 같은 속성 이름은 상세의 Set·경로로 구분한다.
- Abilities: Spec active/count/level/input과 관측 가능한 input/tag block을 표시한다. Idle은 활성화 가능을 보증하지 않으며 CanActivateAbility를 자동 호출하지 않는다. 활성 인스턴스의 Task를 펼칠 수 있다. NonInstanced는 실행별 인스턴스 목록이 없다.
- Effects: 현재 활성 효과의 기간·남은 시간·스택·억제 상태와 evaluated modifier를 읽는다. Instant·종료 효과 이력은 없다. modifier 값은 최종 Attribute 기여량과 다를 수 있다.

게임 상태 변경·멀티플레이·원격 연결·실행 이력은 제공하지 않는다. cooldown getter와 Task GetDebugString은 읽기 API 계약을 지켜야 한다. protected metadata 스키마 불일치는 해당 정보의 unsupported schema로 안내한다.

## 문제 해결과 확인 근거

대상이 사라진 Live 화면은 비우고 다시 선택한다. Frozen은 당시 값이 유지되므로 Resume/Refresh로 새 데이터를 얻는다. Use selection이 실패하면 선택 월드와 Actor가 같은지 확인하고 ASC를 직접 선택한다. 에셋 삭제·로드 실패는 상세 안내를 확인한다.

2026-10-06 사용자가 최종 화면 변경 후 정상 동작을 확인하고 커밋을 요청했다. 에이전트는 빌드·테스트를 직접 실행하지 않았다. 개별 회귀 항목과 성능 측정 결과는 별도로 제공되지 않았다. 2026-10-07 배치 유지와 진단 수정 후 사용자가 빌드 성공과 테스트 완료를 보고했다. 에이전트는 빌드·테스트를 실행하지 않았다.

- [플러그인 설정](../../Plugins/KataGASInspector/KataGASInspector.uplugin)
- [화면](../../Plugins/KataGASInspector/Source/KataGASInspectorEditor/Private/SKataGASInspector.cpp)
- [수집기](../../Plugins/KataGASInspector/Source/KataGASInspectorEditor/Private/KataGASSnapshotCollector.cpp)
- [개편 기록](../devlog/2026-10-06-GAS-Inspector-UI.md)
- [설계](../plan/GAS-Inspector-Plan.md)

사용자 요청으로 이슈는 게시하지 않았다.
