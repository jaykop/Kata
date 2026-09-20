# 다음 작업 계획 — Kata 에셋 에디터 이후

갱신: 2026-09-20

이 문서는 기존 Blueprint 클래스 중심 구현 계획을 대체한다.
현재 소스 상태는 [Implementation-Status.md](../devlog/Implementation-Status.md), 조작 방법은
[Editor-Usage.md](../manual/Editor-Usage.md)와 [Runtime-Usage.md](../manual/Runtime-Usage.md)를 따른다.

## 유지할 구조

UKataAction(전용 객체 uasset) → UKataResolvedAction(병합된 설정) → UKataActionInstance(실행 상태).
원본은 ParentAction과 프로퍼티 변경분으로 상속한다.
편집기는 Preview·Timeline·Kata Details·Task Details를 제공한다.
Blueprint/CDO 기반 UKataDefinition 경로는 제거했다. 이를 다시 도입하지 않는다.

## 우선순위

1. 사용자가 에셋 편집·프리뷰 실행과 bSingleFrame 동작을 확인한다.
   Editor 빌드와 Kata Action 에셋 생성은 확인했다. 그 밖의 회귀 여부는 아직 미확인이다.
   에이전트는 명시적인 요청 없이는 테스트·빌드·별도 검사·리뷰를 수행하지 않는다.
   전달받은 컴파일 오류 또는 실제 사용 중 문제를 우선 수정한다.
   프리뷰 뷰 방향·노출과 Target Translate 기즈모에는 아직 사용자 확인이 필요한 버그가 남아 있다.
2. 실제 편집 피드백에 따라 타임라인 조작을 확장한다.
   후보는 다중 선택, 복제·복사 붙여넣기, 트랙 그룹, 의존성 대상 선택 UI, 프리뷰 캐릭터 설정 편의성이다.
3. 프로젝트에서 필요한 기본 태스크를 추가한다.
   후보는 Gameplay Effect, 게임플레이 이벤트, 히트 판정, VFX/SFX다. 태스크별 자원 회수와 프리뷰 실행 특성을 함께 설계한다.
4. 콤보 그래프 에셋 KataGraph를 구현한다.
   노드는 UKataNode(액션 참조), 엣지는 UKataEdge이며 전이는 입력 직접 수신이 아니라 트리거 이벤트 태그로 받는다.
   GenericGraph(MIT)를 Kata 플러그인 안으로 흡수하고 접두사를 Kata화하며, Edges의 TMap을 TMultiMap으로 패치한다.
   모듈은 별도 플러그인이 아니라 Kata 플러그인 안의 KataGraph·KataGraphEditor다. 입력 버퍼 정책은 아직 미확정이다.

위 후보 전체를 자동으로 구현하라는 지시는 아니다. 다음 요청의 범위에 맞춰 진행한다.
AfterMeshPose, 다중 액션 채널, 네트워크·예측, KataAI, 전역 Subsystem은 현재 범위 밖이다.

## 공용 작업 규칙

- 모든 작업은 ProjectKata 루트에서 한다.
- AGENTS.md를 따른다. 주석은 한국어, DisplayName·Category·ToolTip·식별자·진단 문자열은 영어다.
- 테스트 및 스트레스 테스트 코드를 추가하지 않는다. 검증은 사람이 담당한다.
- 작업 중인 기존 콘텐츠와 미커밋 변경을 보존한다.
- 변경 후 실제 구현 상태 문서를 갱신하고, 검증하지 않은 내용을 검증 완료로 보고하지 않는다.
