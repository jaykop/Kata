# 락온 토글과 좌우 전환 입력

작성: 2026-10-04  
갱신: 2026-10-04  
유형: 구현 기록  
대상: KataFramework 입력 설정·처리 컴포넌트  
기준: [#19](https://github.com/jaykop/Kata/issues/19) IN-4 작업 트리. 다른 작업의 미커밋 변경은 포함하지 않는다.

## 배경과 결론

부위 단위 락온 API는 KataTargeting에 있었지만 입력 계층에서 호출하지 않았다.
사용자가 획득·해제 토글, 카메라 기준 왼쪽 전환, 오른쪽 전환의 세 InputAction을 사용하기로 확정해 입력 설정과 바인딩을 추가했다.

## 변경 내용과 결정 이유

- `UKataInputConfig`에 `ToggleLockAction`, `SwitchLockLeftAction`, `SwitchLockRightAction`을 추가했다. 공유 에셋에는 Action 참조만 두고 락온 상태는 타게팅 컴포넌트가 유지한다.
- 입력 핸들러는 폰의 `UKataPlayerTargetingComponent`를 찾아 기존 API를 호출한다. 토글은 `IsLocked()`에 따라 해제하거나 획득을 시도한다.
- 입력을 누르고 있는 동안 반복해서 토글되거나 부위가 바뀌지 않도록 `Started`에 바인딩했다.
- 좌우 판정과 후보 선택은 기존 방향별 Preset에 맡긴다. Input 태그와 그래프 전이를 거치지 않는다.
- 기존 KataFramework → KataTargeting 의존성을 사용하며 새 의존성·타입 리네임·Redirect는 추가하지 않았다.

## 근거

- [입력 설정](../../Plugins/KataFramework/Source/KataFramework/Public/Input/KataInputConfig.h): 세 Action 참조.
- [입력 처리](../../Plugins/KataFramework/Source/KataFramework/Private/Input/KataInputHandlerComponent.cpp): 바인딩과 타게팅 API 호출.
- [입력 사용법](../manual/Input.md#락온-입력): 설정 절차와 실패 시 동작.

## 확인 범위와 결과

소스 작성만 수행했다. 에이전트 빌드·테스트·별도 코드 검사는 실행하지 않았다.
이번 변경의 사용자 빌드·PIE 확인과 샘플 InputAction·IMC·Input Config 설정은 아직 수행되지 않았다.

## 남은 제한과 후속 작업

사용자는 세 Digital InputAction을 IMC에 매핑하고 Input Config에 지정한 뒤 획득·해제·양방향 전환을 확인해야 한다.
락온이 없거나 방향별 후보가 없을 때 상태 유지, 같은 액터의 다른 부위 전환도 확인 대상이다.
카메라 추적과 구도 보정은 [#20](https://github.com/jaykop/Kata/issues/20) CAM-5 범위다.

## 연관 문서 반영

- Input manual: 설정과 실행 계약을 반영했다.
- Input plan: 세 InputAction의 사용자 결정을 반영했다.
- Targeting manual: 기존 타게팅 API 계약은 변경되지 않는다.
- GitHub #19: 공개 게시와 라벨 변경은 하지 않았다. 사용자 확인 후 결과를 게시한다.
