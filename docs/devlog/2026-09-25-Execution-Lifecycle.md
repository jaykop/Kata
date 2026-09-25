# 실행 순서와 태스크 수명 결정

작성: 2026-09-25  
유형: 기존 구현·결정의 정리 기록  
대상: KataRuntime 실행기·스케줄러  
기준: 현재 소스와 기존 상태 기록. 원래 변경일을 알 수 없는 항목은 정리일과 구분한다.  
관련 이슈: [#12](https://github.com/jaykop/Kata/issues/12)

## 배경과 결론

실행기는 시각 0의 반응, 태스크 선후 관계, 게임 프레임당 Tick 횟수와 종료 정리를 한곳에서 관리한다.
Task 설정은 실행 상태를 갖지 않으며, 개별 인스턴스가 실행 수명과 자원을 소유한다.

## 주요 결정과 이유

| 결정 | 이유·계약 |
|---|---|
| 월드 실행 Subsystem | Actor·Component Tick 전 활성 인스턴스를 순차 진행한다. ExecutionPriority가 작을수록 먼저, 같으면 시작 순서다. 미지원 월드에서는 컴포넌트 Tick을 사용한다 |
| 최초 Start 즉시 실행 | 첫 Tick까지 미루면 월드 실행 콜백 뒤의 요청이 한 프레임 늦어진다. 시작 GAS·Pre Commands 뒤 시각 0 태스크와 Tick(0)을 처리한다 |
| 프레임당 태스크 Tick 최대 한 번 | 경계를 여럿 지나도 같은 프레임의 태스크 업데이트를 중복하지 않는다. 종료 태스크는 End 직전, 계속 실행하는 태스크는 진행 끝에 Tick한다 |
| 경계 허용 오차 통일 | 수집과 도달 판정의 오차가 달라 경계 바로 앞에서 AdvanceTo가 끝나지 않던 경우를 막는다 |
| 회차별 다음 프레임 재시작 | 프레임 하나에 반복 횟수를 몰아 처리하지 않는다. 초과 시간은 다음 회차에 넘기지 않아 지연 시 실제 총 시간이 늘어날 수 있다 |
| 종료 요청의 첫 사유 보존 | 콜백·경계 처리 중 요청이 겹쳐도 그래프 Branched를 뒤의 Interrupted가 덮지 않도록 한다 |

시작 시각·Phase·OrderHint·선언 순서와 AfterStart/AfterCompletion 의존성을 사용한다.
완료 의존성이 실행 갱신 중 충족되면 같은 갱신에서 후속을 시작하고, 갱신 밖의 콜백 완료는 다음 갱신에서 반영한다.
서로 다른 태스크의 모든 중간 상태를 Tick에서 관측하는 계약은 아니다. AfterMeshPose는 현재 해석 오류다.

## 순간·Single Frame·루프

- Duration 0은 Start→End, Single Frame은 Start→Tick 한 번→End다. 시작에서 스스로 완료한 태스크에는 Tick을 강제하지 않는다.
- MaxLoopCount는 최초 실행을 포함한다. 0은 무한 반복이며 길이가 0인 루프는 해석 오류다.
- 루프마다 같은 TaskInstance를 ResetForExecution으로 재사용한다. 사용자 변수는 시작 콜백에서 초기화한다.
- Restart On Loop를 끄면 후속 회차에 Skipped가 된다. 그 태스크의 완료를 기다리는 항목도 실행되지 않을 수 있다.
- 시작 조건·GAS 활성 상태·Pre/Post Commands·쿨다운은 회차마다 새로 처리하지 않는다.
- MaxIterationsPerTick과 횟수 초과 종료를 제거했고, PostLoad는 옛 오버라이드 경로만 정리한다.

## 종료와 확장

TaskInstance의 EndTask는 실행한 태스크의 OnTaskEnded를 한 번만 호출한다.
획득한 외부 핸들·타이머·구독은 이 콜백에서 정리한다. 아직 시작하지 않은 태스크의 정리를 이 콜백에 기대지 않는다.
Pre Commands는 시작 판정 뒤에 실행하며 그 구간에서만 대상 변경을 허용한다. Post Commands는
태스크 정리 뒤·GAS 회수 전에 실행하므로 종료 사유와 종료 시점의 활성 상태를 읽을 수 있다.
길이 0이나 시작 중 종료는 Play가 반환하기 전에 알림이 올 수 있다.

프리뷰는 여러 시뮬레이션 단계를 한 화면 프레임에 진행한다. 그 계약은
[프리뷰 실행 시뮬레이션 기록](2026-09-24-Preview-Scrub-Simulation.md)에 별도로 보존한다.
Gather/Commit처럼 모든 인스턴스 상태를 수집한 뒤 효과를 일괄 반영하는 다단계 실행은 구현하지 않았다.

## 근거와 확인 범위

- [KataExecutionWorldSubsystem.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataExecutionWorldSubsystem.cpp): 월드 진행·등록 순서.
- [KataActionInstance.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionInstance.cpp): 시작·경계·루프·종료·Command.
- [KataTaskInstance.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataTaskInstance.cpp): 완료·종료·리셋.
- [KataTaskScheduler.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataTaskScheduler.cpp): 의존성·경계.

기존 Loop·Tick 변경 기록에는 해당 변경의 별도 빌드·실행을 수행하지 않았다고 명시되어 있다.
이번에도 빌드·테스트·UI 실행은 하지 않았으며 과거 다른 기능의 빌드 확인을 이 계약 전체의 검증으로 보지 않는다.
사용 계약은 [Runtime-Usage](../manual/Runtime-Usage.md), 확장 절차는 [Task-Authoring](../manual/Task-Authoring.md)로 옮겼다.
