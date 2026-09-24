# GenericGraph 흡수 기록

KataGraph와 KataGraphEditor 모듈은 아래 오픈소스 플러그인을 가져와 Kata에 맞게 고친 것이다.

- 출처: https://github.com/jinyuliao/GenericGraph
- 커밋: f9b8fe3de6bc2ef39ee771658ac4a8bf48c2e078
- 라이선스: MIT ([LICENSE-GenericGraph.txt](LICENSE-GenericGraph.txt))

## 변경한 내용

- 모든 공개 심볼에 Kata 접두사를 붙였다. `UGenericGraph`→`UKataGraphBase`,
  `UGenericGraphNode`→`UKataGraphNodeBase`, `UGenericGraphEdge`→`UKataGraphEdgeBase`.
- 런타임 3개 클래스는 직접 다시 작성했다. `TObjectPtr` 사용, 한국어 주석,
  순환 그래프에서 끝나도록 단계 순회에 방문 기록 추가.
- `UKataGraphNodeBase::Edges`를 `TMap<Node*, FKataGraphEdgeList>`로 바꿔
  같은 두 노드 사이에 엣지를 여러 개 둘 수 있게 했다.
  UHT가 `TMultiMap`과 중첩 컨테이너를 리플렉션 대상으로 지원하지 않아 구조체로 감쌌다.
- 에셋 등록을 `FAssetTypeActions_Base`에서 `UAssetDefinition`으로 옮겼다.
  원본의 `AssetTypeActions_GenericGraph`, `GenericGraphFactory`, 모듈 등록 코드는 버렸다.
- `Build.cs`를 UE 5.8에 맞춰 다시 썼다. 제거된 `ShadowVariableWarningLevel`과
  `EditorStyle` 모듈 의존을 뺐다.
- 로그는 KataRuntime의 `LogKata`를 공유한다.
- UE 5.8 컴파일 경고를 정리했다. 에디터의 `EditingGraph`를 `TObjectPtr`로 바꿔 증분 GC 경고를 없앴고,
  붙여넣기 위치는 `GetPasteLocation2f`와 `FVector2f`를 쓰며, 자동 배치의 `CoolDown` 상수는 float 리터럴로 바꿨다.

## 주석 규칙

벤더 코드의 원문 주석은 유지한다. 우리가 고치거나 새로 쓴 부분에만 한국어 주석을 단다.
