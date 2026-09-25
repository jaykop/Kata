# 액션 에셋 모델과 명시적 상속

작성: 2026-09-25  
유형: 기존 구현·결정의 정리 기록  
대상: KataRuntime 에셋 해석·KataEditor 상속 편집  
기준: 현재 소스와 기존 Implementation-Status의 기록. 최초 결정일은 이 기록으로 새로 확정하지 않는다.  
관련 이슈: [#12](https://github.com/jaykop/Kata/issues/12)

## 배경과 결론

Kata 콘텐츠는 단일 UObject 타입인 UKataAction을 저장하는 전용 uasset이다.
이전 Blueprint/CDO 정의 경로를 제거하고 ParentAction 객체 체인과 명시적 프로퍼티 변경분으로 상속한다.
현재 설정, 실행용 사본, 캐릭터별 실행 상태를 분리한 결과를 이 기록에 모았다. 이번에 기능을 변경한 것은 아니다.

## 구조와 주요 결정

| 대상 | 계약과 이유 |
|---|---|
| UKataAction | 공유 설정·ParentAction·로컬 태스크와 오버라이드를 저장한다. 매 액션마다 BP 클래스를 만들 필요가 없다 |
| UKataResolvedAction | 부모부터 자식까지 병합한 실행용 사본과 SourceAction을 가진다. 실행마다 해석해 부모 최신 설정을 반영한다 |
| UKataActionInstance·UKataTaskInstance | 시간·태스크 상태·외부 핸들을 실행별로 소유해 같은 에셋의 여러 실행이 섞이지 않게 한다 |
| OverriddenSettings | 직접 수정한 프로퍼티 경로를 기록한다. 부모와 같은 값이어도 의도한 고정값이므로 자동 제거하지 않는다 |
| TimelineTasks·TaskOverrides | 로컬 태스크와 상속 태스크 변경을 TaskId로 구분한다. 배열 위치나 표시 이름을 정체성으로 쓰지 않는다 |

ParentAction 체인은 순환을 거절한다. 해석 결과 캐시는 두지 않는다.
실행 중 원본을 고쳐도 이미 복제된 실행의 설정이 즉시 바뀌지는 않는다.
고유 정책의 직접 구조체 필드는 개별 경로로, 조건·배열·태그 컨테이너와 Pre/Post Commands는 목록·프로퍼티 전체로 덮어쓴다.
부모를 바꿔 대상 태스크가 사라진 오버라이드를 다른 태스크에 임의 적용하지 않는다.

Instanced 객체는 직접 객체·객체 배열과 직접 객체 필드를 가진 구조체 배열까지 복제하는 경로가 있다.
이는 FKataPostCommandEntry 같은 설정 소유권을 보존하기 위한 범위다. Map/Set·더 깊은 중첩의 임의 객체 그래프까지 지원한다는 뜻은 아니다.
프리뷰 설정은 자식 생성 시 복사하며 런타임 상속과 별개다. TimelineGroups는 편집 전용이고 부모 그룹을 상속하지 않는다.

## 제거된 경로와 호환성

UKataDefinition, 클래스 실행 오버로드, SourceClass·DeclaringClass와 Import Legacy를 제거했다.
당시 상태 문서는 변환할 레거시 BP 콘텐츠가 없어 Import Legacy를 유지하지 않았다고 기록했다.
이를 모든 외부 구버전 콘텐츠가 안전하게 자동 변환된다는 의미로 확대하지 않는다.
현재 API·쿨다운·Loop·Distance·캐릭터 이동의 사용자 조치는 [에셋 이전 안내](../manual/Asset-Migration.md)에 있다.

## 근거와 확인 범위

- [KataAction.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Action/KataAction.cpp): CollectActionChain·BuildEffectiveSettings·Resolve·PostLoad.
- [KataPropertyOverride.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Action/KataPropertyOverride.cpp): 명시적 경로와 객체 복제.
- [KataResolvedAction.h](../../Plugins/Kata/Source/KataRuntime/Public/Action/KataResolvedAction.h): 병합 사본의 소유 데이터.
- [KataActionEditor.cpp](../../Plugins/Kata/Source/KataEditor/Private/KataActionEditor.cpp): 변경분 편집·Reset·Create Child.

기존 기록에는 리네임·레거시 제거 후 사용자 Editor 빌드와 액션·그래프 에셋 생성 확인이 있다. 정확한 확인일은 미상이다.
2026-09-24 Command 구조체 배열의 사용자 빌드·Details 확인은 있으나 실행은 미확인이다.
이번에는 문서·관련 소스를 읽었으며 빌드·에셋 로드·상속 회귀 테스트를 실행하지 않았다.

## 연관 문서와 제한

[런타임 사용법](../manual/Runtime-Usage.md)의 설정·상속과 [에디터 사용법](../manual/Editor-Usage.md)의 편집 절차를 갱신했다.
[현재 구현 상태](Implementation-Status.md)에는 현재 지원 범위와 남은 제한을 요약한다.
새로운 직렬화 정책이나 구버전 에셋 변환기를 이번 문서 정비에서 추가하지 않았다.
