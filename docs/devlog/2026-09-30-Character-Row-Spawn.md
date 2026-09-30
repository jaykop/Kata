# 캐릭터 행 적용 순서와 비동기 요청 수명

작성: 2026-09-30  
갱신: 2026-09-30  
유형: 진단·구현 기록  
대상: KataFramework 캐릭터 데이터 테이블과 비동기 생성(#26)  
기준: 2026-09-30 커밋 cce1398과 사용자 에디터 실행·재빌드·PIE 보고

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

## 근거

- [생성 서브시스템](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataCharacterSpawnSubsystem.cpp): 행 복사, 지정 에셋 확인, 로드와 캐릭터 생성 경로.
- [캐릭터 행 적용](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataCharacter.cpp): `OnConstruction`에서 행 값 적용.
- [Blueprint 비동기 노드](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataAsyncAction_SpawnCharacter.cpp): 월드 정리 시 등록 해제.
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
| [현재 구현 상태](Implementation-Status.md) | #26 구현 범위, 사용자 재현 결과, 수정 후 미확인 범위를 반영했다 |
| [캐릭터 데이터 사용법](../manual/Character-Data.md) | 행 작성, 비동기 생성, GameMode 설정과 실패 시 확인 항목을 정리했다 |
| [입력 사용법](../manual/Input.md) | PC 행에서 입력 설정·그래프를 채우는 경로를 연결했다 |
| [캐릭터 데이터 계획](../plan/Character-Definition-Plan.md) | 폐기된 구현 전 상태를 제거하고 적용 순서를 현행화했다 |
