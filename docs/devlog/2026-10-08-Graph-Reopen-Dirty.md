# 그래프 에디터 재개봉 시 KataGraph 에셋이 dirty가 되던 문제

작성: 2026-10-08  
갱신: 2026-10-08  
유형: 진단  
대상: KataGraphEditor `UKataEdGraph::Modify`  
기준: 커밋 `de322aa`

## 배경과 결론

에디터를 시작하면 이전 세션에 열려 있던 그래프 에디터가 복원된다. 이때 SubGraph를 쓰는 KataGraph 에셋(`KG_BlackKnight_GreatSword`, `KG_BlackKnight_GreatSword_BaseCombo`)이 매번 dirty가 됐다.
저장한 직후에는 같은 세션 안에서 다시 dirty가 되지 않았지만, 재시작하면 다시 dirty가 됐다.

원인은 `UKataEdGraph::Modify(bool bAlwaysMarkDirty)` 오버라이드였다. 이 함수는 인자를 무시하고 소유 그래프와 노드의 `Modify()`를 기본값 `true`로 호출했다.
하위 `Modify` 호출에 `bAlwaysMarkDirty`를 그대로 넘기도록 고쳤다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| dirty 발생 경로 | 에디터를 열 때 `FKataGraphBuildContext::Rebuild` → `UKataEdGraph::FlattenSubGraphs`가 복제한 SubGraph의 EdGraph를 `REN_DoNotDirty`로 Transient 패키지에 옮긴다. 엔진의 `UObject::Rename`은 이 플래그가 있으면 `Modify(false)`를 호출한다. 그런데 오버라이드가 `GetKataGraph()->Modify()`를 `true`로 다시 불러 루트 에셋 패키지를 dirty로 만들었다. | `GetKataGraph()->Modify(bAlwaysMarkDirty)`와 `Nodes[i]->Modify(bAlwaysMarkDirty)`로 수정 |
| 저장 직후 재현되지 않은 이유 | 저장 중에는 엔진이 패키지 dirty 표시를 막는다. 저장 시 재구성은 같은 경로를 지나도 표시가 남지 않았다. 재개봉 시 재구성에서만 dirty가 드러났다. | 해당 없음 |
| 원인에서 제외한 경로 | `RefreshDependencyStatus`: 의존 경고 문구가 표시되지 않았다. `RemoveUnusedEmbeddedSubGraphs`: 맵 로드 이후 Undo 기록이 비어 있었다. 두 경로 모두 원인이 아니었다. | 변경 없음 |

## 주요 결정과 이유

- `Rename` 호출부에 `REN_NonTransactional`을 추가하는 대신 `Modify` 오버라이드를 고쳤다. `bAlwaysMarkDirty` 인자를 무시하는 `Modify` 오버라이드는 `Rename` 외의 엔진 경로에서도 같은 문제를 만들기 때문이다.
- 실제 편집은 `Modify()`를 기본값 `true`로 호출한다. 편집 시 소유 그래프와 노드를 함께 트랜잭션에 기록하고 dirty로 표시하는 기존 동작은 그대로다.
- `Modify`를 오버라이드해 다른 객체의 `Modify`를 연쇄 호출할 때는 받은 `bAlwaysMarkDirty`를 그대로 넘긴다.

## 근거

- [KataEdGraph.cpp](../../Plugins/Kata/Source/KataGraphEditor/Private/KataEdGraph.cpp): `UKataEdGraph::Modify`와 `FlattenSubGraphs`의 복제본 정리 `Rename`.
- [KataGraphAssetEditor.cpp](../../Plugins/Kata/Source/KataGraphEditor/Private/KataGraphAssetEditor.cpp): `InitKataGraphEditor`의 재개봉 시 `FKataGraphBuildContext::Rebuild` 호출.
- 엔진 `UObject::Rename`(`Obj.cpp`): `REN_DoNotDirty`일 때 `Modify(false)`를 호출한다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 원인 특정 | 에디터 로그, MCP `is_dirty` 조회, 그래프 에디터 UI와 Undo 기록 확인 (에이전트) | 재개봉 직후 두 에셋 dirty, 의존 경고와 Undo 기록 없음 | 해당 없음 |
| 수정 결과 | 사용자 Editor 빌드 후 에디터 재시작 (2026-10-08) | 복원된 그래프 에디터의 에셋이 dirty가 되지 않음 | 편집 시 dirty 표시 유지 여부는 별도로 보고되지 않음 |

## 남은 제한과 후속 작업

없음.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [Editor-Usage.md](../manual/Editor-Usage.md) | 영향 없음. 재개봉 시 dirty를 만들지 않는다는 기존 서술과 일치한다. |
| [SubGraph 포트와 평탄화](2026-10-03-SubGraph-Flattening.md) | 영향 없음. 평탄화 구조는 바뀌지 않았다. |
