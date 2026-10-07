# Kata 문서

갱신: 2026-10-07

현재 문서는 `devlog`, `manual`, `plan` 세 카테고리로 관리한다. 이 README는 문서 안내이며 네 번째 카테고리는 아니다.
작성 규칙은 [AGENTS.md](../AGENTS.md)의 문서 작성과 동기화를 따른다.

## 분류와 템플릿

| 위치 | 기록할 내용 | 템플릿 |
|---|---|---|
| devlog | 실제 변경, 결정 이유, 진단과 확인 결과 | [기록 템플릿](devlog/_Template.md) |
| manual | 현재 기능의 사용 순서, API·설정 계약, 제한 | [사용법 템플릿](manual/_Template.md) |
| plan | Large 이슈에 딸린 설계 문서. 설계안, 확정·미확정 사항, 작업 순서와 완료 조건 | [계획 템플릿](plan/_Template.md) |

현재 작업의 진행·구현 완료·사용자 확인과 후속 작업은
[GitHub 이슈](https://github.com/jaykop/Kata/issues)를 먼저 읽는다. 진행 상태는 이슈에서만 관리하고, plan은 이슈에서 링크하는 설계 문서로 쓴다. 계획이나 과거 기록을 현재 구현 사양으로 사용하지 않는다.
기능 변경 시 영향을 받는 사용법·계획을 갱신하고, 중요한 결정 이유는 devlog에 기록한다. 전체 구현 상태 요약 문서는 유지하지 않는다.
2026-09-25 기존 설명서와 결정 기록을 현행 코드에 맞췄다. 소스 반영과 실제 실행 확인은 구분하며,
문서 정비 결과와 남은 실행 확인은 [현행화 기록](devlog/2026-09-25-Documentation-Maintenance.md)을 따른다.

## Manual

- [에디터 사용법](manual/Editor-Usage.md)
- [GAS Inspector 사용법](manual/GAS-Inspector.md): Tags·Attributes·Abilities·Effects 화면의 ASC 상태 조회와 Freeze/Refresh.
- [런타임 사용법](manual/Runtime-Usage.md)
- [Action Group 사용법](manual/Action-Group.md): Action·Graph 가중 목록, Payload 확장과 선택 API.
- [태스크·Command 제작](manual/Task-Authoring.md): C++·BP 확장과 실행별 자원 수명.
- [기존 에셋·API 이전](manual/Asset-Migration.md): 자동 처리·수동 재설정·Redirect 범위.
- [기본 조건](manual/Conditions.md)
- [게임플레이 태그 사용법](manual/Gameplay-Tags.md): 태그 ini 구조와 `KataTag` 코드 생성.
- [팩션 사용법](manual/Factions.md): Kata Factions 설정과 `UKataFL_Faction` 관계 판정.
- [타게팅 사용법](manual/Targeting.md): 타게팅 컴포넌트, 소프트 타겟·락온, Preset 확장 태스크.
- [AI 사용법](manual/AI.md): 공유 AI Data·NPC 행 설정, 시각 대상 선택·Preset·Evaluator 바인딩과 StateTree·Perception 수명.
- [입력 사용법](manual/Input.md): 입력 처리 컴포넌트와 입력 설정 에셋, 기본 IMC, 이동·시점, Input·Trigger 태그로 그래프 구동, IMC로 행동 제어.
- [MainHUD 사용법](manual/HUD.md): 기본 락온 마커, Widget Blueprint 확장, 폰 교체와 DPI 보정.
- [Attribute 사용법](manual/Attributes.md): Base·Combat AttributeSet, 버프 ModOp 규칙, 피해 Execution.
- [Gameplay Data 사용법](manual/Gameplay-Data.md): 캐릭터 GAS 데이터 에셋(세트·초기값·Ability·Effect)과 행의 Identity Tags, 적용 순서.
- [캐릭터 데이터 테이블 사용법](manual/Character-Data.md): PC·NPC 행 작성, 비동기 생성 노드, PC 생성 GameMode와 실패 시 확인 항목.
- [스포너 사용법](manual/Spawner.md): 인라인 Spawn Area·Nav Mesh Projection, 공용 예산의 생성·디스폰, 소유 Controller와 종료 옵션·결과 이벤트.
- [장비 사용법](manual/Equipment.md): 장비 행과 장비 ID, 장착 컴포넌트의 슬롯·소켓 설정과 비동기 장착·해제.
- [애니메이션 레이어 사용법](manual/Animation-Layers.md): 메인·레이어 Anim Instance, 무기 종류별 Linked Anim Layer 설정과 프리뷰 링크.
- [카메라 사용법](manual/Camera.md): 카메라 데이터와 Boom Arm·Spline Rail 배치, 태그 지정 레일 편집, GameplayDebugger 2D 패널.

## Devlog

- [스포너 배치 실행 분리](devlog/2026-10-06-Spawner-Batch-Execution.md): 고정 행·영역 Context, 진행 커서와 미제출 수 집계. 실제 프레임 분산 전 단계.
- [스포너 타임슬라이싱](devlog/2026-10-07-Spawner-Time-Slicing.md): 공용 월드 관리자, 위치·제출 분산, 로드 완료 큐와 요청 상한.
- [스포너 디스폰과 Controller 수명](devlog/2026-10-07-Spawner-Despawn-Lifecycle.md): 생성 세대·소유 Controller 기록, 수동 제거·공용 예산과 종료 인계.
- [스포너 개체별 휴면 계약 결정](devlog/2026-10-07-Spawner-Dormancy-Contract.md): 개체 현재 거리와 상태 보존 요구, 슬롯·캡처·복원 경계를 먼저 정한 이유.

- [SubGraph Any State 범위와 PIE 중 재구성](devlog/2026-10-07-SubGraph-AnyState-And-PIE-Rebuild.md): 펼친 Any State 사본의 범위 한정, PIE 중 열기·저장 시 실행 사본 보존.
- [내장 SubGraph 중첩과 외장 의존 갱신](devlog/2026-10-07-Nested-SubGraph-Dependencies.md): 직계 부모 탐색, 하위 트리 복제·Undo, 외장 저장 세대와 갱신 안내.
- [내장 SubGraph의 붙여넣기와 자동 배치 보완](devlog/2026-10-06-Embedded-SubGraph-Editing.md): 내장 노드 독립 복제, 참조 포트 재사용, 저작 연결 기반 배치와 Undo UI 갱신.
- [내장 SubGraph의 저장과 실행 연결](devlog/2026-10-06-Embedded-SubGraph-Runtime.md): 내장 선행 재구성, 단일 노드 입출력과 내부 Entry의 부모 시작 제외.
- [내장 SubGraph의 노드 생성과 페이지 탐색](devlog/2026-10-06-Embedded-SubGraph-Node-Navigation.md): 패널 제거, 노드 생성 시 원본 준비와 입력 콜백 이후 화면 전환.
- [내장 SubGraph의 편집 UI와 그래프 탐색](devlog/2026-10-06-Embedded-SubGraph-Editor.md): 최초 목록 UI 구현과 노드 중심 흐름으로의 후속 변경 링크.
- [내장 SubGraph의 소유 데이터와 포트 참조 계약](devlog/2026-10-06-Embedded-SubGraph-Data.md): 부모 소유 목록, 선택 모드, 직접 참조 검증과 후속 UI·저장 연결의 경계.
- [Gameplay Data의 Ability·Effect 부여와 Identity 태그](devlog/2026-10-06-Gameplay-Data-Grants.md): 행에 둔 Identity 태그, 적용 순서와 회수 보류 이유.
- [캐릭터 스탯 Attribute와 Gameplay Data](devlog/2026-10-05-Character-Attributes.md): 기능 카테고리 세트, 행 배열 조립, Execution 피해 식과 버프 규칙의 결정 이유.
- [GAS Inspector 탭별 화면 개편](devlog/2026-10-06-GAS-Inspector-UI.md): Current 단독 표시, 전용 열·버튼과 Trigger 검색 제거.
- [GAS Inspector의 값 수집과 에디터 도구 구성](devlog/2026-10-05-GAS-Inspector.md): snapshot 수명, metadata, Trigger 인덱스와 표시 계약.

- [Action Group 구현](devlog/2026-10-05-Action-Group.md): 공유 목록·Payload와 순수 가중 선택.

- [KataAI 기반과 StateTree 실행 수명](devlog/2026-10-05-AI-Lifecycle.md): AI-1 모듈·Controller·NPC 설정과 AI-2 시각 대상·Evaluator 연결.

- [전용 테스트 모듈 제거](devlog/2026-10-05-Testing-Module-Removal.md): 입력 실행 경로 확보 후 테스트 소스·등록·Redirect 제거.
- [작업 상태 관리 통합](devlog/2026-10-03-Issue-State-Management.md): 전체 상태 문서 폐지와 Issue·manual·devlog의 역할.
- [스포너 영역·수량·NavMesh 확장과 #21 종료](devlog/2026-10-04-Spawner-Area-And-NavMesh.md): Spawn Area 이름, 구 영역, 최소·최대 수량, 위치 보정 확장 지점과 NavMesh 투영, 계획에서 옮긴 결정.
- [게임 데이터 컬렉션과 행 ID 참조](devlog/2026-10-04-Data-Collection-Row-Id.md): 컬렉션 에셋, 영역별 행 ID, NPC 테이블 목록과 스포너 Source Table 결정.
- [애니메이션 레이어 구조와 ASC 태그 정책](devlog/2026-10-04-Anim-Layer-Structure.md): 몸 구조로 나누지 않는 Anim Instance, 무기 종류 태그와 스켈레톤별 레이어 설정, Status·Identity 태그 루트.
- [그래프 저장 실패와 삭제 후 남은 노드 정리](devlog/2026-10-04-Graph-Save-Orphan-Objects.md): FortniteMain custom version 오류의 원인과 재구성 시 정리.
- [입력 캔슬 창과 창 태그 재구성](devlog/2026-10-04-Cancel-Window.md): Cancel Window 태스크, `TryCancelKata`, 점프 입력, `Window` 태그 루트와 Transition Window 항목 배열.
- [락온 토글과 좌우 전환 입력](devlog/2026-10-04-Lock-On-Input.md): 획득·해제 토글과 카메라 기준 좌우 전환의 세 InputAction 연결.
- [락온 카메라와 MainHUD 연결](devlog/2026-10-04-Lock-On-Camera-HUD.md): 초점 전달, 부위별 구도 데이터, 회전·구도 보정과 기본 마커.
- [락온 카메라 블렌드·정렬 재작성](devlog/2026-10-05-Lock-On-Camera-Rework.md): BlendIn 하나로 통합한 블렌드, 두 층 설정, 좌우 정렬과 조준선.


- [액션 에셋 모델과 상속](devlog/2026-09-25-Action-Asset-Model.md)
- [실행 순서와 태스크 수명](devlog/2026-09-25-Execution-Lifecycle.md)
- [GAS 책임과 기본 태스크](devlog/2026-09-25-GAS-and-Tasks.md)
- [그래프 전이와 대상 유지](devlog/2026-09-25-Graph-Transition.md)
- [그래프 전이의 액션 시작 거절 처리](devlog/2026-10-05-Graph-Start-Rejection.md): 게임플레이 거절 시 액션 보존과 정상 종료 계약.
- [설명서·결정 기록 현행화](devlog/2026-09-25-Documentation-Maintenance.md)
- [C++ 공용 함수 및 Blueprint Task 진단](devlog/Function-Library-and-Blueprint-Task-Diagnosis.md)
- [문서 부채와 분류 진단](devlog/2026-09-24-Documentation-Diagnosis.md): 불일치 근거와 정비 우선순위.
- [Play Montage 포즈 탐색 진단](devlog/2026-09-24-Montage-Scrub-Diagnosis.md): 현재 스크럽과 UE 5.8 몽타주 에디터 경로 비교.
- [Play Montage 포즈 탐색 구현](devlog/2026-09-24-Montage-Scrub-Implementation.md): 직접 포즈 평가와 일반 재생 전환.
- [프리뷰 시간 탐색의 실행 시뮬레이션 전환](devlog/2026-09-24-Preview-Scrub-Simulation.md): 최종 탐색 방식과 한 프레임 동기 진행의 엔진 제약.
- [게임플레이 태그 코드 생성 구현](devlog/2026-09-24-Gameplay-Tag-Generation.md): Native ini 기반 생성과 `+` 접두사 문제.
- [모듈 구조와 AKataCharacter 진단](devlog/2026-09-24-Module-Structure-Diagnosis.md): 플러그인 분리·캐릭터 이동·KataAI 범위 결정.
- [전역 메시지 라우터 도입 검토](devlog/2026-09-24-Gameplay-Message-Router-Review.md): GameplayMessageRouter 보류 결정과 재검토 조건.
- [UKataComponent 이름 변경](devlog/2026-09-26-KataActionComponent-Rename.md): UKataActionComponent로 변경과 Redirect.
- [KataFramework 캐릭터 조합](devlog/2026-09-26-KataFramework-Character-Composition.md): AKataCharacter 컴포넌트 구성, 팀 인터페이스, AKataPlayerCharacter.
- [캐릭터 행 적용 순서와 비동기 요청 수명](devlog/2026-09-30-Character-Row-Spawn.md): Mesh 미적용 진단, Construction Script 이후 적용, 요청 참조 수명.
- [스포너 옵션을 컴포넌트로 분리한 결정](devlog/2026-09-30-Spawner-Component-Design.md): GEComponent 방식 UObject 설정으로 정정, 수량·영역 구성과 후속 옵션의 경계.
- [카메라 Spline 레일 구현](devlog/2026-09-30-Camera-Spline-Rail.md): 태그 식별, 컴포넌트 원점 피벗, 좌표 합성·오류 대체와 2D 디버그.
- [대상·방향 결정 Command와 회전 태스크](devlog/2026-09-26-Targeting-Resolve-Commands.md): TG-4, PC 공격 방향 우선순위와 소프트 타겟 규칙.
- [그래프 에디터 패널과 디테일 커스터마이제이션](devlog/2026-09-26-Graph-Editor-Panels.md): Comment 배치, 노드 검색, Alias 출발지 목록 UI의 결정과 시행착오.
- [SubGraph 포트와 평탄화](devlog/2026-10-03-SubGraph-Flattening.md): 다른 그래프를 끌어다 쓰는 포트, 저장 시 펼침, 구현 중 드러난 결함 다섯 건.

구현 전 단계의 Claude/Codex 설계 제안과 이전 Blueprint 중심 사용법은 현재 구조와 충돌해 폐기했다.

## Plan

- [플레이어 위치 기반 스포너 관리와 타임슬라이싱 계획](plan/Spawner-Scheduling-And-Grid-Plan.md): 개체별 거리·휴면·복원 뒤 그리드를 연결하는 순서, 공용 작업 예산과 수명 관리. 이슈 게시 전 설계안.
- [스포너 개체별 거리·휴면·스냅샷 계약](plan/Spawner-Dormancy-Contract-Plan.md): 슬롯 식별, 캡처·복원 실패, GAS 보존 규칙과 준비 완료 경계. 이슈 게시 전 설계안.

- [GAS Inspector 설계](plan/GAS-Inspector-Plan.md): 한 창의 탭별 전용 화면, 필요한 열·버튼과 상태 정보 간소화 설계. 사용자 요청으로 이슈 미게시.

- [플러그인 분리 모듈화 계획](plan/Plugin-Modularization-Plan.md): 코어·위성·통합 플러그인 구성과 단계. [#1](https://github.com/jaykop/Kata/issues/1).
- [타게팅 시스템 설계](plan/Targeting-Plan.md): KataTargeting의 컴포넌트·Preset·팩션 설계. [#13](https://github.com/jaykop/Kata/issues/13).
- [카메라 시스템 계획](plan/Camera-Plan.md): KataCamera 궤도 트랙, 상태 블렌딩, Shrink, 디더링, 락온 화면 구성. [#20](https://github.com/jaykop/Kata/issues/20).
- [입력 계층 계획](plan/Input-Plan.md): PlayerController, 입력 설정, IMC 추가·제거, 그래프 발동. [#19](https://github.com/jaykop/Kata/issues/19).
- [KataAI 구현 계획](plan/AI-Plan.md): 시각 감지·추적·액션 실행·수색·복귀, 후속 어그로·Pressure 설계. [#22](https://github.com/jaykop/Kata/issues/22).
- [KataActionGroup 계획](plan/Action-Group-Plan.md): Action·Graph의 가중 선택과 항목별 확장 Payload. [#22](https://github.com/jaykop/Kata/issues/22).
- [캐릭터 데이터 테이블과 비동기 생성 계획](plan/Character-Definition-Plan.md): PC·NPC 캐릭터 테이블, 비동기 생성 API, PC 생성 GameMode. [#26](https://github.com/jaykop/Kata/issues/26).
- [액션 게임 기반 시스템 계획](plan/Action-Game-Systems-Plan.md): 카메라·인풋·타게팅·퍼셉션·스포너 후보. 연결 이슈 없음(제안).
- [기본 태스크 확장 계획](plan/Base-Task-Plan.md): [#6](https://github.com/jaykop/Kata/issues/6)·[#7](https://github.com/jaykop/Kata/issues/7)의 설계 참고. 당시 제안과 현재 구현 전제를 구분한다.
- [Hit Trace 계획](plan/Hit-Trace-Plan.md): HitBox 프리셋·컴포넌트, 서브스텝 보정, Hit Subsystem·Handler, Preset 필터. [#6](https://github.com/jaykop/Kata/issues/6).
- [액션 비용 정책 계획](plan/Cost-Policy-Plan.md): [#9](https://github.com/jaykop/Kata/issues/9).
- [프리뷰 멀티 타겟 배치 계획](plan/Preview-Multi-Target-Plan.md): [#10](https://github.com/jaykop/Kata/issues/10).
- [그래프 노드 타입 계획](plan/Graph-Node-Types-Plan.md): 전이 해석 단계와 Conduit·Alias·SubGraph 노드. [#25](https://github.com/jaykop/Kata/issues/25).
- [장비·무기 시스템 계획](plan/Equipment-Plan.md): 부위 슬롯, Equipment·Weapon, 손별 그래프 조각의 런타임 합성, 스켈레톤별 Anim Layer 해석. [#30](https://github.com/jaykop/Kata/issues/30).

2026-09-24 작업 추적을 GitHub 이슈로 옮기면서 다음 작업 계획(Next-Work-Plan)을 삭제했다. 우선순위와 보류 항목은 이슈 #3~#12로,
Cost 정책과 멀티 타겟 배치 설계는 별도 plan으로 옮겼다. Kata·KataAction·KataGraph 요청 메모와 완료된 Play Montage 포즈 탐색 계획도
같은 날 삭제했으며 결과는 관련 devlog와 이슈에 남아 있다.

## 로컬 문서

`docs/localdocs`는 Git·Diversion 추적에서 제외하며 현재 작업 폴더에서만 열람한다.

- 작업별 인계 메모는 `localdocs/threads/<이슈번호>-<slug>.md`에 둔다. 새 작업 시작 시 생성하거나 갱신하며 같은 작업은 같은 파일을 이어 쓴다.
- 완료된 인계 메모는 사용자 요청 시 `localdocs/archive/<YYYY-QN>/`에 보관한다.

- [HKX 변환기](localdocs/Hkx-Converter.md): 스켈레톤 FBX, 표시용 메시, 애니메이션 한 개의 호환성 검사와 UE5 가져오기.
- [은기사 애니메이션 목록·AnimInstance 구성안](localdocs/SilverKnight-Animations.md): 212개 클립의 이동·이벤트·사용 후보와 Locomotion 재구성 제안. [CSV 목록](localdocs/SilverKnight-Animation-Index.csv).
- 추가 캐릭터 3종의 몸체·장비·재질과 694개 애니메이션 사용법, 이름 분류 근거와 CSV 대응표는 `localdocs/`에 보관한다.
- [테스트 콘텐츠 경로 정리](localdocs/2026-10-03-Content-Organization.md): 테스트 아트 이동, Redirector 확인과 남은 제한.
- [HKX 스켈레톤 변환](localdocs/2026-09-30-Hkx-Converter.md): 스켈레톤·캐릭터 변환, Sekiro 공통 몸체·422개 애니메이션 출력과 확인 범위.

- [폐지 전 상태 기록](localdocs/Implementation-Status-Archive-2026-10-03.md): 미커밋 기록과 과거 확인 결과의 보존용 사본. 갱신하지 않는다.

