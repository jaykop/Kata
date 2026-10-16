# 액션 편집기 Task Details 의 중첩 객체 편집 반영

작성: 2026-10-16  
갱신: 2026-10-16  
유형: 구현 기록  
대상: KataEditor `FKataActionEditor` 의 Task Details 편집 반영  
기준: 이 기록을 담은 커밋. 같은 파일의 다른 작업(#119) 미커밋 변경은 포함하지 않았다

## 배경과 결론

Kata 액션 편집기의 Task Details 에서 태스크가 담은 Instanced 서브오브젝트 안의 값을 바꾸면 에셋에 반영되지 않았다. 예를 들어 Hit Trace 태스크의 Hit Handler 에서 Impact Tag 를 바꾸면, 화면에는 잠깐 새 값이 보였다가 원래 값으로 되돌아갔다.

Settings 탭에는 같은 문제를 푸는 보정이 이미 있었다. 이 보정을 Task Details 편집에도 쓰도록 맞췄다. 2026-10-16 사용자가 빌드한 뒤 Impact Tag 변경이 유지되는 것을 확인했다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| Task 편집 반영 경로 | 변경 이벤트의 MemberProperty 이름을 그대로 태스크에서 찾아 복사했다. 서브오브젝트 안을 고치면 그 이름(예: `ImpactTag`)이 태스크에 없어 복사가 실패했고, 오류는 진단 문자열에만 남았다 | MemberProperty 가 태스크 클래스의 프로퍼티가 아니면, 편집된 서브오브젝트를 담은 태스크의 최상위 프로퍼티(예: `HitHandlers`)를 찾아 통째로 반영한다 |
| 최상위 프로퍼티 찾기 함수 | Settings 사본 전용 이름과 설명이었다 | 매개변수를 Root 로 바꿔 Settings 사본과 선택한 태스크에 함께 쓴다. 여러 태스크를 함께 편집하면 다른 태스크의 서브오브젝트는 Outer 비교로 걸러진다 |

결함: Task Details 에서 Instanced 서브오브젝트(Hit Handler) 안의 값을 바꾸면 에셋에 반영되지 않고 되돌아감 — 원인 위치: `Plugins/Kata/Source/KataEditor/Private/KataActionEditor.cpp` — 이슈: 없음

## 주요 결정과 이유

- 엔진은 Details 변경 이벤트의 MemberProperty 를 가장 가까운 오브젝트 노드 기준으로 정한다. 그래서 서브오브젝트 안을 고치면 MemberProperty 는 태스크가 아니라 서브오브젝트 클래스의 프로퍼티가 된다(`FPropertyNode::BuildPropertyChain`, `FPropertyNode::FixPropertiesInEvent`). 이벤트의 편집 객체 목록에는 그 서브오브젝트가 들어간다(`FPropertyHandleBase` 의 값 설정 경로가 값을 가진 객체를 TopLevelObjects 로 넘김).
- 서브오브젝트의 프로퍼티 하나만 따로 복사하는 대신 최상위 프로퍼티를 통째로 반영한다. 기존 복사 함수(`KataPropertyOverride::CopyOverriddenProperty`)가 Instanced 배열을 에셋 소유 사본으로 복제하는 규칙을 그대로 쓰기 위해서다. 상속 태스크의 오버라이드도 같은 경로로 최상위 프로퍼티 단위로 기록된다.
- Details 에 Notify Hook 을 달아 프로퍼티 체인 전체를 받는 방법도 있었지만, 대화형 변경까지 받게 되고 Settings 쪽과 처리 방식이 갈라져 쓰지 않았다.

## 근거

- [KataActionEditor.cpp](../../Plugins/Kata/Source/KataEditor/Private/KataActionEditor.cpp): `FindInstancedRootProperty`, `FKataActionEditor::OnTaskEdited`, `FKataActionEditor::OnSettingsEdited`(기존 보정).
- [KataPropertyOverride.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Action/KataPropertyOverride.cpp): `CopyOverriddenProperty` 의 Instanced 배열 복제.
- UE 5.8 로컬 엔진 소스 `Editor/PropertyEditor/Private/PropertyNode.cpp` 의 `BuildPropertyChain`·`FixPropertiesInEvent`, `PropertyHandleImpl.cpp` 의 값 설정 경로.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 원인 | 에이전트, 소스·엔진 소스 읽기와 에디터 MCP 로 에셋 값 조회 | `KA_BlackKnight_Attack01` 의 Hit Handler Impact Tag 가 편집해도 바뀌지 않는 경로를 확인 | - |
| 수정 후 편집 | 2026-10-16 사용자 빌드와 에디터 확인 | Impact Tag 변경 유지 | 상속 태스크(오버라이드) 안의 서브오브젝트 편집, 여러 태스크 동시 편집 |

## 남은 제한과 후속 작업

- 구조체 배열 안의 Instanced 필드처럼 더 깊게 중첩된 형태는 최상위 프로퍼티 찾기에서 다루지 않는다(Settings 쪽과 같은 제한). 찾지 못하면 그 편집은 반영하지 않는다.
- 편집할 때마다 서브오브젝트 사본을 새로 만든다. 기존 Instanced 배열 편집과 같은 동작이다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [문서 목록](../README.md) | 이 기록 링크 |
| manual | 영향 없음. 사용법은 바뀌지 않고 원래 의도대로 동작한다 |
