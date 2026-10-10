# 애니메이션 레이어 사용법

갱신: 2026-10-11  
대상: KataFramework의 `UKataAnimInstance`·`UKataAnimLayerInstance`·`UKataAnimLayerSetup`으로 캐릭터 Anim Blueprint를 구성하는 사용자  
적용 기준: [애니메이션 레이어 구조 결정](../devlog/2026-10-04-Anim-Layer-Structure.md), [장비·무기 계획](../plan/Equipment-Plan.md) EQ-3, [발 IK 구현 기록](../devlog/2026-10-11-Foot-IK.md)  
확인 상태: 2026-10-04 사용자가 BlackKnight로 Anim Blueprint 편집기 프리뷰와 액션 편집기 프리뷰의 Idle 재생, PIE의 Idle·Walk·Run 전환과 시작 장비 장착 시 무기 레이어 링크를 확인했다. 장착 중 무기 교체·해제에 따른 레이어 교체와 Game 타깃 빌드는 미확인

## 목적과 준비

메인 Anim Blueprint 하나가 상태 머신과 전이를 맡고, 무기마다 다른 애니메이션은 Linked Anim Layer로 바꿔 끼운다.
무기를 바꾸면 상태 전환 규칙은 그대로이고 포즈만 바뀐다. 몸 구조(2족·4족·뱀·비행)나 역할(PC·몬스터)로 C++ Anim Instance를 나누지 않는다.

| 구성 | 종류 | 역할 |
|---|---|---|
| `UKataAnimInstance` | C++, 메인 ABP의 부모 | 이동 값 계산, GAS 태그→변수 매핑 |
| `UKataAnimLayerInstance` | C++, 레이어 ABP의 부모 | 메인 인스턴스를 돌려주는 `Get Main Anim Instance` |
| Anim Layer Interface | 에셋 | 레이어 함수 목록. 무기 레이어용과 Body 레이어용을 따로 둔다 |
| `UKataAnimLayerSetup` | 데이터 에셋, 스켈레톤마다 하나 | Body 레이어, 기본 무기 레이어, `Equipment.Type` 태그별 무기 레이어 |
| `UKataEquipmentComponent` | 컴포넌트 | 레이어 설정에 따라 캐릭터 Mesh에 레이어를 링크 |

캐릭터는 `AKataCharacter` 계열이어야 한다. 레이어 링크는 캐릭터의 `Kata Equipment` 컴포넌트가 한다.

## 사용 순서

공용 에셋은 한 번만 만든다.

1. Anim Layer Interface(예: `ALI_KataWeapon`)를 만들고 레이어 함수(`FullBody_Idle`, `FullBody_Walk`, `FullBody_Run` 등)를 입력 없이 추가한다.
2. 메인 Template ABP(예: `ABP_CharacterBase`)를 만든다. Parent Class는 `Kata Anim Instance`, Interfaces에 1의 ALI를 추가한다.
   상태 머신과 전이 조건은 부모의 변수(`Ground Speed` 등)로 만들고, 각 상태 안에는 같은 이름의 레이어 노드 하나만 둔다. 애니메이션 에셋은 넣지 않는다.
3. 레이어 Template ABP(예: `ABP_WeaponLayersBase`)를 만든다. Parent Class는 `Kata Anim Layer Instance`, Interfaces에 같은 ALI를 추가한다.
   애니메이션·Blend Space 변수를 기본값 없이 선언하고 레이어 함수를 구현한다. Blend Space의 X·Y는 Property Access로 `Get Main Anim Instance` → `Local Right Speed`·`Local Forward Speed`에 바인딩한다.

캐릭터(스켈레톤)마다 반복한다.

4. 2의 자식 ABP(예: `ABP_BlackKnight`)를 만들고 스켈레톤을 지정한다.
5. 무기 종류마다 3의 자식 ABP(예: `ABP_BlackKnight_Layers_Greatsword`)를 만들고, 스켈레톤을 지정한 뒤 Class Defaults에서 애니메이션 변수만 채운다.
6. `Kata Anim Layer Setup` 데이터 에셋을 만들어 `Skeleton`, `Default Weapon Layer`, `Weapon Layers`(태그 → 5의 레이어)를 지정한다.
7. 6을 세 곳에 지정한다.
   - 캐릭터 테이블 행의 `Anim Layer Setup`: 게임에서 쓴다.
   - 캐릭터 Blueprint의 `Kata Equipment` 컴포넌트 `Anim Layer Setup`: 행이 없을 때와 BP 뷰포트·액션 편집기 프리뷰에서 쓴다.
   - 4의 Class Defaults `Preview Anim Layer Setup`: Anim Blueprint 편집기 프리뷰에서만 쓴다.
8. 장비 행의 `Equipment Type`에 무기 종류 태그를 지정한다. 오른손(Equipment Setup의 Default Slot)에 그 장비를 장착하면 해당 무기 레이어로 바뀐다.

## 주요 설정과 실행 계약

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| `Kata Anim Instance`의 `Kata\|Locomotion` 변수 | `Velocity`, `Acceleration`, `Ground Speed`, `Local Forward Speed`, `Local Right Speed`, `Velocity Direction Angle`(-180~180도), `Has Acceleration`, `Is Falling`, `Is On Ground`, `Movement Mode` | 소유자가 캐릭터가 아니면(ABP 프리뷰 등) 기본값. Blueprint에서는 읽기 전용 |
| `Gameplay Tag Property Map` | ASC 태그가 붙고 떨어질 때 ABP의 bool·int·float 변수를 갱신 | ASC가 없으면 갱신하지 않는다. 변수 이름·타입이 틀리면 데이터 검증 오류 |
| `Kata Anim Instance`의 `Kata\|Tilt` 값과 설정 | 대상 방향 Tilt의 `Tilt Pitch`·`Tilt Alpha`(읽기 전용)와 캐릭터별 자식 ABP Class Defaults의 `Tilt Bone Chain`·`Aim Origin Height`·`Expected Target Height`. Template ABP의 Slot 뒤에 Kata Tilt 노드를 두고 쓴다 | 체인이 비어 있으면 Tilt를 적용하지 않는다. 메시에 Template ABP를 직접 지정하면 체인이 비므로 자식 ABP를 지정한다. [대상 방향 Tilt 사용법](Target-Tilt.md) |
| `Get Main Anim Instance` | 레이어가 링크된 메시의 메인 인스턴스 | Thread Safe. 메인이 `Kata Anim Instance`가 아니거나 없으면 None이며 Property Access는 0을 넣는다 |
| `Skeleton` | 레이어 설정의 대상 스켈레톤 | 메시의 스켈레톤과 다르면 링크하지 않고 경고. 비우면 검사하지 않는다 |
| `Body Layer` | 무기와 무관한 레이어(발 IK 등) | 비워도 된다. 무기 레이어를 바꿔도 유지된다 |
| `Default Weapon Layer` | 무기가 없을 때의 무기 레이어 | 비어 있고 무기도 없으면 메인 ABP의 기본 구현(비어 있으면 기준 포즈)이 쓰인다 |
| `Weapon Layers` | `Equipment.Type` 태그별 무기 레이어 | 태그가 없으면 경고 후 기본 무기 레이어 |
| `Set Anim Layer Setup` | 컴포넌트의 레이어 설정 교체 | 즉시 다시 링크한다 |

- 링크 시점: 메시의 Anim Instance가 초기화될 때, 장착·해제할 때, 레이어 설정을 바꿀 때. 캐릭터 행이 Anim Class를 바꿔도 다시 링크된다.
- 레이어 클래스는 하드 참조라 레이어 설정을 로드하면 그 스켈레톤의 모든 무기 레이어와 애니메이션이 함께 로드된다.
- 데이터 검증은 레이어 ABP의 Target Skeleton이 `Skeleton`과 다르면 오류를 낸다. 스켈레톤이 없는 Template 레이어는 검사하지 않는다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| T 포즈가 나온다 | 링크된 레이어가 없어 메인 ABP의 빈 기본 구현이 쓰인다 | 7의 세 곳과 `Default Weapon Layer`를 확인한다. `LogKataFramework`에 `Layers are not linked`가 있으면 스켈레톤이 다르다 |
| 무기를 바꿔도 포즈가 같다 | 장비 행의 Equipment Type이 비었거나 Weapon Layers에 없음, 오른손이 아닌 슬롯에 장착 | 경고 로그 `has no weapon layer for`와 장비 행을 확인한다 |
| BP 뷰포트에 무기가 없다 | 시작 장비는 BeginPlay에서 장착한다 | 정상이다. 액션 편집기 프리뷰는 Preview Setups로 장착한다 |
| 레이어 ABP에서 `Get Main Anim Instance`가 안 보인다 | Parent Class가 `Kata Anim Layer Instance`가 아니거나 빌드 전 | 부모를 확인하고 빌드한다 |
| 레이어에서 `bGuard` 같은 BP 변수를 읽을 수 없다 | 레이어는 메인 인스턴스를 C++ 타입으로만 안다 | 지원하지 않는다. 레이어용 태그 매핑은 아직 없다 |

왼손 무기의 레이어와 손별로 다른 Anim Layer Interface는 아직 없다.

## Body 레이어와 발 IK

Body 레이어는 무기와 무관하게 받은 포즈를 고치는 후처리다. 샘플은 경사면 발 IK를 Control Rig로 넣었다.
Kata 플러그인 코드는 쓰지 않고 샘플 콘텐츠로만 구성한다. 확인 상태와 제한은 [발 IK 구현 기록](../devlog/2026-10-11-Foot-IK.md)을 따른다.

| 구성 | 샘플 에셋 | 역할 |
|---|---|---|
| Body ALI | `ALI_KataBody` | 입력 포즈 하나를 받는 레이어 함수 `Body_PostProcess`. 무기 ALI와 달리 입력 포즈가 있다 |
| 메인 Template ABP | `ABP_CharacterBase` | `ALI_KataBody`를 구현하고 AnimGraph의 Slot 뒤, Kata Tilt 앞에서 `Body_PostProcess`를 호출한다. 기본 구현은 입력을 그대로 돌려준다 |
| 2족 Body 레이어 | `ABP_BodyLayer_Biped`(Template) | `Input Pose` → Control Rig(`CR_FootIK_Biped`) → `Output Pose`. DS3 기사 스켈레톤이 공유한다 |
| 사족 Body 레이어 | `ABP_StarvedHound_Body`(스켈레톤 지정) | 같은 구성에 `CR_FootIK_Quadruped`. 사족은 Template으로 만들면 포즈가 깨져 스켈레톤별로 만든다 |
| 레이어 설정 | `DA_AnimLayerSetup_*`의 `Body Layer` | 캐릭터별로 위 Body 레이어를 지정한다 |

Body 레이어 ABP를 만드는 순서는 다음과 같다.

1. Animation Blueprint를 만들고 Parent Class를 `Kata Anim Layer Instance`로 둔다. Interfaces에 `ALI_KataBody`를 추가한다.
2. `Body_PostProcess` 그래프에서 `Input Pose` → Control Rig 노드 → `Output Pose`로 연결하고 Control Rig Class를 지정한다.
3. Control Rig 노드의 `Is On Ground` 핀을 Property Access로 `Get Main Anim Instance` → `Is On Ground`에 바인딩한다. 일반 함수 노드는 AnimGraph 핀에 연결할 수 없다.
4. 레이어 설정의 `Body Layer`에 지정한다.

발 IK 리그는 다음과 같이 동작한다.

- 다리마다 발 아래로 트레이스해 지면 높이만큼 발 목표를 옮기고, `Pelvis`와 `Spine`의 공통 부모(DS3는 `Root`)를 옮긴 뒤 Two Bone IK로 다리를 굽힌다.
- 2족은 두 발 중 낮은 쪽에 맞춰 몸을 내리고 발을 지면 기울기에 맞춘다. 사족은 몸통 Pitch·Roll도 맞추고, 몸 높이는 다리별 요구값의 최솟값과 평균 사이에서 정한다.
- `Is On Ground`가 false면 보정이 서서히 빠진다. Anim Blueprint 편집기 프리뷰에서는 이 값이 false라 발 IK가 보이지 않는다.

| 리그 변수 | 의미 | 기본값(2족 / 사족) |
|---|---|---|
| `PelvisBone`, 다리별 본 이름 | 골반 보정 본과 다리 체인. 다른 이름의 스켈레톤은 리그를 복제해 기본값만 바꾼다 | `Root`, DS3 본 이름 |
| `TraceUp` / `TraceDown` | 컴포넌트 원점 기준 트레이스 시작 높이와 끝 깊이(cm) | 50 / 75, 120 / 80 |
| `MaxFootOffset` | 발 높이 보정 한도(cm) | 45 / 70 |
| `MaxPelvisDrop` | 골반을 내리는 한도(cm). 사족은 `MaxPelvisRaise`로 올림 한도도 둔다 | 45 / 60 |
| `MaxFootAngle` | 발을 지면 기울기에 맞추는 최대 각도 | 30° / 0° |
| `MaxBodyPitch` / `MaxBodyRoll` | 사족 몸통 기울기 한도 | 사족만 35° / 15° |
| `PelvisAverageWeight` | 사족 몸 높이. 0이면 가장 낮은 요구값, 1이면 평균 | 사족만 0.5 |
| `InterpSpeed` | 보정 보간 속도 | 10 |

| 증상 | 원인·조건 | 할 일 |
|---|---|---|
| 경사 위쪽 발이 파묻힌다 | 발이 몸 중심에서 멀어 지면이 `TraceUp`보다 높다 | `TraceUp`·`MaxFootOffset`을 늘린다 |
| 사족 몸이 낮게 내려앉는다 / 발이 뜬다 | 발을 크게 벌린 자세에서 몸 높이와 접지가 충돌한다 | `PelvisAverageWeight`를 올리거나(몸 높임) 내린다(발 붙임) |
| 리그 동작을 화면으로 보고 싶다 | 사족 리그에 디버그 선이 있다 | 리그의 `bDebugDraw`를 켜고 PIE 콘솔에 `ControlRig.EnableDrawInterfaceInGame 1`, `a.AnimNode.ControlRig.Debug 1`을 입력한다 |

다운·구르기처럼 다리를 지면에 맞추면 안 되는 애니메이션을 끄는 `DisableFootIK` 커브는 리그가 읽도록 만들어 두었지만, 리그 계층에 커브 요소가 없어 아직 동작하지 않는다.

## 확인 상태와 근거

- [KataAnimInstance.h](../../Plugins/KataFramework/Source/KataFramework/Public/Animation/KataAnimInstance.h): 이동 값, 태그 매핑, 프리뷰 레이어.
- [KataAnimLayerInstance.h](../../Plugins/KataFramework/Source/KataFramework/Public/Animation/KataAnimLayerInstance.h): `GetMainAnimInstance`.
- [KataAnimLayerSetup.h](../../Plugins/KataFramework/Source/KataFramework/Public/Animation/KataAnimLayerSetup.h): 레이어 선택, 스켈레톤 검사, 데이터 검증.
- [KataEquipmentComponent.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Equipment/KataEquipmentComponent.cpp): `RefreshAnimLayers`, `OnAnimInitialized` 바인딩.
- 사용자 확인은 문서 머리의 확인 상태를 따른다. [작업 상태](https://github.com/jaykop/Kata/issues/32).
