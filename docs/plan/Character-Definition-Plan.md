# 캐릭터 데이터 테이블과 비동기 생성 계획

작성: 2026-09-27  
갱신: 2026-10-04  
연결 이슈: [#26 캐릭터 정의 데이터와 비동기 생성 (KataFramework)](https://github.com/jaykop/Kata/issues/26) · 로드맵 [#24](https://github.com/jaykop/Kata/issues/24)  
현재 상태 근거: [작업 상태](https://github.com/jaykop/Kata/issues/26) · [입력 계층 계획](Input-Plan.md) · [플러그인 분리 모듈화 계획](Plugin-Modularization-Plan.md) · [액션 게임 기반 시스템 계획](Action-Game-Systems-Plan.md#5-스포너)  
대체 관계: #26 본문의 초기 PrimaryDataAsset 설계를 아래 DataTable 행 설계로 대체한다. 공개 이슈 본문은 아직 초기 설계이며, 확정 결정의 이유는 [캐릭터 생성 기록](../devlog/2026-09-30-Character-Row-Spawn.md#주요-결정과-이유)을 따른다.

## 목적

캐릭터 데이터 테이블의 행 하나가 캐릭터 하나의 조립 정보가 된다. 행의 디테일 패널에서 캐릭터 Blueprint, 스켈레탈 메시,
Anim Blueprint와 PC의 입력 설정·콤보 그래프를 지정하고, 게임을 실행하면 행의 에셋을 비동기로 로드해 캐릭터를 조립한 뒤 게임에 진입한다. GAS 데이터 에셋 슬롯은 후속 범위다.
테이블은 PC용과 NPC·AI용으로 나눈다.

현재 구현과 확인 범위는 [작업 상태](https://github.com/jaykop/Kata/issues/26)에서 관리한다.

## 범위

- 포함: 캐릭터 행 구조(공통·PC·NPC), 비동기 생성 API(C++·Blueprint), 행 적용, `UKataInputHandlerComponent` setter,
  PC를 행으로 생성하는 GameMode, 샘플 PC·NPC 테이블.
- 제외:
  - GAS 데이터 에셋(Attribute·GA·GE 묶음)과 행의 GAS 슬롯. 후속 작업으로 만든다(2026-09-27 사용자 결정).
  - 로딩 화면. 지금은 폰 없이 기다리고 이후 추가한다(2026-09-27 사용자 결정).
  - NPC의 AI 설정(AIController, StateTree, 콤보 그래프)은 KataAI [#22](https://github.com/jaykop/Kata/issues/22)가 NPC 행에 더한다.
  - 스포너 배치·재생성 규칙([#21](https://github.com/jaykop/Kata/issues/21)), 저장·로드, 풀링.

## 구성

### 캐릭터 행

세 행 구조를 `KataFramework`에 둔다. 모든 에셋 참조는 소프트 참조라서 테이블을 열어도 에셋이 로드되지 않는다.

| 구조체 | 항목 | 규칙 |
|---|---|---|
| `FKataCharacterRow` (공통, `FTableRowBase`) | 캐릭터 Blueprint (`TSoftClassPtr<AKataCharacter>`) | 필수. 캡슐·이동 등 기본값은 이 Blueprint가 정한다 |
| | 스켈레탈 메시, Anim Blueprint | 선택. 비우면 Blueprint 기본값을 쓴다 |
| `FKataPlayerCharacterRow` (PC) | 공통 항목 + 입력 설정(`UKataInputConfig`), 콤보 그래프(`UKataGraph`) | 캐릭터 Blueprint는 `AKataPlayerCharacter` 계열이어야 한다. 입력 설정·그래프를 비우면 Blueprint 기본값을 쓴다 |
| `FKataNPCCharacterRow` (NPC·AI) | 공통 항목 | #22가 AI 항목을 더한다 |

- 행 이름이 캐릭터 ID다. PC 테이블과 NPC 테이블은 각자의 행 구조로 만든다.
- "비우면 Blueprint 기본값" 규칙 덕분에 같은 Blueprint로 메시·Anim만 다른 캐릭터를 행 추가만으로 만들 수 있다.
  외형뿐 아니라 동작까지 다르면 Blueprint를 새로 만들어 행에 지정한다.
- 각 행 구조는 로드할 소프트 경로 목록을 돌려주는 함수를 가진다. 파생 행이 자기 항목을 덧붙인다.

### 비동기 로드

행의 소프트 경로를 모아 Asset Manager의 `FStreamableManager`로 비동기 로드하고 완료 콜백을 받는다.
DataTable 행은 Primary Asset이 아니므로 Primary Asset ID와 Asset Bundle 경로는 쓰지 않는다. 프로젝트 설정에 등록할 것도 없다.

### 비동기 생성 API

- `UKataCharacterSpawnSubsystem` (`UWorldSubsystem`)이 월드별 요청을 관리한다.
  - 캐릭터는 `FDataTableRowHandle`(테이블 + 행 이름)로 지정한다. 엔진 구조체라 Blueprint에서 테이블과 행을 드롭다운으로 고를 수 있고,
    PC·NPC 테이블 모두 같은 API로 받는다. 행 구조가 `FKataCharacterRow` 계열이 아니면 실패한다.
  - C++: `RequestSpawn(RowHandle, Transform, Callback)`이 요청 핸들을 돌려주고, 핸들로 취소할 수 있다.
  - Blueprint: `UBlueprintAsyncActionBase` 노드 `Spawn Kata Character`가 `On Spawned`·`On Failed` 핀을 제공한다.
- 흐름:
  1. 행을 찾아 필요한 값을 요청에 복사한다. 테이블이 다시 로드돼도 요청이 영향을 받지 않게 하기 위해서다.
  2. 행의 에셋을 비동기로 로드한다.
  3. 로드가 끝나면 요청과 월드가 유효한지 확인한다. 취소됐거나 월드가 정리 중이면 생성하지 않는다.
  4. `SpawnActorDeferred`로 캐릭터 Blueprint를 만든다. `FinishSpawning` 안에서 Blueprint Construction Script가 끝난 뒤
     `OnConstruction`에서 행을 적용한다. 컴포넌트 초기화와 빙의보다 먼저 입력 설정·그래프를 넣고, 행의 메시가 Blueprint 설정으로 덮이지 않게 한다.
  5. `FinishSpawning` 뒤 완료 콜백을 부른다.
- 실패 사유: 행 없음, 행 구조 불일치, 캐릭터 Blueprint 없음, PC 행의 클래스 계열 불일치, 로드 실패, 생성 실패. 로그와 null 완료 콜백으로 알린다. 요청 단계의 실패는 무효 핸들을 반환한다.
- 명시적 취소와 월드 정리는 결과 콜백을 호출하지 않는다. Blueprint 노드도 Cancel 또는 월드 정리 시 어느 결과 핀도 실행하지 않는다.
- 자원 수명: 로드 핸들은 적용이 끝나면 놓는다. 적용된 에셋은 캐릭터와 컴포넌트의 UPROPERTY가 참조해 유지한다.
  요청 중 원본 테이블은 강한 참조로 유지하고, Blueprint 비동기 노드는 월드 정리 시 등록을 해제한다.

### 행 적용

- `AKataCharacter`가 공통 행을 적용하고(메시·Anim), `AKataPlayerCharacter`가 PC 행을 더 적용한다(입력 설정·그래프).
- `UKataInputHandlerComponent`에 `InputConfig`·`Graph` setter를 추가한다. 빙의 중 호출은 경고 후 무시한다.

### PC 생성 흐름: `AKataGameMode`

- GameMode에 PC 행(`FDataTableRowHandle`)을 지정한다.
- PC 행이 비어 있으면 엔진의 Default Pawn Class 생성 경로를 따른다.
- 플레이어를 시작할 때 기본 폰을 동기로 만들지 않고, PC 행으로 비동기 생성을 요청한 뒤 완료되면 빙의시킨다.
- 로드 중에는 폰이 없다. 로딩 화면은 이후 이 대기 구간에 추가한다.
- 생성에 실패하면 로그를 남기고 폰 없이 둔다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| 조립 단위 | 확정 | 2026-09-27 사용자 결정. DataTable 행이 캐릭터 Blueprint·메시·Anim Blueprint와 PC 입력 설정·그래프를 지정한다. GAS 데이터 에셋은 아래 후속 범위다 |
| 테이블 분리 | 확정 | 2026-09-27 사용자 결정. PC 테이블과 NPC·AI 테이블을 나눈다 |
| 캐릭터 Blueprint | 확정 | 행이 Blueprint를 지정하고, 캡슐 등 캐릭터 기본값은 Blueprint가 정한다. #26 본문의 "캐릭터마다 Blueprint 클래스를 만드는 대신"을 대체한다 |
| 생성 API 형태 | 확정 | 처음부터 비동기 콜백 형태. 스포너(#21)도 이 API를 쓴다. #26 결정 |
| PC 그래프·입력 설정 위치 | 확정 | `UKataInputHandlerComponent`의 `Graph`·`InputConfig`를 PC 행이 채운다. 2026-09-27 사용자 결정. [입력 계층 계획](Input-Plan.md#katagraph-발동-방식) |
| NPC 그래프 | 확정(범위 밖) | 2026-09-27 사용자 결정. KataAI StateTree Task가 지정한다 |
| GAS 데이터 | 확정(후속) | 2026-09-27 사용자 결정. Attribute·GA 등 캐릭터가 쓰는 GAS 데이터를 담는 데이터 에셋을 만들어 행에 지정한다. 이번 범위에서는 만들지 않는다 |
| 로드 중 PC 상태 | 확정 | 2026-09-27 사용자 결정. 지금은 폰 없이 대기, 이후 로딩 화면 |
| 샘플 폴더 | 확정 | 2026-09-27 사용자 결정. `Content/KataTest`를 유지한다 |
| 비동기 로드 방식 | 확정 | 2026-09-28 사용자 결정. 행의 소프트 경로를 `FStreamableManager`로 로드 |
| 캐릭터 지정 방식 | 확정 | 2026-09-28 사용자 결정. `FDataTableRowHandle` 하나로 PC·NPC 테이블 모두 지정. 2026-10-04 [#31](https://github.com/jaykop/Kata/issues/31)의 캐릭터 ID(`FKataCharacterId`) 참조로 대체됐다. [게임 데이터 컬렉션과 행 ID 참조 계획](Data-Collection-Plan.md) |
| 행 값 복사 | 확정 | 2026-09-28 사용자 결정. 요청 시점에 행 값을 복사해 로드 중 테이블 재로드의 영향을 받지 않는다 |
| 행 에셋 비움 규칙 | 확정 | 2026-09-28 사용자 결정. 메시·Anim BP 등 선택 항목을 비우면 Blueprint 기본값을 쓴다 |
| PC 준비 완료 알림 | 확정 | 2026-09-28 사용자 결정. GameplayMessage 계열 메시지 버스는 도입하지 않는다. 생성 서브시스템의 멀티캐스트 델리게이트와 현재 상태 조회 함수(`GetPendingSpawnCount`, `AKataGameMode::IsPlayerCharacterPending`)를 두고, 빙의 시점은 엔진의 `AController::OnPossessedPawnChanged`를 쓴다. 메시지 버스는 발신자가 수신자를 몰라야 하는 방송형 이벤트가 늘어날 때 다시 검토한다 |

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| CD-1 | 높음 | 행 구조: 공통·PC·NPC 행, 샘플 PC·NPC 테이블 | 위 제안 확정 | 에디터에서 PC·NPC 테이블에 행을 추가하고 디테일 패널에서 Blueprint·메시·Anim Blueprint·입력 설정·그래프를 지정할 수 있다 |
| CD-2 | 높음 | 비동기 생성과 적용: 생성 서브시스템, Blueprint 비동기 노드, 행 적용, 입력 처리 컴포넌트 setter | CD-1 | Blueprint에서 NPC 행으로 캐릭터를 생성하면 메시·Anim이 적용되고, 잘못된 행은 실패 핀으로 끝난다 |
| CD-3 | 높음 | PC 생성 흐름: `AKataGameMode`, 샘플 GameMode 지정 | CD-2 | 게임을 시작하면 PC 행으로 조립된 캐릭터에 빙의하고 입력으로 콤보를 실행한다(#26 완료 조건) |
| CD-4 | 보통 | 마무리: 설명서·결정 기록과 이슈 확인 범위 정리, GAS 데이터 에셋·로딩 화면 후속 이슈 연결 | CD-3 | 사용자가 완료를 확인한 뒤 #26을 닫고 이 계획을 정리한다 |

## 영향과 제한

- 모듈 경계: 모든 코드는 `KataFramework`에 둔다. 코어 `Kata`와 `KataTargeting`은 변경하지 않는다. 새 엔진 플러그인 의존은 없다.
- 공개 API: `UKataInputHandlerComponent`에 setter가 추가된다. 기존 Blueprint 기본값 지정 방식은 그대로 동작한다.
- 기존 에셋: 레벨에 직접 놓은 캐릭터는 행 없이 Blueprint 기본값으로 계속 동작한다. 샘플 GameMode는 CD-3에서 `AKataGameMode` 계열로 바꾼다.
- DataTable은 바이너리 uasset이라 여러 사람이 같은 테이블을 동시에 고치면 병합이 어렵다. 필요하면 CSV·JSON 가져오기를 쓸 수 있다.
- 폰 교체: CD-3은 빙의 흐름을 바꾼다. [#19](https://github.com/jaykop/Kata/issues/19) IN-2의 폰 교체 확인이 먼저 끝나 있어야 문제를 구분하기 쉽다.

## 사용자 확인 항목

- 구현 완료: CD-1~CD-3 코드와 샘플 테이블.
- 실행 확인: 사용자 Editor 빌드, 테이블 행 편집, Blueprint 비동기 노드로 NPC 생성, 잘못된 행 실패, PIE에서 PC 행 생성·빙의·콤보 실행.
- 이 목록만으로 에이전트가 빌드·테스트·별도 검사를 수행하지 않는다.

## 완료 시 갱신할 문서

- [작업 상태](https://github.com/jaykop/Kata/issues/26): KataFramework 캐릭터·생성 항목.
- [캐릭터 데이터 사용법](../manual/Character-Data.md): 테이블 행 작성, 생성 노드, GameMode 설정.
- [입력 사용법](../manual/Input.md): 입력 설정·그래프를 PC 행으로 채우는 경로.
- [문서 목록](../README.md): 이 계획과 새 manual 링크.
