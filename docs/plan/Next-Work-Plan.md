# 다음 작업 계획 — Kata 에셋 에디터 이후

갱신: 2026-09-20

이 문서는 기존 Blueprint 클래스 중심 구현 계획을 대체한다.
현재 소스 상태는 [Implementation-Status.md](../devlog/Implementation-Status.md), 조작 방법은
[Editor-Usage.md](../manual/Editor-Usage.md)와 [Runtime-Usage.md](../manual/Runtime-Usage.md)를 따른다.

## 유지할 구조

UKataAction(전용 객체 uasset) → UKataResolvedAction(병합된 설정) → UKataActionInstance(실행 상태).
원본은 ParentAction과 프로퍼티 변경분으로 상속한다.
편집기는 Preview·Timeline·Kata Action Details·Timeline Details를 제공한다.
Blueprint/CDO 기반 UKataDefinition 경로는 제거했다. 이를 다시 도입하지 않는다.

## 우선순위

1. 사용자가 에셋 편집·프리뷰 실행과 bSingleFrame 동작을 확인한다.
   Editor 빌드와 Kata Action 에셋 생성은 확인했다. 그 밖의 회귀 여부는 아직 미확인이다.
   에이전트는 명시적인 요청 없이는 테스트·빌드·별도 검사·리뷰를 수행하지 않는다.
   전달받은 컴파일 오류 또는 실제 사용 중 문제를 우선 수정한다.
   프리뷰 뷰 방향·노출과 Target Translate 기즈모에는 아직 사용자 확인이 필요한 버그가 남아 있다.
2. 실제 편집 피드백에 따라 타임라인 조작을 확장한다.
   다중 선택, 복사·붙여넣기, 태스크 표시 색·주석, 편집기 전용 트랙 그룹은 구현했다.
   다음 후보는 의존성 대상 선택 UI와 프리뷰 캐릭터 설정 편의성이다.
3. 프로젝트에서 필요한 기본 태스크를 추가한다.
   후보는 Gameplay Effect, 게임플레이 이벤트, 히트 판정, VFX/SFX다. 태스크별 자원 회수와 프리뷰 실행 특성을 함께 설계한다.
4. 콤보 그래프 에셋 KataGraph에 고유 타입을 채운다.
   UKataNode, UKataEntryNode, UKataEdge, UKataTask_TransitionWindow와 UKataGraphInstance/UKataGraphComponent 실행 연결을 구현했다.
   전이는 입력 직접 수신이 아니라 트리거 이벤트 태그로 받는다. 수용 구간은 액션 타임라인의 창 태스크가 연다.
   트리거 버퍼는 1단계에서 끄고(도착 프레임에만 유효) 실제로 눌러본 뒤 켠다.
   SubGraph·Conduit·Alias는 현재 표현력으로 부족한 실제 그래프 사례가 확인될 때 다시 검토한다.

위 후보 전체를 자동으로 구현하라는 지시는 아니다. 다음 요청의 범위에 맞춰 진행한다.
AfterMeshPose, 다중 액션 채널, 네트워크·예측, KataAI는 현재 범위 밖이다.
월드 실행 Subsystem은 인스턴스 순차 진행까지 구현했으며 Gather/Commit 다단계 실행은 후속 필요가 생길 때 확장한다.

## 공용 작업 규칙

- 모든 작업은 ProjectKata 루트에서 한다.
- AGENTS.md를 따른다. 주석은 한국어, DisplayName·Category·ToolTip·식별자·진단 문자열은 영어다.
- 테스트 및 스트레스 테스트 코드를 추가하지 않는다. 검증은 사람이 담당한다.
- 작업 중인 기존 콘텐츠와 미커밋 변경을 보존한다.
- 변경 후 실제 구현 상태 문서를 갱신하고, 검증하지 않은 내용을 검증 완료로 보고하지 않는다.
