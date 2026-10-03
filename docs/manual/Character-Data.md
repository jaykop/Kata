# 캐릭터 데이터 테이블 사용법

갱신: 2026-10-03  
대상: KataFramework의 캐릭터 행, 비동기 생성, PC 생성 GameMode  
적용 기준: [#26 캐릭터 정의 데이터와 비동기 생성](https://github.com/jaykop/Kata/issues/26)  
확인 상태: 소스의 생성 경로를 확인했다. 2026-09-30 사용자가 기존 구현의 Mesh 미적용을 보고했고, 적용 순서와 컴파일 오류를 수정한 뒤 Editor 재빌드와 PIE 테스트 완료를 보고했다. 개별 시나리오 결과는 별도 보고되지 않았다.

## 목적과 준비

DataTable 행으로 캐릭터 Blueprint와 선택적인 Mesh·Anim Blueprint를 지정한다. PC 행은 입력 설정과 콤보 그래프도 지정한다. 행의 에셋은 생성 요청 시 비동기로 로드한다. 행 이름이 캐릭터 ID다.

PC 테이블은 `FKataPlayerCharacterRow`, NPC 테이블은 `FKataNPCCharacterRow`를 행 구조로 사용한다. Character Class는 필수다. PC 행의 클래스는 `AKataPlayerCharacter` 계열이어야 한다. 캡슐·이동·팩션 같은 기본값은 캐릭터 Blueprint에서 설정한다.

## 사용 순서

1. DataTable을 만들 때 PC 또는 NPC 행 구조를 고른다. 행을 추가하고 Character Class에 해당 캐릭터 Blueprint를 지정한다.
2. Blueprint 기본값과 다르게 만들 항목만 Skeletal Mesh, Anim Class에 지정한다. PC라면 Input Config와 Graph도 필요에 따라 지정한다. 빈 항목은 Blueprint 기본값을 사용한다.
3. PC 시작 캐릭터는 `AKataGameMode` 파생 GameMode의 Player Character Row에 PC 테이블과 행 이름을 지정한다. 맵의 World Settings → GameMode Override에 그 GameMode를 지정한다.
4. NPC나 별도 캐릭터는 Blueprint의 `Spawn Kata Character` 노드에 테이블 행 핸들과 Transform을 넘긴다. 성공은 On Spawned, 실패는 On Failed에서 처리한다. C++에서는 월드의 `UKataCharacterSpawnSubsystem::RequestSpawn`을 사용한다. 개체 수와 생성 영역을 설정하려면 [스포너 사용법](Spawner.md)을 따른다.
5. PC 생성 후 빙의 시점은 컨트롤러의 `OnPossessedPawnChanged`로, 생성 성공은 서브시스템의 `OnCharacterSpawned`로 받는다. 로드 중인지 확인할 때는 `AKataGameMode::IsPlayerCharacterPending` 또는 서브시스템의 `GetPendingSpawnCount`를 사용한다.

로컬 샘플은 `/Game/KataTest/DataTable/DT_PlayerCharacters`, `/Game/KataTest/DataTable/DT_NPCCharacters`, `/Game/KataTest/BP_KataTestGameMode`다. 프로젝트의 `/Content/`는 `.gitignore`로 제외되므로 샘플 에셋과 행 값은 저장소에 포함되지 않는다.

## 주요 설정과 실행 계약

| UI 항목 또는 API | 의미 | 빈 값·실패 시 동작 |
|---|---|---|
| Character Class | 생성할 캐릭터 Blueprint | 비어 있거나 로드에 실패하면 생성에 실패한다 |
| Skeletal Mesh, Anim Class | Blueprint 기본 외형·애니메이션을 행 값으로 교체한다 | 각각 비어 있으면 해당 Blueprint 기본값을 유지한다. 지정했지만 로드할 수 없으면 생성에 실패한다 |
| Input Config, Graph | PC의 입력 처리 컴포넌트에 적용한다 | 각각 비어 있으면 컴포넌트의 Blueprint 기본값을 유지한다 |
| Player Character Row | GameMode가 비동기로 생성할 PC | 비어 있으면 엔진의 Default Pawn Class 경로를 따른다. 행 기반 생성에 실패하면 폰 없이 남는다 |
| `RequestSpawn` | 행을 복사하고 에셋 로드를 시작한다. 요청 핸들로 취소할 수 있다 | 행·테이블 오류는 null 완료 콜백과 무효 핸들을 즉시 반환한다. 취소·월드 정리는 콜백을 부르지 않는다 |
| `Spawn Kata Character` | Blueprint 비동기 생성 노드 | 실패 시 On Failed를 실행한다. Cancel이나 월드 정리 시 어느 결과 핀도 실행하지 않는다 |

행 적용은 Blueprint Construction Script가 끝난 `OnConstruction`에서 이뤄진다. 따라서 행의 Mesh·Anim Class는 생성된 캐릭터의 초기화와 빙의 전에 적용된다. 입력 설정과 Graph를 빙의 중에 바꾸려고 하면 `UKataInputHandlerComponent`가 경고를 남기고 변경을 무시한다.

## 제한과 문제 해결

| 증상 또는 제한 | 확인할 곳 | 조치 |
|---|---|---|
| 행의 Mesh 대신 Blueprint Mesh가 보인다 | 실제 GameMode Override, Player Character Row의 테이블·행 이름, Skeletal Mesh 슬롯 | 새 코드로 Editor 빌드 후 같은 레벨에서 재확인한다. 계속되면 `LogKataFramework`의 생성 실패 경고와 캐릭터 Blueprint의 Construction Script를 확인한다 |
| 캐릭터가 생성되지 않는다 | Character Class, 행 구조, 행 이름, 에셋 로드 실패 경고 | PC 테이블은 PC 행 구조와 `AKataPlayerCharacter` 계열 클래스를 사용한다 |
| PC가 로드 중이다 | `IsPlayerCharacterPending` | 로드 중에는 폰이 없으며 로딩 화면은 아직 제공하지 않는다 |
| NPC의 AI 설정이 필요하다 | KataAI 작업 범위 | AIController·StateTree 항목은 아직 이 행에 없다 |
| 생성한 NPC가 공중에 멈춰 있다 | 컨트롤러가 없어 CharacterMovement가 동작하지 않는다 | `AKataCharacter`는 `Auto Possess AI`를 `Placed in World or Spawned`로 둬 생성 시 `AI Controller Class`의 컨트롤러를 받는다. Blueprint에서 이 값을 `Placed in World`나 `Disabled`로 바꿨다면 되돌린다. `AKataPlayerCharacter`는 GameMode가 빙의시키므로 `Placed in World`다 |

GAS 데이터 에셋과 로딩 화면은 아직 없다. [기본 스포너](Spawner.md)는 NPC Row·개체 수·Box 영역과 명시적 생성·취소를 제공한다. 재생성 규칙은 후속 옵션이다.

## 확인 상태와 근거

- [캐릭터 생성 서브시스템](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataCharacterSpawnSubsystem.cpp): 행 복사, 에셋 로드, 생성·완료 경로.
- [캐릭터 행 적용](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataCharacter.cpp): Construction Script 이후 행 적용.
- [진단과 변경 기록](../devlog/2026-09-30-Character-Row-Spawn.md): 기존 Mesh 미적용 현상과 수정 이유.
- [작업 상태 #26](https://github.com/jaykop/Kata/issues/26): 본문의 초기 PrimaryDataAsset 설계는 [확정된 DataTable 행 계획](../plan/Character-Definition-Plan.md)으로 대체됐다.
