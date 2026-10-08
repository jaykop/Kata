# Action·Graph 가중 그룹과 Payload

작성: 2026-10-05  
갱신: 2026-10-05  
유형: 구현 기록  
대상: KataGraph의 UKataActionGroup  
기준: #22 관련 미커밋 소스

## 배경과 결론

사용자가 AI 실행 Task보다 먼저 그룹 에셋 구현을 승인했다. Task 내부의 가중 목록 대신 UDataAsset에 Action·Graph·Weight·Payload를 모았다. 선택과 실행을 분리해 다른 소비자도 재사용할 수 있다.

## 변경과 결정 이유

KataGraph는 이미 KataRuntime에 의존하므로 그룹도 해당 모듈에 두었다. 일반 Data Asset 생성 경로를 사용하고 전용 에디터·Primary Asset 정책은 추가하지 않았다. FInstancedStruct는 UE 5.8 CoreUObject 공개 헤더에 있어 모듈 의존을 늘리지 않았다.
Payload는 FKataActionGroupPayload 기반의 확장 구조체다. 코어는 설정을 보관·복사하고 의미는 소비 모듈이 해석한다. Pressure 예약·해제는 실행 주체의 책임이다.
선택 함수는 후보 인덱스와 호출자 RandomValue를 받는다. RNG·공유 상태를 변경하지 않고 중복 후보 제거 후 double 누적으로 선택한다. 실행 가능 여부는 소비자가 판단한다. 에디터 검증은 잘못된 에셋·Weight·Payload를 진단하고 런타임도 잘못된 후보를 제외한다.

## 근거와 확인 범위

- [그룹](../../Plugins/Kata/Source/KataGraph/Public/ActionGroup/KataActionGroup.h): 데이터·Payload·Selection 계약.
- [선택](../../Plugins/Kata/Source/KataGraph/Private/FunctionLibraries/KataFL_ActionGroup.cpp): 후보 처리와 가중 구간.
- [사용법](../manual/Action-Group.md): 생성·확장·실행 책임.

소스 구현과 문서 반영만 수행했다. 빌드·테스트·별도 검사·실행·UI 확인은 미실시다. 기존 콘텐츠와 실행 API는 변경하지 않았다.

## 후속과 문서 반영

Play KataActionGroup StateTree Task, Graph 관측 API, Pressure 시스템은 후속 작업이다. Action-Group manual·Runtime-Usage·README에 반영했고 [계획](../plan/Action-Group-Plan.md)은 사용자 확인 전 설계 근거로 유지한다. [#22](https://github.com/jaykop/Kata/issues/22) 공개 게시·라벨 변경은 수행하지 않았다.

## 변수 타입 목록 누락 수정 (2026-10-08)

StateTree 파라미터 타입 선택기에 Kata Action Group이 나오지 않았다. UHT는 `NotBlueprintable` 지정자를 처리할 때 `IsBlueprintBase=false`를 설정하면서 앞서 붙은 `BlueprintType` 메타데이터를 제거한다(UE 5.8 `UhtDefaultSpecifiers.cs`). 그래서 `UCLASS(BlueprintType, NotBlueprintable)`는 Blueprint 변수 타입이 아니게 된다. `UKataActionGroup`과 같은 조합을 쓰던 `UKataAction`을 `UCLASS(BlueprintType, meta = (IsBlueprintBase = "false"))`로 바꿔 블루프린트 상속 차단은 유지하고 변수·파라미터 타입으로 고를 수 있게 했다. 직렬화 이름은 바뀌지 않는다. 빌드와 선택기 확인은 사용자 확인 전이다.
