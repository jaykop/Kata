# 그래프 저장 실패와 삭제 후 남은 노드 정리

작성: 2026-10-04  
갱신: 2026-10-04  
유형: 진단  
대상: KataGraphEditor `UKataEdGraph::RebuildKataGraph`  
기준: 미커밋 작업 트리

## 배경과 결론

새 그래프 `KG_BlackKnight`를 편집하던 중 저장이 매번 실패했다. 노드를 지운 뒤 남은 런타임 노드·엣지가 원인이었다.
그래프를 재구성할 때 그래프에서 닿지 않는 런타임 노드·엣지를 Transient 패키지로 옮기도록 고쳤고, 사용자가 이후 저장 성공을 확인했다.

## 변경 또는 진단 내용

| 항목 | 발견 내용 | 반영 결과 |
|---|---|---|
| 오류 | `LogSavePackage: Error: Unexpected custom version "FortniteMain" found when saving ... Package will not be saved.` 호출 스택은 `WriteGatherableText` | 원인 확인 |
| 발생 시점 | 같은 에셋이 몇십 초 전에는 저장됐고, 이후 편집부터 매번 실패 | 편집 중 생긴 오브젝트가 원인 |
| 원인 | 노드를 지우면 편집기 노드만 사라지고 런타임 노드·엣지는 Undo 기록에 붙잡힌 채 그래프 패키지에 남는다. 저장은 닿는 오브젝트만 직렬화하지만 현지화 텍스트 수집은 패키지의 모든 오브젝트를 훑어, 남은 오브젝트의 FText(`NodeTitle`, `ContextMenuName`)가 수집 단계에서만 직렬화된다 | 저장 직전 재구성에서 정리 |
| 수정 | `FReferenceFinder`로 그래프에서 닿는 직속 하위 오브젝트를 모으고, 화면의 편집기 노드가 가리키는 노드·엣지를 더해, 그 밖의 노드·엣지를 Transient 패키지로 옮긴다 | `EvictUnreachableGraphObjects` |

## 주요 결정과 이유

- 삭제 시점이 아니라 재구성 시점에 정리한다. 재구성은 저장 직전(`OnPreSavePackageWithContext`)에 항상 실행되고, 서브그래프 펼침 사본처럼 삭제 외 경로로 남는 오브젝트도 함께 처리된다.
- 필드를 일일이 열거하지 않고 직렬화 참조로 판정한다. 저장과 같은 기준이라 정상 참조를 잘못 옮길 위험이 적다.
- Undo로 복구한 노드는 기존 재구성이 그래프 밑으로 다시 옮기므로 복구가 유지된다.

## 근거

- [KataEdGraph.cpp](../../Plugins/Kata/Source/KataGraphEditor/Private/KataEdGraph.cpp): `EvictUnreachableGraphObjects`와 호출 위치.
- [KataGraph/UPSTREAM.md](../../Plugins/Kata/Source/KataGraph/UPSTREAM.md): GenericGraph 원본 대비 변경 기록.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| `KG_BlackKnight` 저장 | 사용자가 Live Coding 후 저장 | 저장 성공 | 노드 삭제 후 Undo 복구 경로 |

## 남은 제한과 후속 작업

없음.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| manual | 사용법 변경 없음 |
