# 캐릭터 행 적용 순서와 비동기 요청 수명

작성: 2026-09-30  
갱신: 2026-10-03  
유형: 진단·구현 기록  
대상: KataFramework 캐릭터 데이터 테이블과 비동기 생성(#26)  
기준: 2026-09-30 커밋 cce1398과 사용자 에디터 실행·재빌드·PIE 보고. 2026-10-03에는 현재 작업 트리의 생성 코드·계획·매뉴얼과 #26 본문을 대조했다.

## 배경과 결론

사용자는 테스트 레벨에서 캐릭터 데이터 테이블의 Mesh를 바꿔도 생성된 캐릭터에 적용되지 않는다고 보고했다. 로컬 샘플 에셋의 이름 맵에는 `BP_KataTestGameMode`의 `KataGameMode` 부모와 PC 테이블 참조, PC 테이블의 별도 SkeletalMesh 경로가 들어 있다. 에셋의 실제 행 값 전체와 실행 중 선택된 행은 확인하지 않았다.

기존 소스는 `SpawnActorDeferred` 직후 Mesh를 적용하고 `FinishSpawning`을 호출했다. UE 5.8의 `FinishSpawning`은 Blueprint Construction Script를 실행하므로, 행에서 적용한 Mesh가 그 과정에서 다시 설정될 수 있다. 행 적용을 Construction Script 이후의 `AKataCharacter::OnConstruction`으로 옮겼다. 사용자는 수정 후 Editor 재빌드와 PIE 테스트 완료를 보고했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| 행 적용 시점 | `FinishSpawning` 전에 Mesh·Anim·PC 입력 값을 적용했다 | `FinishSpawningWithCharacterRow`가 행을 잠시 보관하고, `OnConstruction`에서 Construction Script 이후 적용한다 |
| 지정 에셋 로드 실패 | 선택 항목의 `Get()`이 null이어도 Blueprint 기본값으로 계속 생성했다 | 지정된 소프트 경로를 완료 시 확인하고, 하나라도 로드하지 못했으면 경로를 로그에 남기고 생성에 실패한다 |
| 테이블 참조 | 일반 C++ 요청 맵의 `FDataTableRowHandle`은 GC가 추적하지 않는다 | 요청 동안 `TStrongObjectPtr`로 원본 테이블을 유지한다. Blueprint 노드의 행 핸들은 `UPROPERTY`로 추적한다 |
| 월드 정리 | 서브시스템이 요청을 콜백 없이 취소하므로 GameInstance에 등록된 비동기 노드가 남을 수 있다 | 비동기 노드는 대상 월드의 종료 이벤트에서 Cancel하고 등록을 해제한다 |

## 주요 결정과 이유

`OnConstruction`은 UE 5.8의 `ExecuteConstruction`에서 Blueprint Construction Script 뒤에 호출되고, `PostActorConstruction`의 컴포넌트 초기화와 `BeginPlay`보다 앞선다. 이 지점은 Blueprint가 Mesh를 다시 설정한 뒤 행 값을 우선 적용하면서도 PC 입력 설정을 빙의 전에 넣을 수 있다. 사용자가 정한 빈 선택 항목의 Blueprint 기본값 유지 규칙은 그대로 따른다.

행은 요청 시점에 복사해 테이블 재로드의 영향을 피한다. 성공 이벤트가 원본 `FDataTableRowHandle`을 전달하므로 테이블 자체는 요청이 끝날 때까지 강한 참조로 유지한다. 월드 정리 시 결과 핀을 실행하지 않는 기존 취소 계약에 맞춰 Blueprint 비동기 노드도 Cancel한다.

#26의 조립·생성 방식은 다음과 같이 확정됐다. 날짜와 사용자 결정의 출처는 [캐릭터 데이터 계획](../plan/Character-Definition-Plan.md#확정-사항과-미확정-사항)에 남아 있다.

| 결정 | 이유와 범위 |
|---|---|
| DataTable 행을 캐릭터 조립 단위로 사용하고 PC·NPC 테이블을 분리한다 | 행 이름을 캐릭터 ID로 사용한다. 기존 이슈의 캐릭터별 Blueprint 대체·PrimaryDataAsset 방식은 이 결정으로 대체됐다 |
| 행이 캐릭터 Blueprint를 지정한다 | 캡슐·이동 같은 기본값은 Blueprint가 정한다. 선택 항목인 Mesh·Anim Blueprint·PC 입력 설정·그래프를 비우면 해당 Blueprint 기본값을 유지한다 |
| PC와 NPC 모두 `FDataTableRowHandle`로 지정한다 | 테이블과 행을 하나의 엔진 구조체로 전달한다. 요청 시 행을 복사해 로드 중 테이블 재로드의 영향을 피한다 |
| 소프트 참조를 `FStreamableManager`로 비동기 로드한다 | DataTable 행은 Primary Asset이 아니므로 Primary Asset ID나 Asset Bundle 등록을 요구하지 않는다. 스포너(#21)도 같은 비동기 생성 API를 사용한다 |
| PC는 로드 완료 전 폰 없이 기다린다 | `AKataGameMode`가 생성 완료 후 빙의한다. 로딩 화면은 후속 작업으로 분리했다 |
| 준비 상태는 델리게이트와 조회 API로 알린다 | 생성 서브시스템의 델리게이트·대기 건수와 GameMode의 대기 상태를 제공한다. 빙의 변화에는 엔진의 `AController::OnPossessedPawnChanged`를 사용한다. 방송형 이벤트가 늘어나기 전에는 메시지 버스를 도입하지 않는다 |
| GAS 데이터 에셋과 NPC AI 설정을 후속 범위로 둔다 | Attribute·GA·GE 묶음과 행의 GAS 슬롯은 후속 작업에서, AIController·StateTree 설정은 KataAI(#22)에서 다룬다 |

## 근거

2026-10-03 대조에서는 행 구조, Construction Script 이후 적용, GameMode의 행 미지정 시 기본 폰 생성, 취소·월드 정리 시 콜백 생략이 현재 코드와 일치함을 확인했다. 계획의 취소 실패 콜백 설명과 매뉴얼의 샘플 테이블 경로를 바로잡았다. #26 본문에는 초기 PrimaryDataAsset 설계가 남아 있으므로 계획에 대체 관계를 명시했다. 이슈 본문·라벨·완료 상태는 수정하지 않았다.

- [생성 서브시스템](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataCharacterSpawnSubsystem.cpp): 행 복사, 지정 에셋 확인, 로드와 캐릭터 생성 경로.
- [캐릭터 행 적용](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataCharacter.cpp): `OnConstruction`에서 행 값 적용.
- [Blueprint 비동기 노드](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataAsyncAction_SpawnCharacter.cpp): 월드 정리 시 등록 해제.
- [캐릭터 데이터 계획](../plan/Character-Definition-Plan.md): #26의 사용자 확정 결정, 포함·제외 범위와 완료 조건.
- UE 5.8 로컬 엔진 소스 `Actor.cpp`의 `AActor::FinishSpawning`과 `ActorConstruction.cpp`의 `AActor::ExecuteConstruction`: Construction Script와 `OnConstruction` 호출 순서.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 기존 Mesh 적용 | 2026-09-30 사용자 에디터 실행 보고 | 테이블에서 바꾼 Mesh가 테스트 레벨의 캐릭터에 적용되지 않았다 | 정확한 실행 중 테이블·행 값과 Blueprint Construction Script 내용 |
| 수정 경로 | 에이전트 소스 읽기와 변경 | Construction Script 이후, 컴포넌트 초기화 이전에 행을 적용하도록 소스를 수정했다 | 수정 후 Editor 빌드·PIE 실행·Mesh/Anim 표시 |
| 요청 수명 | 에이전트 소스 읽기와 변경 | 테이블 강한 참조와 월드 종료 시 비동기 노드 해제 경로를 추가했다 | 취소·월드 전환 실행 |
| Editor 컴파일 | 2026-09-30 사용자 빌드 보고 | `UWorld::OnWorldBeginTearDown` 참조에서 C2039/C2065가 발생해 UE 5.8의 선언 타입 `FWorldDelegates`로 수정했다 | 재빌드는 아래 수정 후 실행 기록 참조 |
| 수정 후 실행 | 2026-09-30 사용자 보고 | Editor 재빌드와 PIE 테스트를 완료했다 | NPC·잘못된 행·취소·월드 정리 등 개별 결과는 별도 보고되지 않음 |

## 남은 제한과 후속 작업

사용자의 Editor 재빌드와 PIE 테스트는 완료됐다. NPC 행, 잘못된 행, 취소, 월드 정리의 개별 확인 범위는 보고되지 않았다. 로컬 `Content/KataTest`는 `.gitignore`의 `/Content/` 규칙 때문에 저장소에서 추적하지 않는다. GAS 데이터 에셋과 로딩 화면은 [계획](../plan/Character-Definition-Plan.md)의 후속 범위다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [작업 상태 #26](https://github.com/jaykop/Kata/issues/26) | 기존 전체 상태 기록은 로컬에 동결 보존했다. 공개 이슈 본문의 초기 설계와 현재 확정 설계의 차이는 계획에 명시했으며, 이슈 게시·상태 변경은 수행하지 않았다 |
| [캐릭터 데이터 사용법](../manual/Character-Data.md) | 행 작성, 비동기 생성, GameMode 설정과 실패 시 확인 항목을 정리했다 |
| [입력 사용법](../manual/Input.md) | PC 행에서 입력 설정·그래프를 채우는 경로를 연결했다 |
| [캐릭터 데이터 계획](../plan/Character-Definition-Plan.md) | 폐기된 구현 전 상태를 제거하고 적용 순서를 현행화했다 |


> 2026-10-03 이후 전체 상태 요약 문서는 폐지했다. 현재 작업 상태는 [GitHub Issue #26](https://github.com/jaykop/Kata/issues/26)를 따른다. 이번 문서 대조에서는 빌드·테스트·PIE를 새로 실행하지 않았다.
