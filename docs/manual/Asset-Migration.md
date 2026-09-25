# 기존 에셋과 API 이전 안내

갱신: 2026-09-25  
대상: 이전 Kata 에셋·Blueprint·C++ 참조를 가진 사용자  
적용 기준: UE 5.8 현재 PostLoad·설정·샘플 프로젝트 Redirect  
확인 상태: 소스·기존 기록 대조. 구버전 에셋의 실제 로드·재저장은 미실시.

## 목적과 준비

현재 콘텐츠는 UKataAction 객체 uasset이다. 아래에서 사용하는 구버전 표현을 찾아 해당 항목을 적용한다.
변경 전 에셋은 버전 관리나 백업으로 보존하고, 이전 후 참조·상속·실행 설정을 직접 확인한다.
알려지지 않은 모든 구버전 에셋의 자동 변환을 제공하지는 않는다.

## 변경별 처리

| 이전 형태 | 현재 형태 | 자동 처리와 사용자 조치 |
|---|---|---|
| Blueprint/CDO 기반 UKataDefinition | UKataAction·ParentAction | Import Legacy와 클래스 실행 경로는 제거됐다. 새 에셋에 필요한 설정·태스크를 옮긴다. BP→객체 일괄 변환은 없다 |
| Definition/KataAsset.h | Action/KataAction.h | include를 수정한다. PlayKataAction·PlayKataActionOnSelf·CanPlayKataAction은 에셋 포인터를 받는다 |
| 클래스 상속·DeclaringClass | ParentAction·TaskId·TaskOverrides | 부모 객체와 자식 변경분을 지정한다. 부모와 값이 같아져도 명시적 Reset 전까지 오버라이드는 유지한다 |
| 외부 Cooldown Effect·Owner·CooldownTags·EffectLevel | Enabled·Duration·Start Time·Shared Group Tags | 옛 값의 자동 재해석은 없다. 시간과 공유 태그를 다시 지정한다. 공유 태그가 비면 원본 에셋별 쿨다운이다 |
| LoopPolicy.MaxIterationsPerTick | 회차마다 다음 프레임에 재시작 | PostLoad가 삭제된 프로퍼티의 오버라이드 경로만 제거한다. MaxLoopCount와 나머지 변경분은 유지한다 |
| Distance MinDistance·MaxDistance | Comparison·CompareDistance | 자동 변환은 없다. 두 경계가 필요하면 Group(All)과 Distance 둘로 표현한다 |
| Distance 위치의 Mode·ComponentTag | SelfLocation·TargetLocation의 SocketName | 비면 Actor 위치, 있으면 ACharacter의 기본 Mesh 소켓이다. 옛 ComponentTag가 선택하던 메시를 자동 보존하지 않는다. 양쪽 SocketName과 기준 메시를 확인한다 |
| /Script/KataRuntime.KataCharacter | /Script/KataFramework.KataCharacter | 샘플에 ClassRedirects가 있다. 다른 프로젝트는 KataFramework 활성화와 Redirect 설정을 직접 반영한다 |
| /Script/ProjectKata의 테스트 타입 | /Script/ProjectKataTesting | 샘플에 클래스·enum Redirect가 있다. DeveloperTool 모듈이 포함되는 대상에서 사용한다 |
| 옛 안내의 ResetForLoop | ResetForExecution | 현재 함수명으로 읽는다. BP 이벤트가 아니며 사용자 변수는 OnTaskStarted에서 초기화한다 |
| 프리뷰용 SyncToKataTime | 실제 실행 시뮬레이션 | 현재 기본 인스턴스 API에서 제거됐다. 옛 override가 있으면 제거하고 정상 실행 콜백으로 구성한다 |

## Redirect와 설정 저장 범위

샘플 Config/DefaultEngine.ini는 KataTask_Debug·KataTaskInstance_Debug·KataTestActor·EKataTestAction의
ProjectKata→ProjectKataTesting 이동과 KataCharacter의 KataRuntime→KataFramework 이동을 기록한다.
플러그인 복사만으로 다른 프로젝트의 Redirect 설정이 추가되지는 않는다. 대상 프로젝트에서 실제 로드를 확인한다.

캐릭터 이동 후 2026-09-24 사용자 빌드 성공 기록은 있으나, 사용자는 테스트 캐릭터 BP를 새로 만들었다.
따라서 이 결과를 기존 BP Redirect 성공의 근거로 사용하지 않는다.
프리뷰 기본값이 바뀌어도 이미 저장한 Transform·조명·벽 설정이 일괄 갱신되는 것은 아니다.

## 사용자 확인 순서

1. 필요한 플러그인·모듈을 활성화하고 옛 include·타입 참조를 수정한다.
2. 에셋을 열어 ParentAction·명시적 오버라이드·태스크 클래스와 설정을 확인한다.
3. 표의 수동 항목을 재설정하고 사라진 태스크의 오버라이드를 의도에 맞게 복원·제거한다.
4. 프리뷰 캐릭터·Transform·조명·벽 설정을 확인한다.
5. 저장 후 다시 열어 참조와 값을 확인한다. 게임 실행은 별도로 확인한다.

태그 이름 변경은 [게임플레이 태그 사용법](Gameplay-Tags.md)의 GameplayTagRedirects 안내를 따른다.
프리뷰는 게임 초기화를 대신하지 않으며 Content/KataTest는 NeverCook 대상이다.

## 확인 상태와 근거

- [KataAction.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Action/KataAction.cpp): 삭제된 Loop 경로의 PostLoad 처리.
- [KataAction.h](../../Plugins/Kata/Source/KataRuntime/Public/Action/KataAction.h): 현행 에셋·프리뷰 설정.
- [KataConditionTypes.h](../../Plugins/Kata/Source/KataConditions/Public/KataConditionTypes.h): 위치 설정.
- [DefaultEngine.ini](../../Config/DefaultEngine.ini): 샘플 Redirect 선언. 실제 로드 성공과 구분한다.
- [모듈 분리 기록](../devlog/2026-09-24-Module-Structure-Diagnosis.md): 캐릭터 이동·사용자 확인 범위.
- [현재 구현 상태](../devlog/Implementation-Status.md): 지원 범위.
