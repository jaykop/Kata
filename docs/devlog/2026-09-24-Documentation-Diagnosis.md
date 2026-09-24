# 문서 부채와 분류 진단

작성: 2026-09-24  
갱신: 2026-09-24  
유형: 진단 및 문서 운영 변경 기록  
대상: 루트 README·AGENTS와 docs 전체 문서  
기준: 2026-09-24 작업 트리. 기존 소스와 Implementation-Status의 미커밋 변경을 포함한 문서 상태.

## 배경과 결론

문서 분류는 있으나 내용이 함께 갱신되지 않는 문제를 진단하고 카테고리별 템플릿과 작성 지침을 추가했다.
아래 부채 목록은 이번 정비 전후의 차이를 표시한 진단 기록이며, 기능 구현 계획을 확정하는 문서는 아니다.

문서 전체가 방치된 상태는 아니다. `Implementation-Status.md`와 매뉴얼 일부에는 최근 기능이 반영되어 있다.
그러나 구현 상태에 적은 변경이 사용법·상위 계획·요청 메모까지 전달되지 않아 서로 다른 시점의 내용이 공존한다.
상태 문서 하나에 현재 사양, 변경 이유, 오류 대응과 검증 이력이 쌓이면서 필요한 정보를 찾기도 어려워졌다.

해결 우선순위는 **사용을 방해하는 오류 → 잘못된 후속 계획 → 결정·이전 방법 정리 → 문서 구조 정돈**이다.
누락된 과거 작업일이나 확인 결과를 추정해 개발 일지를 재구성해서는 안 된다.

## 발견 내용과 정비 우선순위

높음은 잘못된 사용·중복 구현을 유발하는 항목, 보통은 확장과 유지보수를 어렵게 하는 항목이다.
아래의 근거는 문서 간 대조이며, 런타임 예제의 헤더·함수 선언만 관련 소스를 제한적으로 읽었다.

| ID | 우선순위 | 근거와 발견 내용 | 남겨야 할 기록·완료 조건 | 이번 반영 |
|---|---|---|---|---|
| D01 | 높음 | [루트 README](../../README.md)는 콤보를 후속 범위로 적고 모듈 목록에서 KataGraph·KataGraphEditor·ProjectKataTesting을 누락한다. ProjectKata를 개발·검증용 모듈로 설명한다. [현재 상태](Implementation-Status.md)의 구조와 다르다. | 루트 소개와 모듈 표를 현재 구조로 정리하고, 확인 범위는 상태 문서에 연결한다. | 미반영 |
| D02 | 높음 | [Runtime-Usage](../manual/Runtime-Usage.md)의 예제는 `Definition/KataAsset.h`를 포함하지만 현재 에셋 헤더는 `Action/KataAction.h`다. 기본 태스크도 Play Montage만 안내한다. | 현재 헤더·의존성·준비 조건으로 최소 실행 예제를 정리한다. 기존 이벤트·GE·Loose Tag·전이 창 태스크의 설정, 대상 ASC, 지속·정리 규칙을 설명한다. | 미반영 |
| D03 | 높음 | [Editor-Usage](../manual/Editor-Usage.md)는 자체 Perspective/Top/Right/Back 버튼, Self 방향 기준 Back, 클래스 미지정 시 구체, Target -X 200cm를 설명한다. [현재 상태의 프리뷰 절](Implementation-Status.md#프리뷰)은 엔진 툴바·월드 축 뷰·빈 액터·-X 500cm를 기록한다. | 카메라·배치·벽 표시·재생 종료 Reset·Repeat·시간 탐색을 현재 조작 기준으로 다시 쓴다. Add Task 목록도 함께 갱신한다. UI 실행 확인 전에는 소스·상태 기록 기준임을 표시한다. | 미반영 |
| D04 | 높음 | Next-Work-Plan(2026-09-24 삭제, 이슈로 대체)은 GE·게임플레이 이벤트를 추가 후보로, Conduit을 재검토 대상으로 둔다. [상태 문서](Implementation-Status.md)는 이들의 구현을 기록한다. | 완료한 범위와 남은 후보를 분리하고 결과 기록에 연결한다. Conduit은 구현, SubGraph·Alias는 보류 근거와 재검토 조건을 구분한다. | 진단 링크만 추가; 항목 정비는 미반영 |
| D05 | 높음 | [Base-Task-Plan](../plan/Base-Task-Plan.md)은 검토 당시 태스크 두 개를 전제로 하고 `FKataTargetSpec` 선행 도입을 제안한다. 실제 상태는 `EKataTaskTargetSource`를 사용한다. “입력 소비·버퍼는 이미 다룬다”는 서술도 입력 버퍼 미구현 기록과 다르다. | 당시 제안은 보존하고 현재 적용 결과를 덧붙인다. 대상 선택 구현과 미구현 출력 채널·프리뷰 정책·버퍼를 구분하고 항목별 상태를 기록한다. | 미반영 |
| D06 | 보통 | Kata, KataAction, KataGraph 요청 메모는 질문만 있고 처리 상태·결론 링크가 없다. Base-Task-Plan이 인용하는 KataAction 1번 질문도 현재 내용과 다르다. | 원래 질문을 보존하면서 안정적인 항목 ID, 완료·보류·미확정 상태와 결과 링크를 붙인다. 바뀌는 질문 번호에 의존한 참조를 고친다. Comment 위치·검색 요청은 근거 확인 전까지 완료로 단정하지 않는다. | 2026-09-24 메모 삭제로 대체. 남은 Comment 위치 항목은 Next-Work-Plan으로 이동(이후 이슈 #3) |
| D07 | 보통 | [조건 매뉴얼](../manual/Conditions.md)에는 공용 `UKataFL_Condition` 사용 안내가 없고 “이번 변경에 에디터는 포함하지 않는다”는 과거 작업 표현과 영어 예제 주석이 남아 있다. | `CheckAngle`·`CheckDistance`·`CheckTag`·`CompareValue`의 용도와 오류 반환, UObject 조건과의 책임 차이를 추가한다. 현재 사용법에서 과거 작업 표현을 걷어내고 예제 주석을 지침에 맞춘다. | 미반영 |
| D08 | 보통 | 상태 문서에 에셋 모델 변경, 쿨다운 필드 폐기, Loop 필드 제거와 테스트 모듈 Redirect가 흩어져 있다. 조건 매뉴얼에는 Distance 이전 설정을 수동 재지정해야 한다고 적혀 있다. | `manual/Asset-Migration.md` 등의 이전 안내에 변경 전후 타입·필드, 자동 처리 여부, 수동 재설정, 사용자 확인 범위를 모은다. 알 수 없는 구버전 에셋 복구 경로를 보장하지 않는다. | 미반영 |
| D09 | 보통 | [현재 상태](Implementation-Status.md)에 소유권·실행 순서·그래프 전이·프리뷰 제약과 결정 이유가 함께 누적되어 있다. [BP Task 진단](Function-Library-and-Blueprint-Task-Diagnosis.md)의 제작 경로는 사용법으로 찾기 어렵다. | 아래 주제별 기록을 추출하고 현재 확장 계약을 manual에 연결한다. 상태 요약에서 상세 설명을 줄이는 작업은 이동 대상이 갖춰진 뒤 진행한다. | 미반영 |
| D10 | 보통 | Editor-Usage 머리말의 빌드·UI 미검증 문구와 하단의 사용자 빌드 확인, Next-Work-Plan의 일부 프리뷰 사용자 확인이 서로 다른 범위를 설명하지만 시점·대상이 불분명하다. 상태 문서도 이후 미커밋 변경을 포함한다. | 기능·변경 단위로 소스 구현, 사용자 보고, 아직 확인하지 않은 회귀 범위를 구분한다. 확인 날짜나 커밋을 모르면 미상으로 둔다. | 향후 작성 규칙만 반영; 기존 이력 정리는 미반영 |
| D11 | 보통 | 기존 docs README에 세부 계획·요청 메모·BP Task 진단이 빠져 있었고 카테고리별 작성·동기화 규칙이 없었다. | 문서 목록, 세 템플릿, AGENTS의 작성·연관 문서 갱신 규칙을 제공한다. | 이번 작업에서 반영 |

## 작성 대상 선별

기존 문서를 보강할 대상과 새 파일이 필요한 대상을 분리했다. 아래 파일명은 정비할 때 사용할 제안 이름이며,
아직 존재하지 않는 문서를 완료한 것으로 표시하지 않는다. 우선순위는 현재 사용자를 잘못 안내하는 정도와
여러 곳에 흩어진 계약을 한 번에 찾을 필요성을 기준으로 정했다.

### Manual

| 순서 | 대상 | 처리 | 꼭 남길 내용과 이유 |
|---|---|---|---|
| 1 | [Runtime-Usage.md](../manual/Runtime-Usage.md) | 기존 문서 수정 | 현재 헤더·최소 실행 예제, ASC 준비, 액션·그래프 시작, 쿨다운·반복·종료 계약, 기본 태스크의 대상·제거 정책. 현재 잘못된 include와 태스크 목록이 사용을 방해한다. |
| 2 | [Editor-Usage.md](../manual/Editor-Usage.md) | 기존 문서 수정 | 현행 프리뷰 툴바·카메라·기본 배치·벽 표시, 타임라인·그룹·상속 편집, 재생·정지·탐색, 그래프 연결 방법. 오래된 UI 설명을 우선 고친다. |
| 3 | `manual/Asset-Migration.md` | 새 문서 | UKataDefinition 제거, ParentAction 전환, 쿨다운·Loop·Distance 필드 변경, 테스트 모듈 Redirect별 자동 처리 여부와 수동 재설정. 기존 uasset 사용자가 수행할 조치를 모으기 위해 필요하다. |
| 4 | `manual/Task-Authoring.md` | 새 문서 | C++·BP에서 UKataTask와 UKataTaskInstance를 연결하는 절차, 인스턴스 상태·루프 초기화·종료 정리, 설정 검사와 프리뷰 제약. BP 제작 경로가 현재 진단 기록에만 있어 사용법으로 찾기 어렵다. |
| 5 | [Conditions.md](../manual/Conditions.md) | 기존 문서 수정 | UKataFL_Condition 함수의 용도·오류 반환, 조건 UObject와 Context의 책임, 오래된 작업 표현 정리. 기존 조건별 설명은 유지한다. |

그래프의 **조작**은 Editor-Usage, **실행 API**는 Runtime-Usage에 남긴다. 두 문서에서 그래프 설명이
반복적으로 커져 탐색이 어려워질 때만 별도 Graph-Usage 문서로 분리한다.

### Devlog

| 순서 | 대상 | 처리 | 꼭 남길 결정과 근거 |
|---|---|---|---|
| 유지 | [Implementation-Status.md](Implementation-Status.md) | 현재 상태 요약으로 갱신 | 구현 범위와 제한의 진입점. 과거 결정의 상세 이유를 무한히 추가하지 않는다. |
| 1 | `devlog/Action-Asset-Model.md` | 새 기록 | Blueprint/CDO에서 전용 UObject 에셋으로 바뀐 이유, ParentAction·명시적 오버라이드·병합 사본의 선택, 이전 에셋 영향. 변경 당시 근거가 없으면 정리 시점과 미상을 표시한다. |
| 2 | `devlog/Execution-Lifecycle.md` | 새 기록 | 월드 실행 순서, 최초 즉시 실행, 프레임당 Tick·Loop 경계, 태스크 자원 회수의 이유와 남은 제약. 프리뷰 시계와 게임 시계의 차이는 관련 진단에 연결한다. |
| 3 | `devlog/GAS-and-Tasks.md` | 새 기록 | GAS에 비용·쿨다운을 맡기는 경계, GE·Loose Tag 분리, 이벤트·대상·효과 핸들 수명 결정. 미구현 Cost 정책은 확정 구현으로 옮기지 않고 계획에 연결한다. |
| 4 | `devlog/Graph-Transition.md` | 새 기록 | Trigger·Window·자동 전이, Conduit 채택, Branched 종료, Failed 미도입, 입력 버퍼 보류의 근거. 당시 확정과 현재 소스 상태를 구분한다. |
| 유지 | [C++ 공용 함수 및 Blueprint Task 진단](Function-Library-and-Blueprint-Task-Diagnosis.md) | 기존 기록 유지 | 조건 함수 도입과 BP 태스크 진단의 당시 근거. 현재 제작 절차는 새 Task-Authoring으로 옮겨 연결한다. |
| 유지 | [Play Montage 포즈 탐색 진단](2026-09-24-Montage-Scrub-Diagnosis.md) | 기존 기록 유지 | 프리뷰 탐색의 구체적인 문제·근거. 전체 프리뷰 설계 문서를 지금 추가하지 않는다. |

`ProjectKataTesting`의 모듈 분리와 GenericGraph 출처는 현재 상태 및 해당
[UPSTREAM.md](../../Plugins/Kata/Source/KataGraph/UPSTREAM.md)로 추적할 수 있다.
새 독립 기록은 후속 변경에서 이유·호환성 판단이 더 필요해질 때 만든다.
빌드 오류 대응, 주석 검수, 사소한 UI 조정도 별도 devlog를 일괄 생성할 필요는 없다.

## 별도로 남길 가치가 큰 주제

다음 표는 위 선별의 근거가 된 주제별 배치다. 새 기능 제안이나 각 행마다 새 파일을 만들라는 뜻은 아니다.

| 주제 | devlog에 보존할 결정·이유 | manual에 설명할 현재 계약 | 현재 근거 |
|---|---|---|---|
| 에셋과 상속 | Blueprint/CDO 제거, ParentAction 선택, 명시적 오버라이드 유지 이유 | 원본·병합 사본·실행 인스턴스, Reset, 지원하지 않는 복제 범위 | Implementation-Status의 원본 에셋과 실행, Runtime-Usage의 설정과 상속 |
| 실행 수명과 시계 | 최초 즉시 실행, 프레임당 Tick 제한, 루프 경계, 월드 Subsystem 선택 이유 | Duration 0과 Single Frame, 완료 의존성, 종료·자원 회수, 프리뷰 시간 동기화의 한계 | Implementation-Status의 유지한 런타임과 조건·프리뷰 |
| GAS와 기본 태스크 | 비용·쿨다운 책임, GE와 Loose Tag를 나눈 이유 | 호출 Ability 유무, 대상 선택, 제거 정책, 이벤트 수신 준비, 프리뷰 한계 | Base-Task-Plan 및 Implementation-Status |
| 그래프 전이 | Conduit 채택, Branched 구분, Failed 미도입, 버퍼 보류 이유 | Entry→Action 최소 구성, Trigger·Window, 우선순위와 동률, Immediate·OnActionEnd, 자동 전이 | Implementation-Status의 그래프 계층 |
| 태스크 확장 | 설정 UObject와 실행 인스턴스 분리, C++·Blueprint 확장점 경계 | BP 두 클래스 연결, 루프 시 사용자 변수 초기화, 종료 시 자원 정리, 설정 검사 한계 | Function-Library-and-Blueprint-Task-Diagnosis |
| 프리뷰 환경 | 월드 유형·툴바·시간 탐색·몽타주 동기화 구현 이유 | 캐릭터·Anim Blueprint·ASC 준비, Reset·Repeat, 게임과 다른 초기화 환경 | Implementation-Status의 프리뷰·전용 에디터 |
| 개발 코드 분리 | ProjectKataTesting 분리와 Redirect·NeverCook 정책 | 개발 하네스 전제, 콘솔 명령, cooked Game에서의 제한 | Implementation-Status의 프로젝트 테스트 하네스 |

GenericGraph의 출처·수정 이력은 기존 [UPSTREAM.md](../../Plugins/Kata/Source/KataGraph/UPSTREAM.md)를 원문으로 유지한다.
docs에는 역할과 해당 파일 링크만 두어 같은 이력을 이중 관리하지 않는다.

## 카테고리 적정성

실제 하위 폴더는 **devlog·manual·plan 세 개**다. docs 루트의 README까지 세면 네 항목이지만 README는 안내 문서다.
사용자가 언급한 네 번째 카테고리는 현재 경로에서 확인되지 않았으므로 새로운 분류가 이미 있다고 가정하지 않았다.

현재 규모에서는 **세 카테고리와 README 안내로 충분하다**. 부족한 기록도 다음과 같이 수용할 수 있다.

| 기록 목적 | 현재 배치 |
|---|---|
| 현재 구현 범위 | devlog/Implementation-Status.md |
| 변경 이유·설계 결정·진단 | devlog의 주제별 기록 |
| 사용·확장 계약·이전 안내·문제 해결 | manual의 주제별 문서 |
| 후보·결정 대기·보류·정비 순서 | plan, 완료 시 devlog·manual 연결 |
| 문서 탐색과 템플릿 | docs/README.md와 각 카테고리의 _Template.md |

네 번째 분류가 필요해진다면 `architecture`를 검토할 수 있다. 여러 모듈을 아우르는 현재 구조·불변식 문서가
여러 개 생겨 manual의 작업 절차나 devlog의 당시 결정과 구분하기 어려워지는 시점이 도입 기준이다.
지금 폴더부터 늘리면 내용 없이 분류만 있는 문제가 반복되므로 이번에는 추가하지 않았다.
템플릿·검증 기록·보관 문서도 당장은 독립 카테고리 없이 다룰 수 있다.

## 주요 결정과 운영 변경

- 템플릿은 각 카테고리에 둔다. 사용법을 찾는 위치와 작성 기준을 찾는 위치를 일치시킨다.
- 현재 상태 요약과 후속 계획은 기존 구조를 유지한다. 모든 문서를 매번 같은 틀로 전면 재작성하지 않는다.
- 기능 변경 시 관련 매뉴얼과 상위·세부 계획을 함께 갱신한다. 영향이 없는 문서나 사소한 변경의 devlog를 억지로 만들지 않는다.
- 중요한 결정과 진단은 근거·확인 범위를 갖춘 주제별 기록으로 남긴다. 기존 내용에서 소급 정리한 기록은 정리 날짜를 쓰고 실제 결정일은 모르면 미상으로 둔다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 문서 분류와 내용 | 에이전트가 폴더 목록과 기존 docs 문서, 루트 README·AGENTS·CLAUDE를 읽고 대조 | 위 D01~D11의 누락·불일치 식별 | 전체 소스와 문서의 완전한 일치 여부 |
| 런타임 예제 | 에이전트가 Public/Action 파일 목록과 KataComponent·KataActionInstance 공개 선언을 읽음 | 현재 헤더 위치와 PlayKataAction·GetResolvedDefinition 선언 확인 | 예제 컴파일·실행 |
| 기존 변경 | Git 상태와 Implementation-Status의 기존 diff 확인 | 미커밋 변경을 보존하고 문서 운영 기록을 추가 | 다른 작업의 완료·검증 여부 |

이번에는 빌드·UHT·자동화 테스트·UI 실행·별도 코드 검사를 수행하지 않았다.
기존 문서에 적힌 사용자 확인 결과를 이번 변경에 대한 성공 결과로 간주하지 않는다.

## 남은 제한과 후속 작업

템플릿·지침 마련과 부채 진단에 이어 작성 대상을 선별했다. D01~D10의 기존 기능 문서 전면 정비와
위 표의 새 문서 본문 작성은 수행하지 않았다.
다음 문서 정비는 D01~D03, D04~D06, D07~D10 순서로 진행하는 것을 권장한다.
기능 동작이 불분명한 항목은 관련 소스를 필요한 범위에서 읽어 기록하고, 실행 확인은 사용자 담당으로 남긴다.
이 진단은 시점 기록으로 유지하며 후속 완료 상태는 아래 계획 진입점에 연결한다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [AGENTS.md](../../AGENTS.md) | 템플릿 참조, 문서 역할, 갱신 시점, 완료 상태와 확인 범위 기록 규칙 추가 |
| [문서 목록](../README.md) | 템플릿과 누락된 기존 문서·이번 진단 링크 추가 |
| [현재 구현 상태](Implementation-Status.md) | 문서 운영 변경과 남은 정비 범위 추가. 기존 미커밋 기록 보존 |
| 다음 작업 계획(2026-09-24 삭제, 이슈로 대체) | 문서 정비 후보와 본 진단 링크 추가. 기존 기능 계획의 결정을 새로 확정하지 않음 |
| 기존 manual·세부 plan·루트 README | D01~D10으로 정비 범위를 기록. 이번에는 진단 대상으로 유지 |
