# GAS Inspector의 값 수집과 에디터 도구 구성

작성: 2026-10-05  
갱신: 2026-10-05  
유형: 구현 기록  
대상: KataGASInspector / KataGASInspectorEditor  
기준: HEAD `7a67164` 이후 현재 작업 트리의 신규 도구 소스. 2026-10-05 사용자 빌드 성공 및 테스트 완료 보고.

## 배경과 결론

GAS 상태를 대상별로 확인하고 콘텐츠의 Ability Trigger 설정을 찾는 Editor 전용 도구를 추가했다. 기존 ASC를 관찰하며 게임플레이 플러그인에는 의존하지 않는다. 사용자 요청에 따라 `KataGASInspector`라는 별도 플러그인으로 만들고 관련 이슈는 게시하지 않았다.

게임 실행 상태와 에셋 설정 검색은 수명·갱신 경로가 다르므로 세션·수집기·metadata reader·Trigger 인덱스·Slate 화면으로 나눴다. UI는 수집한 값만 사용하고 게임의 spec/container 포인터를 보관하지 않는다. 기능의 사용 순서는 [사용법](../manual/GAS-Inspector.md)을 따른다.

## 변경 내용

| 항목 | 구현 내용 | 이유·제약 |
|---|---|---|
| 플러그인 | KataGASInspectorEditor 단일 Editor 모듈 | Game 타깃에 UI·수집 코드가 들어가지 않음 |
| 진입점 | Window → GAS Inspector, Kata.GASInspector.Open | 에디터에서 같은 탭 열기 |
| 대상 | Editor/PIE WorldContext와 ASC 목록, 검색·에디터 선택 연결 | 창별 weak 참조, 초기 선택과 대상 소멸을 구분 |
| snapshot | 수집 시각·월드·대상·초기화 상태와 값 행 | Freeze 시 tooltip·remaining까지 같은 시점 유지 |
| Tags | owned count와 ability blocked 구분, 정의 출처·주석 | 차단 횟수를 owned count로 추정하지 않음 |
| Attributes | 엔진 지원 속성 열거, Set 식별과 base/current | 동명 속성·음수·읽기 실패 구분 |
| Abilities | Spec active/count/level/input, 관측 block·tags·cooldown, 기본·동적 triggers, 활성 인스턴스·Task | CanActivateAbility 자동 호출 없음. Idle은 활성화 가능을 뜻하지 않음 |
| GE | 공개 active handle 조회, inhibition·기간·스택·context·tags·modifier 값 | active effect 설정/값 표시, Instant effect 이력 없음 |
| 검색/정렬 | 원본 목록을 검색 결과로 필터, 안정된 key와 문자열 정렬 | 선택·확장 유지, 동일 값 comparator의 엄격한 순서 |
| 에셋 열기 | Blueprint source soft path, 클릭 시 재조회 | native class에는 원본 에셋 없음 |
| Trigger 인덱스 | registry 파생 후보·참조 보강·로드 클래스, CDO 설정·dedup, exact/자식 검색 | 콘텐츠 scope와 로드된 native 범위를 명시 |
| 검색 수명 | 배치 로드·취소·partial/stale, asset/compile 변경 구독 | 에셋 수정/저장/컴파일을 수행하지 않음 |
| 설정 | 개인 Editor ini의 주기·페이지·정렬·숨긴 열·토글·scope | 공용 Config와 게임 에셋 변경 없음 |

## 주요 결정과 이유

### 관찰과 활성화 판단을 구분한다

active 상태·input 차단·asset tag 차단은 직접 관찰하여 별도로 표시한다. 자동 CanActivateAbility 호출은 사용자의 C++/Blueprint 활성화 검사 코드까지 반복 실행하므로 기본 갱신에는 넣지 않았다. 정확한 활성화 점검·실패 이력은 별도 기능이 필요할 때 설계한다. cooldown과 Task debug string은 해당 객체의 읽기 API를 호출한다.

### 화면의 수집 시점을 고정한다

GAS 데이터는 숫자·문자열·tag 값·soft path로 복사한다. tooltip도 같은 행의 값을 읽는다. Freeze는 수집을 멈추고 Refresh는 한 번 갱신하며 frozen 상태를 유지한다. 종료한 PIE에서도 frozen 데이터를 볼 수 있지만 live 대상을 잃은 경우에는 이전 데이터를 현재 값으로 남기지 않는다.

### 보호 metadata의 표현을 엔진에 맡긴다

Trigger/Task에는 일반 외부 getter가 없어 reflected 배열의 inner 타입을 확인한 뒤 읽는다. Task는 object property accessor로 해석하고 구조체는 일치하는 ScriptStruct의 복사 API를 쓴다. FProperty 포인터를 캐시하지 않아 재컴파일 뒤 주소를 재사용하지 않는다. schema가 다르면 정보 지원 불가를 표시한다.

### 에셋 검색은 명시적으로 갱신한다

GeneratedClass 파생 metadata로 Blueprint 후보를 만들고 SearchableName 참조로 보강한다. 결과는 CDO의 실제 Trigger 설정을 확인한 뒤 class/tag/source로 중복을 제거한다. 틱당 최대 두 개·배치 기준 약 4ms로 나누지만 개별 에셋의 동기 로드 시간을 제한하지는 못한다.

에셋 변경·Blueprint compile는 인덱스를 stale로 만들며 사용자가 Scan으로 다시 만든다. partial·cancelled·실패/지원 불가를 함께 표시하여 빈 검색 결과와 미완료 검색을 구분한다. raw UObject를 결과 행에 장기 보관하지 않는다.

## 근거

- [플러그인 설정](../../Plugins/KataGASInspector/KataGASInspector.uplugin): Editor 모듈과 GameplayAbilities 의존.
- [모듈](../../Plugins/KataGASInspector/Source/KataGASInspectorEditor/Private/KataGASInspectorEditorModule.cpp): 탭·메뉴·명령 등록과 종료 정리.
- [세션](../../Plugins/KataGASInspector/Source/KataGASInspectorEditor/Private/KataGASInspectionSession.cpp): 월드 정책·대상·Freeze·행 재사용.
- [수집기](../../Plugins/KataGASInspector/Source/KataGASInspectorEditor/Private/KataGASSnapshotCollector.cpp): 네 GAS 데이터 영역.
- [metadata](../../Plugins/KataGASInspector/Source/KataGASInspectorEditor/Private/KataGASAbilityMetadata.cpp): schema 검사와 값 추출.
- [Trigger 인덱스](https://github.com/jaykop/Kata/blob/4864a25/Plugins/KataGASInspector/Source/KataGASInspectorEditor/Private/KataGASAbilityTriggerIndex.cpp): 카탈로그·배치·검색 수명.
- [화면](../../Plugins/KataGASInspector/Source/KataGASInspectorEditor/Private/SKataGASInspector.cpp): 실제 UI와 개인 설정.

## 확인 범위와 결과

| 대상 | 수행 | 결과 | 미확인 범위 |
|---|---|---|---|
| 소스 | 기능 구현 | 위 책임별 소스와 샘플 활성화 설정 작성 | 컴파일·실행 |
| 관련 UE API | 구현에 필요한 로컬 UE 5.8.2 헤더 읽기 | public API·reflection accessor·등록 인터페이스 기준으로 작성 | 새 코드의 빌드 적합성 |
| 빌드·테스트 | 사용자 완료 보고 (2026-10-05) | 빌드 성공 및 테스트 완료 | 개별 타깃·항목은 별도 보고 없음 |
| 자동화·lint·별도 코드 검사·성능 측정 | 에이전트 미실시 | 별도 수행 결과 없음 | 측정·검사 결과 |

## 남은 제한과 후속 작업

사용자가 제공한 첫 빌드 로그에서 `SSearchBox::FArguments`에 없는 `Text` 사용으로 컴파일이 실패했다. 두 검색창을 UE 5.8의 `InitialText`로 수정했다. 트리 자식 조회 함수는 `GetRowChildren`으로 바꿔 `SCompoundWidget::GetChildren`을 숨기는 경고를 제거했다. 기존 NonInstanced 에셋을 진단하는 비교에는 한정된 deprecated 경고 억제를 적용했다. 이후 2026-10-05 사용자가 빌드 성공 및 테스트 완료를 보고했다. 에이전트는 빌드와 테스트를 직접 실행하지 않았다.

Editor/Game 빌드와 PIE 수명·Frozen 시점·Task/Trigger·에셋 변경/GC의 실제 동작은 사용자가 확인한다. [설계](../plan/GAS-Inspector-Plan.md)의 확인 항목을 따른다. 실제 콘텐츠 규모에서 갱신 주기와 로드 반응성을 확인한 뒤 필요하면 조정한다.

멀티플레이·원격 프로세스·상태 강제 변경·완전한 실행 이력은 포함하지 않는다. 미로드 native 클래스·레지스트리에 없는 에셋까지 전부 검색한다고 보장하지 않는다.

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [사용법](../manual/GAS-Inspector.md) | 메뉴·데이터 의미·검색 scope·Freeze·지원 범위 |
| [설계](../plan/GAS-Inspector-Plan.md) | 실제 구조·설계 기준·사용자 확인 항목 |
| [문서 목록](../README.md) | 사용법·구현 기록·설계 링크 |
| AGENTS.md·[모듈화 계획](../plan/Plugin-Modularization-Plan.md) | 별도 Editor 개발 도구와 의존 경계 |
| GitHub Issue | 사용자 요청에 따라 미게시 |

## 후속 변경

2026-10-06 [탭별 화면 개편](2026-10-06-GAS-Inspector-UI.md)에서 Current 단독 표시와 전용 열·버튼을 도입하고 별도 Trigger 검색을 제거했다. 이 문서의 검색 구현 설명은 초기 버전 기록이다. 현재 계약은 manual을 따른다.
