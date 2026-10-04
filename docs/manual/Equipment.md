# 장비 사용법

갱신: 2026-10-04  
대상: KataFramework의 장비 행(`FKataEquipmentRow`), 장비 ID(`FKataEquipmentId`), 장비 설정(`UKataEquipmentSetup`), 장착 컴포넌트(`UKataEquipmentComponent`)  
적용 기준: [#30 장비·무기 시스템](https://github.com/jaykop/Kata/issues/30) EQ-1, [장비·무기 시스템 계획](../plan/Equipment-Plan.md)  
확인 상태: 2026-10-04 사용자 Editor 빌드, 장비 ID 드롭다운, 캐릭터 행의 시작 장비로 무기가 손 소켓에 붙는 것을 확인했다. 교체·해제·GE·태그·방패의 개별 결과는 보고되지 않았다

## 목적과 준비

장비를 데이터 테이블 행으로 정의하고, 캐릭터의 부위 슬롯에 장착·해제한다. 장착하면 부품 메시가 슬롯 소켓에 붙고, 소유자 ASC에 Gameplay Effect와 루즈 태그가 적용된다.
무기별 그래프·Anim Layer 교체는 이후 단계(EQ-2~EQ-3)에서 제공한다.

- `AKataCharacter`는 `KataEquipment` 컴포넌트(`UKataEquipmentComponent`)를 기본으로 가진다. 다른 액터에는 컴포넌트를 직접 추가한다.
- 데이터 컬렉션과 Project Settings 설정은 [캐릭터 데이터 사용법](Character-Data.md)과 같다.
- 슬롯 태그는 플러그인이 정의하지 않는다. 슬롯 선택기는 `Equipment.Slot` 아래 태그만 보여 준다. 샘플 프로젝트는 `Config/Tags/Equipment.ini`에 `Equipment.Slot.Hand.Right`, `Equipment.Slot.Hand.Left`를 둔다.

## 사용 순서

1. DataTable을 만들 때 행 구조로 `KataEquipmentRow`를 고르고, 장비마다 행을 추가한다. 행 이름이 장비 ID다.
2. 행에 `Allowed Slots`(장착 대상으로 고를 수 있는 슬롯), `Parts`(메시·부착 슬롯·상대 Transform), 필요하면 `Granted Effects`·`Granted Tags`를 지정한다.
3. 데이터 컬렉션의 `Equipment Tables` 목록에 그 테이블을 넣는다.
4. Content Browser의 Miscellaneous > Data Asset에서 `KataEquipmentSetup`을 만든다. `Slot Sockets`에 슬롯별 소켓 이름(예: 오른손 슬롯 → 캐릭터 메시의 손 소켓)을, `Default Slot`에 기본 슬롯을 넣는다. 같은 스켈레톤을 쓰는 캐릭터끼리 하나를 공유한다.
5. 캐릭터 테이블 행의 `Equipment Setup`에 그 데이터 에셋을, `Starting Equipment`에 시작 장비(장비 ID와 슬롯)를 넣는다. 캐릭터가 생성되면 행의 설정이 장착 컴포넌트에 들어가고, 시작 장비는 캐릭터 BeginPlay에서 장착된다.
6. 실행 중 교체는 Blueprint나 C++에서 `Equip(Equipment Id, Target Slot)`을 호출한다. `On Equipped`·`On Unequipped`·`On Equip Failed`로 결과를 받고, `Unequip(Slot)`·`Unequip All`로 해제한다.

행을 쓰지 않는 액터는 `KataEquipment` 컴포넌트의 `Equipment Setup`에 데이터 에셋을 직접 지정한다. 행이 `Equipment Setup`을 비워 두면 이 Blueprint 기본값을 쓴다.

## 주요 설정과 동작 규칙

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| Allowed Slots | 장착 대상으로 고를 수 있는 슬롯 | 비어 있으면 장착할 수 없다. 같은 장비를 양손 어디에나 장착하려면 두 슬롯을 넣는다 |
| Parts | 장착할 때 만드는 메시 부품 | 메시는 Static Mesh 또는 Skeletal Mesh. `Slot`을 비우면 장착 대상 슬롯에, 지정하면 그 슬롯에 붙고 장비가 그 슬롯도 점유한다 |
| Granted Effects | 장착하는 동안 적용할 GE | 지속형 GE는 해제할 때 제거한다. 즉시형은 되돌리지 않는다 |
| Granted Tags | 장착하는 동안 ASC에 더할 루즈 태그 | 해제할 때 같은 수만큼 뺀다 |
| Equipment Setup의 Slot Sockets | 슬롯별 부착 소켓 | 슬롯이 없거나 소켓이 None이면 부착 대상의 원점에 붙는다. 없는 소켓이면 경고 후 원점에 붙는다. Equipment Setup이 없으면 모든 부품이 원점에 붙는다 |
| Equipment Setup의 Default Slot | 대상 슬롯을 비웠을 때 쓰는 슬롯 | 없거나 장비가 허용하지 않으면 장비의 첫 허용 슬롯을 쓴다 |
| 캐릭터 행의 Equipment Setup | 장착 컴포넌트에 넣을 장비 설정 | 비우면 컴포넌트의 Blueprint 기본값. 캐릭터 생성 때 함께 비동기로 로드한다 |
| 캐릭터 행의 Starting Equipment | BeginPlay에서 장착할 장비와 슬롯 | 슬롯을 비우면 Default Slot 규칙을 따른다. 장비 에셋은 장착할 때 로드한다 |
| `Set Equipment Setup` | 장비 설정 교체 | 이미 장착한 장비의 위치는 바뀌지 않고 다음 장착부터 적용된다 |
| `Equip` | 행을 복사하고 에셋을 비동기로 로드한 뒤 장착한다 | 행이 없거나 대상 슬롯을 허용하지 않으면 경고 후 false(`On Equip Failed`는 부르지 않는다). 점유 슬롯이 겹치는 대기 요청은 취소한다. 에셋이 이미 로드되어 있으면 호출 안에서 바로 장착될 수 있다 |
| 장착 시 교체 | 점유할 슬롯을 쓰던 장비 | 먼저 해제하고 `On Unequipped`를 알린다 |
| `On Equip Failed` | 요청을 받은 뒤 에셋 로드 실패 | 일부만 장착하지 않는다 |
| `Unequip` / `Unequip All` | 해제 | 메시를 파괴하고 GE·태그를 되돌린다. 장비가 점유한 다른 슬롯도 함께 비워진다 |
| EndPlay | 소유자 종료 | 대기 요청을 취소하고 알림 없이 자원만 되돌린다 |

점유 슬롯은 장착 대상 슬롯과 부품이 지정한 슬롯의 합이다. 쌍검 장비는 `Allowed Slots`에 오른손을 두고, 두 부품의 `Slot`을 오른손·왼손으로 지정하면 양손을 함께 점유한다.
부품 메시는 캐릭터면 캐릭터 Mesh, 아니면 소유자 루트 컴포넌트에 붙는다. 장비 메시에는 충돌을 켜지 않는다. 공격 판정은 Hit Trace가 맡는다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| `Equip`이 false | 행 없음, 대상 슬롯 미허용 | `LogKataFramework` 경고와 컬렉션의 Equipment Tables, Allowed Slots를 확인한다 |
| 메시가 손이 아닌 원점에 붙는다 | Equipment Setup이 없거나, Slot Sockets에 슬롯이 없거나, 소켓 이름이 틀림 | 캐릭터 행 또는 컴포넌트의 Equipment Setup과 캐릭터 메시의 소켓 이름을 확인한다 |
| 시작 장비가 장착되지 않는다 | 행의 Starting Equipment 비어 있음, 장비 ID 없음 | 캐릭터 생성 경로(GameMode·스포너)로 생성했는지, `LogKataFramework`의 Equip 경고를 확인한다. 레벨에 직접 배치한 캐릭터는 행이 적용되지 않는다 |
| GE·태그가 적용되지 않는다 | 소유자에 ASC가 없음 | 경고 로그를 확인한다. `AKataCharacter`는 ASC를 가진다 |
| 무기 그래프·Anim Layer가 바뀌지 않는다 | EQ-1 범위 밖 | EQ-2~EQ-3에서 제공한다 |
| 히트 판정이 장비 메시를 쓰지 않는다 | HitBox 연결은 EQ-4 | EQ-4에서 제공한다 |
| 에디터 프리뷰에 장비가 붙지 않는다 | 프리뷰 장착은 EQ-5 | EQ-5에서 제공한다 |

방어구의 Leader Pose 처리, 장비 메시 충돌, Blueprint 핀용 장비 ID 드롭다운은 제공하지 않는다.

## 확인 상태와 근거

- [KataEquipmentRow.h](../../Plugins/KataFramework/Source/KataFramework/Public/Equipment/KataEquipmentRow.h): 행 구조와 점유 슬롯 규칙.
- [KataEquipmentComponent.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Equipment/KataEquipmentComponent.cpp): 비동기 장착, 교체, 해제.
- 2026-10-04 사용자 확인 범위는 문서 머리의 확인 상태를 따른다.
