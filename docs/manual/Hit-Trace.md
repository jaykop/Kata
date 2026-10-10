# Hit Trace 사용법

갱신: 2026-10-10  
대상: 액션 작성자, 캐릭터·무기 설정 담당 / KataFramework `HitTrace`, `UKataTask_HitTrace`  
적용 기준: UE 5.8, KataFramework(엔진 `TargetingSystem` 플러그인 의존), [Hit Trace 설계 결정](../devlog/2026-10-10-Hit-Trace.md)  
확인 상태: 소스 기준으로 작성했다. 사용자가 2026-10-10에 실행 확인 완료를 보고했다(개별 항목 기록 없음). 같은 날 추가한 `Impact Tag`와 이동 방향 기록은 소스 기준이며 실행 확인 전이다.

## 목적과 준비

액션 타임라인의 한 구간 동안 공격 판정을 하고, 맞은 대상마다 Gameplay Event 전송이나 Gameplay Effect 적용 같은 처리를 실행한다.
판정 상대는 피격 영역 `Kata Hurt Box` 컴포넌트뿐이다. 캐릭터 캡슐, Physics Asset, 일반 콜리전은 맞지 않는다.

준비할 것:

- 프로젝트에 HurtBox 전용 Object Channel과 콜리전 프로필이 있어야 한다. 샘플 프로젝트 설정은 아래 "프로젝트 설정"을 따른다.
- 공격하는 캐릭터에 `Kata Hit Box` 컴포넌트를 붙인다. 없으면 Character 기준은 `ACharacter`의 Mesh로 대신하지만, 직전 포즈가 없어 판정 첫 프레임 구간을 쓸지 못한다. Weapon 기준은 판정하지 않는다.
- 맞을 캐릭터의 메시 본·소켓에 `Kata Hurt Box` 컴포넌트를 붙인다.
- 피해를 주려면 공격자와 대상에 ASC가 있어야 한다. 피해 수치는 GE가 정한다([Attribute 사용법](Attributes.md)).

## 사용 순서

### 프로젝트 설정

1. 프로젝트 설정 Collision에 HurtBox용 Object Channel을 만든다. 기본 응답은 Ignore로 둔다.
2. 같은 Object Type을 쓰고 모든 채널을 무시하는 질의 전용(QueryOnly) 프로필을 만든다.
3. 프로젝트 설정 **Kata Hit Trace**의 `Hurt Box Collision Profile`에 그 프로필을 고른다. 값은 `DefaultGame.ini`에 저장된다.
4. 에디터를 다시 켠다. `Kata Hurt Box` 기본 콜리전은 클래스 기본값을 만들 때 설정을 읽는다.

샘플 프로젝트는 `DefaultEngine.ini`에 Object Channel `KataHurtBox`(GameTraceChannel1)와 프로필 `KataHurtBox`, `DefaultGame.ini`에 `HurtBoxCollisionProfile=(Name="KataHurtBox")`를 둔다.

### 피격 영역 배치

1. 맞을 캐릭터 Blueprint에 `Kata Hurt Box`를 추가하고 메시의 본이나 소켓에 붙인다. 머리·몸통처럼 부위마다 하나씩 둔다.
2. `Shape`(Sphere·Capsule·Box)와 크기를 정한다. 컴포넌트 스케일이 크기에 반영된다.
3. 약점이나 판정 제외 같은 속성이 필요하면 `Hurt Box Tags`에 `HurtBox.*` 태그를 넣는다. 태그의 의미는 프로젝트가 정한다.

### 공격 판정 영역(Hit Box Preset)

1. 콘텐츠 브라우저에서 Data Asset → **Kata Hit Box Preset**을 만든다. 같은 무기를 쓰는 공격들이 하나의 프리셋을 공유한다.
2. `Mode`를 고른다.
   - **SocketTrace**: 칼날처럼 긴 판정. `Sockets`에 칼날을 따라 놓인 소켓을 2개 이상 순서대로 넣는다. 직선 칼날은 손잡이 쪽과 끝 두 개면 되고, 휜 칼날은 휘는 지점마다 소켓을 더한다. 직전 프레임과 이번 프레임의 소켓 점을 이은 면이 판정 영역이다.
   - **ShapeSweep**: 주먹·발·둔기처럼 덩어리 판정. `Socket` 하나와 `Relative Transform`, `Shape`와 크기를 정한다.
3. 특정 HurtBox만 맞히려면 `Hurt Box Tag Query`를 쓴다(예: `HurtBox.Disabled` 태그가 없을 것).

### 액션에 태스크 추가

1. 액션 편집기 타임라인에서 **Kata Task: Hit Trace**(Combat 분류)를 추가하고 판정할 구간 길이로 맞춘다.
2. `Hit Box Preset`을 지정한다.
3. `Mesh Source`를 고른다. 맨몸 공격은 `Character`, 장착 무기는 `Weapon`이다. `Weapon`이면 `Weapon Slot`에 무기를 든 장비 슬롯을 지정한다([장비 사용법](Equipment.md)).
4. 아군을 거르려면 `Filter Preset`에 Filter 태스크만 담은 Targeting Preset을 지정한다. 팩션 판정은 KataTargeting의 **Kata Filter Faction**을 그대로 넣는다([팩션 사용법](Factions.md)).
5. `Hit Handlers`에 처리기를 추가한다.
   - **Apply Gameplay Effect**: 공격자 ASC로 대상에게 `Effect Class`를 적용한다. 피해 GE면 `Set By Caller Magnitudes`에 Kata Combat 설정의 Damage Set By Caller Tag로 공격 계수를 넣는다.
     공격 등급은 `Impact Tag`로 고른다. 스펙의 동적 Asset Tag로 들어가며, 비우면 피격 반응 쪽이 Kata Combat 설정의 Default Impact Tag를 쓴다.
   - **Send Gameplay Event**: `Recipient`(Target·Instigator)에게 `Event Tag` 이벤트를 보낸다. Payload의 TargetData에 HitResult, TargetTags에 맞은 HurtBox의 태그가 들어간다.

결과는 처리기의 효과(대상 Attribute 변화, 이벤트를 받은 Ability)와 아래 디버그 표시로 확인한다.

## 주요 설정과 실행 규칙

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| `Hit Box Preset` | 판정 영역 정의 | 비어 있으면 설정 오류다. 데이터 검증이 진단한다 |
| `Mesh Source` / `Weapon Slot` | 소켓을 읽을 메시 | `Weapon Slot`을 비우면 등록한 장비 메시가 하나일 때만 쓴다. 메시가 없으면 경고 후 이 판정을 건너뛴다 |
| `Check On Start` | 구간이 시작되는 순간 이미 판정 영역 안에 있는 대상도 맞힌다 | 기본 켜짐 |
| `Filter Preset` | Filter 태스크만 담은 Targeting Preset | 비우면 거르지 않는다. Selection·Sort 태스크가 섞이면 데이터 검증이 경고한다 |
| `Hit Handlers` | 맞은 대상마다 순서대로 호출하는 처리기 | 처리기는 공유 에셋의 일부라 실행 상태를 저장하지 않는다 |
| `Thickness` (SocketTrace) | 판정 면과 대상 도형 사이 허용 거리, cm | 기본 0 |
| `Max Step Distance` / `Max Step Angle` / `Max Substeps` | 한 프레임을 나누는 서브스텝 기준 | 기본 20cm / 15° / 32. 상한은 순간이동 같은 큰 이동의 비용을 막는다 |
| `Sample Animation` | 서브스텝 중간 포즈를 활성 몽타주 원본에서 다시 샘플링한다 | 기본 켜짐. 몽타주가 없거나 재생 위치가 끊기면 선형 보간으로 대신한다 |
| `UKataHitBoxComponent::RegisterEquipmentMesh` | 장비 슬롯에 Weapon 기준 메시 등록 | 장비 시스템을 쓰면 장착 컴포넌트가 자동으로 호출한다. 등록 직후 한 프레임은 첫 구간 대신 시작 시점 판정만 한다 |
| `UKataHitBoxComponent::SetCharacterMesh` | Character 기준 메시 지정 | nullptr이면 소유 `ACharacter`의 Mesh로 되돌린다 |
| `UKataHitHandler::HandleHit` | 사용자 정의 처리기 | Blueprint에서 구현하거나 C++에서 상속한다. `TargetActor`는 호출 시점에 유효하다 |

실행 규칙:

- 판정은 태스크 Tick이 아니라 `UKataHitSubsystem`이 같은 프레임의 포즈 확정 뒤에 한다. 찾은 히트는 같은 프레임 안에서 제출 순서대로 처리기에 전달된다.
- 태스크 구간 동안 같은 대상은 한 번만 맞는다. HurtBox 태그 조건은 이 규칙보다 먼저 검사한다.
- `HitResult.Component`는 맞은 HurtBox, `BoneName`은 그 HurtBox가 붙은 소켓(또는 본)이다. 시작 시점 겹침으로 찾은 히트는 `bStartPenetrating`이 true다.
- `TraceStart`→`TraceEnd`는 접촉이 일어난 서브스텝 동안의 이동이다. SocketTrace는 접촉한 칼날 지점, ShapeSweep은 판정 도형 중심의 이동이며 `TraceEnd`가 접촉 시점이다.
  구간 시작 순간의 판정(SocketTrace 시작 칼날, ShapeSweep 시작 겹침)은 이전 위치가 없어 두 값이 같다. 이 값은 피격 방향 선택에 쓴다([피격 반응 계획](../plan/Hit-Reaction-Plan.md)).
- 정상 완료로 끝나면 종료 시각까지 잘라낸 마지막 판정을 한다. 취소·중단·소유자 파괴로 끝나면 마지막 판정 없이 정리한다. 캔슬된 공격이 끝에서 맞히지 않게 하기 위해서다.
- 구간이 한 프레임보다 짧아도 시작 판정, 구간 판정, 해제가 한 프레임에 모두 일어나 최소 한 번은 판정한다.
- 판정과 처리 사이에 대상이 파괴되면 그 히트는 건너뛴다.

## 디버그 시각화

| 위치 | 켜는 방법 | 값 |
|---|---|---|
| 게임 월드 | 콘솔 변수 `Kata.HitTrace.Debug` | 0 끔, 1 판정 영역, 2 서브스텝 궤적·시작·종료 판정·히트 지점 포함 |
| 액션 편집기 프리뷰 | 뷰포트 툴바 **Hit Trace** 메뉴 | Off / Hit Area / Detailed. 선택은 에디터 사용자 설정에 저장된다 |
| 코드 | `UKataHitSubsystem::SetDebugDrawMode` | 월드별 값이며 CVar보다 우선한다 |

궤적은 파랑, 맞은 도형과 판정에 쓰인 첫 교차·히트 지점은 빨강, 같은 대상과 교차한 나머지 면이나 도형은 주황으로 1초 동안 그린다.
시각화를 켜고 꺼도 판정 결과는 같다. Shipping처럼 `ENABLE_DRAW_DEBUG`가 꺼진 빌드에는 그리는 코드가 들어가지 않는다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 아무것도 맞지 않는다 | 대상에 `Kata Hurt Box`가 없거나, HurtBox의 Object Type이 설정 프로필과 다르다 | HurtBox를 붙이고 콜리전 프리셋을 설정의 프로필로 둔다. 설정을 바꿨으면 에디터를 다시 켠다 |
| 특정 부위만 맞지 않는다 | 프리셋 `Hurt Box Tag Query`가 그 HurtBox 태그를 거른다, 또는 콜리전이 꺼져 있다 | 태그 조건과 HurtBox의 `Collision Enabled`를 확인한다 |
| Weapon 판정이 경고 후 건너뛰어진다 | 해당 슬롯에 등록한 장비 메시가 없거나, `Weapon Slot`이 비었는데 장비 메시가 둘 이상이다 | `Weapon Slot`을 지정하고 장착 상태를 확인한다 |
| 판정 첫 프레임 구간이 빠진다 | `Kata Hit Box`가 없거나, 스폰·텔레포트·메시 등록 직후라 직전 포즈가 무효다 | 컴포넌트를 붙인다. 무효인 프레임은 시작 시점 판정으로 대신한다 |
| 아군이 맞는다 | `Filter Preset`이 없거나 팩션 설정이 없다 | Kata Filter Faction을 담은 Preset을 지정하고 Kata Factions와 캐릭터 팩션을 설정한다 |
| 프리뷰에서 Target 더미가 맞지 않는다 | 더미에 HurtBox가 없다 | 프리뷰 Target 액터 클래스에 `Kata Hurt Box`를 붙인다. GE 처리기는 더미의 ASC에 적용된다 |
| 프리뷰에서 Blueprint 필터가 동작하지 않는다 | 프리뷰 월드에는 GameInstance Subsystem인 `UTargetingSubsystem`이 없다 | 필터 태스크가 `GetTargetingSubsystem`에 의존하지 않게 작성한다 |
| 프레임 사이 궤적이 원래 호와 다르다 | 서브스텝은 두 프레임 사이를 직선·보간으로 자른다. 재샘플링은 기준 메시의 활성 몽타주 첫 슬롯만 본다 | `Max Step Distance`·`Max Step Angle`을 줄이거나 `Sample Animation`을 켠다 |
| 타임라인을 뒤로 스크럽해도 판정하지 않는다 | 프리뷰 월드가 진행할 때만 판정한다 | 재생이나 앞으로 탐색으로 확인한다 |

다단히트(재판정 간격), 태스크 간 히트 목록 공유, HitScan, 투사체, 피격자 쪽 처리(가드·패리), 히트스톱은 아직 없다.
피격 반응과 히트스톱은 [#23](https://github.com/jaykop/Kata/issues/23)에서 다룬다.

## 확인 상태와 근거

이 문서는 2026-10-10 소스 기준으로 작성했다. 같은 날 사용자가 Hit Trace 실행 확인 완료를 보고했으며, 확인한 개별 항목은 전달되지 않았다.

- [KataTask_HitTrace.h](../../Plugins/KataFramework/Source/KataFramework/Public/Tasks/KataTask_HitTrace.h): `UKataTask_HitTrace`, `UKataTaskInstance_HitTrace`.
- [KataHitBoxPreset.h](../../Plugins/KataFramework/Source/KataFramework/Public/HitTrace/KataHitBoxPreset.h): `UKataHitBoxPreset`.
- [KataHitBoxComponent.h](../../Plugins/KataFramework/Source/KataFramework/Public/HitTrace/KataHitBoxComponent.h): `UKataHitBoxComponent`.
- [KataHurtBoxComponent.h](../../Plugins/KataFramework/Source/KataFramework/Public/HitTrace/KataHurtBoxComponent.h): `UKataHurtBoxComponent`.
- [KataHitHandler.h](../../Plugins/KataFramework/Source/KataFramework/Public/HitTrace/KataHitHandler.h): `UKataHitHandler`과 기본 처리기 두 종.
- [KataHitSubsystem.h](../../Plugins/KataFramework/Source/KataFramework/Public/HitTrace/KataHitSubsystem.h): 판정 실행 시점, 디버그 모드.
- [KataHitTraceSettings.h](../../Plugins/KataFramework/Source/KataFramework/Public/HitTrace/KataHitTraceSettings.h): `UKataHitTraceSettings`.
- [Hit Trace 설계 결정](../devlog/2026-10-10-Hit-Trace.md): 설계 결정과 이유, 진단 기록.
- [작업 상태](https://github.com/jaykop/Kata/issues/6).
