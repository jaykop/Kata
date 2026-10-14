# MainHUD 사용법

갱신: 2026-10-14  
대상: KataFramework 플레이어 HUD를 설정하거나 확장하는 사용자  
적용 기준: [#33 MainHUD 기반과 락온 마커](https://github.com/jaykop/Kata/issues/33), [#67 MVVM 기반](https://github.com/jaykop/Kata/issues/67), [#68 PC 상태 바](https://github.com/jaykop/Kata/issues/68), [#69 적 체력바](https://github.com/jaykop/Kata/issues/69)  
확인 상태: 2026-10-05 사용자 락온 마커 표시·전환·해제 확인. 상태 바는 2026-10-13 사용자 에디터 빌드 성공, 샘플 위젯 컴파일 성공, 사용자 PIE에서 부활 후 HP 반영·스태미나 소모·HP 지연 감소 확인. 적 체력바는 2026-10-14 사용자 에디터 빌드 성공, 샘플 위젯 컴파일 성공, 사용자 PIE 확인. 범위는 아래 "확인 상태와 근거"를 따른다

## 목적과 준비

`AKataPlayerController`가 로컬 플레이어마다 `UKataMainHUD`를 만들고 플레이어 화면에 추가한다.
현재 범위는 MainHUD 기반, 락온 마커, PC의 HP·스태미나·마나 바, 적 체력바다. 보스 UI는 포함하지 않는다.
폰의 `UKataPlayerTargetingComponent`가 알리는 지점 변경을 컨트롤러가 HUD와 카메라에 전달한다.
상태 바와 적 체력바는 UMG Viewmodel(엔진 `ModelViewViewModel` 플러그인, KataFramework가 의존)로 값과 위젯을 분리한다. 구조와 결정 이유는 [MainHUD MVVM 결정 기록](../devlog/2026-10-14-Main-HUD-MVVM.md)을 따른다.

## 사용 순서

1. Player Controller Class를 `AKataPlayerController` 파생으로 설정한다. Main HUD Class 기본값은 네이티브 `Kata Main HUD`다.
2. 폰에 PC용 타게팅 컴포넌트와 [락온 입력](Input.md#락온-입력), 대상에 [타겟 지점](Targeting.md)을 설정한다.
3. 락온을 획득하면 해당 지점에 흰 마름모가 표시되고, 전환하면 새 지점으로 옮겨지며 해제하면 사라진다.
4. 디자인을 바꾸려면 `UKataMainHUD` 파생 Widget Blueprint를 만들어 컨트롤러의 Main HUD Class로 지정한다.
   자체 마커를 만들 때는 Draw Default Lock Marker를 끄고 `GetLockMarkerPosition`의 좌표와 성공 여부를 사용한다.

별도 텍스처·Widget Blueprint 없이 기본 마커를 제공한다.

### 상태 바

1. 폰에 ASC와 `UKataAttributeSet_Base`를 둔다. 컨트롤러가 빙의 폰을 로컬 플레이어의 `UKataHUDSubsystem`에 전달하고, 서브시스템이 Health·Stamina·Mana와 각 최대값을 `UKataVM_PlayerStatus`에 채운다.
2. 게이지 Widget Blueprint는 `Kata Gauge Widget`을 부모로 만들고 이름이 `FillBar`인 Progress Bar를 둔다. 지연 바를 쓰려면 `DelayedBar` Progress Bar를 FillBar 뒤에 겹친다.
3. 상태 위젯에 Viewmodel `Kata Player Status Viewmodel`을 추가하고 Creation Type을 Resolver, Resolver를 `Kata HUD Viewmodel Resolver`로 지정한다.
   비율 함수(`GetHealthRatio`·`GetStaminaRatio`·`GetManaRatio`)를 각 게이지의 `SetPercent`에 One Way로 바인딩한다.
4. 상태 위젯을 `UKataMainHUD` 파생 Widget Blueprint에 배치하고 컨트롤러의 Main HUD Class로 지정한다.

샘플은 `Content/KataSample/UI`의 `WBP_KataGauge`·`WBP_PlayerStatus`·`WBP_MainHUD`를 쓰고 `BP_PlayerController`가 `WBP_MainHUD`를 지정한다.
스태미나처럼 줄어든 직후 계속 회복하는 자원은 지연 바가 이전 값에 머문 채 채움 바가 다시 올라와 어색하게 보이므로, 샘플은 스태미나 게이지의 Use Delayed Bar를 끈다.
Resolver로 받은 ViewModel은 로컬 플레이어마다 클래스별로 하나이며, HUD 위젯을 다시 만들어도 같은 인스턴스에 바인딩된다. 폰을 교체하면 이전 타게팅 구독을 해제하고 새 폰을 구독하며, 컨트롤러 종료 시 HUD를 화면에서 제거한다.

### 적 체력바

1. 대상에 ASC와 `UKataAttributeSet_Base`를 두고, 사망 뒤 자동으로 숨기려면 `UKataDeathComponent`도 둔다. 관찰 폰과 적대 관계가 되도록 팩션을 설정한다([팩션 사용법](Factions.md)).
   Base 세트가 피해로 Health를 실제로 줄이면 월드의 `UKataCombatEventSubsystem`이 알리고, HUD 서브시스템이 조건을 확인해 `UKataVM_EnemyHealthList`에 항목을 넣는다.
2. 항목 Widget Blueprint는 `Kata Enemy Health Bar Widget`을 부모로 만들고 게이지를 배치한다. Viewmodel `Kata Enemy Health Viewmodel`을 추가해 Creation Type을 **Manual**로 두고 `GetHealthRatio`를 게이지의 `SetPercent`에 바인딩한다.
3. 레이어 Widget Blueprint는 `Kata Enemy Health Bar Layer`를 부모로 만들고 이름이 `BarCanvas`인 Canvas Panel을 둔다. Entry Widget Class에 2의 항목 위젯을 지정한다.
4. 레이어를 MainHUD Widget Blueprint에 화면 전체를 덮도록(앵커 0,0~1,1, 오프셋 0) 배치한다. 레이어가 Tick마다 목록에 맞춰 항목 위젯을 꺼내고, 대상 위치를 화면에 투영한다.

샘플은 `WBP_EnemyHealthBar`(120x8 게이지)와 `WBP_EnemyHealthBarLayer`를 `WBP_MainHUD`의 PlayerStatus 뒤에 둔다.
표시 규칙은 프로젝트 설정 **Plugins > Kata HUD > Enemy Health Bar**에서 바꾼다.

| 상황 | 동작 |
|---|---|
| 피해 | 새 항목은 Damage Source 조건, 관찰 폰과 적대, Base 세트, Max Distance 조건을 만족하면 표시한다. 사망 컴포넌트가 있는 대상은 이미 사망했으면 새로 표시하지 않는다. Display Duration 동안 유지하고 다시 맞으면 시간을 갱신한다 |
| 락온 | 새 항목은 관찰 폰과 적대, Base 세트 조건을 만족하면 피해 여부와 거리 제한 없이 표시한다. 사망 컴포넌트가 있는 대상은 이미 사망했으면 새로 표시하지 않는다. 락온 중에는 표시 시간으로 만료시키지 않고, 해제하면 그때부터 Display Duration을 잰다 |
| 사망 | 대상의 UKataDeathComponent가 사망을 알리면 Dead Hide Delay 뒤 지운다. 락온 중이어도 적용한다. 사망 컴포넌트가 없으면 이 처리는 적용되지 않는다 |
| 개수 상한 | Max Enemy Bars를 넘으면 락온하지 않은 항목 중 가장 먼저 사라질 항목을 지운다. 모두 락온 중이면 새로 띄우지 않는다 |
| 화면 밖·카메라 뒤 | 항목 위젯을 숨기고 다시 들어오면 표시한다 |
| 월드 이동 | 표시 중인 바를 모두 지우고 새 월드의 피해 알림을 구독한다. 같은 월드 안의 재시작에서는 유지한다 |

표시 시간은 실시간으로 재므로 히트스톱·시간 배율의 영향을 받지 않는다.

## 주요 설정과 실행 계약

| 항목 | 의미 | 빈 값·실패 시 동작 |
|---|---|---|
| Main HUD Class | 컨트롤러가 만들 위젯 클래스 | 비우면 HUD를 만들지 않는다 |
| Draw Default Lock Marker | 기본 마름모 표시 | 끄면 기본 마커를 그리지 않는다. 별도 마커는 Widget Blueprint에서 구성한다 |
| Lock Marker Radius / Color | DPI 보정 위젯 공간에서 마름모 중심부터 꼭짓점까지의 거리와 색 | 기본 거리 10, 흰색 |
| Get Main HUD | 생성된 HUD 조회 | 생성 전이나 종료 후 null |
| Kata Player Status Viewmodel | Health·MaxHealth·Stamina·MaxStamina·Mana·MaxMana, 비율 함수, HasStatus | 비율은 최대값이 0 이하면 0. 폰에 ASC나 Base 세트가 없으면 값 0, HasStatus false |
| Kata HUD Viewmodel Resolver | 위젯을 소유한 로컬 플레이어의 HUD 서브시스템에서 요청 클래스의 ViewModel을 받는다 | 소유 로컬 플레이어가 없으면(디자이너 미리보기 등) 값을 받지 않는다. 위젯이 사라져도 ViewModel은 유지된다 |
| Gauge: Set Percent | 표시 비율. 0~1로 제한 | 처음 받은 값은 두 막대에 즉시 반영한다 |
| Gauge: Fill Color | FillBar 채움 색 | 기본 빨강. 같은 게이지를 여러 용도로 쓸 때 인스턴스마다 정한다 |
| Gauge: Use Delayed Bar | 값이 줄 때 DelayedBar를 늦게 줄인다 | 끄거나 DelayedBar가 없으면 즉시 함께 줄어든다. 회복값이 현재 DelayedBar 값 이상이면 즉시 맞추고, 그보다 낮으면 기존 대기·감소를 이어 간다 |
| Gauge: Delayed Bar Hold Time / Drain Rate | 감소 뒤 대기 시간(초), 초당 줄어드는 비율 | 기본 0.5초, 0.5. 대기 중 다시 줄면 대기를 다시 시작한다. 위젯 Tick 시간이라 시간 배율의 영향을 받지 않는다 |
| Kata HUD: Damage Source | 적 체력바를 띄울 피해의 출처. Player Only는 관찰 폰(또는 그 컨트롤러)이 가해자인 피해만, Any는 모든 피해 | 기본 Player Only. 락온 대상은 이 값과 관계없이 표시한다 |
| Kata HUD: Display Duration | 마지막 피해·락온 해제 뒤 유지 시간(초) | 기본 5 |
| Kata HUD: Max Enemy Bars | 동시에 표시할 최대 개수 | 기본 8, 최소 1. 상한은 없으며 바마다 위젯·투영·구독 비용이 든다 |
| Kata HUD: Max Distance | 관찰 폰에서 이 거리(cm)보다 먼 대상은 띄우지 않고 표시 중이면 지운다 | 기본 3000. 락온 대상은 제외 |
| Kata HUD: Dead Hide Delay | UKataDeathComponent의 사망 알림 뒤 지울 때까지 시간(초) | 기본 1 |
| Kata HUD: Bar Height Offset | 대상 루트 캡슐 꼭대기에서 월드 Z 방향으로 바를 띄우는 높이(cm). 루트가 캡슐이 아니면 콜리전이 활성인 컴포넌트의 바운드 꼭대기 기준 | 기본 30. 프로젝트 전체에 하나의 값이다 |
| Kata Enemy Health Viewmodel | Health·MaxHealth, `GetHealthRatio`, IsLockedOn, IsDead, `GetBarWorldLocation` | 대상이 사라지면 위치 조회가 false다 |
| Enemy Health Bar Layer: Entry Widget Class | 항목마다 만들 위젯 | 비우거나 BarCanvas가 없으면 그리지 않는다 |
| Get Lock Point | 현재 지점 조회 | 해제·파괴 후 null |
| Get Lock Marker Position | 뷰포트 상대 투영을 DPI 보정한 좌표 | 뒤쪽·화면 밖·비활성·미등록 지점은 false, 좌표는 0 |
| On Lock Point Changed | 지점이 바뀔 때 Blueprint 이벤트 | 이동만으로는 호출되지 않는다 |

기본 마커는 입력을 가로채지 않는다. 지점 참조는 약한 참조이며 표시 때문에 대상 수명을 연장하지 않는다.
마커는 장애물에 가려진 대상도 화면 안이면 표시하며 화면 가장자리 화살표는 제공하지 않는다.

## 제한과 문제 해결

| 증상 | 원인·조건 | 확인할 설정 |
|---|---|---|
| 마커가 없다 | 락온 실패, 지점이 화면 밖, 컨트롤러 또는 HUD 클래스가 다름 | 타게팅 디버거, Player Controller Class, Main HUD Class |
| SilverKnight가 선택되지 않는다 | 현재 샘플 BP에는 타겟 지점이 없다 | 대상 BP에 TargetPoint.LockOn 역할의 활성 지점 추가 |
| 상태 바가 움직이지 않는다 | 폰에 Base 세트가 없음, Viewmodel Creation Type이 Resolver가 아님, 바인딩 없음 | ASC 세트 구성, 상태 위젯의 Viewmodels·View Bindings 패널 |
| 사망 중에도 스태미나·마나 바가 찬다 | HUD는 시체 폰의 실제 Attribute를 계속 보여 준다. 회복 GE가 Dead 태그로 막혀 있지 않다 | 회복 GE의 Ongoing Tag Requirements |
| 적을 때려도 바가 뜨지 않는다 | 관찰 폰과 적대가 아님, 대상에 Base 세트 없음, Damage Source가 Player Only인데 가해자가 플레이어가 아님, 대미지 무시로 Health가 줄지 않음 | 팩션 관계, 대상 ASC 세트, Kata HUD 설정 |
| 바가 뜨지만 비어 있다 | 항목 위젯의 Viewmodel이 Manual이 아니거나 바인딩이 없다. Output Log에 `could not receive an enemy health viewmodel` | 항목 위젯의 Viewmodels·View Bindings 패널 |
| 큰 대상의 바가 머리에 겹친다 | 높이 오프셋이 프로젝트 전체에 하나뿐이다 | Bar Height Offset을 올린다. 캐릭터별 값은 아직 없다 |
| 마커는 있지만 카메라가 추적하지 않는다 | 다른 매니저를 사용하거나 카메라 배치가 없음 | [카메라 사용법](Camera.md#락온-카메라) |

## 확인 상태와 근거

2026-10-05 사용자가 기본 마커 표시·전환·해제를 확인했다. DPI·화면 밖 처리, 폰 교체와 대상 파괴는 별도 사용자 확인 결과가 없다. 이전 에디터 빌드 성공은 #33 구현 결과 댓글에 기록돼 있다.

2026-10-13 #68 작업 기록에 사용자 PIE에서 부활 후 HP 반영, 스태미나 소모와 HP 지연 감소를 확인했다고 남아 있다. 최대값 변경과 HP·마나 회복 GE를 통한 확인 결과는 없다. 이번 작업은 설명과 소스를 대조한 교정이며 빌드·PIE를 새로 실행하지 않았다.

2026-10-14 #69 변경 뒤 사용자가 Editor 빌드 성공과 PIE 확인을 보고했다. 대상은 피해 시 표시, 5초 뒤 사라짐, 락온 시 표시·유지, 해제 뒤 사라짐, 여러 적, 사망 뒤 숨김, 화면 밖·카메라 뒤 숨김, 거리 초과 제거, 기본 설정에서 적끼리의 피해 무시, Any 설정, 플레이어 재시작이다. 많은 수의 바를 띄운 상태의 성능은 측정하지 않았다.

- [HUD 기반](../../Plugins/KataFramework/Source/KataFramework/Public/UI/KataMainHUD.h).
- [적 체력바 레이어](../../Plugins/KataFramework/Source/KataFramework/Public/UI/KataEnemyHealthBarLayer.h), [적 ViewModel](../../Plugins/KataFramework/Source/KataFramework/Public/UI/KataVM_EnemyHealth.h), [Kata HUD 설정](../../Plugins/KataFramework/Source/KataFramework/Public/UI/KataHUDSettings.h), [피해 알림](../../Plugins/KataFramework/Source/KataFramework/Public/Attributes/KataCombatEventSubsystem.h).
- [HUD 서브시스템](../../Plugins/KataFramework/Source/KataFramework/Public/UI/KataHUDSubsystem.h), [상태 ViewModel](../../Plugins/KataFramework/Source/KataFramework/Public/UI/KataVM_PlayerStatus.h), [게이지 위젯](../../Plugins/KataFramework/Source/KataFramework/Public/UI/KataGaugeWidget.h).
- [컨트롤러 연결](../../Plugins/KataFramework/Source/KataFramework/Private/Player/KataPlayerController.cpp).
- [락온 카메라와 MainHUD 기록](../devlog/2026-10-04-Lock-On-Camera-HUD.md), [MainHUD MVVM 결정 기록](../devlog/2026-10-14-Main-HUD-MVVM.md).
