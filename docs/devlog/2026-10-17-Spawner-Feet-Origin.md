# 스포너 기준점을 발 높이로 변경

작성: 2026-10-17  
갱신: 2026-10-17  
유형: 결정 기록  
대상: KataFramework 스포너(`AKataCharacterSpawner`), 캐릭터 생성 Subsystem, Nav Mesh Projection, 샘플 맵 LV_FeatureGallery  
기준: 2026-10-17 작업 트리. 같은 스포너 파일에 있던 다른 작업의 미커밋 변경은 포함하지 않는다.

## 배경과 결론

스포너를 바닥에 놓아도 액터 위치(기즈모)는 캡슐 중심 기준이어서, 미리보기 메시와 영역이 허리 높이에 맞춰졌다. 사용자는 기즈모와 영역이 메시 발 높이(바닥)에 오기를 요청했다.
스포너가 고르는 위치를 발 위치로 바꾸고, 실제 생성과 미리보기 모두 Character Class 캡슐의 절반 높이만큼 올려 캡슐 중심을 놓도록 했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 생성 위치 | Spawn Area·NavMesh에서 고른 위치를 캡슐 중심으로 사용 | 고른 위치를 발 위치로 보고, 생성 직전에 캡슐 절반 높이만큼 월드 Z로 올린다 |
| 생성 Subsystem | 요청 Transform을 그대로 SpawnActorDeferred에 전달 | `RequestSpawnFromRow`·`RequestSpawnFromRowBudgeted`에 `bLocationAtFeet`(기본 false)를 추가했다 |
| 더미 재생성 | 기록한 액터 위치를 고정 Transform으로 사용 | 고정 Transform은 이미 캡슐 중심이므로 높이를 더하지 않는다 |
| 미리보기 메시 | 영역 중심에 캡슐 중심을 놓음 | 실제 생성과 같이 캡슐 절반 높이를 더해 발이 영역 중심에 닿는다 |
| Nav Mesh Projection `Height Offset` | 캡슐 절반 높이를 넣어야 바닥 위에 섰다 | 기본 0이면 NavMesh 표면에 선다. 이전에 넣은 값은 그만큼 공중 생성이 된다 |
| 캡슐 높이 조회 | 없음 | `KataFL::GetCharacterClassHalfHeight`가 Class 기본 객체의 캡슐 높이에 상대 Z 스케일을 곱해 돌려준다 |

## 주요 결정과 이유

- 캡슐 높이 보정은 Subsystem이 클래스 로드를 끝낸 뒤 적용한다. 행의 Character Class는 비동기로 로드되므로 스포너가 요청 시점에 캡슐 높이를 알 수 없다. 스포너에서 동기 로드하면 게임 중 멈춤이 생긴다.
- 보정 여부는 요청 매개변수로 넘기고 기본값을 false로 둔다. GameMode, Async Action, 런타임 스포너 패널처럼 캡슐 중심 Transform을 넘기는 기존 호출자는 동작이 바뀌지 않는다.
- 높이는 생성 회전의 위쪽 축이 아니라 월드 Z로 더한다. 영역 회전이 기울어 있어도 캐릭터 캡슐은 수직으로 서기 때문이다.
- 기본 객체는 등록되지 않아 월드 Transform이 단위 행렬이다. 그래서 `GetScaledCapsuleHalfHeight` 대신, 그 함수와 같이 루트 캡슐의 상대 Z 스케일을 직접 곱한다. 처음에는 세 축 중 가장 작은 스케일을 곱했으나, Codex 설명 검토에서 UE 5.8의 이 함수가 Z 스케일을 쓴다는 지적을 받고 엔진 계산에 맞췄다.

## 기존 배치 처리

- LV_FeatureGallery의 스포너 24개를 사용자 요청으로 MCP를 써서 옮겼다. 액터는 트레이스로 잰 바닥 높이에 두고 회전은 유지했다. 영역 Z와 `Height Offset`은 0, 상자 영역은 Z Extent를 0으로 바꿨다. 구 영역 3개는 반지름 200을 유지했다.
- MCP 이동 중 두 가지 문제가 있었다. 위치만 넘기니 액터 회전이 0으로 덮였고, Construction이 다시 돌지 않아 미리보기 메시가 바닥에 파묻혀 보였다. 맵을 저장하지 않고 다시 연 뒤 회전까지 넘겨 다시 적용했다. 미리보기는 프로퍼티를 같은 값으로 다시 설정해 Construction을 갱신했다.
- LV_CombatSandbox에도 `Height Offset` 값이 들어간 Nav Mesh Projection이 있다. 이 맵은 고치지 않았다.

## 근거

- [KataCharacterSpawnSubsystem.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataCharacterSpawnSubsystem.cpp): `SpawnFromRequest`의 `bLocationAtFeet` 보정.
- [KataCharacterSpawner.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataCharacterSpawner.cpp): `SubmitNextSpawnRequest`에서 고정 Transform을 제외하고 보정을 요청한다.
- [KataCharacterSpawner_Preview.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataCharacterSpawner_Preview.cpp): `UpdateCharacterPreview`의 높이 보정.
- [KataFL_Spawning.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataFL_Spawning.cpp): `GetCharacterClassHalfHeight`.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 코드 빌드 | 사용자 Editor 빌드 보고 | 새 빌드를 에디터에 올렸다고 보고 | Game 타깃 빌드 |
| 미리보기 메시 | MCP로 `CharacterPreview` 상대 위치를 읽음 | 24개 모두 상대 Z -2(메시 상대 -90 + 캡슐 88)로 발이 바닥에 닿음 | 뷰포트 육안 확인 |
| 맵 값 | MCP로 다시 읽음 | 회전 유지, 영역 Z·`Height Offset`·상자 Z Extent 0 | PIE 생성 위치 |

## 남은 제한과 후속 작업

- `L3-Spawner-Ledge`는 Z 150 턱 위에 반지름 200 구 영역이 있다. NavMesh 투영이 가장 가까운 표면을 고르므로 아래쪽 후보가 1층 바닥으로 투영될 수 있다.
- LV_CombatSandbox의 `Height Offset`은 아직 이전 기준값이다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [스포너 사용법](../manual/Spawner.md) | 미리보기 배치, 영역 높이 기준, 이전 배치 처리, `Height Offset` 설명을 갱신했다 |
| [캐릭터 데이터 사용법](../manual/Character-Data.md) | 생성 API 표에 `bLocationAtFeet`를 더했다 |
| 연결 이슈 | 없음 |
