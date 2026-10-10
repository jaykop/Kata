# Action Template 1단계 상속

작성: 2026-10-10
갱신: 2026-10-10
유형: 결정 기록
대상: KataRuntime 액션 에셋 구조·KataEditor 액션 에디터와 생성 메뉴
기준: main 3206973 이후 미커밋 작업 트리. #47 측정 스코프 미커밋 변경 포함

## 배경과 결론

`UKataAction`은 `ParentAction`으로 같은 타입의 부모를 참조해 상속 깊이에 제한이 없었다. 중첩 상속이 많아지면 값의 출처를 추적하기 어려워 관리가 힘들다는 사용자 판단에 따라,
상속을 1단계로 제한하고 부모 역할을 별도 에셋 타입이 맡도록 바꿨다([#50](https://github.com/jaykop/Kata/issues/50)).

- `UKataActionAssetBase`(추상): 공통 설정·타임라인·프리뷰 필드와 해석·검증 로직.
- `UKataActionTemplate`: 부모 전용 타입. 추가 필드가 없고 부모를 가질 수 없으며 런타임에서 재생하지 않는다.
- `UKataAction`: 재생 가능한 액션. `ParentAction`(`UKataActionTemplate`)·`TaskOverrides`·`OverriddenSettings`를 추가로 가진다.

2026-10-10 사용자가 Editor 빌드 통과와, `KAT_PC_Attack` Template을 만들어 시험한 결과 정상 동작한다고 보고했다. 항목별 결과는 따로 보고되지 않았다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 상속 깊이 | `UKataAction` 체인 무제한, 순환은 해석 오류 | [Template, Action] 또는 자기 자신. 타입으로 순환이 불가능해 `ParentActionCycle` 진단과 에디터 순환 검사를 제거 |
| 부모 타입 | `UKataAction` | `UKataActionTemplate`. 표시 이름은 Parent Template |
| 해석 함수 | `UKataAction::Resolve` 등 | `UKataActionAssetBase`로 이동. 편집용 사본은 원본과 같은 클래스로 만든다 |
| Template 재생 | 해당 없음 | 재생 API는 `UKataAction`만 받는다. 에디터 프리뷰는 Template을 부모로 둔 Transient 임시 액션으로 재생 |
| 자식 생성 | 액션 에디터의 Create Child 버튼 | Template 에디터의 Create Child 버튼과 Content Browser 우클릭 Create Child Action. Action에는 버튼을 숨긴다 |
| 오버라이드 UI | 모든 에셋에 표시 | Reset Override·Reset Task Override는 Parent Template이 있는 Action에서만 표시 |

## 주요 결정과 이유

- **별도 타입으로 1단계를 강제한다.** 깊이 검사를 검증 코드로 두면 저장된 에셋이 규칙을 어길 수 있다. 부모 필드의 타입을 Template으로 좁히고 Template에서 부모 필드를 없애면
  다단계와 순환이 표현 자체가 불가능해진다. `UKataActionTemplate`을 `UKataAction`의 하위 클래스로 두지 않은 이유도 같다. 하위 클래스면 부모 필드를 물려받고 재생 슬롯에도 들어간다.
- **Template은 재생하지 않는다.** 사용자 결정이다. 부모를 그대로 재생하고 싶으면 변경분 없는 자식 Action을 만든다.
- **필드 이름을 바꾸지 않는다.** UE는 프로퍼티를 이름으로 직렬화하므로 같은 이름으로 기반 클래스에 옮기면 기존 `UKataAction` 에셋이 그대로 로드된다. 클래스 경로도 같아 Redirect가 필요 없다.
  타입이 바뀐 프로퍼티는 `ParentAction` 하나이며, 작업 시점 샘플 콘텐츠에 이 값을 저장한 에셋은 없었다.
- **프리뷰는 임시 자식 액션으로 재생한다.** 컴포넌트에 해석 결과를 직접 받는 공개 경로를 더하는 대안은 런타임 API를 늘리고 `SourceAction`·`GetKataAction()` 타입을 넓혀 Blueprint 핀까지 바꾼다.
  변경분 없는 자식은 매 해석마다 Template의 최신 값을 쓰므로 결과가 같고 런타임 계약은 그대로다.
- **Template에도 같은 데이터 검증을 적용한다.** Template의 오류는 그 Template을 쓰는 모든 자식에 나타나므로 원천에서 표시한다. 작성 중인 미완성 태스크는 지금처럼 오류로 보지 않는다.
- 별도 plan 문서는 만들지 않았다. 결정이 이슈 게시 전 대화에서 확정되어 이 기록에 남긴다.

## 근거

- [KataActionAssetBase.h](../../Plugins/Kata/Source/KataRuntime/Public/Action/KataActionAssetBase.h)·[KataActionAssetBase.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Action/KataActionAssetBase.cpp): 공통 필드, `CollectActionChain`·`MakeEffectiveSettings`·`Resolve`.
- [KataAction.h](../../Plugins/Kata/Source/KataRuntime/Public/Action/KataAction.h)·[KataActionTemplate.h](../../Plugins/Kata/Source/KataRuntime/Public/Action/KataActionTemplate.h): 두 에셋 타입.
- [KataActionEditor.cpp](../../Plugins/Kata/Source/KataEditor/Private/KataActionEditor.cpp): Action 전용 편집 분기.
- [SKataPreviewViewport.cpp](../../Plugins/Kata/Source/KataEditor/Private/SKataPreviewViewport.cpp): `GetPlayableAction`의 임시 액션.
- [KataEditorModule.cpp](../../Plugins/Kata/Source/KataEditor/Private/KataEditorModule.cpp)·[KataActionFactory.cpp](../../Plugins/Kata/Source/KataEditor/Private/KataActionFactory.cpp): Create Child Action 메뉴와 Factory.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 기존 에셋의 `ParentAction` 사용 | 에이전트가 `Content` uasset에서 이름 문자열 검색 | 저장된 값 없음 | 사용자 로컬의 저장소 밖 에셋 |
| 재생 슬롯의 타입 | 소스 읽기 | KataGraph·KataAI·KataFramework·샘플 스포너가 `UKataAction`만 받아 변경 불필요 | 빌드 |
| Editor 빌드 | 사용자 보고 | 통과 | Game 타깃 빌드 |
| Template 사용 | 사용자 보고(`KAT_PC_Attack` 생성·시험) | 정상 동작한다는 보고 | 테스트 가이드 항목별 결과, 기존 에셋 회귀, PIE |

## 남은 제한과 후속 작업

- 기존 Action을 Template으로 바꾸는 변환 도구는 없다. 필요하면 [에셋 이전 안내](../manual/Asset-Migration.md)의 수동 절차를 따른다.
- [#47](https://github.com/jaykop/Kata/issues/47) 측정 스코프 이름이 `UKataActionAssetBase::*`로 바뀌었다.

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [#50](https://github.com/jaykop/Kata/issues/50) | 구현 완료, 사용자 Editor 빌드·Template 시험 보고 |
| [런타임 사용법](../manual/Runtime-Usage.md) | 구성 표, 설정과 상속 규칙 |
| [에디터 사용법](../manual/Editor-Usage.md) | 생성 메뉴, 부모 Template과 변경분, Template 프리뷰 |
| [에셋 이전 안내](../manual/Asset-Migration.md) | 다단계·Action 부모 에셋의 수동 이전 |
| [해석 결과 캐시 계획](../plan/Resolved-Action-Cache-Plan.md) | 체인 단순화와 함수 위치 |
| [액션 에셋 모델 기록](2026-09-25-Action-Asset-Model.md) | 후속 변경 연결 |
| `AGENTS.md` | 상속 규칙 |
