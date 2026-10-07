# 그래프 에디터 PIE 디버거

작성: 2026-10-07  
갱신: 2026-10-07  
유형: 구현 기록  
대상: KataGraph, KataGraphEditor, Editor-Usage manual  
기준: 8d07016 이후 미커밋 작업 트리

## 배경과 결론

[#29](https://github.com/jaykop/Kata/issues/29)는 원래 GameplayDebugger와 화면 텍스트로 그래프 상태를 보여 주는 안이었다.
사용자는 애니메이션 블루프린트처럼 그래프 에디터 캔버스에서 실행 중인 노드를 강조하는 방식을 요청했다.
그래서 PIE 액터를 선택하는 툴바 콤보, 노드 테두리 강조, 전이 기록을 보여 주는 Debug 탭을 그래프 에디터에 구현했다.
이전 Codex 구현은 사용자 요청으로 철회됐으며 이번 구현과 관계가 없다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| 대상 수집 | 그래프 인스턴스는 `UKataGraphComponent::StartGraph`에서만 만든다 | PIE 월드 컴포넌트 가운데 활성 인스턴스의 그래프가 루트인 것을 모은다 |
| SubGraph 노드 대응 | 실행 노드는 펼칠 때 `DuplicateObject`로 만든 사본이고 원본 정보가 없다 | 노드에 편집기 전용 `DebugSourceNodeGuid`와 `DebugSubGraphPath`를 저장 재구성 때 기록한다 |
| 페이지 식별 | 내장 원본에는 저장된 식별자가 없다 | 경로 이름으로 결정적 GUID를 만든다. 별도 저장 필드는 추가하지 않는다 |
| 상태 조회 | 예약 대상은 private이었다 | `GetPendingTargetNode`, `GetPendingEdge`를 추가했다 |
| 실행 기록 | 기록이 없었다 | `#if WITH_EDITOR` 인스턴스별 최근 32개 기록(전이·예약·거절·종료)을 남긴다 |
| 화면 | 미사용 `ActiveDebugging` 색만 있었다 | 노드 테두리 강조 4종, 툴바 Debug Object 콤보, Debug 탭과 기록 더블클릭 이동을 추가했다 |

## 주요 결정과 이유

- 출처는 포인터가 아니라 GUID로 기록한다. 내장 원본을 통째로 복제하면 원본 내부를 가리키던 참조가 사본 쪽으로 바뀌기 때문이다.
- 페이지 식별자는 경로에서 계산한다. 기존 에셋에 저장 필드를 추가하지 않아도 되는 대신, 경로가 바뀌면 부모를 다시 저장해야 한다.
- 대상은 컴포넌트로 고른다. 그래프를 다시 시작하면 인스턴스가 새로 만들어지므로 인스턴스를 들고 있으면 선택이 풀린다.
- 후보가 하나면 자동으로 선택하고 뷰포트 선택과는 연동하지 않는다. PIE 중 액터를 고르려면 Eject해야 해서 흐름이 끊기기 때문이다(사용자 승인).
- 외장 SubGraph 에셋을 따로 열었을 때의 강조는 후속으로 미뤘다. 출처 경로가 페이지 식별자를 담고 있으므로 데이터 변경 없이 확장할 수 있다(사용자 승인).
- 기록은 공유 에셋이 아니라 인스턴스에 두고 약한 참조를 쓴다. PIE 중 저장으로 버려진 사본을 기록이 붙잡지 않게 하기 위해서다.

## 근거

- [KataGraphInstance.h](../../Plugins/Kata/Source/KataGraph/Public/KataGraphInstance.h): `FKataGraphDebugRecord`, 예약 대상 getter.
- [KataGraphNodeBase.h](../../Plugins/Kata/Source/KataGraph/Public/KataGraphNodeBase.h): 출처 필드.
- [KataEdGraph.cpp](../../Plugins/Kata/Source/KataGraphEditor/Private/KataEdGraph.cpp): 저작 노드와 SubGraph 사본의 출처 기록.
- [KataGraphDebugger.cpp](../../Plugins/Kata/Source/KataGraphEditor/Private/KataGraphDebugger.cpp): 대상 수집, 강조 계산, 노드 해석.
- [SKataGraphDebugView.cpp](../../Plugins/Kata/Source/KataGraphEditor/Private/SKataGraphDebugView.cpp): Debug 탭.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 구현 코드 | 소스 작성·에이전트 | 구현함 | 에이전트 빌드·실행 미실시 |
| 에디터 빌드·PIE 동작 | 사용자 빌드·테스트 보고(2026-10-07) | 빌드 성공, 테스트 완료, Debug 탭 아이콘 표시 확인 | 개별 테스트 항목별 결과는 보고되지 않음 |

## 남은 제한과 후속 작업

- 외장 SubGraph 에셋을 단독 편집기로 열었을 때의 강조는 미지원이다.
- 마지막 전이 엣지의 캔버스 강조는 하지 않는다. 전이는 Debug 탭 기록으로 확인한다.
- 이 기능 이전에 저장한 그래프는 다시 저장해야 SubGraph 안쪽 강조가 맞는다.
- 그래프 에디터 레이아웃 키를 v6으로 올렸으므로 기존 탭 배치가 한 번 초기화된다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#29](https://github.com/jaykop/Kata/issues/29) | 본문을 에디터 캔버스 디버거 범위와 확정 결정으로 갱신함 |
| [Editor-Usage](../manual/Editor-Usage.md) | 그래프 에디터에 "PIE 실행 상태 보기" 절 추가 |
