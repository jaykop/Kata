# KataActionGroup 데이터 에셋 계획

작성: 2026-10-05  
갱신: 2026-10-05  
연결 이슈: [#22 KataAI](https://github.com/jaykop/Kata/issues/22)  
현재 상태 근거: KataGraph·KataRuntime 소스, [AI 실행 Task 계획](AI-Plan.md#ai-3-실행-task-확장-계획)  
대체 관계: AI-3의 Task 내부 가중 목록 제안을 그룹 에셋으로 대체

## 목적과 현재 구조

사용자는 UKataActionGroup 데이터 에셋을 먼저 계획하도록 요청했다. Action과 Graph를 같은 목록에서 Weight에 따라 선택하고 각 항목에 향후 Pressure Value 같은 Payload를 담는다. 그룹을 StateTree 외의 실행 경로에서도 재사용할 수 있도록 공유 데이터·선택·실행을 분리한다.

UKataGraph는 KataGraph 모듈에 있고 KataGraph는 이미 KataRuntime에 의존한다. 따라서 두 에셋을 참조하는 그룹도 KataGraph에 둔다. KataRuntime이 Graph나 KataAI를 참조하게 만들지 않는다. 기존 Action과 Graph의 실행 API·에셋은 유지한다.

## 범위와 결정 구분

| 항목 | 구분 | 내용 |
|---|---|---|
| 에셋 이름 | 확정 요구 | UKataActionGroup |
| 항목 | 확정 요구 | Action 또는 Graph, Weight, 확장 Payload |
| 실행 상태 | 확정된 구조 원칙 | 공유 에셋에는 인스턴스·대상·추첨 기록·예약 핸들을 저장하지 않음 |
| 모듈·기반 | 제안 | KataGraph의 UDataAsset. Primary Asset 관리가 필요해질 때 별도 결정 |
| Payload 표현 | 제안 | FKataActionGroupPayload 기반 FInstancedStruct, 비어 있어도 허용 |
| 선택 정책 | 제안 | 호출자가 제공한 후보 인덱스에서 가중 선택. 시작 가능 여부를 그룹이 추측하지 않음 |
| 중첩 그룹 | 제외 | 항목은 Action·Graph만 허용. 재귀 그룹과 순환 처리 없음 |
| 실행·Pressure | 후속 | StateTree Task, Graph 관측 API, Pressure 평가·예약·해제 |

이번 요청은 구현 계획이다. 소스·콘텐츠 에셋 생성은 구현 요청 후 수행한다.

## 데이터 계약

### UKataActionGroup

UCLASS BlueprintType, NotBlueprintable, DisplayName="Kata Action Group"인 UDataAsset으로 제안한다. Content Browser의 Data Asset 생성 경로를 사용하고 이번 범위에서는 전용 에디터·Factory를 추가하지 않는다.

Entries는 EditAnywhere·BlueprintReadOnly인 TArray<FKataActionGroupEntry>다. 배열 순서는 저장 순서이며 기본 선택 우선순위가 아니다. 개별 항목에는 다음 설정을 둔다.

| 필드 | 타입·기본값 | 계약 |
|---|---|---|
| Type | EKataActionGroupEntryType, Action | Action 또는 Graph |
| Action | TObjectPtr<UKataAction> | Action 타입일 때만 사용·표시 |
| Graph | TObjectPtr<UKataGraph> | Graph 타입일 때만 사용·표시 |
| Weight | float, 1.0 | 유한한 0 이상. 0이면 추첨에서 제외 |
| Payload | FInstancedStruct | 항목별 추가 설정. 빈 값 허용 |

Type에 따른 EditCondition·EditConditionHides로 선택한 에셋 필드만 표시한다. 타입을 바꿨을 때 숨겨진 참조를 자동 삭제하지 않는다. 런타임은 선택된 Type의 참조만 사용하고, 비활성 참조는 에디터에서 사용하지 않는 값으로 안내한다.

참조는 하드 참조로 제안한다. 그룹을 로드할 때 구성 Action·Graph와 Payload의 UObject 참조도 함께 로드되어, 선택 중 비동기 로드 상태를 처리하지 않아도 된다. 대규모 목록의 메모리 비용이 생기면 소프트 로딩을 별도 설계한다.

### Payload

FKataActionGroupPayload는 비어 있는 BlueprintType USTRUCT 기반이다. Payload의 BaseStruct 메타로 해당 타입의 파생 구조체만 선택하게 한다. 빈 Payload를 표현할 수 있으며, 기본 구조체 자체를 선택하는 것은 숨기는 안을 제안한다.

코어는 Payload를 보관·반환만 하고 필드 의미를 해석하지 않는다. 이후 KataAI가 FKataAIActionGroupPayload 같은 파생 구조체에 PressureValue를 정의할 수 있다. 소비자는 실제 구조체 타입을 확인해서 읽는다. 잘못된 타입이나 미지정 값의 처리 정책은 해당 소비자가 정의한다.

Payload는 설정 전용이다. Pressure 예약 핸들·실행 중 시간·현재 대상은 Task 또는 캐릭터별 실행 객체에 둔다. 일반 선택 결과로 Payload 사본을 돌려주고 공유 Entries를 수정하지 않는다. 단순 float 하나 때문에 UObject를 항목마다 생성하지 않도록 Instanced UObject 대신 구조체로 제안한다.

## 가중 선택 API 제안

UKataFL_ActionGroup에 순수한 선택 함수를 둔다. 입력은 Group, CandidateIndices, RandomValue이며 출력은 bool과 FKataActionGroupSelection이다. Kata 내부 실행 Context를 함수 매개변수로 받지 않는다.

- CandidateIndices는 호출자가 허용한 원본 Entries 인덱스다. 빈 배열이면 선택 실패이며 모든 항목이라는 의미로 사용하지 않는다. 모든 항목을 사용할 때는 호출자가 전체 인덱스를 구성한다.
- 유효하지 않은 인덱스는 제외하고 중복 인덱스는 한 번만 반영한다. 실제 후보는 유효한 활성 에셋과 유한한 양수 Weight를 가진 항목이다.
- RandomValue는 호출자가 만든 유한한 [0, 1) 값이다. 범위 밖이면 실패한다. 그룹 함수가 전역 RNG나 공유 RandomStream을 변경하지 않아 순수 평가가 가능하다.
- 후보 Weight 합은 double로 계산한다. RandomValue를 누적 가중 구간에 대응시키고 부동소수점 경계에서 마지막 양수 후보를 반환한다. 데이터 검증은 음수·NaN·무한대를 오류로 진단한다.
- 성공 결과에는 EntryIndex와 선택된 FKataActionGroupEntry의 사본을 넣는다. 그 안에 Type·Action/Graph·Weight·Payload가 포함된다. 실패 시 결과를 초기화하고 무효 인덱스로 둔다.
- 동일한 Action 또는 Graph를 서로 다른 항목에 넣는 것은 허용한다. 각 항목의 Weight와 Payload가 별도로 적용된다. 고유 인덱스는 이번 선택의 식별자이며 실행 중 그룹 목록을 수정하는 기능은 제공하지 않는다.

예를 들어 Weight 1·3인 두 후보의 확률은 25%·75%다. 하나가 호출자 필터에서 빠지면 남은 후보들의 Weight로 다시 정규화한다. 선택 함수는 실제 실행 성공을 보장하지 않는다.

### 실행 가능 여부와 Graph 제한

Action 후보는 소비자가 CanPlayKataAction으로 사전 판단할 수 있다. 실제 Play의 초기화·시작 결과는 다시 확인한다. Graph에는 같은 수준의 CanStart API가 없고 StartGraph는 자동 진입 거절에도 초기화 성공을 반환할 수 있다. 따라서 그룹이 임의로 Graph를 실행해 사전 검사하거나 진입 경로를 복제 탐색하지 않는다.

첫 그룹 에셋 구현에는 선택 API만 포함한다. AI 소비자의 후보 필터·Graph 시작 결과 처리·실패 후 재시도는 후속 Task 계획에서 구체화한다. 선택 이후 실행 실패 시 같은 호출에서 다른 항목을 반복 실행하는 기능은 그룹에 두지 않는다.

Graph의 EntryTrigger는 이번 데이터 필드에 추가하지 않는다. 기본 자동 진입 Graph를 사용할 수 있고, Trigger 진입은 실행 Task의 입력 또는 후속 별도 Payload 계약으로 구성한다. 현재 범용 Payload를 임의로 EntryTrigger 구조체로 해석하지 않는다.

## 데이터 검증·수명

IsDataValid는 WITH_EDITOR 안에서 빈 목록, 활성 에셋 누락, 음수·비유한 Weight, 전체 Weight 0인 목록을 진단한다. Weight 0인 항목도 활성 참조 계약은 지킨다. Payload 내부의 Pressure 값을 코어 검증이 판단하지 않는다. 잘못된 Payload 기반 타입은 검증 오류로 처리한다.

런타임 선택도 활성 참조·Weight·인덱스를 확인한다. 에디터 검증을 통과했다고 가정하지 않는다. UObject 참조는 UPROPERTY로 GC 추적하고 FInstancedStruct 안의 참조도 엔진 추적을 사용한다. 선택 함수는 자원·이벤트 구독·Pressure 예약을 획득하지 않아 별도 정리 수명이 없다.

기존 타입·프로퍼티·에셋 경로를 바꾸지 않는 신규 에셋이므로 Redirect는 필요하지 않다. 기존 Action의 부모·자식 오버라이드 및 Graph 전이 규칙은 변경하지 않는다. UObject를 참조하는 Payload는 제공 모듈과 타입이 런타임 빌드에도 있어야 한다.

## 파일별 계획과 작업 순서

| 파일 | 역할 |
|---|---|
| KataGraph/Public/ActionGroup/KataActionGroup.h | Type·Payload 기반·Entry·Selection·Data Asset 타입 |
| KataGraph/Private/ActionGroup/KataActionGroup.cpp | 에디터 데이터 검증 |
| KataGraph/Public/FunctionLibraries/KataFL_ActionGroup.h | Blueprint·C++ 선택 계약 |
| KataGraph/Private/FunctionLibraries/KataFL_ActionGroup.cpp | 후보 검증·중복 제거·가중 선택 |
| KataGraph.Build.cs | FInstancedStruct의 실제 엔진 모듈 확인 후 필요한 의존만 반영 |
| docs/manual/Action-Group.md | 생성·항목·Payload·선택 함수·제한 |
| docs/devlog 주제 기록·docs/README.md | 결정 이유와 사용법 진입점 |

위 소스 경로는 Plugins/Kata/Source 아래다. 재사용 코드를 샘플 프로젝트에 넣지 않는다.

순서: 데이터 타입·에셋 → Payload 편집 메타·데이터 검증 → 선택 함수 → manual·devlog. 이후 AI-3는 Play KataAction·Play KataGraph·Play KataActionGroup 세 Task로 재계획한다. 그룹 Task는 한 번 선택한 Action 또는 Graph의 수명 전체를 기다리고 SelectedIndex·Payload를 Output으로 제공한다. 그룹 에셋이 직접 실행하지 않는다.

## 완료 조건과 사용자 확인

구현 완료 조건은 Data Asset 생성 타입·Action/Graph 항목 편집·Payload 확장·가중 선택·빈/잘못된 후보 실패·GC 참조 계약·문서 반영이다. 런타임 실행 Task와 Pressure 시스템은 이 완료 조건에 포함하지 않는다.

사용자 확인은 Editor 빌드, 그룹 에셋 생성, Type 변경 표시, 파생 Payload 선택, Weight 0·빈 목록·잘못된 설정 진단, 지정 RandomValue에 따른 경계 선택이다. 스트레스 테스트 코드·자동화 테스트는 추가하지 않는다. 이번 계획에서는 소스 읽기만 수행했으며 빌드·테스트·별도 검증을 실행하지 않았다.

## 완료 시 갱신할 문서

- [#22](https://github.com/jaykop/Kata/issues/22): 그룹 에셋 선행 범위와 후속 Task. 게시 초안은 사용자 확인 후 반영.
- [AI 계획](AI-Plan.md): 기존 가중 목록 Task 대신 그룹 소비 Task로 연결.
- Action-Group manual·주제 devlog: 구현 후 현재 계약과 결정 이유.
- [문서 목록](../README.md): 신규 문서 진입점.
