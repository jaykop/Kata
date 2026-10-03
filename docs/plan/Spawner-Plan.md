# 컴포넌트 기반 최소 스포너 계획

작성: 2026-09-30  
갱신: 2026-10-03  
연결 이슈: [#21 최소 스포너](https://github.com/jaykop/Kata/issues/21) · 로드맵 [#24](https://github.com/jaykop/Kata/issues/24)  
현재 상태 근거: [작업 상태](https://github.com/jaykop/Kata/issues/21) · [스포너 사용법](../manual/Spawner.md)  
결정 근거: [스포너 컴포넌트 결정 기록](../devlog/2026-09-30-Spawner-Component-Design.md)  
대체 관계: [액션 게임 기반 시스템 계획](Action-Game-Systems-Plan.md#5-스포너)의 후보를 구체화한다. 이전 초안의 파괴 기준 자동 재생성·독립 개체 수/영역 컴포넌트 구성은 아래 사용자 결정으로 대체한다.

## 목적과 현재 상태

스포너에서 NPC DataTable과 생성할 Row를 선택하고, Details의 Spawner Components 배열에 GEComponent 방식의 인라인 UObject 설정을 추가한다. Spawn Settings에서 생성 개체 수와 생성 영역을 함께 설정한다.
수량 계산과 위치 선택은 함수로 분리해 필요할 때 각각 확장한다.
기존 #26 비동기 생성 API를 사용하며 별도의 에셋 로더는 만들지 않는다.
현재 구현과 실행 확인 범위는 [작업 상태](https://github.com/jaykop/Kata/issues/21)에서 관리한다.

## 범위

- 초기 범위: 테이블·Row 선택을 가진 스포너 액터, 개체 수·Box 영역을 함께 가진 기본 컴포넌트, 명시적 생성·취소, 결과 이벤트, 추가 옵션 컴포넌트의 생성 완료 통지.
- 향후 옵션: 최소·최대 개체 수 정책, 재생성 조건, Roaming 루트, StateTree·Sense Override, 팩션 Override, 조우·웨이브 발동.
- 각 향후 옵션의 동작과 클래스는 필요할 때 설계한다. 예시를 이번 구현의 필수 기능으로 바꾸지 않는다.
- 자동 NPC 제거·재생성, AI 초기화 변경, 지면·NavMesh 투영, 개체 사이 간격 보장은 초기 범위에서 제외한다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| 테이블·Row 선택 | 확정 | 2026-09-30 사용자 결정. 스포너 본체에 둔다 |
| 옵션 구성 | 확정 | 2026-09-30 사용자 정정. GEComponent처럼 인라인 UObject 설정을 배열에 추가한다 |
| 기본 컴포넌트 | 확정 | 2026-09-30 사용자가 개체 수·생성 영역을 하나의 기본 SpawnerComponent에 묶는 제안을 채택했다 |
| 재생성 규칙 | 확정(후속) | 필요한 옵션에 따라 나중에 추가한다. 파괴 이벤트를 자동 재생성 기준으로 삼지 않는다 |
| 모듈 위치 | 구현 선택 | 기존 캐릭터 생성 API와 향후 위성 기능을 조합하는 KataFramework에 둔다. 새 모듈 의존성은 없다 |
| Row 지정 방식 | 구현 선택 | `FDataTableRowHandle CharacterRow`로 NPC DataTable과 Row Name을 선택한다 |
| 개체 수 | 구현 선택 | `SpawnCount`는 한 번의 명시적 요청에서 만들 수다. 0은 생성 없이 완료하고 음수는 설정 오류다 |
| 생성 영역 | 구현 선택 | SpawnAreaTransform과 BoxExtent 수치로 액터 기준 영역을 정하고 내부 위치를 균일하게 표본 추출한다. 초기 Z Extent는 0이다 |
| 생성 시작 시점 | 구현 선택 | `bSpawnOnBeginPlay`(기본 true)가 켜져 있으면 BeginPlay에서 한 번 생성한다. 2026-09-30 사용자가 스포너를 배치하고 게임에 들어갔는데 생성되지 않는다고 보고해 추가했다. 끄면 명시적 호출로만 생성한다 |
| 영역 미리보기 | 구현 선택 | 에디터 전용 `UBoxComponent`(`SpawnAreaPreview`)가 활성화된 첫 Spawn Settings의 상대 Transform과 Box Extent를 그린다. 2026-09-30 사용자 보고로 추가했다 |
| 캐릭터 미리보기 | 구현 선택 | 에디터 전용 `USkeletalMeshComponent`(`CharacterPreview`)가 Character Row의 메시(비면 캐릭터 Blueprint 기본 메시)를 영역 중심·영역 회전에 Blueprint Mesh 상대 Transform을 곱해 그린다. 2026-09-30 사용자 요청으로 추가했다 |
| 기본 설정 없음 | 구현 선택 | 활성 Spawn Settings가 없으면 액터 Transform에서 1개를 생성한다 |
| 중복 설정 | 구현 선택 | 활성 Spawn Settings가 둘 이상이면 요청을 거절한다. 다른 옵션 설정은 같은 배열에 추가할 수 있다 |
| 향후 AI 초기 설정 | 결정 필요 | 행동 시작 전 적용할 설정, 추가 소프트 참조 로딩, 적용 순서와 충돌 계약은 관련 옵션을 추가할 때 정한다 |

## 구성과 책임

| 타입 | 역할 |
|---|---|
| `AKataCharacterSpawner` | Row 선택, 설정과 위치 복사, 기존 생성 API 호출, 작업 추적·취소, 성공·실패·완료 이벤트 |
| `UKataSpawnerComponent` | EditInlineNew·DefaultToInstanced UObject 기반. 활성 여부와 생성 완료 통지 확장점 |
| `UKataSpawnerComponent_SpawnSettings` | 개체 수·상대 Box 영역·생성 충돌 방식 설정. `CalculateSpawnCount`와 `GetSpawnTransform`을 각각 C++·Blueprint로 확장한다 |

기본 컴포넌트를 수량/영역 컴포넌트로 다시 분리할 시점은 독립적인 교체나 공유 요구가 생길 때 검토한다.
스포너는 Instanced 배열로 설정을 소유한다. 요청마다 활성 설정을 복사하며, 대기 핸들·개체 목록 같은 실행 상태는 스포너가 관리한다. 설정 객체에는 실행 상태를 저장하지 않는다.

## 생성과 수명 계약

1. Blueprint·C++의 `SpawnCharacters`로 작업을 시작한다. 한 스포너는 한 번에 하나의 작업만 받는다.
2. NPC Row·컴포넌트·개체 수·후보 Transform을 확인하고 요청 설정을 복사한다. 준비 중 같은 스포너를 재호출하면 거절한다.
3. 전체 요청을 먼저 예약한다. `RequestSpawn`이 즉시 실패 콜백을 부르더라도 전체 작업이 제출 도중 잘못 완료되지 않게 한다.
4. 개체마다 기존 비동기 API로 생성하고 개별 성공·실패 뒤 전체 완료를 알린다.
5. 취소와 종료 시 대기 요청을 정리한다. 결과 콜백은 작업 식별자와 객체 수명을 확인한다.
6. 생성 완료 개체는 약한 참조로 조회한다. 스포너의 취소·제거가 생성 완료 NPC를 제거하거나 다시 생성하지 않는다.

진행 중에 Row·개체 수·영역을 바꿔도 다음 요청부터 적용한다.
현재 개체 수와 대기 예약 수를 구분해 조회하며, 미래의 최대 개체 수 정책은 대기 요청도 포함해야 한다.
옵션 완료 통지는 캐릭터의 BeginPlay 이후다. StateTree·Sense의 초기 설정 계약을 만족하는 훅으로 간주하지 않는다.

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| SP-1 | 높음 | 인라인 Spawn Settings | 현재 사용자 결정 | 레벨 액터 Details에서 수량과 Box 영역을 함께 편집하고 두 계산 함수를 각각 확장한다 |
| SP-2 | 높음 | 스포너 액터와 비동기 작업 관리 | SP-1 | NPC Row를 생성하고 개별 결과·전체 완료·취소를 받는다 |
| SP-3 | 보통 | 옵션 설정 연결 | SP-2 | 배열에 추가한 활성 설정의 요청용 사본이 생성 완료 캐릭터와 요청 Row를 받는다 |
| SP-4 | 보통 | 문서와 사용자 실행 확인 | SP-1~SP-3 | 지정 수·영역·실패·취소·종료 계약의 실제 확인 범위를 기록한다 |

후속 옵션의 진행 순서와 이슈 범위는 필요가 생길 때 정한다.

## 영향과 제한

- 신규 소스는 KataFramework의 Public/Spawning·Private/Spawning에 둔다. 기존 Row 구조와 생성 API의 서명은 유지한다.
- 기본 영역은 수치로 정의하는 위치 선택용 Box다. 영역 스케일은 위치 계산에만 적용하며 캐릭터 스케일은 1이다. 영역 시각화·기즈모는 초기 범위에 포함하지 않는다.
- 초기 ActorComponent 프로토타입은 인라인 UObject 구조로 대체한다. 에디터 종료·전체 재빌드·재시작이 필요하며, 기존에 부착한 프로토타입 컴포넌트는 제거하고 배열 설정을 다시 작성한다. 자동 변환은 제공하지 않는다.
- 엔진 충돌 처리로 최종 위치가 후보 영역에서 벗어날 수 있다. 지면·NavMesh·간격 정책은 별도로 설계한다.
- AI 기능 자체는 KataAI에, 스포너와 AI를 조합하는 옵션은 통합 계층에 둔다. 위성 플러그인의 KataFramework 역참조는 추가하지 않는다.
- 공개 이슈의 초기 범위와 현재 사용자 결정이 다르다. 이슈 변경은 별도 초안 확인 후 반영한다.

## 사용자 확인 항목

- 사용자 Editor 빌드와 레벨 액터·Blueprint 기본값의 Spawner Components 클래스 선택·인라인 편집.
- Row 외형, 지정 수량, 상대 Spawn Area Transform·Box Extent에 따른 후보 위치.
- 0개·잘못된 Row·로드 실패·중복 요청·명시적 취소·PIE 종료.
- 생성 완료 NPC의 독립 수명과 옵션 완료 통지.
- 이 목록만으로 에이전트가 빌드·테스트·별도 검사를 수행하지 않는다.

## 완료 시 갱신할 문서

- [작업 상태](https://github.com/jaykop/Kata/issues/21): 구현 범위·제한과 실제 실행 확인.
- [스포너 사용법](../manual/Spawner.md): Row 선택, 인라인 설정 추가, 생성·취소와 결과 이벤트.
- [캐릭터 데이터 사용법](../manual/Character-Data.md): 스포너 생성 경로.
- [스포너 결정 기록](../devlog/2026-09-30-Spawner-Component-Design.md): 결정 이유와 확인 결과.
- [문서 목록](../README.md): 관련 링크.
