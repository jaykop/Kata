# 스포너 거리 이탈을 개체별 휴면으로 다루는 결정

작성: 2026-10-07  
갱신: 2026-10-07  
유형: 결정 기록·소스 진단  
대상: KataFramework 스포너의 후속 거리·상태 보존 설계  
기준: 배치 실행 분리 커밋 `da3eada` 이후 타임슬라이싱·수동 디스폰을 포함한 미커밋 작업 트리

## 배경과 결론

앞선 계획은 스포너 원점과 플레이어의 거리로 세대를 제거하고 재진입하면 새 배치를 생성하는 안이었다. 사용자는 전투 여부보다 스폰된 액터와 PC의 실제 거리가 적절하며, 사망이 아닌 거리 제거라면 상태를 스냅샷하고 재생성 때 복원해야 한다고 요청했다. 이어 개체별 거리 판단·휴면 기록·스냅샷 복원 계약을 먼저 정하는 순서를 확정했다.

이에 거리 이탈을 개체별 휴면으로 다루는 계약안을 작성했다. 사용자 요구와 구체 설계 제안을 구분했으며 코드 구현은 이 문서 작업에 포함하지 않았다.

## 변경 또는 진단 내용

| 항목 | 이전 안 또는 현재 코드 | 반영 방향 |
|---|---|---|
| 거리 이탈 | 스포너 원점에서 멀어지면 세대 일괄 제거 | 사용자가 요청한 NPC 현재 위치와 PC Pawn의 거리로 개체별 판정 |
| 재진입 | 새 수량·위치의 배치 생성 제안 | 같은 개체의 스냅샷 복원 요구 반영 |
| 검색 위치 | 스포너 원점만 인덱싱 | 최초 생성 원점과 휴면 스냅샷 위치를 나눠 인덱싱하는 안 |
| 식별 | 배치 세대 ID와 약한 Actor·Controller 참조 | 재생성에도 유지되는 슬롯 ID가 필요 |
| 상태 적용 | 생성 행·기본 GameplayData 적용 후 BeginPlay·AI 시작 | 기본 ASC 초기화 뒤 복원하고 준비 완료 전 AI를 보류하는 경계 필요 |
| 수동 제거 | `DespawnCharacters`로 전체 생성 기록 제거 | 기존 API와 스냅샷 기반 개체 휴면을 분리 |

## 주요 결정과 이유

개체별 거리를 사용하면 스포너 원점에서 이동한 NPC도 플레이어 가까이에 있는 동안 유지할 수 있다. 휴면 복원도 저장 위치를 기준으로 해야 원점에서 반복 생성·제거되거나 이동한 NPC를 찾지 못하는 문제가 생기지 않는다. 최초 생성 원점과 저장 위치의 구분은 구체적인 설계 제안이다.

캡처에 실패하면 NPC를 유지하고, 복원에 실패하면 원본 스냅샷을 유지하는 계약을 제안했다. 실제 NPC·Controller 정리와 새 개체 생성의 경계를 둬 중복 개체와 상태 손실을 막는다. 사망 통지와 외부 제거를 별도 사유로 다루고 거리 경로로 부활시키지 않는다.

GAS의 Current 값에는 효과가 반영될 수 있으므로 그 값을 Base로 복사하고 효과를 다시 적용하는 방식은 피해야 한다. 보존할 Attribute·효과와 공급자별 복원 규칙을 명시하고, 지원하지 않는 필수 지속 상태가 있으면 휴면을 거절하는 안을 정리했다. 휴면 중 시간 정지 여부와 언로드를 넘는 보존 범위는 미확정이다.

## 근거

- [전체 계획](../plan/Spawner-Scheduling-And-Grid-Plan.md): 이전 원점·세대 단위안을 수정하고 계약을 선행 단계로 연결.
- [휴면 계약안](../plan/Spawner-Dormancy-Contract-Plan.md): 거리, 슬롯 상태, 캡처·복원 순서와 실패 조건의 원문.
- [KataCharacter.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataCharacter.cpp): 행 적용과 `PostInitializeComponents`의 ASC·GameplayData 초기화.
- [KataAICharacter.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataAICharacter.cpp): BeginPlay 이후 AI 준비·시작 경계.
- [KataGameplayData.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataGameplayData.cpp): AttributeSet·초기값·효과·능력·태그의 기본 적용 경로.
- [생성 소유 기록](../../Plugins/KataFramework/Source/KataFramework/Public/Character/KataCharacterSpawnOwnership.h): 세대와 현재 Actor·Controller를 공유하되 지속 스냅샷은 보관하지 않음.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 거리·휴면 방향 | 사용자 대화 | 개체별 거리, 거리 제거 전 저장·복원, 계약 선행 순서 요청 | 세부 시간·보존 수명·공급자 설정 |
| 생성·GAS·AI 수명 | 에이전트의 구현 관련 파일 읽기 | 복원 Context와 준비 완료 경계가 필요함 | 실제 복원·빌드·실행·성능 |
| 이번 변경 | 설계 문서 작성 | 소스·공개 API 변경 없음 | 빌드·테스트·별도 검사 미실시 |

## 남은 제한과 후속 작업

계약안에 따라 슬롯·개체별 거리, 캡처·휴면, 복원 준비 완료, 그리드 순서로 구현 범위를 나눈다. 휴면 중 버프·쿨다운의 시간 정책과 초기 보존 대상은 해당 구현 전에 정해야 한다. World Partition·세이브 영속화는 초기 권고 범위 밖이다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| 연결 이슈 | 미게시 초안의 범위·작업 순서만 개정. 외부 게시 없음 |
| [스포너 사용법](../manual/Spawner.md), [캐릭터 데이터 사용법](../manual/Character-Data.md) | 실제 API 변경이 없어 이번 설계로 현재 사용법을 바꾸지 않음 |
| [전체 계획](../plan/Spawner-Scheduling-And-Grid-Plan.md) | 사용자 요구와 슬롯·복원 선행 단계 반영 |
| [휴면 계약안](../plan/Spawner-Dormancy-Contract-Plan.md) | 구체 계약과 미확정 사항 작성 |
| [문서 목록](../README.md) | 계약안·결정 기록 링크 추가 |
