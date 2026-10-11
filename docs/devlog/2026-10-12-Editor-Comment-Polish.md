# 에디터 주석의 표현과 실행 로직 대조

작성: 2026-10-12  
갱신: 2026-10-12  
유형: 진단·교정 기록  
대상: Kata 코어·위성·통합 플러그인의 에디터 설정·API 주석  
기준: 2026-10-12 작업 트리. 이 작업의 주석 변경을 포함하며 기존 에디터 설정 변경은 대상에서 제외했다.

## 배경과 결론

사용자가 에디터에서 사람이 읽는 주석을 자연스러운 한국어로 고치고, 설명이 실제 로직에 맞는지 검토해 달라고 요청했다. 설정·태스크·조건의 주석과 해당 값을 사용하는 구현을 함께 읽었다. 동작 조건이나 기준점이 빠진 설명을 보완하고, 이전 구현을 설명하던 문장을 현재 구현에 맞췄다. 실행 로직·기본값·식별자·문자열·리플렉션 메타데이터는 변경하지 않았다.

## 변경 또는 진단 내용

| 항목 | 이전 설명 또는 발견 내용 | 반영 결과 |
|---|---|---|
| 액션 업데이트 | 컴포넌트 Tick으로만 처리한다고 설명 | 월드 Subsystem 처리와 Subsystem이 없는 월드의 컴포넌트 Tick 경로를 구분 |
| 몽타주 종료 | 액션 전체를 항상 중단한다고 설명 | 자연 종료는 Completed, 재생 중단은 Interrupted로 설명 |
| 조건 그룹 | 자식의 Invalid를 모두 전파하는 것처럼 설명 | 배열 순서에 따른 조기 종료와 실제로 평가한 자식의 Invalid만 전파한다는 점을 명시 |
| AI 설정·트리거 | 이동 재시도 상한을 StateTree 파라미터로 설명하고 전이 창을 무조건 요구 | AI Data의 MaxMoveRetries와 엣지의 창 태그 지정 조건으로 수정 |
| 판정·생성 도형 | 반경·반높이·반 크기의 대상이 모호함 | 캡슐 전체 높이의 절반, 중심에서 면까지 거리와 cm 단위를 설명 |
| 서브스텝 | 거리·각도 기준이 항상 지켜지는 것처럼 설명 | MaxSubsteps 상한에 걸리면 간격이 기준을 넘을 수 있음을 명시 |
| 거리 보정·카메라 | 최소 이동이나 장애물 회피 결과를 보장 | 보정값의 범위, 후속 전진 제한·충돌, 스윕 시작점과 복귀 보간을 구분 |
| 장비 효과 | 장비 해제 시 모든 GE 결과를 되돌리는 것처럼 설명 | 유지 중인 효과 제거와 Instant 효과로 이미 바뀐 값의 차이를 명시 |
| 루트 모션 추출 | 재추출은 수동 Apply만 가능하다고 설명 | Reapply Post Owner Change 설정에 따른 자동 재적용을 설명 |

## 주요 결정과 이유

설정 툴팁은 역할을 먼저 설명하고, 값의 단위와 적용 조건을 이어 쓴다. 설정 목적을 결과 보장처럼 쓰지 않는다. 액터 중심·피벗·캡슐 표면·HurtBox 표면처럼 계산 기준이 다른 위치는 구분해서 적는다.

직렬화 호환성과 UHT 메타데이터 선택 이유는 개발자에게 필요한 정보이므로 별도 구현 주석으로 옮겼다. 설정 주석에는 상속 방식과 실행 조건을 남겼다. 명시적인 영어 ToolTip 메타데이터는 유지했다. 따라서 한국어 주석 변경이 모든 UI 툴팁의 한국어 표시를 뜻하지는 않는다.

사용자의 요청에 따라 Claude의 doc-polish에 이 기준을 보강하고, Codex용 editor-comment-polish를 별도 프로젝트 스킬로 작성했다. 이 로컬 스킬들은 저장소의 추적 제외 정책을 따른다.

## 근거

- [액션 컴포넌트](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionComponent.cpp): 실행 Subsystem 등록 여부에 따라 컴포넌트 Tick 사용을 결정한다.
- [몽타주 태스크](../../Plugins/Kata/Source/KataRuntime/Private/Tasks/KataTask_PlayMontage.cpp): HandleMontageEnded에서 자연 종료와 재생 중단을 구분한다.
- [조건 그룹](../../Plugins/Kata/Source/KataConditions/Private/Conditions/KataCondition_Group.cpp): 결과가 확정되면 이후 자식을 평가하지 않는다.
- [거리 보정](../../Plugins/KataFramework/Source/KataFramework/Private/Animation/KataRootMotionCurveComponent.cpp): ComputeDistanceScale의 이동량 범위와 후속 ApplyApproachLimit 처리를 구분한다.
- [장비 컴포넌트](../../Plugins/KataFramework/Source/KataFramework/Private/Equipment/KataEquipmentComponent.cpp): 적용 후 유효한 활성 효과 핸들을 보관하고 해제 시 제거한다.
- [추출 수정자](../../Plugins/KataFramework/Source/KataFrameworkEditor/Private/Animation/KataRootMotionCurveModifier.cpp): 수동·자동 재적용이 같은 커브 덮어쓰기 경로를 사용한다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 변경한 주석 | 2026-10-12 에이전트가 관련 구현과 원문을 읽어 대조 | 위 표의 불일치를 교정하고 표현·단위·대상을 정리 | 소스 전체 주석의 전수 검토 |
| 실행·화면 표시 | 수행하지 않음 | 이번 교정에 대한 실행 확인 결과 없음 | 빌드·테스트·lint·별도 검사·에디터 UI |

과거 매뉴얼의 사용자 확인 기록은 유지했다. 이번 주석 교정의 빌드·실행 결과로 사용하지 않는다.

## 남은 제한과 후속 작업

UHT가 생성한 메타데이터와 실제 에디터 툴팁 표시는 사용자 빌드·UI 확인이 필요하다. 코드 변경이 필요한 결함 수정이나 UI 한글화는 이번 교정 범위에 포함하지 않았다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [루트 모션 커브](../manual/Root-Motion-Curve.md) | 자동 재적용 조건과 거리 보정 상·하한의 적용 범위 수정 |
| [Hit Trace](../manual/Hit-Trace.md) | 서브스텝 상한과 거리·각도 간격의 관계 보완 |
| [장비](../manual/Equipment.md) | 해제 시 유지 중인 효과 제거와 Instant 효과의 차이 보완 |
| 그 밖의 관련 manual | 이번 교정으로 사용 계약을 바꾸지 않아 별도 변경 없음 |
| plan | 설계 변경 없음 |

