# 스포너 옵션을 컴포넌트로 분리한 결정

작성: 2026-09-30  
갱신: 2026-10-03  
유형: 결정 기록  
대상: #21 최소 스포너 설계  
기준: 2026-09-30 사용자 결정·GEComponent 구조 정정, 에디터 기동 어설션 보고, UE 5.8 엔진 소스와 현재 미커밋 스포너 수정. UObject 구조 변경 후 재빌드·실행은 미확인이다.

## 배경과 결론

이전 계획 초안은 캐릭터 액터의 파괴를 지연 재생성의 기준으로 제안했다.
사용자는 재생성 규칙을 필요한 옵션이 정해질 때 추가하고, 컴포넌트 구조로 옵션을 설정하도록 요청했다.
스포너 본체는 DataTable과 생성할 Row를 선택하며, 초기 계획은 개체 수와 생성 영역을 정의하는 컴포넌트 중심으로 바꾼다.

초기 초안은 수량과 영역을 독립 컴포넌트로 나눴다. 이후 사용자는 두 설정을 하나의 기본 SpawnerComponent에 묶고 계산 함수를 분리하는 제안을 채택해 구현 진행을 요청했다.
초기 구현은 이를 실제 ActorComponent로 해석해 작성했다. 이후 사용자가 GE에서 설정하는 Component 구조를 뜻했다고 정정했다.
현재 구현은 `AKataCharacterSpawner`의 Instanced 배열과 `UKataSpawnerComponent` UObject 기반, `UKataSpawnerComponent_SpawnSettings`로 대체했다. 기존 `UKataSpawnerOptionComponent`는 제거했다.

## 주요 결정과 이유

| 결정 | 이유와 범위 |
|---|---|
| 테이블·Row 선택을 스포너 본체에 둔다 | 어떤 캐릭터를 생성할지 한곳에서 지정한다 |
| 생성 옵션을 인라인 UObject 배열로 확장한다 | 사용자 정정에 따라 GEComponent 방식으로 설정 종류를 선택하고 같은 Details에서 값을 편집한다 |
| 개체 수와 생성 영역을 기본 SpawnerComponent에 함께 둔다 | 초기 설정을 한곳에서 편집하며, 수량 계산·위치 선택 함수를 독립적으로 확장한다. 별도 교체·공유 요구가 생기면 컴포넌트 분리를 재검토한다 |
| 재생성 규칙은 후속 옵션으로 남긴다 | 파괴·사망·휴식 등 어떤 사건이 재생성을 요구하는지 아직 결정하지 않았다 |
| 최소·최대 개체 수, Roaming, StateTree·Sense Override는 확장 예시로 기록한다 | 사용자가 가능한 옵션으로 제시한 항목이며, 세부 기능의 구현 확정은 아니다 |

Spawn Settings는 상대 `SpawnAreaTransform`과 `BoxExtent` 수치로 영역을 설정한다. ActorComponent 수명·물리 객체 없이 설정 객체로 구성한다.
추가 AI 옵션의 적용 단계와 로딩·충돌 계약은 후속 설계로 남긴다.
이전 초안의 조우 볼륨, 고정 팩션 옵션과 파괴 기준 자동 재생성은 초기 구현 범위에서 제외한다.

## 구현 내용

- 스포너 본체는 NPC `FDataTableRowHandle`을 가지고 기존 비동기 생성 API를 호출한다. 명시적 생성·취소와 개별 결과·전체 완료 이벤트를 제공한다.
- 기본 컴포넌트가 개체 수와 후보 Transform을 계산한다. 생성 영역 스케일은 캐릭터 스케일로 전달하지 않는다.
- 즉시 실패 콜백을 고려해 전체 요청을 먼저 예약한다. 요청 준비 중 테이블을 강한 참조로 유지하고, 완료 후에는 생성 개체를 약한 참조로 조회한다.
- 요청 시 활성 설정 객체를 복사해 진행 중 편집의 영향을 막는다. 설정 사본은 GC 추적 참조로 유지하고, 대기 핸들·개체 목록은 스포너에 둔다.
- 설정 사본에 생성 완료를 통지한다. 이 훅은 BeginPlay 이후이므로 AI 초기 설정 기능으로 간주하지 않는다.
- 취소·스포너 종료는 대기 요청을 정리한다. 생성 완료 NPC의 제거·재생성 정책은 도입하지 않는다.

## 에디터 기동 어설션과 수정

2026-09-30 사용자는 엔진을 켜면 `UKataSpawnerComponent` 생성자의 `SetBoxExtent`에서 디버거가 멈추는 화면을 보고했다.
화면의 객체는 `Default__KataSpawnerComponent`이며, 호출 스택은 `SetBoxExtent` → `UpdateBodySetup` → `CreateShapeBodySetupIfNeeded` → `NewObject<UBodySetup>` → `FObjectInitializer::AssertIfInConstructor`였다.

UE 5.8의 `SetBoxExtent`는 겹침 갱신 여부와 관계없이 BodySetup을 갱신한다. 기본 객체를 만드는 생성자에서 호출하면 이름 없는 `NewObject` 생성자 어설션으로 이어진다.
당시에는 렌더·물리 갱신 없이 Box Extent만 설정하는 `InitBoxExtent`로 바꿨다. 이 수정은 초기 UBoxComponent 프로토타입에 대한 기록이다.
이후 GEComponent 구조 정정으로 UBoxComponent 자체를 제거했다. 현재 설정은 UObject의 FVector 값이며 BodySetup 생성 경로를 사용하지 않는다.

## GEComponent 구조 정정

사용자는 레벨에 스포너를 배치한 뒤 Component 설정 방법을 물었고, 실제 ActorComponent가 아니라 GE의 설정 Component 구조를 요청했다고 명시했다.
UE 5.8 `UGameplayEffectComponent`의 UObject·EditInlineNew·DefaultToInstanced 패턴과 GameplayEffect의 Instanced 배열 소유 방식을 참고했다.
스포너 Details의 `Spawner Components`에서 항목을 추가하고 `Spawn Settings`를 선택하는 방식으로 변경했다.
클래스 기반 변경은 전체 Editor 재빌드·재시작이 필요하다. 기존 프로토타입 ActorComponent를 인라인 설정으로 자동 변환하지 않는다.

## 근거

- 2026-09-30 사용자 요청: 재생성은 필요한 옵션에 따라 나중에 추가하고, 스포너의 테이블·Row 선택과 옵션 컴포넌트 구조를 먼저 계획한다.
- 최소 스포너 계획: 당시 초기 구성과 확장 경계를 담았다. #21 종료로 삭제했고 결정은 [후속 결정 기록](2026-10-04-Spawner-Area-And-NavMesh.md)으로 옮겼다.
- [캐릭터 데이터 사용법](../manual/Character-Data.md): 기존 비동기 생성 API.
- [스포너 액터](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataCharacterSpawner.cpp): 요청 예약·완료·취소와 옵션 통지.
- [설정 기반](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataSpawnerComponent.h): 인라인 UObject와 완료 통지.
- [수량·영역 설정](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataSpawnerComponent_SpawnArea.cpp): 수량 계산과 Box 내부 위치 선택. 당시 파일은 `KataSpawnerComponent_SpawnSettings.cpp`였고 2026-10-04 이름을 바꿨다.
- 2026-09-30 사용자 정정: 실제 ActorComponent가 아닌 GE의 설정 Component 구조를 요청했다.
- UE 5.8 엔진 소스 `GameplayEffectComponent.h`와 `GameplayEffect.h`: 인라인 UObject 선언과 Instanced 배열 소유 패턴.
- 2026-09-30 사용자 에디터 기동 화면: 기본 객체와 생성자 어설션 호출 스택.
- UE 5.8 엔진 소스 `BoxComponent.h`의 `InitBoxExtent`, `BoxComponent.cpp`의 `SetBoxExtent`, `ShapeComponent.cpp`의 `CreateShapeBodySetupIfNeeded`: 초기화 API와 BodySetup 생성 경로.

## 확인 범위와 결과

초기 코드 작성 후 사용자의 에디터 기동에서 생성자 어설션이 보고돼 관련 엔진 소스를 읽고 초기화 API를 수정했다. 이후 사용자의 Component 의미 정정에 따라 인라인 UObject로 구조를 변경했다.
에이전트는 샘플 에셋 생성·빌드·자동 검사·PIE 확인을 수행하지 않았다. 2026-10-03 사용자가 구조 변경과 Character Row의 NPC 행 필터 추가 후 빌드하고 스폰이 정상 동작함을 보고했다. 개별 시나리오의 결과는 별도로 보고되지 않았다.

## 남은 제한과 후속 작업

수량·영역·실패·취소·PIE 종료와 Character Row 선택기 필터의 개별 확인 결과는 아직 없다.
Box 후보 위치는 지면·NavMesh·개체 간격을 보장하지 않는다. AI 옵션은 KataAI의 관련 기능과 연결하며, 재생성 규칙은 실제 사용 요구에 맞춰 설계한다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| 최소 스포너 계획(삭제됨, [후속 결정 기록](2026-10-04-Spawner-Area-And-NavMesh.md)) | GEComponent 방식 UObject 배열과 수량·영역 설정, 후속 경계를 갱신했다 |
| [폐지 전 상태 기록](../localdocs/Implementation-Status-Archive-2026-10-03.md) | 스포너·컴포넌트 구현과 빌드·실행 미확인을 기록했다 |
| [스포너 사용법](../manual/Spawner.md) | 레벨 액터 Details의 인라인 설정, 이전 프로토타입 처리와 함수·이벤트 계약을 정리했다 |
| [문서 목록](../README.md) | 변경한 계획의 설명과 이 결정 기록을 연결했다 |



> 2026-10-03 이후 전체 상태 요약 문서는 폐지했다. 위 상태 기록 링크는 당시 기록 보존용 로컬 자료다. 현재 작업 상태는 [GitHub Issue](https://github.com/jaykop/Kata/issues)를 따른다.
