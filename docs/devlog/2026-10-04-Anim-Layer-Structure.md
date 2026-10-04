# 애니메이션 레이어 구조와 ASC 태그 정책

작성: 2026-10-04  
갱신: 2026-10-04  
유형: 결정 기록  
대상: KataFramework `UKataAnimInstance`·`UKataAnimLayerInstance`·`UKataAnimLayerSetup`, 장비 행과 장착 컴포넌트, 게임플레이 태그 정책  
기준: 미커밋 작업 트리. [#32](https://github.com/jaykop/Kata/issues/32), [#30](https://github.com/jaykop/Kata/issues/30) EQ-3

## 배경과 결론

캐릭터 Anim Instance를 2족·4족 또는 Human·Creature로 나누고 Player용 하위 클래스를 둘지 검토했다.
몸 구조와 역할로 C++ 클래스를 나누지 않고, 공통 베이스 하나와 Linked Anim Layer로 구성하기로 했다.
무기별 애니메이션은 레이어로 교체하고, 무기는 애니메이션을 직접 참조하지 않으며 스켈레톤별 레이어 설정이 무기 종류 태그로 레이어를 고른다.
같은 검토에서 ASC에 넣는 태그를 `Status`·`Identity` 두 루트로 제한했다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 메인 Anim Instance | 없음. 샘플 ABP가 EventGraph에서 이동 값을 계산 | `UKataAnimInstance`가 이동 값을 thread-safe하게 계산하고 GAS 태그를 변수에 매핑 |
| 레이어 Anim Instance | 없음 | `UKataAnimLayerInstance::GetMainAnimInstance`로 레이어가 메인의 값을 읽음 |
| 무기 레이어 지정 | 계획상 Weapon 행이 Anim Layer를 직접 지정 | 장비 행은 `EquipmentType` 태그만 갖고 `UKataAnimLayerSetup`이 태그→레이어를 정함 |
| 레이어 링크 | 없음 | `UKataEquipmentComponent`가 Anim Instance 초기화·장착·해제·설정 변경 때 링크 |
| 프리뷰 | 레이어가 링크되지 않아 T 포즈 | 컴포넌트의 `OnRegister` 바인딩과 `PreviewAnimLayerSetup`으로 프리뷰에서도 링크 |
| 장비 부여 태그 | `Categories = "Equipment"` | `Categories = "Status"` |

## 주요 결정과 이유

- **몸 구조로 클래스를 나누지 않는다.** 다리 개수는 클래스가 아니라 설정값이다. 발 IK·몸통 정렬은 AnimGraph 노드와 Control Rig가 처리하고, C++ Anim Instance는 몸 구조와 무관한 입력값(속도, 이동 모드, 태그)만 준비한다. 2족·4족 분류는 뱀·박쥐·다족을 덮지 못하고, Human·Creature는 콘텐츠 분류라 애니메이션 차이와 맞지 않는다.
- **Player 전용 Anim Instance를 두지 않는다.** 입력은 Controller→Character·Movement로 흐르고 Anim Instance는 가속 등 Pawn 상태만 읽는다. AI 조종이나 빙의 전환에도 같은 클래스가 동작한다.
- **에셋 슬롯은 메인 Anim Instance가 아니라 레이어 ABP의 Blueprint 변수에 둔다.** 필요한 슬롯이 몸 구조마다 다르고 계속 늘어나므로 C++ 필드로 고정하지 않는다. 변수는 레이어 베이스에 한 번 선언하고 캐릭터별 자식은 값만 채운다.
- **레이어 함수는 입력 없이 둔다.** 이동 값은 레이어가 Property Access로 메인에서 가져간다. 입력을 두면 모든 레이어 구현의 시그니처가 고정된다.
- **ALI는 교체 원인별로 나눈다.** 무기 레이어(이동·상체·AimOffset)와 Body 레이어(발 IK·몸통 정렬)를 분리하면 무기 레이어를 바꿔도 Body 레이어가 유지된다.
- **무기는 종류 태그만 갖는다.** 무기가 레이어를 직접 지정하면 스켈레톤이 다른 캐릭터가 같은 무기를 들 때 맞지 않는 레이어가 링크되고, 항목이 무기 수 × 스켈레톤 수만큼 반복된다. 스켈레톤별 설정에 태그→레이어를 두면 불일치가 구조로 막히고 항목은 스켈레톤 수 × 무기 종류 수가 된다. 스켈레톤을 태그로 표현하지 않고 실제 `USkeleton`으로 검사한다.
- **레이어 설정은 캐릭터 행과 컴포넌트에 둔다.** 사용자 요청으로 `EquipmentSetup`과 같은 위치·우선순위를 따른다. Anim Instance Class Defaults에 두는 대안은 프리뷰 처리가 단순했지만 채택하지 않았다.
- **레이어 클래스는 하드 참조다.** 비동기 로드 경로를 늘리지 않기 위해서다. 무기 수가 늘어 메모리가 문제가 되면 소프트 참조로 바꾼다.
- **그래프는 레이어 설정에 넣지 않는다.** 무기별 그래프 차이는 기존 결정대로 ASC 태그로 전이를 분기한다.
- **ASC에는 `Status`와 `Identity`만 넣는다.** 아이템 분류(`Equipment.Type`)와 슬롯은 데이터 키이므로 ASC에 넣지 않는다. 쿨다운 태그도 `Status.Cooldown`으로 통일한다.
- **매핑이 없으면 기본 무기 레이어를 쓴다.** 처음에는 현재 레이어 유지를 제안했으나, Anim Instance가 다시 초기화된 직후에는 유지할 레이어가 없어 기본 레이어와 경고로 바꿨다.
- **Anim Blueprint 편집기 프리뷰는 별도 경로를 쓴다.** 그 프리뷰에는 장착 컴포넌트가 없으므로 `UKataAnimInstance`의 에디터 전용 `PreviewAnimLayerSetup`을 `OnAnimInitialized` 이후에 링크한다. 편집기는 컴파일마다 인스턴스를 새로 만들어 바인딩이 쌓였으므로(관측 시 29개) 핸들러가 한 번 불리면 바인딩을 해제한다.

## 근거

- [KataAnimInstance.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Animation/KataAnimInstance.cpp): 스냅샷·파생값 분리, 태그 매핑, 프리뷰 링크.
- [KataAnimLayerSetup.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Animation/KataAnimLayerSetup.cpp): 레이어 선택과 스켈레톤 검사.
- [KataEquipmentComponent.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Equipment/KataEquipmentComponent.cpp): `RefreshAnimLayers`, `OnRegister` 바인딩.
- [KataCharacter.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataCharacter.cpp): 행의 레이어 설정을 메시 변경 전에 넣고 마지막에 한 번 링크.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| Editor 빌드 | 사용자 빌드 | 새 클래스가 에디터에 로드됨 | Game 타깃 빌드 |
| Anim Blueprint 편집기 프리뷰 | 사용자 확인, 출력 로그 | BlackKnight 대검 Idle 재생, 링크 로그 확인 | 바인딩 해제 수정 후 재확인 |
| 액션 편집기 프리뷰 | 사용자 확인 | Idle 재생 | 프리뷰 장비 장착 시 레이어 교체 |
| PIE | 사용자 확인 | Idle·Walk·Run 전환, 시작 장비(대검) 장착 시 무기 레이어 링크 | 장착 중 무기 교체·해제 |
| 에디터 데이터 설정 | Unreal MCP로 설정 후 값 재조회 | 태그, 장비 행, 레이어 설정, 캐릭터 행·BP·ABP 연결 저장 | 없음 |

처음 Anim Blueprint 편집기 프리뷰가 T 포즈였던 원인은 확정하지 못했다. 진단 로그를 넣은 뒤 다시 열었을 때부터 정상 동작했다.

## 남은 제한과 후속 작업

- 레이어 ABP가 메인 ABP의 Blueprint 변수(`bGuard` 등)를 읽을 경로가 없다. `UKataAnimLayerInstance`에 태그 매핑을 두는 안이 제안 상태다.
- 왼손 무기의 레이어와 손별 ALI 키는 [장비·무기 계획](../plan/Equipment-Plan.md) EQ-6에서 정한다.
- Glaive·Greataxe처럼 이동 애니메이션이 없는 무기 종류가 어느 레이어를 쓸지 정하지 않았다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#32](https://github.com/jaykop/Kata/issues/32) | 사용자 실행 확인 후 2026-10-04 닫음 |
| [#30](https://github.com/jaykop/Kata/issues/30) | EQ-3 진행 상황. 게시는 사용자 확인 후 |
| [애니메이션 레이어](../manual/Animation-Layers.md) | 신규 사용법 |
| [장비 사용법](../manual/Equipment.md) | Equipment Type, Granted Tags 루트, Anim Layer Setup |
| [게임플레이 태그](../manual/Gameplay-Tags.md) | 루트 표와 ASC 태그 정책 |
| [장비·무기 계획](../plan/Equipment-Plan.md) | Anim Layer 해석 항목, 결정 표, EQ-3 |
