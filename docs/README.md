# Kata 문서

갱신: 2026-10-17

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
기간 목표는 [GitHub Milestone](https://github.com/jaykop/Kata/milestones), 작업 순서는 [이슈 우선순위 확인 도구](manual/Issue-Priority.md)를 따른다.
기능 변경 시 영향을 받는 사용법·계획을 갱신하고, 중요한 결정 이유는 devlog에 기록한다. 전체 구현 상태 요약 문서는 유지하지 않는다.
2026-09-25 기존 설명서와 결정 기록을 현행 코드에 맞췄다. 소스 반영과 실제 실행 확인은 구분하며,
문서 정비 결과와 남은 실행 확인은 [현행화 기록](devlog/2026-09-25-Documentation-Maintenance.md)을 따른다.

## Manual

- [설명 변경 검토](manual/Description-Review.md): Claude의 주석·manual·태그 설명 변경 기록과 Codex의 부분 교정. ko-review 플러그인을 Kata에서 쓰는 값과 절차.
- [이슈 우선순위 확인 도구](manual/Issue-Priority.md): 중요도×규모 등급, Blocked by에 따른 유효 등급, 마일스톤별 순서표·용량·경고와 grade 라벨 동기화. workflow 플러그인의 `next-task`로 실행.
- [AGENTS.md 규칙 검사](manual/Agents-Rules-Check.md): push·PR마다 GitHub Actions가 실행하는 플러그인 의존 방향·런타임 에디터 의존·샘플 전용 플러그인·복제 선언 텍스트 검사, 로컬 실행과 한계.
- [자동화 테스트 작성·실행](manual/Automation-Tests.md): 대상 모듈 `Private/Tests/`의 테스트 형식, `Kata.<모듈>.<영역>.<경우>` 이름 규칙과 그룹 목록, Editor·headless 실행.
- [에디터 사용법](manual/Editor-Usage.md)
- [런타임 스포너 사용법](manual/Runtime-Spawner.md): Kata Debug 탭의 Spawner 패널. PIE 중 플레이어 정면 배치(일렬·원형), AI 정지와 Action·Graph 반복.
- [Kata Debug 탭 사용법](manual/Debug-Tab.md): 샘플 Editor 모듈의 SlateIM 도킹 탭. PIE 월드 대상 치트·스포너 패널, 공용 대상 선택, 패널 추가 방법.
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
- [MainHUD 사용법](manual/HUD.md): 기본 락온 마커, Widget Blueprint 확장, 폰 교체와 DPI 보정, MVVM 상태 바, 적 체력바와 Kata HUD 설정.
- [Attribute 사용법](manual/Attributes.md): Base·Combat AttributeSet, 버프 ModOp 규칙, 피해 Execution.
- [치트 사용법](manual/Cheats.md): PC 무적·대미지 무시·무한 스태미나·재생성, 대상 사망·그로기, AI 인지 제외·정지 콘솔 명령과 대상 선택자.
- [Gameplay Data 사용법](manual/Gameplay-Data.md): 캐릭터 GAS 데이터 에셋(세트·초기값·Ability·Effect)과 행의 Identity Tags, 적용 순서.
- [캐릭터 데이터 테이블 사용법](manual/Character-Data.md): PC·NPC 행 작성, 비동기 생성 노드, PC 생성 GameMode와 실패 시 확인 항목.
- [스포너 사용법](manual/Spawner.md): 인라인 Spawn Area·Nav Mesh Projection·AI Override, 공용 예산의 생성·디스폰, Activation(Begin Play·Manual·Player Distance), 소유 Controller와 종료 옵션·결과 이벤트.
- [장비 사용법](manual/Equipment.md): 장비 행과 장비 ID, 장착 컴포넌트의 슬롯·소켓 설정과 비동기 장착·해제.
- [시간 배율 요청 사용법](manual/Time-Dilation.md): 액터·월드 시간 배율 요청과 합성·만료 규칙, Time Dilation 태스크.
- [시간대 사용법](manual/Time-Of-Day.md): KataWorld의 현재 시각·시간대 상태, Kata World 설정과 레벨 단위 시간 설정, 시간대 변경 알림, 시간 치트.
- [연출 태스크 사용법](manual/FX-Tasks.md): Play Sound·Camera Shake·Spawn VFX·Trail 설정, 완료·중단 정리 규칙, MetaSound 입력, Trail의 Niagara 파라미터 계약, 프리뷰와 게임의 차이.
- [Hit Trace 사용법](manual/Hit-Trace.md): HurtBox 프로젝트 설정·배치, Hit Box Preset, Hit Trace 태스크·필터·처리기, 디버그 표시.
- [피격 반응 사용법](manual/Hit-Reaction.md): 공격·판정·반응의 역할 분리, 반응 Ability 자식 만들기, 방향 규칙.
- [사망 사용법](manual/Death.md): 사망 컴포넌트와 사망 Ability 설정, Action·Ragdoll 연출과 시체 DimOut, 타게팅·AI 정리, PC 재시작, 캐릭터가 아닌 피해 대상.
- [애니메이션 레이어 사용법](manual/Animation-Layers.md): 메인·레이어 Anim Instance, 무기 종류별 Linked Anim Layer 설정과 프리뷰 링크.
- [대상 방향 Tilt 사용법](manual/Target-Tilt.md): Kata Tilt 노드 배치와 캐릭터별 본 체인, Target Tilt 태스크 설정, 조준점 계산, 디버그 콘솔 변수.
- [루트 모션 커브 사용법](manual/Root-Motion-Curve.md): 시퀀스 루트 모션 커브 추출 수정자, 몽타주 커브 굽기, 커브 편집과 축 읽는 법, 캐릭터의 커브 대체 컴포넌트와 제한.
- [카메라 사용법](manual/Camera.md): 카메라 데이터와 Boom Arm·Spline Rail 배치, 태그 지정 레일 편집, GameplayDebugger 2D 패널.
- [액션 실행 이력 사용법](manual/Action-Trace.md): 캐릭터별 시작·거절·종료·창·트리거 기록을 GameplayDebugger `KataAction`과 `Kata.ActionTrace.Dump`로 보는 법, 출력 형식과 `detail` 값, 제한.

## Devlog

- [낮밤·날씨의 시간 상태와 KataWorld 선행 생성](devlog/2026-10-17-Time-Of-Day-State.md): 상태와 연출을 나눈 이유, DaySequence 대신 직접 구현, 레벨 단위 설정 액터, 시간 배율을 무시하는 시계, `TimeOfDay`·`Weather` 태그 루트, #63보다 먼저 플러그인을 만든 경위. [#124](https://github.com/jaykop/Kata/issues/124).
- [락온 대상이 없을 때 카메라 중앙 정렬](devlog/2026-10-17-Lock-On-Recenter.md): 락온 획득 실패를 Framework가 카메라에 넘기는 경로, `ProcessViewRotation`에서 시점 입력을 세는 이유, R3 오입력을 막는 누적 2도 취소 임계값, 락온 우선과 확인 범위. [#123](https://github.com/jaykop/Kata/issues/123).
- [액션 편집기 타임라인 표시 개선](devlog/2026-10-17-Action-Editor-Timeline-Display.md): 프레임 이동 버튼(30fps 고정), 진단 영역을 Diagnostics 탭으로 옮기며 탭 배치 이름을 유지한 이유, 막대 안 태스크 이름과 sRGB 기준 보색 글자, 그룹을 카테고리 묶음으로 대체하지 않은 이유. [#119](https://github.com/jaykop/Kata/issues/119).
- [커밋 직전 Codex 설명 검토 넘기기](devlog/2026-10-16-Codex-Review-Handoff.md): 사용자 전달에서 Claude의 `codex exec` 넘기기로 바꾼 이유, 커밋 직전으로 정한 이유, 동시 세션 대응과 한 번 호출 규칙, 첫 실행의 CLI·지시문 문제.
- [타임라인 범위 선택·검색과 눈금 줄 위치](devlog/2026-10-15-Timeline-Selection-Search.md): Shift·Ctrl+Shift 범위 선택의 기준 규칙, 상태에 맞춘 Enable/Disable 한 항목, 검색 중 선택 정리와 그룹 처리, `SScrollBox::GetScrollOffset`이 요청 값을 돌려줘 눈금 줄이 어긋나던 진단과 잘림 영역 기준 계산. [#117](https://github.com/jaykop/Kata/issues/117).
- [궤적(Trail) 태스크와 공용 소켓 경로 샘플러](devlog/2026-10-15-Trail-Task.md): Hit Trace 경로 계산 분리와 판정 진단 수정, 샘플을 Niagara User 배열로 넘기는 계약, Solo·마지막 틱 그룹 실행, 프리뷰에서만 안 그려지던 원인(에디터의 배열 라이브러리 재초기화)과 월드별 쓰기 경로. [#108](https://github.com/jaykop/Kata/issues/108), [#109](https://github.com/jaykop/Kata/issues/109)
- [자동화 테스트의 GameplayTag 출처](devlog/2026-10-15-Automation-Test-Tags.md): 테스트가 ProjectKata 태그를 빌려 쓰고 테스트 전용 태그를 만들지 않는 결정, 채택하지 않은 네이티브 태그 대안, 자식 없는 태그를 고르는 이유. [#102](https://github.com/jaykop/Kata/issues/102).
- [로코모션 Sync Group과 발 Sync Marker](devlog/2026-10-15-Locomotion-Sync.md): 레이어 템플릿의 Sync Group, 발 마커를 엔진 모디파이어 대신 수동으로 맞춘 이유, Airborne 상태와 대기 포즈 낙하, 클립 가정 속도로 본 발 밀림 진단과 확인 범위. [#89](https://github.com/jaykop/Kata/issues/89).
- [시작 프레임의 Kata 시각과 몽타주 동기화](devlog/2026-10-15-Start-Frame-Montage-Sync.md): 콤보 전이·입력으로 시작한 액션에서 Kata 시각이 몽타주보다 한 프레임 늦던 진단, 메시 포즈 갱신 여부로 시작 프레임을 따라잡는 결정과 채택하지 않은 대안, 확인 범위. [#115](https://github.com/jaykop/Kata/issues/115).
- [MainHUD MVVM 전환](devlog/2026-10-14-Main-HUD-MVVM.md): ViewModel·Resolver·HUD 서브시스템 계층, 고정 Resolver와 Manual 항목 ViewModel, 보조 저작 툴셋, 타입이 정해진 피해 알림, 적 체력바 대상·출처·락온·만료 규칙과 확인 범위. [#61](https://github.com/jaykop/Kata/issues/61).
- [C++ 자동화 테스트 정책](devlog/2026-10-14-Automation-Test-Policy.md): 유닛·반복 불변식 테스트 허용과 시간 단언 테스트 제외, 테스트 위치 규칙, 2026-10-05 테스트 모듈 제거와의 관계, 첫 대상 세 곳.
- [연출 태스크](devlog/2026-10-14-FX-Tasks.md): 타임라인 연출과 히트 Cue의 역할 분리, 태스크별 플러그인 위치, 완료·중단 정리 규칙, 프리뷰 Seek 무음과 UI 사운드, 셰이크·Niagara 풀 재사용 대응, 확인 범위. [#7](https://github.com/jaykop/Kata/issues/7).
- [advisors 고문 적용과 PM 고문](devlog/2026-10-14-Advisors-PM.md): advisors를 켠 범위, PM 고문을 고문으로 만든 이유와 next-task와의 역할 분리, 다음 작업 진입점, 비공개 기준 노트와 용량 기준.
- [Blockout Tools 도입과 기획 문서 분리](devlog/2026-10-14-Blockout-Tools-And-Design-Docs.md): Fab 플러그인을 샘플 전용·Diversion 전용으로 둔 이유, `.uproject` 미등록, 기획 문서 폴더를 manual·plan과 나눈 이유와 확인 범위. [#74](https://github.com/jaykop/Kata/issues/74)
- [AI 작업 스킬의 공용 플러그인 전환](devlog/2026-10-13-Shared-Agent-Plugins.md): 작업 흐름·한국어 검수 스킬을 workflow·ko-review 플러그인으로 일반화한 이유, 프로필 분리와 Codex 지원, `.claude/` 추적 범위. [#94](https://github.com/jaykop/Kata/issues/94).
- [AGENTS.md 규칙 CI 검사 도입](devlog/2026-10-13-Agents-Rules-CI.md): 컴파일 대신 텍스트 규칙만 검사하는 이유, 규칙 8개 선정, 조건부 에디터 의존·구성 오류 처리, 워크플로 보안 설정. [#93](https://github.com/jaykop/Kata/issues/93).
- [피격 반응 Knock.Back·Knock.Down 샘플과 기상 무적](devlog/2026-10-17-Hit-Reaction-Knock.md): 넉백·다운 반응의 클립 배정과 캔슬·기상 무적 시각, Apply Loose Tag로 무적을 표시한 이유, 재피격 차단, 확인용 샘플 공격 변경. [#71](https://github.com/jaykop/Kata/issues/71).
- [피격 반응 은기사·사냥개 샘플](devlog/2026-10-17-Hit-Reaction-Other-Characters.md): 흑기사 Ability 복제 구조, 두 캐릭터의 원본 배정과 캔슬 시각, 은기사 Flinch 좌우 정정, 머리 높이로 정한 기상 무적, 사냥개 Groggy 제외. [#122](https://github.com/jaykop/Kata/issues/122).
- [피격 반응 Medium·Heavy 샘플과 제자리 반응의 밀림](devlog/2026-10-17-Hit-Reaction-Medium-Heavy.md): `Medium` 등급 추가와 흑기사 반응 배정·캔슬 시각, 원본 클립에 루트 이동이 없어 밀리지 않는 원인과 넉백 태스크 분리. [#70](https://github.com/jaykop/Kata/issues/70).
- [피격 Cue의 재질별 이펙트](devlog/2026-10-17-Hit-Cue-Surface-Tags.md): 물리 재질 대신 HurtBox 태그를 Cue 대상 태그로 넘긴 이유, Kata GCN Hit Burst의 첫 일치 항목 실행 규칙, 샘플 불꽃·피의 가시성 보정과 복제 Cue가 재생되지 않던 진단, 재질별 합성 피격음·때린 쪽 Cue 공유·샘플 히트스톱 결정과 오디오 출력 장치 진단. [#104](https://github.com/jaykop/Kata/issues/104).
- [피격 연출 Cue](devlog/2026-10-12-Hit-Cue.md): 맞은 쪽·때린 쪽 Cue 분리와 공격별 지정, 듀얼센스 진동 미전송·패드 배정 정책·GAS Cue 경로 섹션 진단. [#73](https://github.com/jaykop/Kata/issues/73).
- [히트스톱](devlog/2026-10-12-Hit-Stop.md): 처리기의 공격별 정지 시간과 Combat 설정의 배율·Priority, 기본값 0.05·100의 이유, 사용자 확인 범위. [#72](https://github.com/jaykop/Kata/issues/72).
- [시간 배율 요청 관리](devlog/2026-10-12-Time-Dilation-Requests.md): 실행기가 액터 배율을 따르지 않던 진단, 요청 관리 배치·합성·만료 시계·일시정지 결정과 프리뷰 탐색 보정. [#65](https://github.com/jaykop/Kata/issues/65).
- [마일스톤과 우선순위 체계](devlog/2026-10-12-Milestone-Priority-System.md): 로드맵 이슈를 Milestone으로 바꾼 이유, 중요도×규모 등급과 선행 상속 규칙, 놓치기 쉬운 작업 진단과 이슈 재배정 결과. [#64](https://github.com/jaykop/Kata/issues/64).
- [Claude 작성·Codex 설명 검토 파이프라인](devlog/2026-10-12-Description-Review-Pipeline.md): 공유 목록을 채택한 이유, 작업 범위·내용 해시·대기 전환과 로컬 스킬 연결.
- [에디터 주석의 표현과 실행 로직 대조](devlog/2026-10-12-Editor-Comment-Polish.md): 설정 조건·단위·종료 사유·거리 기준과 태그 설명 교정, manual의 현재 사용법 반영, 교정 스킬 개선과 확인 범위.
- [Kata Debug 탭 상태 저장과 대상 무적·대미지 무시](devlog/2026-10-12-Debug-Tab-State-And-Target-Cheats.md): 마지막 패널·플레이어 치트 복원, 대상 보호 태그와 PC Pawn 분기, PIE 종료 뒤 SlateIM 슬롯 정렬 갱신 누락과 컨테이너 교체 우회. [#55](https://github.com/jaykop/Kata/issues/55) 후속.
- [치트 매니저 확장](devlog/2026-10-11-Cheat-Manager.md): 플러그인별 CheatManager 확장과 명령, 게임플레이·치트 태그 분리, 엔진 God·자극원 해제를 쓰지 않은 이유, 대상 선택자 위치, 쿨다운 제외·재생성 결정. [#54](https://github.com/jaykop/Kata/issues/54).
- [디버그 UI를 인게임 메뉴에서 에디터 탭으로 되돌림](devlog/2026-10-11-Debug-Tab-Editor-Return.md): 인게임 SlateIM 메뉴를 화면 가림으로 제거한 결정, SlateIM 위젯 재사용으로 입력값이 깨진 진단과 대응.
- [액션 실행 이력](devlog/2026-10-11-Action-Trace.md): RE:Log 원리를 가져온 배경, 코어 액션 컴포넌트에 둔 고정 필드 기록, C++ 전용 API와 Shipping 제외, 상시 기록·보관 개수·L1 조건 사유 결정, 실패 경로에서만 상세를 구하는 이유와 확인 범위. [#58](https://github.com/jaykop/Kata/issues/58).
- [AI Send Trigger 거절 재시도 정리](devlog/2026-10-11-AI-Send-Trigger-Retry.md): 그래프 트리거 결과 API, 게임플레이 거절 시 구간 내 재시도 중단, 검토한 대안(재시도 간격·로그 수준·간접 판정)과 확인 범위. [#57](https://github.com/jaykop/Kata/issues/57).
- [진행도·퀘스트 시스템 사전 검토](devlog/2026-10-11-Progress-Quest-System-Review.md): 레코드·상태 분리, 태그가 붙은 관계, 목표 감시자, 월드 플래그 후보와 피할 설계, 착수 시 정할 질문. 착수 보류. [#59](https://github.com/jaykop/Kata/issues/59).
- [Kata 액션과 Gameplay Ability 경계](devlog/2026-10-11-Kata-Action-Ability-Boundary.md): GAS 취소·차단이 Kata에 닿지 않던 엔진 근거, 범용 GA 경유를 택하지 않은 이유, 공용 중단·차단 연동·Tick 정리와 항목별 착수 시점, 확인 범위. [#46](https://github.com/jaykop/Kata/issues/46).
- [캐릭터 사망 흐름과 제거](devlog/2026-10-11-Character-Death.md): 처리 주체 검토 경과(Component·GA), 풀링 제외, 스포너 사망 기록, PC 재시작과 빙의 해제 순서, DimOut 디더 식과 확인 범위. [#41](https://github.com/jaykop/Kata/issues/41).
- [액션 해석 비용 측정과 시작 판정 경량화](devlog/2026-10-11-Resolved-Action-Start-Check.md): 해석 비용 측정, 전체 캐시 보류 이유, 판정 전용 해석과 조건·명령 이중 복제 제거. [#47](https://github.com/jaykop/Kata/issues/47).
- [대상 방향 Tilt 구현](devlog/2026-10-10-Target-Tilt.md): 조준 위치 API, Tilt 컴포넌트와 Tick 순서, 척추 체인 AnimNode와 UncookedOnly 모듈, 디버그 콘솔 변수, Template Anim Class·디버그 문자열 함정과 확인 범위.
- [루트 모션 이동량 커브](devlog/2026-10-10-Root-Motion-Curve.md): 시퀀스·몽타주 커브 위치, 루트 모션 델리게이트 대체와 LOD·URO 근거, 섹션 경로 복원, Motion Warping 미도입과 확인 결과.
- [오토 대시](devlog/2026-10-10-Auto-Dash.md): #37 종료 시 계획에서 옮긴 루트 모션 처리 단계 분리, 거리 보정 방식과 한도, 락온 방향 버그 수정과 확인 범위.
- [전진 제한](devlog/2026-10-10-Approach-Limit.md): #38 종료 시 계획에서 옮긴 루트 모션 3단계 전진 제한, HurtBox 후보 선정과 표면 기준, 오토 대시 기준 정렬과 확인 범위.
- [듀얼센스 플러그인 도입과 원본 결함 수정](devlog/2026-10-10-DualSense-Plugin.md): 독립 벤더 플러그인 도입, 다른 장치 끊김·핸들 누수·분리 시 입력 고정 진단, 장치별 리더 스레드와 샘플 IMC 게임패드 매핑.
- [방향별 회피와 그래프 편집 스크립팅](devlog/2026-10-10-Directional-Dodge.md): 방향별 액션과 Conduit 조건 분기, 연속 회피, 비락온 회전, 내장 SubGraph, 리타게팅 상체 회전 진단.
- [입력 계층 결정](devlog/2026-10-10-Input-Layer.md): KataFramework 입력 처리 컴포넌트, Input·Trigger 태그 분리, IMC 행동 제어, 그래프 발동과 캔슬 방식.
- [Shrink의 피벗 선행 스윕](devlog/2026-10-10-Camera-Shrink-Pivot-Sweep.md): 벽을 마주보면 카메라가 벽 안으로 들어간 원인(피벗 오프셋), 뷰 타깃-피벗 선행 스윕과 `Pivot Base` 표시, 보류한 피벗 컴포넌트 구조.
- [락온 방향의 피벗 높이 기준](devlog/2026-10-10-Lock-On-Pivot-Height.md): 락온한 채 다가가면 카메라가 내려가던 원인(캡슐 중심 기준 Pitch), 피벗 높이만 반영하고 수평 오프셋을 빼는 이유.
- [DS3 가산 애니메이션의 자식 공간 규칙과 UE 변환](devlog/2026-10-11-DS3-Additive-Child-Space.md): 다리가 들리던 Flinch 진단, blendHint 2의 자식 공간 의미와 측정 근거, 대기 자세 기준 재추출과 UE 근사 오차.
- [피격 반응 샘플과 좌우·이동 잠금·가산 변환 규칙](devlog/2026-10-10-Hit-Reaction-Sample.md): 흑기사 `Light`·`Flinch` 반응, 맞은 쪽 기준 좌우, 루트 모션으로 반응 중 이동 잠금, DS3 가산 클립의 UE 설정 복원과 Slot Group 순서.
- [Hit Trace 설계 결정](devlog/2026-10-10-Hit-Trace.md): HurtBox 전용 판정, Subsystem 실행 시점, 면 판정·포즈 재샘플링, 구간 경계 보정과 필터·디버그 위치.
- [AI 콤보 트리거를 액션 태스크로 발신](devlog/2026-10-09-AI-Send-Trigger.md): 타임라인 배치를 택한 이유, Single Frame·Order Hint 기본값, 조건 대상, 보류한 창 검증. Add Task 카테고리 메뉴와 Auto Resize 변경 포함.
- [캐릭터 팩션을 행과 스포너에서 지정](devlog/2026-10-08-Character-Faction-Source.md): 스포너·행·Class 기본값 순서, 행 사본에 기록하는 이유, AI가 PC만 노리는 것처럼 보인 관계표 진단.
- [런타임 스포너 에디터 도구](devlog/2026-10-10-Runtime-Spawner-Tool.md): 샘플 Editor 모듈에 둔 이유, 원형 포위 배치, Character Class 폴더 필터, 행 사본으로 AI를 끄는 반복 모드, SGraphActionMenu 선택 메뉴.
- [스포너의 AI 설정 덮어쓰기](devlog/2026-10-10-Spawner-AI-Override.md): 배치 단위 값만 필드로 덮어쓰는 이유, 행 사본 Transient 필드 전달, 캐릭터별 합성 사본과 AI 끄기.
- [스포너 더미 설정](devlog/2026-10-15-Spawner-Dummy.md): 설정·NPC 컴포넌트 분리, 범용 `ScheduleRespawn`, 고정 위치 배치로 재생성하는 이유, Invincible 태그 설정과 보호 범위, 반복을 컴포넌트 Tick으로 옮기고 순간이동으로 복귀하는 이유와 패널 보호 변경, 피격 정보를 메타 Attribute와 반응 이벤트로 모으는 이유.
- [스포너 관리자 구성과 타임슬라이싱 계획 종료](devlog/2026-10-16-Spawner-Scheduling-Decisions.md): 삭제한 스포너 계획의 관리자·생성 Subsystem·스포너 책임 분담, 거리·경계 정책과 단계별 기록 대응. [#76](https://github.com/jaykop/Kata/issues/76).
- [스포너 기준점을 발 높이로 변경](devlog/2026-10-17-Spawner-Feet-Origin.md): 캡슐 높이 보정을 생성 Subsystem에서 하는 이유, 더미 고정 위치 예외, `Height Offset` 기준 변경과 샘플 맵 이전.
- [스포너 배치 실행 분리](devlog/2026-10-06-Spawner-Batch-Execution.md): 고정 행·영역 Context, 진행 커서와 미제출 수 집계. 실제 프레임 분산 전 단계.
- [스포너 타임슬라이싱](devlog/2026-10-07-Spawner-Time-Slicing.md): 공용 월드 관리자, 위치·제출 분산, 로드 완료 큐와 요청 상한.
- [스포너 디스폰과 Controller 수명](devlog/2026-10-07-Spawner-Despawn-Lifecycle.md): 생성 세대·소유 Controller 기록, 수동 제거·공용 예산과 종료 인계.
- [스포너 개체별 휴면 계약 결정](devlog/2026-10-07-Spawner-Dormancy-Contract.md): 개체 현재 거리와 상태 보존 요구, 슬롯·캡처·복원 경계를 먼저 정한 이유. 삭제한 휴면 계약안의 요지와 미확정 항목.
- [스포너 거리 활성화와 작업 순서 재정렬](devlog/2026-10-07-Spawner-Distance-Activation.md): 거리 컴포넌트를 스냅샷보다 먼저 둔 이유, 원점 진입·개체별 제거·재진입 수 정책.
- [거리 관리 스포너의 그리드 조회](devlog/2026-10-07-Spawner-Distance-Grid.md): XY 셀 등록, 플레이어 주변 셀과 활성 스포너만 평가하는 이유.
- [스포너 생성 방식을 Activation 하나로 통합](devlog/2026-10-07-Spawner-Activation-Mode.md): Distance Activation 항목을 Player Distance로 옮긴 이유, 이전 설정 자동 이전과 선택 시 거리 미리보기.

- [그래프 실행 불가 노드 타입 결정](devlog/2026-10-16-Graph-Node-Types.md): 전이 해석의 원자성, Conduit·Alias 엣지 의미, SubGraph 포트 구성, 도입하지 않은 Reroute·ByTag·포트 이름과 재검토 조건. [#25](https://github.com/jaykop/Kata/issues/25).
- [그래프 에디터 PIE 디버거](devlog/2026-10-07-Graph-PIE-Debugger.md): 디버그 대상 선택, 실행 노드 출처 GUID와 페이지 식별, 노드 강조와 Debug 탭 기록.
- [SubGraph Any State 범위와 PIE 중 재구성](devlog/2026-10-07-SubGraph-AnyState-And-PIE-Rebuild.md): 펼친 Any State 사본의 범위 한정, PIE 중 열기·저장 시 실행 사본 보존.
- [내장 SubGraph 중첩과 외장 의존 갱신](devlog/2026-10-07-Nested-SubGraph-Dependencies.md): 직계 부모 탐색, 하위 트리 복제·Undo, 외장 저장 세대와 갱신 안내.
- [내장 SubGraph의 붙여넣기와 자동 배치 보완](devlog/2026-10-06-Embedded-SubGraph-Editing.md): 내장 노드 독립 복제, 참조 포트 재사용, 저작 연결 기반 배치와 Undo UI 갱신.
- [내장 SubGraph의 저장과 실행 연결](devlog/2026-10-06-Embedded-SubGraph-Runtime.md): 내장 선행 재구성, 단일 노드 입출력과 내부 Entry의 부모 시작 제외.
- [내장 SubGraph의 노드 생성과 페이지 탐색](devlog/2026-10-06-Embedded-SubGraph-Node-Navigation.md): 패널 제거, 노드 생성 시 원본 준비와 입력 콜백 이후 화면 전환.
- [내장 SubGraph의 편집 UI와 그래프 탐색](devlog/2026-10-06-Embedded-SubGraph-Editor.md): 최초 목록 UI 구현과 노드 중심 흐름으로의 후속 변경 링크.
- [내장 SubGraph의 소유 데이터와 포트 참조 계약](devlog/2026-10-06-Embedded-SubGraph-Data.md): 부모 소유 목록, 선택 모드, 직접 참조 검증과 후속 UI·저장 연결의 경계.
- [Gameplay Data의 Ability·Effect 부여와 Identity 태그](devlog/2026-10-06-Gameplay-Data-Grants.md): 행에 둔 Identity 태그, 적용 순서와 회수 보류 이유.
- [캐릭터 스탯 Attribute와 Gameplay Data](devlog/2026-10-05-Character-Attributes.md): 기능 카테고리 세트, 행 배열 조립, Execution 피해 식과 버프 규칙의 결정 이유.
- [GAS Inspector 탭별 화면 개편](devlog/2026-10-06-GAS-Inspector-UI.md): Current 단독 표시, 전용 열·버튼과 Trigger 검색 제거, 삭제한 설계 문서의 제외 범위.
- [GAS Inspector의 값 수집과 에디터 도구 구성](devlog/2026-10-05-GAS-Inspector.md): snapshot 수명, metadata, Trigger 인덱스와 표시 계약.

- [Action Group 구현](devlog/2026-10-05-Action-Group.md): 공유 목록·Payload와 순수 가중 선택.

- [KataAI 기반과 StateTree 실행 수명](devlog/2026-10-05-AI-Lifecycle.md): AI-1 모듈·Controller·NPC 설정과 AI-2 시각 대상·Evaluator 연결.

- [전용 테스트 모듈 제거](devlog/2026-10-05-Testing-Module-Removal.md): 입력 실행 경로 확보 후 테스트 소스·등록·Redirect 제거.
- [작업 상태 관리 통합](devlog/2026-10-03-Issue-State-Management.md): 전체 상태 문서 폐지와 Issue·manual·devlog의 역할.
- [스포너 영역·수량·NavMesh 확장과 #21 종료](devlog/2026-10-04-Spawner-Area-And-NavMesh.md): Spawn Area 이름, 구 영역, 최소·최대 수량, 위치 보정 확장 지점과 NavMesh 투영, 계획에서 옮긴 결정.
- [게임 데이터 컬렉션과 행 ID 참조](devlog/2026-10-04-Data-Collection-Row-Id.md): 컬렉션 에셋, 영역별 행 ID, NPC 테이블 목록과 스포너 Source Table 결정.
- [애니메이션 레이어 구조와 ASC 태그 정책](devlog/2026-10-04-Anim-Layer-Structure.md): 몸 구조로 나누지 않는 Anim Instance, 무기 종류 태그와 스켈레톤별 레이어 설정, Status·Identity 태그 루트.
- [발 IK 구현과 사족 리그 진단](devlog/2026-10-11-Foot-IK.md): Body 레이어 Control Rig 구성, 사족 Template 실패, RigVM 평가 순서·Roll·트레이스 범위·골반 혼합 문제와 수정. [#52](https://github.com/jaykop/Kata/issues/52).
- [그래프 저장 실패와 삭제 후 남은 노드 정리](devlog/2026-10-04-Graph-Save-Orphan-Objects.md): FortniteMain custom version 오류의 원인과 재구성 시 정리.
- [입력 캔슬 창과 창 태그 재구성](devlog/2026-10-04-Cancel-Window.md): Cancel Window 태스크, `TryCancelKata`, 점프 입력, `Window` 태그 루트와 Transition Window 항목 배열.
- [락온 토글과 좌우 전환 입력](devlog/2026-10-04-Lock-On-Input.md): 획득·해제 토글과 카메라 기준 좌우 전환의 세 InputAction 연결.
- [락온 카메라와 MainHUD 연결](devlog/2026-10-04-Lock-On-Camera-HUD.md): 초점 전달, 부위별 구도 데이터, 회전·구도 보정과 기본 마커.
- [락온 카메라 블렌드·정렬 재작성](devlog/2026-10-05-Lock-On-Camera-Rework.md): BlendIn 하나로 통합한 블렌드, 두 층 설정, 좌우 정렬과 조준선.
- [락온 시야 조건](devlog/2026-10-09-Lock-On-Line-Of-Sight.md): 카메라 기준 가림 판정, 후보 필터와 유지 중 유예 시간.
- [타게팅 시스템 결정 기록](devlog/2026-10-10-Targeting-Decisions.md): #13 종료 시 설계 문서에서 옮긴 팩션·컴포넌트·소프트 타겟·락온 지점 결정과 엔진 제약.
- [소프트 타겟 디버그 표시와 정면 각도 필터](devlog/2026-10-10-Soft-Target-Debug-Forward-Angle.md): 디버거 범위·후보·부채꼴 표시, Forward Angle 필터, 필터를 정렬 앞에 두는 이유.


- [액션 에셋 모델과 상속](devlog/2026-09-25-Action-Asset-Model.md)
- [Action Template 1단계 상속](devlog/2026-10-10-Action-Template.md): 부모 전용 Template 타입 분리, 다단계 상속 제거와 기존 에셋 영향.
- [실행 순서와 태스크 수명](devlog/2026-09-25-Execution-Lifecycle.md)
- [GAS 책임과 기본 태스크](devlog/2026-09-25-GAS-and-Tasks.md)
- [그래프 전이와 대상 유지](devlog/2026-09-25-Graph-Transition.md)
- [그래프 전이의 액션 시작 거절 처리](devlog/2026-10-05-Graph-Start-Rejection.md): 게임플레이 거절 시 액션 보존과 정상 종료 계약.
- [그래프 트리거 버퍼](devlog/2026-10-10-Trigger-Buffer.md): 창이 닫혀 막힌 마지막 트리거 보관, 창 개방 시 재평가와 PreAcceptSeconds 보관 폭, 캔슬 창 제외 이유.
- [설명서·결정 기록 현행화](devlog/2026-09-25-Documentation-Maintenance.md)
- [액션 편집기 Task Details 의 중첩 객체 편집 반영](devlog/2026-10-16-Action-Editor-Nested-Edit.md): Hit Handler 같은 Instanced 서브오브젝트 안의 값이 되돌아가던 결함, 엔진 MemberProperty 기준과 Settings 쪽 보정을 Task 에도 쓴 이유.
- [KataFL_ 공용 함수 파일을 주제 폴더로 통일](devlog/2026-10-16-KataFL-File-Location.md): `FunctionLibraries/` 의 4개 파일 이동, 주제 폴더로 정한 이유와 코딩 규칙 반영.
- [C++ 공용 함수 및 Blueprint Task 진단](devlog/Function-Library-and-Blueprint-Task-Diagnosis.md)
- [문서 부채와 분류 진단](devlog/2026-09-24-Documentation-Diagnosis.md): 불일치 근거와 정비 우선순위.
- [Play Montage 포즈 탐색 진단](devlog/2026-09-24-Montage-Scrub-Diagnosis.md): 현재 스크럽과 UE 5.8 몽타주 에디터 경로 비교.
- [Play Montage 포즈 탐색 구현](devlog/2026-09-24-Montage-Scrub-Implementation.md): 직접 포즈 평가와 일반 재생 전환.
- [프리뷰 시간 탐색의 실행 시뮬레이션 전환](devlog/2026-09-24-Preview-Scrub-Simulation.md): 최종 탐색 방식과 한 프레임 동기 진행의 엔진 제약.
- [게임플레이 태그 코드 생성 구현](devlog/2026-09-24-Gameplay-Tag-Generation.md): Native ini 기반 생성과 `+` 접두사 문제.
- [액션 게임 기반 시스템 로드맵 종료](devlog/2026-10-16-Action-Game-Systems-Roadmap.md): #24 로드맵 결정의 기록 위치와 구현하지 않은 후보가 이어지는 이슈. [#76](https://github.com/jaykop/Kata/issues/76).
- [모듈 구조와 AKataCharacter 진단](devlog/2026-09-24-Module-Structure-Diagnosis.md): 플러그인 분리·캐릭터 이동·KataAI 범위 결정.
- [플러그인 분리 모듈화 결정 기록](devlog/2026-10-10-Plugin-Modularization-Decisions.md): #1 종료 시 설계 문서에서 옮긴 플러그인 구성·의존 결정, PM-2 Command·Keep Target 결정과 계획 대비 변경.
- [프리뷰 멀티 타겟 배치 보류 결정](devlog/2026-10-10-Preview-Multi-Target-Waived.md): #10을 진행하지 않기로 한 결정과 보존한 설계안.
- [전역 메시지 라우터 도입 검토](devlog/2026-09-24-Gameplay-Message-Router-Review.md): GameplayMessageRouter 보류 결정과 재검토 조건.
- [UKataComponent 이름 변경](devlog/2026-09-26-KataActionComponent-Rename.md): UKataActionComponent로 변경과 Redirect.
- [KataFramework 캐릭터 조합](devlog/2026-09-26-KataFramework-Character-Composition.md): AKataCharacter 컴포넌트 구성, 팀 인터페이스, AKataPlayerCharacter.
- [캐릭터 행 적용 순서와 비동기 요청 수명](devlog/2026-09-30-Character-Row-Spawn.md): Mesh 미적용 진단, Construction Script 이후 적용, 요청 참조 수명.
- [스포너 옵션을 컴포넌트로 분리한 결정](devlog/2026-09-30-Spawner-Component-Design.md): GEComponent 방식 UObject 설정으로 정정, 수량·영역 구성과 후속 옵션의 경계.
- [카메라 Spline 레일 구현](devlog/2026-09-30-Camera-Spline-Rail.md): 태그 식별, 컴포넌트 원점 피벗, 좌표 합성·오류 대체와 2D 디버그.
- [대상·방향 결정 Command와 회전 태스크](devlog/2026-09-26-Targeting-Resolve-Commands.md): TG-4, PC 공격 방향 우선순위와 소프트 타겟 규칙.
- [그래프 에디터 패널과 디테일 커스터마이제이션](devlog/2026-09-26-Graph-Editor-Panels.md): Comment 배치, 노드 검색, Alias 출발지 목록 UI의 결정과 시행착오.
- [SubGraph 포트와 평탄화](devlog/2026-10-03-SubGraph-Flattening.md): 다른 그래프를 끌어다 쓰는 포트, 저장 시 펼침, 구현 중 드러난 결함 다섯 건.
- [그래프 에디터 재개봉 시 dirty 문제](devlog/2026-10-08-Graph-Reopen-Dirty.md): `UKataEdGraph::Modify`가 `bAlwaysMarkDirty`를 무시해 재구성이 에셋을 dirty로 만들던 원인과 수정.
- [그래프 엣지 직선 그리기와 왕복 엣지 라벨 배치](devlog/2026-10-10-Graph-Edge-Drawing.md): 폐기된 `FVector2D` 오버로드 재정의가 호출되지 않던 원인, 반대 방향 엣지 라벨 오프셋.

구현 전 단계의 Claude/Codex 설계 제안과 이전 Blueprint 중심 사용법은 현재 구조와 충돌해 폐기했다.

## Plan

- [카메라 시스템 계획](plan/Camera-Plan.md): KataCamera 궤도 트랙, 상태 블렌딩, Shrink, 디더링, 락온 화면 구성. [#20](https://github.com/jaykop/Kata/issues/20).
- [KataAI 구현 계획](plan/AI-Plan.md): 인지·타게팅·StateTree 역할, 마스터 트리 조립, 행동 배치, 실패 정책·Pressure 미확정 설계. [#22](https://github.com/jaykop/Kata/issues/22).
- [AI 그래프 트리거 발신 계획](plan/AI-Graph-Trigger-Plan.md): 액션 타임라인의 AI 트리거 발신 태스크, 판정 시점, AI 이탈의 Cancel Window 원칙. [#22](https://github.com/jaykop/Kata/issues/22).
- [KataActionGroup 계획](plan/Action-Group-Plan.md): Action·Graph의 가중 선택과 항목별 확장 Payload. [#22](https://github.com/jaykop/Kata/issues/22).
- [기본 태스크 확장 계획](plan/Base-Task-Plan.md): [#6](https://github.com/jaykop/Kata/issues/6)·[#7](https://github.com/jaykop/Kata/issues/7)의 설계 참고. 당시 제안과 현재 구현 전제를 구분한다.
- [액션 비용 정책 계획](plan/Cost-Policy-Plan.md): [#9](https://github.com/jaykop/Kata/issues/9).
- [장비·무기 시스템 계획](plan/Equipment-Plan.md): 부위 슬롯, Equipment·Weapon, 손별 그래프 조각의 런타임 합성, 스켈레톤별 Anim Layer 해석. [#30](https://github.com/jaykop/Kata/issues/30).
- [피격 반응 계획](plan/Hit-Reaction-Plan.md): Stance 세트와 반응 판정, 반응 GA, 상태·Impact 태그, 방향·재피격 규칙, 히트스톱과 피격 연출 Cue. [#23](https://github.com/jaykop/Kata/issues/23).
- [레벨 이벤트 계획](plan/Level-Event-Plan.md): KataWorld의 이벤트 채널, 트리거·지역 볼륨, 월드 플래그, 조우 StateTree와 사망 시 리셋 경계. [#63](https://github.com/jaykop/Kata/issues/63).
- [애니메이션 시스템 계획](plan/Animation-System-Plan.md): ABP 와 KataAction·KataGraph 의 역할 분담, 판정 우선순위·입력 종류별 창, 블렌드·회전 기준, 이동(제자리 회전·방향·락온·출발·정지) 결정 대기 항목. [#107](https://github.com/jaykop/Kata/issues/107).
- [상호작용 시스템 계획](plan/Interaction-Plan.md): 대상 영역 겹침으로 고르는 후보, 상호작용 GA와 확정 이벤트, 배치·액션 지정·위치 정렬·프롬프트 미확정 설계. [#62](https://github.com/jaykop/Kata/issues/62).

2026-09-24 작업 추적을 GitHub 이슈로 옮기면서 다음 작업 계획(Next-Work-Plan)을 삭제했다. 우선순위와 보류 항목은 이슈 #3~#12로,
Cost 정책과 멀티 타겟 배치 설계는 별도 plan으로 옮겼다. Kata·KataAction·KataGraph 요청 메모와 완료된 Play Montage 포즈 탐색 계획도
같은 날 삭제했으며 결과는 관련 devlog와 이슈에 남아 있다.

2026-10-16 [#76](https://github.com/jaykop/Kata/issues/76)에서 닫힌 이슈나 연결 이슈가 없는 계획 여섯 건(그래프 노드 타입, 캐릭터 데이터, 액션 게임 기반 시스템, GAS Inspector, 스포너 관리·타임슬라이싱, 스포너 휴면 계약)을 삭제했다.
결정은 [그래프 노드 타입 결정](devlog/2026-10-16-Graph-Node-Types.md), [캐릭터 행 적용 기록](devlog/2026-09-30-Character-Row-Spawn.md), [로드맵 종료](devlog/2026-10-16-Action-Game-Systems-Roadmap.md), [GAS Inspector 개편](devlog/2026-10-06-GAS-Inspector-UI.md), [스포너 관리자 구성](devlog/2026-10-16-Spawner-Scheduling-Decisions.md), [휴면 계약 결정](devlog/2026-10-07-Spawner-Dormancy-Contract.md)으로 옮겼다.

## 기획 문서

`docs/design`은 샘플 게임의 기획 문서를 둔다. 공개 저장소(Git)에서 제외하고 Diversion에서만 추적한다.
레벨 제작 절차(`Level-Process.md`), 공통 레벨 메트릭(`Metrics.md`), 레벨별 도안(`levels/<맵 이름>.md`와 평면도 SVG)이 있다.

## 로컬 문서

`docs/localdocs`는 공개 저장소(Git)에서 제외하고 Diversion에서만 추적한다.

- 작업별 인계 메모는 `localdocs/threads/<이슈번호>-<slug>.md`에 둔다. 새 작업 시작 시 생성하거나 갱신하며 같은 작업은 같은 파일을 이어 쓴다.
- 완료된 인계 메모는 사용자 요청 시 `localdocs/archive/<YYYY-QN>/`에 보관한다.
- 원본 게임 분석 문서는 `localdocs/animation/`에 둔다. 공개 문서(manual·devlog·plan)에 옮기지 않는다.

- [HKX 변환기](localdocs/Hkx-Converter.md): 스켈레톤 FBX, 표시용 메시, 애니메이션 한 개의 호환성 검사와 UE5 가져오기.
- [은기사 애니메이션 목록·AnimInstance 구성안](localdocs/SilverKnight-Animations.md): 212개 클립의 이동·이벤트·사용 후보와 Locomotion 재구성 제안. [CSV 목록](localdocs/SilverKnight-Animation-Index.csv).
- 추가 캐릭터 3종의 몸체·장비·재질과 694개 애니메이션 사용법, 이름 분류 근거와 CSV 대응표는 `localdocs/`에 보관한다.
- [테스트 콘텐츠 경로 정리](localdocs/2026-10-03-Content-Organization.md): 테스트 아트 이동, Redirector 확인과 남은 제한.
- [HKX 스켈레톤 변환](localdocs/2026-09-30-Hkx-Converter.md): 스켈레톤·캐릭터 변환, Sekiro 공통 몸체·422개 애니메이션 출력과 확인 범위.

- [폐지 전 상태 기록](localdocs/Implementation-Status-Archive-2026-10-03.md): 미커밋 기록과 과거 확인 결과의 보존용 사본. 갱신하지 않는다.

