# KataActionGroup 사용법

갱신: 2026-10-05  
대상: Action·Graph 가중 목록과 Payload  
적용 기준: KataGraph의 UKataActionGroup·UKataFL_ActionGroup, [#22](https://github.com/jaykop/Kata/issues/22)  
확인 상태: 소스 구현. 빌드·실행·에디터 UI 미확인

## 목적과 준비

Kata Action Group은 Action과 Graph를 같은 목록에 저장하고 항목별 Weight와 추가 설정을 제공한다. 그룹은 실행 상태를 저장하지 않는다. 선택 함수도 실제 실행을 시작하지 않으며 소비자가 기존 Action·Graph API를 호출한다.

## 사용 순서

1. Content Browser에서 Data Asset을 만들고 `Kata Action Group` 클래스를 선택한다. 샘플 에셋은 Content/KataTest에 둔다.
2. Entries에 항목을 추가하고 Type을 Action 또는 Graph로 선택한다. 표시된 에셋 필드에 실행 대상을 지정한다.
3. Weight를 지정한다. 기본값은 1이며 0이면 선택되지 않는다. Payload는 필요할 때만 지정한다.
4. 실행 주체가 허용할 항목의 원본 인덱스를 Candidate Indices 배열로 준비한다. 전체 목록을 사용할 때는 0부터 Entries.Num()-1까지 넣는다.
5. Random Value를 한 번 생성해 변수에 저장한 뒤 `Select Kata Action Group Entry`에 전달한다. 입력은 0 이상 1 미만이다.
6. true이면 Selection의 Entry Index와 Entry를 사용한다. false이면 실행하지 않는다. 선택한 에셋의 실제 시작 결과도 확인한다.

Blueprint Pure 함수는 연결에 따라 다시 평가될 수 있다. 한 번의 추첨 결과를 여러 곳에서 사용하려면 Random Value와 Selection을 변수로 보관한다.

## 주요 설정과 계약

| 항목 | 계약 |
|---|---|
| Type | Action 또는 Graph. 선택한 종류의 참조만 사용 |
| Action / Graph | 하드 참조. 그룹 로드 시 구성 에셋도 로드 |
| Weight | 유한한 0 이상. 선택에는 양수만 반영 |
| Payload | FKataActionGroupPayload 기반 FInstancedStruct. 빈 값 허용 |
| Candidate Indices | 원본 인덱스. 중복은 한 번 반영, 범위 밖은 제외, 빈 배열은 실패 |
| Random Value | 유한한 [0, 1). 잘못된 값은 실패 |
| Selection | EntryIndex와 Entry 사본. 실패하면 INDEX_NONE·기본 Entry로 초기화 |

`UKataFL_ActionGroup::TrySelectEntry`는 유효한 활성 에셋·Payload와 양수 Weight를 가진 후보를 순서대로 모아 선택한다. Weight 1·3이면 확률은 25%·75%다. 누적 구간 경계는 다음 후보에 속한다. 같은 에셋을 서로 다른 항목에 지정하면 각 항목의 Weight와 Payload가 별도로 적용된다.
Type 변경 시 숨겨진 참조는 자동 삭제하지 않는다. 데이터 검증은 남은 비활성 참조를 경고하며 실행에는 사용하지 않는다. 빈 목록·활성 참조 누락·음수 또는 비유한 Weight·전체 Weight 0·잘못된 Payload 타입은 오류다.

## Payload 확장

소비 모듈에서 FKataActionGroupPayload를 상속한 BlueprintType USTRUCT를 정의한다. 타입 선택기는 해당 기반의 파생 구조체로 제한한다. PressureValue 같은 설정은 소비자가 실제 타입을 확인해 읽는다. 기본 기반 구조체 자체는 선택기에서 숨긴다.
그룹은 Payload의 필드 의미를 해석하지 않는다. 대상·타이머·예약 핸들은 실행 주체에 보관한다. Payload와 항목의 UObject 참조는 엔진 GC 추적을 따른다. C++ 소비자는 KataGraph에 의존하고 ActionGroup/KataActionGroup.h·FunctionLibraries/KataFL_ActionGroup.h를 포함한다.

## 제한과 확인 상태

선택은 실행 성공을 보장하지 않는다. Action의 조건·쿨다운은 소비자가 CanPlayKataAction으로 판단하고 실제 시작 결과를 확인한다. Graph의 초기화 성공과 첫 Action 실행 성공도 서로 다르다. 그룹은 사전 검사를 위해 Graph를 실행하지 않는다.
그룹을 실행하는 KataAI의 `Play KataActionGroup` StateTree Task는 [AI 사용법](AI.md)을 따른다. Pressure 시스템·중첩 그룹·비동기 소프트 로딩은 제공하지 않는다. 기존 실행 API·에셋은 변경하지 않았고 샘플 uasset은 생성하지 않았다.
빌드·테스트·별도 검사·UI 확인은 실행하지 않았다. 사용자 확인 항목은 에셋 생성, Type에 따른 필드 표시, 파생 Payload 선택, 데이터 검증, 후보·Random Value에 따른 선택 결과다.

- [그룹](../../Plugins/Kata/Source/KataGraph/Public/ActionGroup/KataActionGroup.h)
- [선택 API](../../Plugins/Kata/Source/KataGraph/Public/FunctionLibraries/KataFL_ActionGroup.h)
- [변경 기록](../devlog/2026-10-05-Action-Group.md)

