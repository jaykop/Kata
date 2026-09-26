# 그래프 에디터 패널과 디테일 커스터마이제이션

갱신: 2026-09-26

## 요약

KataGraph 에디터에서 Comment 생성 위치를 고치고 노드 검색을 추가했으며, Alias의 출발지를 고르는
디테일 패널 UI를 만들었다. 세 작업 모두 엔진 디테일 패널과 그래프 패널의 제약에 부딪혀 방식을 바꿨다.

## 배경

그래프가 커지면서 편집이 불편해졌다. Comment가 엉뚱한 위치에 생기고, 노드를 눈으로 찾아야 하고,
Alias를 도입하면서 그래프 안의 노드를 고를 수단이 필요해졌다. 셋 다 엔진 기능을 그대로 쓰면 될 것
같았지만 실제로는 각각 다른 벽이 있었다.

## 구현 요약

Comment는 선택한 노드를 감싸고, `Ctrl+F`로 노드와 전이를 찾으며, Alias의 출발지는 그래프 노드
목록의 체크 상자로 고른다.

## 파일 구조

| 파일 | 역할 | 신규/수정 |
|---|---|---|
| `Plugins/Kata/Source/KataGraphEditor/Public/KataGraphSchema.h` | `ShouldAlwaysPurgeOnModification`을 false로 둔다 | 수정 |
| `Plugins/Kata/Source/KataGraphEditor/Private/KataGraphAssetEditor.cpp` | Comment 배치, 검색 탭과 `Ctrl+F` | 수정 |
| `Plugins/Kata/Source/KataGraphEditor/Private/SKataFindInGraph.h/.cpp` | 엔진 `SFindInGraph`를 상속한 검색 패널 | 신규 |
| `Plugins/Kata/Source/KataGraphEditor/Private/KataAliasSourceSetCustomization.h/.cpp` | Alias 출발지 목록 UI | 신규 |

## 핵심 동작 흐름

1. Comment는 노드를 추가하기 전에 선택 영역과 커서 위치를 구해 둔다. 선택이 있으면 여백 50으로
   감싸고, 없을 때만 커서 위치에 기본 크기로 만든다.
2. 검색은 엔진 `SFindInGraph`가 노드 제목·주석·핀을 훑고, `MatchTokensInNode`에서 액션 에셋
   이름과 경로, 노드 클래스, 전이의 트리거 태그와 창 태그를 더한다.
3. Alias 출발지는 `FKataAliasSourceSet` 구조체에 담기고, 그 타입의 커스터마이제이션이 편집기
   그래프를 훑어 머무를 수 있는 노드를 체크 상자로 나열한다.

## 의사결정 기록

| 결정 | 대안 | 채택 사유 |
|---|---|---|
| 스키마의 `ShouldAlwaysPurgeOnModification`을 false로 둔다 | 엔진 기본값 true 유지 | 기본값 true는 변경 알림마다 그래프 패널의 노드 위젯을 통째로 다시 만든다. 수정 직후 위젯 상태를 묻는 코드가 조용히 실패하고, 노드를 하나 더할 때마다 전체 위젯이 다시 생긴다. `EdGraphSchema_K2`도 false를 쓴다 |
| Comment 배치 정보를 `AddNode` 전에 구한다 | 노드를 만든 뒤 선택 영역을 묻는다 | 그래프를 수정하면 패널이 위젯을 다시 만들 수 있어 그 뒤에 물으면 빈 결과가 나온다. 엔진의 Comment 생성도 같은 순서다 |
| 검색을 엔진 `SFindInGraph` 상속으로 만든다 | 검색 위젯을 직접 만든다 | 검색창·결과 트리·토큰 파싱·따옴표 묶음 검색이 기반 클래스에 있고 오버라이드 세 개만 채우면 된다 |
| Alias 출발지를 `USTRUCT`로 감싸고 `IPropertyTypeCustomization`을 쓴다 | `UKataAliasNode`에 `IDetailCustomization`을 건다 | 디테일 패널의 루트는 `UKataEdNode`이고 `KataNode`가 `Instanced`라 안쪽 속성이 인라인으로 펼쳐진다. 인라인 하위 객체는 자기 클래스의 레이아웃 커스터마이제이션을 타지 않는다. 프로퍼티 타입 커스터마이제이션은 중첩 깊이와 무관하게 적용된다 |
| 후보 목록을 편집기 그래프에서 모은다 | `UKataGraphBase::AllNodes`를 쓴다 | `AllNodes`는 저장 시점의 `RebuildKataGraph`에서 채우므로 편집 중에는 뒤처진다. 방금 만든 노드가 목록에 없으면 쓸 수 없다 |
| 갱신에 `IDetailCustomNodeBuilder`를 쓴다 | `IPropertyUtilities::RequestForceRefresh` | 패널 전체를 다시 만들면 펼쳐 둔 상태가 풀린다. 빌더의 `SetOnRebuildChildren`은 이 목록의 자식만 다시 만든다 |
| 접을 수 있는 단계를 모두 없앤다 | 목록을 그룹으로 감싼다 | 갱신할 때마다 다시 접히는 것이 실제 불편이었다. 대신 목록이 길어져도 접을 수 없다 |

## 시행착오

**Comment가 엉뚱한 위치에 생기던 원인은 두 겹이었다.** 처음에는 `GetPasteLocation()`이 낡은 값을
준다고 보고 호출 순서만 고쳤는데 고쳐지지 않았다. 실제 원인은 스키마의
`ShouldAlwaysPurgeOnModification`이 기본값 true여서 `AddNode` 직후 `NodeToWidgetLookup`이
비워지고, 그 뒤에 부른 `GetBoundsForSelectedNodes`가 위젯을 찾지 못해 항상 실패한 것이다.

**Alias 출발지를 오브젝트 배열로 두었더니 쓸 수 없는 UI가 나왔다.** 디테일 패널이
`TArray<TObjectPtr<UKataNode>>`를 에셋 피커로 그리는데, 노드는 에셋이 아니라 그래프 에셋의
하위 객체라 피커로는 고를 수 없다.

**목록이 갱신할 때마다 접히는 문제는 원인을 두 번 잘못 짚었다.** 처음에는 갱신 방식
(`RequestForceRefresh`가 패널 전체를 다시 만든다)을 의심했고, 다음에는 빌더의 초기 펼침 상태를
봤다. 실제로 접혀 있던 것은 `Category = "Kata|Alias"`가 만든 하위 카테고리였다. 하위 카테고리는
그 자체가 접히는 단계여서, 그 아래에서 무엇을 하든 소용이 없었다.

접히는 단계를 없애는 방법은 세 곳에 나뉘어 있다. 카테고리는 `Kata`로 올려 최상위에 두고,
속성 행은 `CustomizeHeader`를 비워 `FDetailPropertyRow::ShowOnlyChildren`을 참으로 만들고,
빌더 행은 `GenerateHeaderRowContent`를 비워 `FDetailCustomBuilderRow::ShowOnlyChildren`을
참으로 만든다. 뒤의 둘은 엔진 `FMaterialList`가 쓰는 방식이다.

**갱신 요청은 즉시 실행하면 안 된다.** `IPropertyUtilities::ForceRefresh`는 바로 실행되므로
그래프 변경 알림을 받는 도중에 부르면 지금 델리게이트를 실행 중인 커스터마이제이션 객체가 그 자리에서
파괴된다. 엔진 헤더도 `RequestForceRefresh`를 권한다.

## 검증 상태

2026-09-26 사용자 Editor 빌드와 동작 확인.

- Comment가 선택한 노드를 감싸고, 선택이 없으면 커서 위치에 생긴다.
- 노드 검색이 동작한다.
- Alias 디테일 패널에 그래프의 노드가 나열되고 체크 상자가 보인다.

`ShouldAlwaysPurgeOnModification`을 false로 바꾼 뒤의 연결 생성·끊기, 노드 삭제 시 엣지 노드
정리, 붙여넣기와 Undo·Redo는 별도로 확인하지 않았다.

## 남은 제한과 향후 확장

- Alias 목록은 접을 수 없다. 노드가 많은 그래프에서는 길어진다. Any State를 켜면 목록이 숨는다.
- 노드를 추가하면 `AddNode`가 새 노드를 선택해 디테일 패널이 Alias에서 떠난다. 목록 자동 갱신이
  눈에 보이는 경로는 Undo나 자동 정렬처럼 선택이 바뀌지 않는 변경이다.
- 검색 패널에 그래프 검증 결과를 함께 띄울 자리를 남겨 두었다. 액션이 비어 있는 노드, 도달 불가
  노드, 막다른 Conduit이 대상이다.
