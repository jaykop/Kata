# GAS 책임과 기본 태스크 결정

작성: 2026-09-25  
유형: 기존 구현·결정의 정리 기록  
대상: KataRuntime GAS 연결·기본 태스크  
기준: 현재 소스·기존 상태 기록·기본 태스크 검토. 당시 제안과 실제 채택을 구분한다.  
관련 이슈: [#11](https://github.com/jaykop/Kata/issues/11)·[#12](https://github.com/jaykop/Kata/issues/12)

## 배경과 결론

Kata는 태스크의 실행 시점과 수명을 관리하고 ASC의 태그·효과·쿨다운·Ability 체계를 사용한다.
GAS와 별개의 Attribute·비용 시스템을 만들지 않는 것이 공용 지침이다.

## 주요 결정과 실제 결과

| 결정 | 현재 구현과 이유 |
|---|---|
| ASC 필수 | 실행 전에 없으면 MissingAbilitySystem을 반환한다. 명시 ASC가 있으면 우선하고 Avatar·Owner에서 조회한다 |
| 쿨다운 설정 단순화 | 내부 Duration GE를 사용해 Enabled·Duration·Start Time과 공유 태그만 작성한다. 외부 GE 선택·Owner 선택을 제거했다 |
| 개별 쿨다운의 원본 참조 | 병합 사본은 매 실행 달라지므로 SourceAction으로 같은 에셋을 식별한다. 공유 태그가 있으면 ASC 태그로 차단한다 |
| GE와 Loose Tag 분리 | 효과 핸들과 태그 참조 수의 수명은 다르다. 태그 하나에 GE 에셋을 강제하지 않는다 |
| 최소 대상 선택 | Avatar·Owner·ContextTarget을 공용 enum으로 제공한다. 공용 출력 채널·소켓·선행 태스크 출력까지 포함하는 TargetSpec은 아직 없다 |
| Gameplay Event | 대상 ASC의 HandleGameplayEvent로 발송한다. 태스크는 수신 동작을 알지 않으며 한 번 보낸 뒤 완료한다 |
| 몽타주 재생 경로 | 호출 Ability가 있으면 ASC, 없으면 Avatar의 AnimInstance 경로를 사용한다. 몽타주 종료 시계와 타임라인 종료를 별도 정책으로 둔다 |

활성 Loose Tag와 Ability 차단은 실행 시작에 적용하고 종료에서 회수한다. On End 쿨다운은 Branched를 포함해
활성화된 액션의 모든 종료에서 적용한다. 루프는 한 액션 실행이므로 회차마다 쿨다운을 다시 적용하지 않는다.
공유 쿨다운은 GroupTags 중 하나라도 보유하면 차단하므로 동일 태그를 외부에서 부여해도 판정에 영향을 준다.

## 태스크 자원 수명

Apply Gameplay Effect는 받은 ASC와 자신이 적용한 핸들을 보관한다. UseEffectDuration은 GE 수명을 따르고,
RemoveOnTaskEnd는 해당 핸들을 제거한다. Instant GE는 제거할 활성 효과가 없어 설정 시 경고한다.
Apply Loose Tag는 실제 붙인 태그·ASC를 기억해 정확히 짝을 맞춰 회수한다. 구간 없는 설정은 거절한다.
Play Montage는 재생 ID를 확인해 자신이 시작한 재생만 정지하며, 재생 실패는 경고와 태스크 완료로 처리한다.
Transition Window는 ASC 태그가 아니라 액션 인스턴스에 열린 창을 기록한다.

## 미구현 제안과 제한

현재 Kata 자체 Cost 정책, VFX/SFX·Hit Trace, 공용 태스크 출력 채널, 태스크별 프리뷰 정책은 구현하지 않았다.
계획의 비용 판정 순서는 현재 실행 사양이 아니다. 비용은 현재 호출 Ability·게임 코드의 책임이며
후속은 [Cost 이슈 #9](https://github.com/jaykop/Kata/issues/9)를 따른다.
AbilityTask의 일반 종료 분기는 Branched를 구분하지만 Activate 안에서 이미 끝난 인스턴스는
OnCompleted(Completed)로 알리는 제한이 있다. 문서 정비 중 발견한 현재 동작이며 이번에 변경하지 않았다.

## 근거와 확인 범위

- [KataGasBridge.cpp](../../Plugins/Kata/Source/KataRuntime/Private/GAS/KataGasBridge.cpp): 활성 태그·차단·쿨다운.
- [KataRuntimeTypes.cpp](../../Plugins/Kata/Source/KataRuntime/Private/KataRuntimeTypes.cpp): Actor·ASC 선택.
- [Apply Gameplay Effect](../../Plugins/Kata/Source/KataRuntime/Private/Tasks/KataTask_ApplyGameplayEffect.cpp), [Apply Loose Tag](../../Plugins/Kata/Source/KataRuntime/Private/Tasks/KataTask_ApplyLooseTag.cpp): 회수 소유권.
- [Play Montage](../../Plugins/Kata/Source/KataRuntime/Private/Tasks/KataTask_PlayMontage.cpp), [AbilityTask](../../Plugins/Kata/Source/KataRuntime/Private/GAS/AbilityTask_PlayKataAction.cpp): 두 시계와 종료 알림.

기존 기록은 GE·이벤트·Loose Tag의 개별 실행을 미확인으로 남겼다. 이후 통합 빌드·설정 화면의 성공 보고만으로
각 효과·수신·회수를 확인했다고 보지 않는다. 이번에는 문서·소스만 대조하고 빌드·테스트를 실행하지 않았다.
현재 사용법은 [Runtime-Usage](../manual/Runtime-Usage.md#기본-태스크), 제작·수명 계약은 [Task-Authoring](../manual/Task-Authoring.md)에 연결했다.
