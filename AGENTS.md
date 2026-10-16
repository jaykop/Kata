# Kata 공용 작업 지침

이 파일은 Codex와 Claude를 포함한 모든 코딩 에이전트의 공용 지침이다. `CLAUDE.md`에는 이 문서의 참조만 유지한다. 지침의 변경은 이 파일에서 수행한다.

## 작업 경로

- 유일한 작업 루트는 `C:\Users\jeyko\Documents\Unreal Projects\ProjectKata`다.
- 모든 명령의 작업 디렉터리, 소스 수정, 문서 작성, Git 작업은 이 경로를 기준으로 수행한다.
- `C:\Users\jeyko\Documents\ChatGPT\UE5 액션 플러그인`에는 소스·문서·설치 스크립트의 작업 사본을 만들거나 유지하지 않는다.
- 쓰기 권한이 필요하면 실제 프로젝트 경로에 대한 권한을 요청한다. 다른 폴더에서 복제·스테이징한 뒤 복사하는 방식을 사용하지 않는다.

## 프로젝트와 현재 범위

- Unreal Engine **5.8**, C++ 프로젝트 `ProjectKata`, 플러그인 **Kata**.
- **싱글플레이 전용이며 GAS가 필수**다. 복제, RPC, 클라이언트 예측, NetScope를 추가하지 않는다.
- 재사용 코드는 모두 플러그인에 둔다. **프로젝트 `ProjectKata`는 샘플 전용**이며 재사용 코드를 두지 않는다.
  실행 확인은 샘플 프로젝트의 입력 경로를 사용하며, 테스트 에셋은 `Content/KataSample`에 둔다.
- 현재 플러그인은 코어 `Kata`(모듈 `KataConditions`, `KataRuntime`, `KataGraph`, `KataEditor`, `KataGraphEditor`)와
  통합 `KataFramework`(모듈 `KataFramework`, `KataFrameworkEditor`, UncookedOnly `KataFrameworkAnimGraph`), 위성 `KataTargeting`(모듈 `KataTargeting`)·`KataAI`(모듈 `KataAI`)·`KataCamera`(모듈 `KataCamera`) 다섯 개다.
  KataAI는 AI Data(StateTree·Sense·Targeting Preset 설정), AIController와 Pawn 설정 인터페이스를 제공하고, Pawn·ASC 준비 후 엔진 StateTree AI Component와 Controller별 Perception을 실행한다. 시각 인지 후보 선택·AI 타게팅·StateTree Evaluator를 제공한다. Action·Graph·Action Group 실행 StateTree Task를 제공한다. 어그로·Pressure는 후속 단계다.
  KataTargeting은 팩션 설정·판정, 타게팅 기반·PC 컴포넌트, 부위 단위 타겟 지점 컴포넌트와 지점 펼침 태스크, 대상·방향 결정 Command와 회전 태스크를 제공한다. 몬스터 파생 컴포넌트는 KataAI에서 구현한다.
  KataCamera는 플레이어 카메라 매니저, 단계 파이프라인, 카메라 데이터·Boom Arm·Spline Rail 배치, 장애물 Shrink Feature, 카메라 StateTree와 궤도 공간 블렌드 스택,
  GameplayTag 지정 레일 컴포넌트와 GameplayDebugger 카테고리를 제공한다.
  프로젝트에는 샘플 전용 `ProjectKata` Runtime 모듈과 `ProjectKataEditor` Editor 모듈이 있다. Editor 모듈은 PIE 월드를 대상으로 하는 Kata Debug 탭(치트, 런타임 스포너)처럼
  샘플에서만 쓰는 에디터 도구를 두며, 재사용 기능은 두지 않는다.
- 엔진 Experimental 플러그인 SlateIM은 샘플 프로젝트의 Editor 타깃에서만 활성화한다. `ProjectKataEditor`만 사용하며 Kata 플러그인은 참조하지 않는다.
- 엔진 Experimental 플러그인 `MVVMToolset`은 HUD 위젯 저작용으로 샘플 프로젝트의 Editor 타깃에서만 활성화한다. 엔진 툴셋에 없는 기능은 `ProjectKataEditor`의 `UKataMVVMAuthoringToolset`이 보완하며 Kata 플러그인은 이 툴셋을 참조하지 않는다.
- 엔진 Experimental 플러그인 `ModelContextProtocol`은 에이전트의 에디터 조작(MCP 서버)용으로 샘플 프로젝트의 Editor 타깃에서만 활성화한다. Kata 플러그인은 이 플러그인을 참조하지 않는다.
- 별도 개발 도구 `KataGASInspector`는 단일 `KataGASInspectorEditor` Editor 모듈로 GAS 상태를 탭별 전용 화면에서 읽는다.
  엔진과 GAS에만 의존하며 코어·위성·통합 플러그인은 이 도구를 참조하지 않는다. Game 타깃에는 포함하지 않는다.
- 외부 벤더 플러그인 `WindowsDualsense_ds5w`(Win64 듀얼센스 입력, MIT)는 샘플 프로젝트에서만 활성화한다. Kata 플러그인은 이 플러그인을 참조하지 않는다.
  원본 대비 수정은 플러그인 루트의 `UPSTREAM.md`에 기록한다. 배경은 [#44](https://github.com/jaykop/Kata/issues/44)를 따른다.
- 엔진 플러그인 `ChaosClothAsset`·`ChaosClothAssetEditorCore`·`ChaosClothAssetDataflowNodes`와 Editor 전용 `ChaosClothAssetToolset`은 샘플 캐릭터의 천 시뮬레이션용으로 샘플 프로젝트에서만 활성화한다. Kata 플러그인은 이 플러그인들을 참조하지 않는다.
- 외부 벤더 플러그인 Blockout Tools(`Plugins/Blockout`, 모듈 `BlockoutToolsPlugin`·`BlockoutToolsEditorPlugin`, Fab)는 샘플 테스트 레벨의 블록아웃용으로 샘플 프로젝트에서만 사용한다. Kata 플러그인은 이 플러그인을 참조하지 않는다.
  마켓플레이스 라이선스 때문에 공개 저장소(Git)에서 제외하고 Diversion에서만 추적한다. Git 사본에 없으므로 `.uproject`에 등록하지 않고 프로젝트 플러그인 기본 활성으로 쓴다. 출처와 원본 대비 수정은 플러그인 루트의 `UPSTREAM.md`에 기록한다.
- **플러그인 분리를 결정했다.** 목표 구성은 코어 `Kata`, 위성 `KataTargeting`·`KataAI`·`KataCamera`, 통합 `KataFramework`다.
  구성·의존 결정과 이유는 `docs/devlog/2026-10-10-Plugin-Modularization-Decisions.md`에 있다. 새 시스템 코드는 목표 구조의 해당 플러그인에 둔다.
- **KataAI는 StateTree 기반으로 `KataAI` 플러그인에서 구현한다.** Perception 연결, 어그로, AIController,
  KataAction을 실행하는 StateTree Task가 대상이다. BT(Behavior Tree) 의존성과 어댑터는 추가하지 않는다.
- 기본 조건, Kata 액션 에셋·런타임, 타임라인·프리뷰 에디터, 그래프 자료구조·Kata 고유 노드와 엣지,
  그래프 에디터의 소스가 작성되어 있다. 현재 사용법과 제한은 관련 `docs/manual/` 문서, 작업의 구현·확인 상태는 연결 GitHub Issue를 따른다.

## 설계 판단의 우선순위

1. 사용자의 현재 요청과 명시적으로 확정한 결정.
2. 이 파일의 구조·모듈 경계와 관련 manual에 정리된 현재 사용 계약.
3. 열린 GitHub 이슈와 이슈에서 링크한 `docs/plan/` 설계 문서.

기술 변경 시 영향을 받는 manual·devlog를 갱신하고, 작업 상태는 연결 GitHub Issue에서 관리한다. 폐기된 구현 전 설계 문서를 현재 사양으로 사용하지 않는다.

## 모듈 경계

- `KataConditions`: 공용 조건·Context를 위한 런타임 계층. GAS를 사용할 수 있지만 KataRuntime과 에디터에 의존하지 않는다.
- `KataRuntime`: 액션·콤보·태스크·실행기·GAS 연결을 위한 런타임 계층. KataConditions에 의존한다.
- `KataGraph`: 그래프 자료구조와 콤보 전이를 위한 런타임 계층. KataRuntime에 의존한다.
- `KataEditor`: 액션 편집기·타임라인·프리뷰를 위한 Editor 전용 계층. 런타임 계층을 참조한다.
- `KataGraphEditor`: 그래프 편집기를 위한 Editor 전용 계층. KataGraph를 참조한다.
- 플러그인 간 의존은 위성·통합 플러그인 → 코어 `Kata` 한 방향이다. 코어는 엔진과 GAS에만 의존하고 위성 플러그인을 참조하지 않으며,
  확장 지점(기반 클래스, 인터페이스, 델리게이트)만 제공한다. 추가 엔진 플러그인 의존이 생기는 기능은 코어에 넣지 않는다.
  위성 간에는 계획에 정한 `KataAI` → `KataTargeting` 의존을 허용한다. 여러 플러그인의 조합은 `KataFramework`가 맡으며 코어 역참조·순환 의존은 금지한다.
- 코어에는 Character·Controller 같은 게임 프레임워크 구체 클래스를 두지 않는다. 여러 플러그인을 조합하는 클래스는 `KataFramework`에 둔다.
- 의존 방향을 역전시키거나 순환 의존을 만들지 않는다. UnrealEd, AssetTools, Slate 편집 기능 등 에디터 전용 코드를 런타임에 넣지 않는다.
- 런타임 에셋이 소유해야 하는 편집 전용 데이터는 `WITH_EDITORONLY_DATA`, 데이터 검증과 편집 훅은
  `WITH_EDITOR`로 제한할 수 있다. 에디터 UI와 도구 구현 자체는 Editor 모듈에 둔다.
- 디버그 기능은 그 기능을 구현한 모듈에 둔다. 디버그 시각화, 디버그용 CVar·콘솔 명령, 에디터 디버그 토글이 여기에 해당한다.
  런타임 디버그 코드는 `ENABLE_DRAW_DEBUG`처럼 Shipping에서 빠지는 조건으로 감싸고, 에디터 토글 UI는 해당 플러그인의 Editor 모듈에 둔다.
- 전용 테스트 모듈(ProjectKataTesting)은 제거했다. C++ 자동화 테스트는 대상 모듈의 `Private/Tests/`에 `WITH_DEV_AUTOMATION_TESTS`로 감싸 둔다.
  운영 중 오류 진단에 필요한 로그는 Runtime에 둘 수 있다.
- Public 헤더에서 필요한 의존성만 Public으로 노출하고 구현 전용 의존성은 Private에 둔다. 신규 의존성과 플러그인은 실제 기능에 필요할 때 추가한다.

## 구현 원칙

- 소스 코드의 설명 주석과 API 문서 주석은 한국어로 작성한다. 기술 식별자와 고유 API 이름은 원문 표기를 유지할 수 있다.
- `DisplayName`, `Category`, 명시적 `ToolTip` 등 메타데이터 값과 코드 식별자, 로그·진단 문자열 등 주석 외의 코드 표기는 영어로 유지한다.
- 기존 코드의 주석을 번역할 때 실행 로직, 문자열 리터럴, 리플렉션 메타데이터 및 API 이름은 변경하지 않는다.
- 외부에서 가져온 벤더 코드는 원문 주석을 유지한다. 우리가 고치거나 새로 쓴 부분에만 한국어 주석을 단다. 출처와 변경 내역은 해당 모듈의 `UPSTREAM.md`에 적는다.
- 공개 API 주석은 역할, 입력·출력 계약, 수명, 부작용 및 실패 조건처럼 호출자가 코드만 보고 알기 어려운 내용을 설명한다.
- 구현 주석은 코드의 동작을 그대로 되풀이하지 않고 해당 구현을 택한 이유, 엔진 제약, 경계 조건과 불변식을 기록한다.
- 주석은 주어와 서술어가 분명한 자연스러운 문장으로 작성한다. 번역투, 불완전한 명사형 문장, 의미 없는 영문 용어의 음역은 피한다.
- 주석에서 타입과 함수 이름을 언급할 때는 현재 코드의 정확한 식별자를 사용한다. 리네임이나 동작 변경 시 관련 주석도 함께 갱신한다.

- Unreal 명명 규칙과 타입 접두사 U/A/F/E/I/S를 사용하고 공개 심볼에는 Kata 접두사를 사용한다.
- 새 Kata 콘텐츠는 UKataAction 전용 오브젝트 uasset으로 작성한다. 전용 에디터에 프리뷰 월드, 타임라인, Kata Details, Task Details를 제공한다. Blueprint/CDO 기반 UKataDefinition 경로는 제거했으며 다시 도입하지 않는다.
- 상속은 1단계다. 부모는 재생하지 않는 `UKataActionTemplate`만 될 수 있고, `UKataAction`이 `ParentAction`으로 Template 하나를 참조해 명시적인 프로퍼티 변경분만 보존한다.
  Template은 부모를 갖지 않으며 다단계 상속을 다시 도입하지 않는다. 값이 부모와 같아졌다는 이유로 오버라이드를 자동 삭제하지 않는다.
- 에셋의 설정과 캐릭터별 실행 상태를 분리한다. 공유 에셋·조건 객체에 실행 중 시간, 대상 기록, 이펙트 핸들을 저장하지 않는다.
- 조건 평가는 부작용 없이 수행한다. Attribute 비용 지불, Gameplay Effect 적용, 입력 소비는 실행 처리에서 수행한다.
- 반복 사용하는 조건 판정은 `UKataFL_Condition`의 순수 함수로 공용화한다. 공용 판정 함수(`UKataFL_*`)의 매개변수에는
  Context처럼 실행기가 만드는 Kata 내부 구조체를 사용하지 않으며, 조건 UObject는 설정·Context 변환·Invalid 진단·Invert 책임을 유지한다.
- GAS의 비용·쿨다운·Attribute 시스템을 중복 구현하지 않는다.
- 태그를 받는 프로퍼티와 Blueprint 매개변수는 용도별 태그 루트로 `Categories` 메타를 지정해 선택기를 좁힌다(예: `Trigger`, `Equipment.Slot`).
  플러그인은 태그를 정의하지 않고 루트만 정하며, 정한 루트는 `docs/manual/Gameplay-Tags.md`의 루트 목록에 적는다. 임의 태그를 받아야 하는 범용 항목은 예외로 하고 주석에 이유를 적는다.
- 실행 태스크는 완료·취소·중단·소유자 파괴 시 획득한 자원과 이벤트 구독을 정리하도록 설계한다.
- C++ virtual과 Blueprint 확장 지점을 구분한다. UObject 참조 수명과 GC 추적을 검토한다.
- 에셋 경로·모듈명·반영 타입명을 변경할 때 직렬화된 에셋과 필요한 Redirect를 확인한다.
- 엔진 소스나 생성 파일을 수정해 프로젝트 문제를 우회하지 않는다.
- Kata 코드가 애니메이션 API(AnimInstance, AnimNode, Anim Layer Interface 등)에 직접 묶이는 범위를 늘리지 않는다. 새 C++ AnimNode나 ABP 전용 기능은 꼭 필요할 때만 더하고, 절차적 애니메이션 처리는 Control Rig에 둔다. 엔진의 차세대 애니메이션 프레임워크로 옮길 비용을 줄이기 위해서다.

## 함께 작업할 때

- 시작 전에 `git status --short`와 관련 파일의 현재 내용을 확인한다. 다른 작업자의 미커밋 변경을 덮어쓰거나 되돌리지 않는다.
- 요청 범위에 필요한 변경만 수행하고, 같은 파일을 변경하는 작업이 발견되면 현재 상태를 확인한 뒤 이어서 작업한다.
- 문서와 완료 보고는 기본적으로 한국어로 작성한다. 코드 식별자는 영어를 사용한다.
- 대화에서 그림·도식으로 설명할 때는 텍스트 그림(ASCII)을 쓰지 않고, 에이전트 환경이 제공하는 시각화 도구를 사용한다.
  Claude는 `show_widget`(visualize)을 사용한다. 시각화 도구가 없는 환경에서만 텍스트로 설명한다.
- 대화에서 사용자에게 설명할 때는 결론과 핵심 몇 줄만 쓰고, 사용자가 되물으면 그 부분을 답한다. 배경·근거 목록·선택지 표를 한꺼번에 내지 않는다.
  파일에 남는 내용(문서, 리포트, 커밋, 이슈)과 사용자에게 결정을 묻는 질문은 온전한 문장으로 쓴다.
- 대화에서 이슈 번호나 기록 ID(예: `#71`, `TD-D-004`, `Q-0016`)를 말할 때는 뒤에 괄호로 제목을 붙인다. 커밋 메시지처럼 형식이 정해진 곳은 그 형식을 따른다.
- 사용자 요청에 없는 기능이나 미확정 설계 결정을 구현으로 확정하지 않는다.
- 커밋에는 변경 이유와 검증 결과를 추적할 수 있는 설명을 사용한다. 공개 저장소에는 비밀 값과 개인 로컬 설정을 추가하지 않는다.

## AI 작업 플러그인

- 작업 절차 스킬은 공용 마켓플레이스 [jaykop/claude-plugins](https://github.com/jaykop/claude-plugins)의 플러그인으로 설치한다.
  `workflow`는 `next-task`·`feature-impl`·`feature-doc-sync`·`commit-doc-review`·`context-handoff`를, `ko-review`는 `polish`·`record`·`review`를 제공한다.
  Claude Code와 Codex가 같은 스킬을 쓴다. 맞춤법·문체 스킬은 `DaleSeo/korean-skills`에서 설치한다.
- `advisors`(Claude Code 전용)는 기획·기술 동향·시장·최적화·PM·TD 고문을 제공한다. 고문은 판단과 제안까지만 하고 리포트는 추적하지 않는 `.claude/advisors/reports/`에 둔다.
  PM 고문(`/advisors:pm`)은 다음 작업, 주간 점검, 마일스톤 계획·회고, 위험 목록, 고문 정기 점검표를 맡는다. 이슈 순서 확정과 게시는 `next-task`로 넘긴다.
  PM 고문의 기준 노트는 `docs/localdocs/pm/`에 둔다. 강의 자료 요약이므로 공개 문서에 옮기지 않는다.
  TD 고문(`/advisors:td`)은 코드 구조·설계 선택·기술 부채를 맡는다.
- `.claude/settings.json`이 마켓플레이스와 사용할 플러그인을 지정한다. 저장소를 연 Claude Code가 설치를 안내한다.
  Codex는 `codex plugin marketplace add jaykop/claude-plugins`로 추가한 뒤 플러그인을 설치한다.
- Kata의 값은 `.claude/workflow/profile.md`, `.claude/ko-review/profile.md`, `.claude/advisors/profile.md`, Kata 전용 금지어는 `.claude/ko-review/terms.json`에 둔다.
  이 파일들은 Git과 Diversion에서 추적한다. 하루 작업 가능 시간 같은 개인 값은 추적하지 않는 `.claude/workflow/profile.local.md`에 둔다.
- 규칙이 겹치면 이 문서가 플러그인 스킬보다 우선한다. 플러그인 자체의 수정은 플러그인 저장소에서 하고, Kata 쪽은 프로필만 고친다.

## 커밋과 저장소별 추적 범위

- 사용자가 "커밋해"라고 요청하면 해당 작업을 GitHub와 Diversion 양쪽에 커밋·업로드하라는 뜻으로 해석한다. 한쪽만 요청하거나 로컬 커밋만 지정하면 그 범위를 따른다.
- GitHub는 `jaykop/Kata`의 현재 작업 브랜치에 커밋하고 push한다. 기본 브랜치 반영·병합은 별도 요청을 따른다. Diversion은 현재 `ProjectKata` 브랜치에 커밋하고 클라우드 동기화 완료를 확인한다.
- GitHub는 소스·설정·공유 문서를 추적하고 프로젝트 루트 `/Content/`는 제외한다. Diversion은 이 항목들과 `/Content/`의 `.uasset`·`.umap`, HKX 변환 도구, `docs/localdocs/`, 기획 문서 `docs/design/`, Blockout Tools 플러그인 `Plugins/Blockout/`을 포함한다. Git의 파일 목록만으로 Diversion 커밋 대상을 정하지 않는다.
- `.dvignore`에서 Git의 `/Content/`·`Scripts/HkxConverter/`·`docs/localdocs/`·`docs/design/`·`Plugins/Blockout/` 제외를 명시적으로 해제한다.
  변환 도구, 로컬 문서(작업 인계 기록·초안·변환 도구 문서·원본 게임 분석 문서 `docs/localdocs/animation/`), 기획 문서와 마켓플레이스 플러그인은 공개 저장소에 올리지 않고 Diversion에서만 관리한다. 생성 파일·인증 정보·로컬 에이전트 설정·Python 캐시는 양쪽 모두 제외한다.
  `.claude/`에서는 플러그인 지정과 공유 프로필(위 "AI 작업 플러그인")만 양쪽에서 추적한다.
- 작업에 해당하는 변경만 커밋한다. 다른 작업자의 미커밋 변경과 무관한 에셋을 일괄 포함하지 않는다. 양쪽 커밋 ID, 업로드·동기화 결과 및 미반영 항목을 보고한다.
- Diversion `ProjectKata`는 앱의 `Jay Kop's organization`에 있다. 목록에서 보이지 않으면 조직 선택과 저장소 ID를 먼저 확인한다. 저장소 ID는 `dv.repo.08d85374-f8c5-487f-b151-da7c3d69f081`이다.

## 작업 추적 (GitHub Issue)

- 진행 중인 작업 단위의 기준은 GitHub Issue(`jaykop/Kata`)다. 여러 파일에 걸치는 기능·개선·버그 수정과
  설계 결정이 필요한 작업은 이슈 하나에 대응시킨다. 오탈자, 수치 조정, 단일 파일의 단순 수정은 이슈 없이 진행할 수 있다.
- 새 작업을 시작하거나 기존 작업을 수정할 때 먼저 열린 이슈를 찾아 이어 쓴다. 맞는 이슈가 없을 때만 새 이슈를 만든다.
- **저장소는 공개다.** 이슈 본문과 댓글에는 목표, 범위, 상태, 확정된 결정 요약과 관련 plan·devlog·커밋 링크만 적는다.
  시행착오, 로컬 경로, 개인 설정, 세션 메모는 에이전트의 로컬 인계 메모에 둔다.
- 이슈에 설계 상세를 복사하지 않는다. 설계와 미확정 사항은 plan, 결정 이유는 devlog, 완료된 현재 상태는
  관련 manual에 기록하고 이슈에서 링크한다.
- 이슈 생성, 댓글, 라벨 변경, 닫기는 외부에 게시되는 작업이다. 에이전트는 초안을 보여 주고 사용자 확인을 받은 뒤 수행한다.
  Discussion의 생성, 댓글, 이슈 전환도 같다. Discussion에도 이슈와 같은 공개 기준을 적용한다.
- 이슈가 있는 작업의 커밋 메시지에는 `#번호`를 적는다. `Closes #번호`처럼 이슈를 자동으로 닫는 키워드는
  사용자가 완료를 확인한 경우에만 사용한다.
- 구현 완료와 실행 확인 완료를 구분한다. 구현만 끝나고 사용자 빌드·실행 확인 전인 이슈는 `status: needs-verification`
  라벨을 붙여 열어 두고, 사용자가 확인한 뒤 닫는다.
- 라벨은 종류(`type: feature`·`type: improvement`·`type: bug`·`type: docs`), 상태(`status: in-progress`·
  `status: needs-verification`·`status: blocked`), 대상 플러그인(`plugin: kata`·`plugin: targeting`·`plugin: ai`·
  `plugin: camera`·`plugin: framework`·`plugin: world`), 중요도(`priority: must`·`should`·`could`),
  규모(`size: S`·`M`·`L`·`XL`), 등급(`grade: A1`~`C2`)으로 나눈다.
- 기간 목표는 GitHub Milestone으로 관리한다. 마일스톤 설명에는 목표, 완료 기준, 용량만 적는다.
  마일스톤에 없는 이슈는 백로그이며, 샘플이나 진행 중인 작업이 요구할 때 마일스톤으로 옮긴다.
  마일스톤을 닫을 때 Discussions의 Announcements 카테고리에 목표, 완료 기준 결과, 관련 이슈·devlog 링크를 올린다.
- 우선순위는 중요도 a와 규모 b의 곱으로 정한다.
  - a는 이슈가 속한 마일스톤 목표를 기준으로 매기고, 백로그는 Kata 범용 기반 목표를 기준으로 한다.
    필수 3은 없으면 완료 기준을 채울 수 없는 작업과 진행을 막는 버그, 중요 2는 결과의 질이나 재작업 비용을 크게 바꾸는 작업,
    선택 1은 편의·정리 작업이다.
  - b는 사용자가 직접 손대는 시간(빌드·확인, 에디터 작업, 결정)이다. S 3은 1시간 이하, M 2는 3시간 이하, L 1은 8시간 이하다.
    8시간을 넘는 XL은 등급 없이 sub-issue로 나누고 상위 이슈는 묶음으로만 쓴다. 불확실한 작업은 한 단계 크게 잡는다.
  - 등급은 9가 A1, 6이 A2, 4가 B1, 3이 B2, 2가 C1, 1이 C2다. 같은 등급이면 a가 높은 쪽이 먼저다.
    C 등급은 마일스톤에 두지 않고 쪼개거나 백로그로 보낸다.
- 선행 관계는 GitHub Blocked by로 연결한다. 선행 작업은 자기가 막는 작업 중 가장 높은 등급을 물려받고,
  선행 작업이 열린 이슈는 대기로 둔다.
- "다음 뭐 해" 같은 다음 작업 요청은 PM 고문(`/advisors:pm 다음 작업`)으로 시작한다. PM 고문은 이슈, 때가 된 고문 점검, 결정 시점이 다가온 위험 가운데 하나를 추천하고, 이슈면 `next-task`로 넘긴다.
- 이슈 순서는 workflow 플러그인의 `next-task`(`issue_priority.py`)로 확인한다([사용법](docs/manual/Issue-Priority.md)). 정렬은 마일스톤 마감 순, 작업 가능 여부, 유효 등급, a, 번호 순이다.
  규모 합산은 칸의 가운데 값(S 1, M 2, L 5.5시간)을 쓰고, 필수 작업의 합이 마일스톤 용량의 60%를 넘으면 목표를 줄인다.
  `--apply`로 grade 라벨을 맞추는 것은 게시 작업이다.
- 이슈 초안에는 a·b·선행 작업을 함께 적는다. 마일스톤을 시작하거나 옮길 때, 범위가 바뀔 때 다시 매긴다.
- plan에만 있고 이슈가 없는 후속 작업은 잊히지 않도록 백로그 이슈로 만든다.
- 할 일로 확정하지 않아 a·b를 매길 수 없는 아이디어는 이슈로 만들지 않고 Discussions의 Ideas 카테고리에 둔다.
  이슈 목록에는 채점할 수 있는 작업만 남긴다. 아이디어를 할 일로 확정하면 Discussion에서 이슈를 만들고 a·b·선행 작업을 매긴다.

## 테스트·검사 담당 원칙

- **테스트, 빌드, 검증 및 별도 코드 검사는 사용자가 직접 담당한다.** 사용자가 해당 작업을 명시적으로 요청한 경우에만 에이전트가 수행한다.
- C++ 자동화 테스트는 입력과 결과로 확인할 수 있는 로직(유닛 테스트)과, 시작·취소·중단을 반복해도 자원과 구독이 남지 않는다는 불변식(반복 테스트)에 쓴다.
  시간을 단언하는 성능·부하 테스트 코드는 작성하지 않는다. 성능은 측정으로, 블루프린트·에셋·UI·연출은 플레이로 확인한다.
- 기능 구현 요청만으로 빌드·자동화 테스트·lint·정적 분석·별도 리뷰를 실행하거나 테스트 코드를 추가·확장하지 않는다.
  테스트 코드는 이슈로 정한 대상이나 사용자 요청이 있을 때만 추가한다.
- 검증을 위한 프로젝트 사본이나 별도 테스트 환경을 만들지 않는다. 반복 점검과 광범위한 검사로 토큰을 소비하지 않는다.
- 구현에 필요한 관련 파일 읽기와 기존 변경 파악은 최소한으로 수행한다. 다른 작업자의 변경은 보존한다.
- 완료 보고에는 구현 내용과 실제 검증 여부만 간결하게 적는다. 실행하지 않은 검사를 성공했다고 보고하지 않는다.
- 검사를 실행해 달라는 요청이 없다면 자동으로 이어서 실행하지 않는다.
- GitHub Actions는 push·pull_request마다 `Scripts/CheckAgentsRules.py`로 모듈 경계·현재 범위 규칙 중 텍스트로 판정할 수 있는 것(플러그인 의존 방향, 런타임 모듈의 에디터 의존, 샘플 전용 플러그인·모듈 참조, 복제·RPC 선언)을 자동으로 검사한다([사용법](docs/manual/Agents-Rules-Check.md)).
  컴파일과 테스트는 CI에서 하지 않으며 CI 결과는 사용자의 빌드·실행 확인을 대신하지 않는다. 에이전트는 사용자가 요청할 때만 이 스크립트를 로컬에서 실행한다.
  플러그인 구성이나 이 규칙을 바꾸면 스크립트의 표와 manual을 같은 작업에서 갱신한다.

사용자가 직접 빌드할 때 프로젝트 루트에서 다음 명령을 사용할 수 있다.

```powershell
.\Scripts\Build.ps1 -Target Editor
.\Scripts\Build.ps1 -Target Game
```

- 에디터나 사용자 프로세스를 강제로 종료하지 않는다.
- `Binaries`, `Intermediate`, `Saved`, DDC, 생성 솔루션 파일은 커밋하지 않는다. `.uasset`·`.umap`은 기존 Git LFS 설정을 사용한다.
- `Content/KataSample`는 테스트 전용이며 cooked 빌드에 포함하지 않는다.

## 로컬 작업 인계 기록

- 새 작업을 시작할 때 작업 단위로 docs/localdocs/threads/<이슈번호>-<slug>.md를 만들거나 기존 파일을 갱신한다. 이슈가 없으면 <slug>.md를 쓴다.
- 목표, 현재 상황, 연결 이슈, 결정 사항과 다음 행동을 기록하고 작업 중 중요한 변경 및 종료 시 갱신한다. 같은 작업은 같은 파일을 이어 쓴다.
- 기존 작업 파일을 먼저 읽어 맥락을 복원한다. 완료 기록은 사용자 요청 시 docs/localdocs/archive/<YYYY-QN>/로 옮긴다.
- docs/localdocs/는 Git 추적에서 제외하고 Diversion에서 추적한다. 인계 기록은 공개 이슈의 작업 상태를 대체하지 않는다. 인증 정보와 비밀 값은 이 폴더에도 적지 않는다.

## 설명 변경의 공유 검토

- 자체 소스의 주석, `docs/manual/`, 태그 ini의 `DevComment`를 추가·변경하거나 로직 변경으로 기존 설명에 영향을 주는 작업은 작업별 `docs/localdocs/threads/<이슈번호>-<slug>.descriptions.json`에 검토 항목을 남긴다. 이슈가 없으면 번호를 생략한다. 이 목록은 Git에서 제외하고 Diversion에서 추적한다.
- 수정 전에 ko-review 플러그인의 `record` 스킬(`review_queue.py start`)로 명시적 작업 파일의 시작 상태를 기록한다. 같은 작업은 기존 목록을 이어 쓰고 새 파일은 수정 전에 `scope`로 추가한다. 종료 시 문서 반영 여부와 함께 `capture`·`register`로 설명 위치, 변경 이유와 관련 구현을 기록한다. 설명을 고치지 않은 로직 변경도 영향 항목을 넣으며, 영향이 없으면 `no-impact`에 이유를 남긴다. 문서화 후속 질문이나 사용자 빌드 확인을 기다리는 동안에도 기록을 미루지 않는다.
- Claude는 작성·영향 기록을 담당하고, Codex는 대기 항목과 주변 문맥·관련 구현을 검토한다. 설명·근거가 바뀌면 해당 항목을 다시 대기로 돌린다. 기록 자체를 문장 품질이나 빌드·실행 확인의 근거로 쓰지 않는다.
- Codex 검토는 커밋 직전, `commit-doc-review`보다 먼저 Claude가 넘긴다. 커밋할 작업의 목록에 대기 항목이 있으면 `codex exec`로 그 작업의 목록 파일 하나만 지정해 ko-review의 `review` 스킬을 실행하게 하고, 수정 범위를 그 목록의 `taskPaths`로 제한한다. 여러 세션이 같은 작업 트리를 쓰므로, Codex가 끝나면 diff와 검토 결과를 사용자에게 보여 주고 해당 경로에서 이 작업에 속하는 변경 구간만 스테이징한다. `finish`가 거절되면 원인을 확인한다. 직접 고친 설명의 최종 해시를 빠뜨렸다면 최종 본문을 다시 읽고 `--description-hash`를 넘겨 완료 기록을 다시 시도한다. 근거가 바뀌었거나 다른 작업자가 설명을 고쳤다면 사용자에게 알리고, 같은 Codex 호출 안에서 현재 내용을 읽고 `begin`부터 다시 검토한다. 한 번의 커밋에서 Codex는 한 번만 부른다. 검토 뒤에도 대기 항목이 남으면(Codex가 고친 설명이 다른 항목의 근거인 경우) 다시 부르지 않고 사용자에게 보여 준다. 이 경우와 사용자 요청 외에는 다른 AI를 실행하거나 메시지를 보내지 않는다. ko-review 스킬의 "검토 요청은 사용자가 전달한다"보다 이 규칙이 우선한다.
- 사용법과 기록 형식은 [설명 변경 검토](docs/manual/Description-Review.md)를 따른다. 작성 쪽은 workflow의 `feature-impl`·`feature-doc-sync`와 ko-review의 `record`·`polish`, 검토 쪽은 ko-review의 `review`를 쓴다. 도구를 쓸 수 없으면 동일한 형식으로 직접 기록하고 제한을 보고한다. 목록이 없다는 이유로 전체 프로젝트를 자동 검토하지 않는다.

## 작업 상태 기록

작업의 구현 범위, 진행·완료 상태, 사용자 확인 여부와 남은 작업은 GitHub Issue를 기준으로 관리한다. 이슈 게시 절차는 위의 사용자 확인 규칙을 따른다.
현재 사용법과 제한은 manual, 중요한 변경과 결정 이유는 devlog에 기록한다. 별도의 전체 구현 상태 요약 문서는 유지하지 않는다. 사용자의 새로운 결정이 이 지침과 충돌하면 지침과 해당 문서만 수정한다.

## 문서 작성과 동기화

- 문서 진입점과 전체 목록은 `docs/README.md`다. 새 문서를 만들거나 경로를 바꾸면 목록과 관련 링크도 갱신한다.
- 문서 작성·수정 전에 해당 카테고리의 템플릿을 읽고 적용한다.
  - 변경·결정·진단 기록: `docs/devlog/_Template.md`.
  - 현재 기능의 사용법: `docs/manual/_Template.md`.
  - Large 이슈의 설계 문서: `docs/plan/_Template.md`.
- 템플릿은 해당 폴더에 둔다. 새 문서는 템플릿을 복사하고 안내문·자리표시를 제거한다.
  기존 문서는 수정하는 범위부터 필요한 항목을 적용하며, 형식을 맞추기 위해 전체를 다시 쓰지 않는다.
- 샘플 게임의 기획 문서(레벨 제작 절차, 레벨 메트릭, 레벨별 도안)는 Kata 사용법이 아니므로 `docs/design/`에 둔다. 이 폴더는 Git에서 제외하고 Diversion에서만 추적한다.
  절차와 레벨 문서 양식은 `docs/design/Level-Process.md`를 따른다. 공개 문서(manual·devlog·plan)에서는 이 폴더의 파일에 링크하지 않는다.
- 진행 중인 작업과 완료·확인 대기 상태의 진입점은 GitHub Issue다. manual·devlog·plan에 이슈 상태를 복사해 누적하지 않는다.
- devlog에는 무엇을 바꿨고 왜 결정했는지, manual에는 현재 어떻게 사용하는지, plan에는 설계 결정이 필요한 이슈의
  설계안과 미확정 사항을 기록한다. 같은 상세 설명을 여러 문서에 복사하지 말고 원문에 링크한다.
- plan에는 진행 상태를 적지 않고 연결 이슈를 적는다. 이슈를 닫으면 결정 이유를 devlog로 옮기고 plan을 삭제한다.
- API·UI·설정·실행 계약·기존 에셋 처리 방법이 바뀌면 관련 manual을 같은 작업에서 갱신한다.
  관련 이슈의 상태와 라벨도 실제 상태로 맞추고 결과 기록을 연결한다. 구현 완료와 실행 확인 완료는 구분한다.
- 구조 변경, 호환성 변경, 반복될 수 있는 문제의 해결 이유, 중요한 사용자 결정과 진단은 주제별 devlog로 남긴다.
  단순 오탈자나 사소한 수정마다 새 기록을 만들 필요는 없다. 영향 없는 카테고리의 문서를 억지로 추가하지 않는다.
- 과거 기록의 당시 판단은 보존하고 후속 변경·대체 문서를 연결한다. 제안을 사용자 확정 결정으로 바꾸거나,
  기록에 없는 날짜·이유·검증 결과를 추정해 채우지 않는다. plan의 원래 질문은 결과와 연결해 추적 가능하게 남긴다.
- 갱신일은 해당 문서를 실제로 수정한 날짜다. 소스 구현, 문서 대조, 사용자 빌드 확인, 실행·UI 확인을 구분하고
  근거가 있는 대상과 시점만 기록한다. 과거 성공을 이후 미커밋 변경의 검증 결과로 사용하지 않는다.
- 완료 전 요청 범위의 문서 반영 여부를 정리한다. 미반영 문서가 있으면 대상·이유·후속 작업을 기록한다.
  이 문서 작성 절차는 빌드·테스트·별도 코드 검사의 실행 권한을 부여하지 않는다.


