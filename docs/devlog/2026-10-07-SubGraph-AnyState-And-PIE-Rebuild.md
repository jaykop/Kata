# SubGraph Any State 범위와 PIE 중 재구성

작성: 2026-10-07  
갱신: 2026-10-07  
유형: 구현 기록  
대상: KataGraph, KataGraphEditor, Editor-Usage manual  
기준: 3a22281 이후 미커밋 작업 트리

## 배경과 결론

#27의 SubGraph 구현을 점검하다가 두 가지 위험을 발견했다.
하나는 SubGraph 안의 Any State가 부모 그래프 전체로 범위가 새는 문제다. 다른 하나는 PIE 중 재구성이 실행 중인 인스턴스의 연결을 끊는 문제다.
두 문제를 [#43](https://github.com/jaykop/Kata/issues/43)에서 수정했다. 외장 복제본의 RF_Standalone 잔류 가능성도 함께 확인했으며, 수정할 필요가 없다고 판단했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| SubGraph 안 Any State | `CoversNode`는 그래프 전체의 실행 가능 노드를 덮는다. 펼친 뒤에는 부모와 다른 SubGraph 사본까지 출발지가 됐다 | `FlattenSubGraphs`가 복사한 Any State 사본을 끄고 `ResolvedSourceNodes`를 그 사본의 실행 가능 노드로 채운다 |
| PIE 중 에디터 열기 | 툴킷 초기화가 매번 `Rebuild`를 호출했다. `ClearGraph`가 이전 사본의 연결을 지워 실행 중인 인스턴스가 전이를 잃을 수 있었다 | PIE 중에는 열기 재구성을 건너뛴다 |
| PIE 중 저장 | 저장 전 재구성도 같은 방식으로 이전 사본의 연결을 지웠다 | `UKataEdGraph::Clear`는 목록만 비우고 저작 노드의 연결만 초기화한다. 저장 후에는 PIE를 재시작해야 반영된다는 알림을 띄운다 |
| 외장 복제본 RF_Standalone | `DuplicateObject`가 플래그를 복사하므로 GC가 복제본을 회수하지 못할 가능성을 의심했다 | UE 5.8 GC는 Garbage 객체를 Keep 플래그와 무관하게 루트에서 제외한다(`GarbageCollection.cpp` 4442행). 껍데기는 `MarkAsGarbage`되므로 수정하지 않았다 |

## 주요 결정과 이유

- SubGraph 안의 Any State는 그 SubGraph(중첩 하위 포함)만 덮는다. 재사용 조각이 넣는 위치에 따라 바깥 그래프 동작을 바꾸면 안 되기 때문이다. 루트의 Any State는 기존대로 SubGraph 내부까지 덮는다.
- 원본 페이지는 그대로 두고 실행 사본만 바꾼다. 원본을 고치면 다음 저장에서 사용자 설정이 사라진다.
- PIE 중 저장은 막지 않는다. 언리얼의 일반 편집 흐름을 유지하고 대신 경고로 안내하는 방식을 사용자가 선택했다.
- 이전 사본의 연결을 유지하므로 실행 중인 인스턴스는 SubGraph 안에서 기존 구조를 따라간다. 다만 이전 Port Out 별칭은 새 `AllNodes`에 없다. 그래서 인스턴스가 이전 사본에 머무는 동안에는 Port Out을 통한 가로채기 전이가 평가되지 않는다. 이 상태는 PIE를 재시작하면 해소된다.

## 근거

- [KataEdGraph.cpp](../../Plugins/Kata/Source/KataGraphEditor/Private/KataEdGraph.cpp): `Clear`, `FlattenSubGraphs`.
- [KataGraphAssetEditor.cpp](../../Plugins/Kata/Source/KataGraphEditor/Private/KataGraphAssetEditor.cpp): `InitKataGraphEditor`의 재구성 조건.
- [KataGraphEditorModule.cpp](../../Plugins/Kata/Source/KataGraphEditor/Private/KataGraphEditorModule.cpp): 저장 전 재구성과 PIE 알림.
- [KataAliasNode.cpp](../../Plugins/Kata/Source/KataGraph/Private/KataAliasNode.cpp): `CoversNode`의 Any State 판정.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 수정 코드 | 소스 읽기·에이전트 | 구현함 | 빌드·실행 미실시 |
| RF_Standalone 영향 | 엔진 소스 읽기·에이전트 | Garbage 객체는 Keep 플래그로 유지되지 않음 | 실제 메모리 측정 미실시 |

## 남은 제한과 후속 작업

- 이 수정 전에 저장한 그래프는 다시 저장해야 Any State 범위가 반영된다.
- 그래프 실행 상태 디버그 표시는 #29에서 진행한다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#43](https://github.com/jaykop/Kata/issues/43) | 구현 완료, 사용자 확인 전 |
| [Editor-Usage](../manual/Editor-Usage.md) | SubGraph 주의할 점에 Port In 트리거, Port Out Priority, Any State 범위, PIE 동작 추가 |
