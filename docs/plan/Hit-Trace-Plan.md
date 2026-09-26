# Hit Trace 계획

작성: 2026-09-24  
갱신: 2026-09-26  
연결 이슈: [#6 Hit Trace 태스크](https://github.com/jaykop/Kata/issues/6)  
현재 상태 근거: [현재 구현 상태](../devlog/Implementation-Status.md) · [기본 태스크 확장 계획](Base-Task-Plan.md#hit-trace의-설계-요점) · [타게팅 계획](Targeting-Plan.md)  
대체 관계: [기본 태스크 확장 계획](Base-Task-Plan.md)의 "Hit Trace의 설계 요점"을 이 문서가 구체화한다. 두 문서가 충돌하면 이 문서를 따른다.

## 목적과 현재 상태

액션 타임라인의 한 구간 동안 공격 판정을 수행하는 Hit Trace 태스크를 추가한다.
현재 구현된 태스크는 Play Montage, Transition Window, Send Gameplay Event, Apply Gameplay Effect, Apply Loose Tag이며
히트 판정 코드는 없다.

해결할 문제는 다음과 같다.

- 판정 영역을 어느 메시의 어느 소켓 기준으로 그릴지 정하는 방법이 없다.
- 프레임 간격이 넓거나 애니메이션이 빠르면 두 프레임 사이를 지나간 대상을 놓친다.
- 판정 결과를 GE·이벤트 같은 처리로 넘기는 경로가 없다. 태스크 간 값 전달 경로도 아직 없다([Base-Task-Plan 1.2](Base-Task-Plan.md#12-태스크-출력-채널)).
- `UKataActionComponent`는 `TG_PrePhysics`에서 Tick하므로 태스크 Tick에서 소켓을 읽으면 이번 프레임의 포즈가 아닐 수 있다.
  `EKataTaskUpdateHook::AfterMeshPose`는 정의만 있고 거절된다.

## 범위

- 포함
  - `UKataTask_HitTrace` 태스크.
  - HitBox 정의 에셋 `UKataHitBoxPreset`. SocketTrace·ShapeSweep 두 방식을 지원한다.
  - 기준 메시 제공 컴포넌트 `UKataHitBoxComponent`.
  - 서브스텝 보간을 이용한 저프레임·빠른 동작 보정.
  - SocketTrace 면 판정: 소켓 목록의 이전·현재 위치로 만든 삼각형 띠와 대상 도형의 교차 판정(HT-9).
  - 애니메이션 원본 재샘플링과 차이값 보간을 이용한 프레임 사이 포즈 보정(HT-10). 면 판정 다음 단계다.
  - Filter 전용 `UTargetingPreset`을 이용한 대상 필터.
  - 피격 영역 `UKataHurtBoxComponent`(Sphere·Capsule·Box, 속성 태그)와 HurtBox 전용 판정(HT-11). 판정 대상은 HurtBox뿐이다.
  - HurtBox 콜리전 프로필을 정하는 프로젝트 설정 `UKataHitTraceSettings`.
  - 히트 수집·처리 Subsystem `UKataHitSubsystem`과 처리기 `UKataHitHandler`, 기본 처리기 두 종(Gameplay Event 전송, GE 적용).
  - 태스크당 대상 1회 판정.
  - 태스크 시작·종료 시점 판정과 짧은 구간 보정.
  - 프리뷰 월드에서의 판정과 처리기 실행.
  - 판정 영역 디버그 시각화. Shipping 빌드에서 제외한다.
- 제외 (2차 이후)
  - 베이크 궤적 보정(에디터에서 몽타주를 샘플링해 궤적을 에셋에 저장). HT-10이 같은 문제를 런타임 재샘플링으로 다루므로 계획하지 않는다.
  - Tilt(LookAt IK 등 대상 쪽으로 몸을 기울이는 애니메이션 보정). 히트 판정과 별개이므로 별도 이슈로 다룬다.
  - 태스크 출력 채널. Base-Task-Plan 1.2에서 따로 설계한다.
  - HitScan `Ray` 모드.
  - 다단히트 `ReHitInterval`, 태스크 간 히트 목록 공유 `HitGroup`.
  - 피격자 쪽 처리기(가드·패리), 히트스톱.
  - 장비 시스템. 무기 메시 등록은 `UKataHitBoxComponent`의 함수 호출로만 제공한다.
  - 투사체. 히트스캔과 투사체의 HurtBox 연결 방향은 아래 "HurtBox"의 이후 확장에 적는다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| 플러그인 위치 | 확정 | 2026-09-24 사용자 결정. `KataFramework`. 태스크가 코어에 있을 필요는 없다 |
| 판정 방식 | 확정 | 2026-09-24 사용자 결정. SocketTrace(BaseSocket·TipSocket, 샘플 수, 두께)와 ShapeSweep(Capsule·Sphere·Box, 소켓 하나와 상대 오프셋)을 모두 구현한다. SocketTrace의 소켓 구성과 판정 방법은 2026-09-25에 아래 두 항목으로 바뀌었다 |
| SocketTrace 소켓 구성 | 확정 | 2026-09-25 사용자 결정. BaseSocket·TipSocket·SampleCount 대신 소켓 목록(`Sockets`, 2개 이상)을 받는다. 곡선 칼날은 소켓을 더 두고, 직선 칼날은 2개면 된다 |
| SocketTrace 판정 방법 | 확정 | 2026-09-25 사용자 요구와 승인. 1차 구현은 소켓마다 이전→현재 경로를 선으로 추적해 경로 사이에 빈틈이 있었다. 이전·현재 프레임의 소켓 점을 이어 만든 삼각형 띠(면)로 판정해야 한다. 판정 상대는 궁극적으로 HurtBox로 명명한 PrimitiveComponent다(사용자 방향). 방법은 아래 "SocketTrace 면 판정"(삼각형-도형 직접 교차)으로 확정했다 |
| 프레임 사이 포즈 보정 | 확정 | 2026-09-25 사용자 결정. Stellar Blade 발표(Unreal Fest)의 방식처럼 애니메이션 원본을 중간 시각에서 재샘플링하고, 실제 포즈와 원본 포즈의 차이를 양 끝에서 구해 보간해 적용한다. 면 판정 다음 단계로 #6에 포함한다. 세부 설계는 아래 "프레임 사이 포즈 보정"(2026-09-25 확정) |
| 기준 메시 | 확정 | 2026-09-24 사용자 결정. "기준 메시"는 공격 HitBox의 소켓을 읽는 메시다. HurtBox가 아니다. 컴포넌트 태그로 찾지 않고 부착 위치(주 손·보조 손)를 태그로 만들지 않는다 |
| 기준 메시 제공자 | 확정 | 2026-09-24 사용자 결정. 인터페이스가 아니라 `UKataHitBoxComponent`가 Character·Weapon 기준 메시를 보관한다. 상속 없이 어떤 액터에도 붙일 수 있고, 무기는 `SetWeaponMesh()`로 등록하므로 이후 장비 시스템과 연결하기 쉽다 |
| HitBox 정의 위치 | 확정 | 2026-09-24 사용자 결정. 프리셋 에셋 `UKataHitBoxPreset`에만 둔다. 태스크 인라인 정의는 두지 않는다. 같은 무기를 쓰는 여러 공격이 정의를 공유한다 |
| 보정 범위 | 확정 | 2026-09-24 사용자 결정. 1차는 서브스텝 보간만. 베이크 궤적은 2차. 2026-09-25에 베이크 대신 "프레임 사이 포즈 보정"(HT-10)을 #6 안에서 하기로 바뀌었다 |
| 결과 처리 구조 | 확정 | 2026-09-24 사용자 결정. 태스크는 판정만 하고 히트 기록을 `UKataHitSubsystem`에 제출한다. Subsystem이 같은 프레임 끝에 제출 순서대로 처리한다 |
| 처리기 보유자 | 확정 | 2026-09-24 사용자 결정. Hit Trace 태스크가 Instanced `UKataHitHandler` 배열을 가진다. 피격자 쪽 처리기는 2차 |
| 대상 필터 | 확정 | 2026-09-24 사용자 결정. Filter 태스크만 담은 `UTargetingPreset`을 선택적으로 지정한다. KataTargeting이 팩션 필터 태스크를 제공하면 그것을 재사용한다 |
| HitScan·다단히트·HitGroup·출력 채널 | 확정 | 2026-09-24 사용자 결정. 2차로 미룬다 |
| 데미지 | 확정 | 태스크와 프리셋은 수치를 갖지 않는다. 수치는 GE와 Ability가 가진다([Base-Task-Plan](Base-Task-Plan.md#hit-trace의-설계-요점)) |
| 시작·종료 시점 판정 | 확정 | 2026-09-25 사용자 결정. 태스크 시작과 종료 시점에도 판정한다. 프레임보다 짧은 구간 때문에 판정을 놓치지 않도록 종료 시점에 판정 영역을 보정한다. 방법은 아래 "구간 경계와 짧은 구간 보정" |
| 프리뷰 동작 | 확정 | 2026-09-25 사용자 결정. 프리뷰에서도 판정과 처리기를 실행한다. `UKataHitSubsystem`은 `DoesSupportWorldType`을 재정의해 EditorPreview 월드를 지원한다(엔진 기본값은 Game·Editor·PIE만) |
| 디버그 시각화 | 확정 | 2026-09-25 사용자 결정. 판정 영역을 디버그 라인으로 확인하는 코드를 둔다. Shipping에서는 제외한다 |
| 스윕 실행 시점 | 확정 | 2026-09-25 사용자 승인. 태스크 Tick이 아니라 `UKataHitSubsystem`(`UTickableWorldSubsystem`)의 Tick에서 소켓을 읽고 스윕한다. UE 5.8 `LevelTick.cpp`에서 `FTickableGameObject::TickObjects`가 `TG_PostPhysics` 뒤, `TG_PostUpdateWork` 앞에 실행됨을 확인했다. `UKataActionComponent`(`TG_PrePhysics`)보다 늦으므로 `AfterMeshPose` 없이 이번 프레임 포즈를 읽는다. 병렬 애니메이션 평가가 이 시점에 끝나 있는지는 실행 확인 항목이다 |
| 종료 사유별 처리 | 확정 | 2026-09-25 사용자 결정. 정상 완료(`Completed`)만 종료 시점 판정을 한다. 취소·중단·소유자 파괴 때는 마지막 판정 없이 등록을 해제한다. 캔슬된 공격이 마지막에 맞히는 일을 막기 위해서다 |
| 이전 포즈 캐시 | 확정 | 2026-09-25 사용자 결정. 판정 구간 첫 프레임의 이전 포즈는 `UKataHitBoxComponent`가 캐시한다. 태스크 인스턴스는 `Ts`에 시작되므로 그 전 프레임을 기록할 수 없고, 컴포넌트는 액터 수명 동안 기록하므로 액션 0초에 시작하는 콤보 판정도 보정된다. 방법은 아래 "구간 경계와 짧은 구간 보정" |
| 디버그 기능 위치 | 확정 | 2026-09-25 사용자 결정. 디버그 기능은 구현한 모듈에 둔다([AGENTS.md 모듈 경계](../../AGENTS.md#모듈-경계) 개정). 게임 CVar는 KataFramework, 프리뷰 토글은 KataFramework의 Editor 모듈에 둔다 |
| 프리뷰 디버그 토글 | 확정 | 2026-09-25 사용자 결정. 프리뷰 에디터에서도 토글로 디버그를 켜고 끈다 |
| 시작 시점 오버랩 | 확정 | 2026-09-25 사용자 승인. 태스크에 `bCheckOnStart`(기본 true)를 둔다. 켜면 시작 시점 포즈에서 판정 영역 안에 이미 있는 대상도 맞힌다 |
| 디버그 켜기 방식 | 확정 | 2026-09-25 사용자 승인. 디버그 모드는 월드별 값이다(`UKataHitSubsystem::SetDebugDrawMode`, 0 끔, 1 판정 영역, 2 서브스텝 궤적과 히트 지점 포함). 게임 월드는 CVar `Kata.HitTrace.Debug`를 따르고, 프리뷰 월드는 뷰포트 토글을 따른다. 둘은 서로 영향을 주지 않는다 |
| 프리뷰 토글 연결 | 확정 | 2026-09-25 사용자 승인. 새 Editor 모듈 `KataFrameworkEditor`가 Kata 프리뷰 뷰포트 툴바(`UToolMenus`로 등록된 이름 있는 메뉴)를 확장해 "Hit Trace Debug" 항목을 넣는다. 코어는 KataFramework를 모른다. 메뉴 이름 상수가 지금은 `SKataPreviewViewport.cpp` 안에 있으므로 `KataEditor` Public 헤더로 옮기는 코어 변경이 필요하다. 토글이 프리뷰 월드를 찾을 수 있도록 메뉴 컨텍스트에서 뷰포트의 월드를 얻는 경로도 확인해야 한다 |
| 서브스텝 상한 | 확정 | 2026-09-25 사용자 승인. `MaxSubsteps` 기본값 32, `MaxStepDistance` 20cm 유지. 20fps 로그에서 칼끝이 한 프레임에 약 290cm 움직인 공격이 10fps까지 상한에 걸리지 않는다 |
| 컴포넌트가 없을 때 | 확정 | 2026-09-25 사용자 승인. Character 기준은 `ACharacter::GetMesh()`로 대체한다. Weapon 기준은 경고 로그를 남기고 판정하지 않는다 |
| Trace Channel | 확정 | 2026-09-25 사용자 승인. 플러그인은 프리셋에 채널 선택만 노출한다. 전용 채널(예: `KataHit`) 정의는 샘플 프로젝트 설정이 소유한다. 2026-09-26에 아래 HurtBox 항목들로 바뀌었다(채널 정의를 프로젝트가 소유하는 원칙은 유지) |
| 판정 대상 | 확정 | 2026-09-26 사용자 결정. Physics Asset 바디 대신 HurtBox를 먼저 구현한다. Hit Trace는 `UKataHurtBoxComponent`만 맞히며 Physics Asset·일반 콜리전 판정 경로는 없앤다 |
| HurtBox 도형 | 확정 | 2026-09-26 사용자 결정. `UPrimitiveComponent`를 상속한 컴포넌트 하나가 Sphere·Capsule·Box를 지원한다. 엔진 `UShapeComponent`의 BodySetup 생성 도우미가 모듈 밖에 공개되지 않아 BodySetup을 직접 만든다 |
| HurtBox 태그 | 확정 | 2026-09-26 사용자 결정. HurtBox는 속성(약점, 판정 제외 등)을 나타내는 `FGameplayTagContainer`를 가진다. 프리셋의 `HurtBoxTagQuery`로 맞힐 HurtBox를 거르고, 이 검사는 대상 1회 규칙보다 먼저 한다. 태그 정의와 의미는 프로젝트가 정한다. `SendGameplayEvent` 처리기는 태그를 `TargetTags`로 넘긴다 |
| HurtBox 조회 방식 | 확정 | 2026-09-26 사용자 승인. HurtBox는 전용 Object Channel(샘플: `KataHurtBox`)로 분류하고 Hit Trace는 Object Type 질의로 찾는다. HurtBox는 모든 채널을 무시한다. 판정이 "무엇에 반응하는가"가 아니라 "HurtBox인가"를 묻기 때문이다 |
| HurtBox 설정 위치 | 확정 | 2026-09-26 사용자 결정. KataFramework의 `UKataHitTraceSettings`(DeveloperSettings, `DefaultGame.ini`)의 `HurtBoxCollisionProfile` 하나가 HurtBox 기본 콜리전과 판정 Object Type을 함께 정한다. 프리셋마다 Object Type을 두지 않는다. 코어 `Kata`에 공용 설정을 두면 의존 방향을 어기므로 기능 플러그인에 둔다(선례 `UKataFactionSettings`) |

## 설계 개요

### 데이터 (공유 에셋, 실행 상태 없음)

- `UKataHitBoxPreset` (DataAsset)
  - `Mode`: `SocketTrace` / `ShapeSweep`.
  - SocketTrace: `Sockets`(2개 이상, 칼날을 따라 놓인 순서), `Thickness`(판정 면과 대상 도형 사이 허용 거리).
  - ShapeSweep: `Socket`, `RelativeTransform`, `Shape`(Sphere 반지름 / Capsule 반지름·반높이 / Box 크기).
  - `HurtBoxTagQuery`(맞힐 HurtBox의 태그 조건, 비면 모두).
  - 서브스텝: `MaxStepDistance`, `MaxStepAngle`, `MaxSubsteps`.
- `UKataTask_HitTrace`
  - `HitBoxPreset`, `MeshSource`(`Character` / `Weapon`), `FilterPreset`(선택, `UTargetingPreset`), `HitHandlers`(Instanced 배열).
  - 데이터 검증: 프리셋 누락, FilterPreset에 Selection·Sort 태스크 포함, 처리기 누락을 진단한다.
- `UKataHitHandler` (EditInlineNew, 추상)
  - `HandleHit(const FKataHitRecord&)`를 BlueprintNativeEvent로 둔다. 공유 에셋의 서브오브젝트이므로 상태를 저장하지 않는다.
  - 기본 구현: `UKataHitHandler_SendGameplayEvent`(대상 또는 공격자에게 이벤트 태그와 히트 정보 전송), `UKataHitHandler_ApplyGameplayEffect`(공격자 ASC로 대상에게 GE 적용).

### 실행 상태 (캐릭터별)

- `UKataHitBoxComponent`: Character·Weapon 기준 메시(`UMeshComponent`)를 약참조로 보관한다. Character 기본값은 소유 `ACharacter`의 Mesh다.
  매 프레임 포즈 확정 뒤 기준 메시의 월드 트랜스폼을 기록해 판정 첫 프레임의 이전 포즈를 제공한다.
- `UKataTaskInstance_HitTrace`: Subsystem 등록 핸들만 가진다. 종료·취소·중단·소유자 파괴 시 등록을 닫는다.
  히트 목록(`TSet<TWeakObjectPtr<AActor>>`)과 직전 포즈는 Subsystem의 등록 항목(`FKataActiveHitBox`)이 가진다. 정상 완료 뒤 마지막 판정은 태스크 인스턴스가 끝난 다음에 일어나기 때문이다. 등록 항목은 실행별 상태이므로 공유 에셋에 상태를 두지 않는 원칙은 그대로다.
- `UKataHitSubsystem` (`UTickableWorldSubsystem`)
  - 활성 HitBox 목록: 기준 메시, 프리셋, 직전 샘플의 액션 시각과 소켓 트랜스폼, 태스크 구간 `[Ts, Te]`, 종료 중 표시, 태스크 인스턴스 약참조.
  - 프레임 처리 순서: 활성 HitBox마다 서브스텝 스윕 → 자기 자신·부착 액터·이미 맞은 대상 제외 → FilterPreset 즉시 실행 → `FKataHitRecord` 큐 제출 → 큐를 제출 순서대로 처리기에 전달 → 큐 비움.
  - 같은 HitBox의 히트는 서브스텝 순서, 같은 서브스텝 안에서는 `HitResult.Time` 순으로 제출한다.
- `FKataHitRecord`: 공격자·대상 약참조, `FHitResult`, 출처 태스크, 제출 순번. 큐는 프레임마다 비우므로 강참조를 오래 쥐지 않는다.

### HurtBox (HT-11)

- `UKataHurtBoxComponent`: 캐릭터 메시의 본·소켓에 붙이는 피격 영역. `Shape`(Sphere·Capsule·Box)와 크기, `HurtBoxTags`를 가진다.
  - 도형 하나만 담은 Transient BodySetup을 컴포넌트마다 만든다. 스케일 규칙은 엔진 `FK*Elem::GetFinalScaled`와 같아 물리 질의와 직접 교차 계산이 같은 크기를 본다.
  - `GetBodySetup`은 BodySetup만 채우고 물리 상태를 다시 만들지 않는다. 엔진이 물리 상태 생성 도중에 호출하기 때문이다(아래 알려진 함정).
  - 기본 콜리전은 `UKataHitTraceSettings::HurtBoxCollisionProfile`이고, 프로필이 없으면 질의 전용·모든 채널 무시·WorldDynamic이다.
- 판정: 넓은 단계·시작 오버랩·ShapeSweep 모두 설정 프로필의 Object Type으로 질의한다. 결과 가운데 HurtBox이면서 태그 조건을 통과한 것만 후보가 된다.
  Object Type Multi 질의는 결과가 모두 Touch라 ShapeSweep이 Block에서 멈추지 않는다(UE 5.8 `CollisionQueryFilterCallback.cpp`). 그래서 재추적 루프가 필요 없다.
- 결과: `HitResult.Component`는 HurtBox, `BoneName`은 HurtBox의 부착 소켓(또는 본)이다.
- 샘플 설정: `DefaultEngine.ini`에 Object Channel `KataHurtBox`(GameTraceChannel1, 기본 Ignore)와 프로필 `KataHurtBox`(QueryOnly, 엔진 채널 모두 Ignore), `DefaultGame.ini`에 `HurtBoxCollisionProfile=KataHurtBox`.
- 알려진 함정: 1차 구현은 `GetBodySetup`에서 물리 상태를 다시 만들어, 바디가 이중으로 초기화되고 소유 컴포넌트가 사라진 바디가 씬에 남았다. 질의 결과의 컴포넌트가 None으로 나오며 정상적으로 붙어 있는 HurtBox는 맞지 않았다(2026-09-26 수정).
- 이후 확장(2026-09-26 사용자와 정리, 구현 전):
  - 히트스캔: 벽·유리처럼 물체마다 다른 반응이 필요하므로 전용 Trace Channel(예: `KataWeapon`)을 두고, HurtBox 프로필에 그 채널 Block을 추가한다. 근접 판정 코드는 바뀌지 않는다.
  - 이동 투사체: 전용 Object Type(예: `KataProjectile`)을 두고 HurtBox 프로필에 그 타입 Overlap을 추가한다.
  - 근접 판정과 히트스캔이 한 Trace Channel을 공유하면 총알을 막는 벽이 근접 후보에 섞이므로 공유하지 않는다.

### 서브스텝 보간

- 단계 수 = `ceil(max(끝점 이동거리 / MaxStepDistance, 회전각 / MaxStepAngle))`. 1 이상, `MaxSubsteps` 이하.
- 위치는 Lerp, 회전은 Slerp로 보간한다. 두 프레임 사이의 호 궤적은 서브스텝 직선들로 근사된다.
- SocketTrace(1차 구현, 교체 예정): 칼날 위 샘플 점마다 이전 위치에서 현재 위치로 Line Trace 또는 Sphere Sweep을 한다. 점 경로 사이의 면은 검사하지 않아 빈틈이 생긴다. "SocketTrace 면 판정"으로 바꾼다.
- ShapeSweep: 엔진 `SweepMulti`는 회전 하나만 받아 스윕 중 도형이 회전하지 않는다. 그래서 각 서브스텝 구간을 해당 구간의 보간 회전으로 스윕한다.

### SocketTrace 면 판정

- 삼각형 띠: 서브스텝 한 칸의 시작 포즈 소켓 점 `A0..An`과 끝 포즈 소켓 점 `B0..Bn`에서 이웃한 두 소켓마다
  `(Ai, Ai+1, Bi+1)`, `(Ai, Bi+1, Bi)` 두 삼각형을 만든다. 칸이 여러 개면 띠가 이어져 칼날이 쓸고 간 면 전체를 덮는다.
- 후보 찾기(넓은 단계): 이번 Tick 삼각형 전체의 AABB(두께만큼 부풀림)로 엔진 `OverlapMultiByChannel`을 한 번 호출해 채널에 반응하는 컴포넌트를 모은다.
- 정밀 판정(좁은 단계): 후보에서 판정 도형을 꺼내 삼각형과 직접 교차 계산한다.
  - 도형 출처: HurtBox의 도형(HT-11). 1차 구현의 Physics Asset 바디·Shape 컴포넌트·BodySetup 집합 도형 경로는 2026-09-26에 없앴다.
  - 계산: 삼각형-구는 최근접점 거리, 삼각형-캡슐은 선분-삼각형 거리, 삼각형-상자는 분리축 판정. 두께는 도형을 두께만큼 부풀려 반영한다.
    Convex 요소는 감싸는 상자로 근사한다.
  - 결과: 부위(`BoneName`), 접촉점, 컴포넌트를 `FHitResult`에 채운다. 순서는 서브스텝 칸, 소켓 구간 순이다.
- HurtBox와의 관계: HT-11에서 후보 수집을 HurtBox Object Type 질의로 바꾸고 좁은 단계 계산은 그대로 썼다.
  엔진 스윕에 묶인 근사(얇은 상자 스윕)는 HurtBox 도입 때 다시 써야 하므로 택하지 않는다.
- 결정 경과: 2026-09-25 사용자는 HurtBox 교차가 최종 목표라고 밝혔고, 삼각형-도형 직접 교차 제안을 승인했다.

### 프레임 사이 포즈 보정 (HT-10)

2026-09-25 사용자 결정: 샘플 대상은 기준 메시의 활성 몽타주, 캐릭터 메시에 직접 붙은 무기까지 보정, 프리셋 옵션 `bSampleAnimation`(기본 켜짐),
디버그에서 재샘플링한 칸과 선형으로 대체한 칸을 색으로 구분한다.

- 샘플 원본: 소켓을 읽는 스켈레탈 메시(무기면 무기가 붙은 캐릭터 스켈레탈 메시)의 AnimInstance 활성 몽타주.
  시간 기준은 Kata 시각이 아니라 몽타주 재생 위치다. 직전·현재 프레임의 재생 위치를 기록하고 그 사이를 나눈다. 재생 속도가 저절로 반영된다.
- 원본 포즈 `R(p)`: 재생 위치 `p`의 몽타주 슬롯 트랙 구간에서 시퀀스와 시퀀스 안의 위치를 찾고, 소켓 부모 본부터 루트까지 각 본의 로컬 트랜스폼을
  `UAnimSequence::GetBoneTransform`으로 추출해 컴포넌트 공간으로 합성한다. 패키징 빌드에서도 동작하도록 Raw가 아니라 압축 데이터를 쓴다.
  소켓과 부모 본 사이의 고정 오프셋(무기의 부착 위치 포함)은 현재 프레임의 실제 트랜스폼에서 구한다.
- 차이값 보정: 양 끝에서 `D = R(p)⁻¹ · 실제(컴포넌트 공간)`를 구하고 중간 시각에는 `R(p) · Lerp(D0, D1)`을 쓴다. 블렌드·IK·애디티브의 영향이 양 끝에서 맞춰진다.
  컴포넌트 월드 트랜스폼은 두 프레임 사이를 보간해 루트 모션 이동을 반영한다.
- 대체(선형 서브스텝): 활성 몽타주가 없거나 두 프레임의 몽타주가 다를 때, 재생 위치가 뒤로 가거나 예상 진행량과 크게 다를 때(섹션 이동·루프),
  시퀀스가 아니거나 애디티브이거나 스켈레톤이 다를 때, 직전 포즈 기록이 없을 때.
- 알려진 제한: 몽타주 슬롯이 여러 개면 첫 슬롯 트랙만 본다. 몽타주 외 애니메이션(스테이트머신·블렌드스페이스)과 섞인 비율은 양 끝 차이값으로만 반영된다.

### 구간 경계와 짧은 구간 보정

Kata 시계는 `TG_PrePhysics`에서 태스크를 시작·종료하고, 스윕은 같은 프레임의 Subsystem Tick에서 한다.
Subsystem은 HitBox마다 직전 샘플의 액션 시각 `T0`와 포즈, 이번 프레임의 액션 시각 `T1`과 포즈를 가진다.
태스크 구간은 `[Ts, Te]`다.

- 이전 포즈: `UKataHitBoxComponent`가 매 프레임 포즈 확정 뒤(Subsystem Tick) 등록된 기준 메시의 월드 트랜스폼을 기록한다.
  첫 프레임의 `T0` 소켓 포즈는 스켈레탈 메시면 `소켓 로컬 × 엔진의 직전 프레임 본 트랜스폼(GetPreviousComponentTransformsArray) × 기록한 메시 월드 트랜스폼`,
  스태틱 메시(무기)면 `소켓 로컬 × 기록한 메시 월드 트랜스폼`으로 계산한다. 두 번째 프레임부터는 Subsystem이 직전 스윕에서 읽은 포즈를 쓴다.
- 이전 포즈가 무효일 때: 스폰·텔레포트 직후, 무기 메시를 방금 등록한 직후, 엔진 직전 본 버퍼가 갱신되지 않은 경우에는 첫 구간 스윕을 건너뛰고 시작 시점 오버랩으로 대신한다.
  엔진 확인 결과(HT-1, UE 5.8 `SkinnedMeshComponent.cpp`): 직전 본 버퍼는 애니메이션 결과를 확정할 때(`FlipEditableSpaceBases`) 바뀌고, 그때마다 본 트랜스폼 리비전이 1 오른다.
  텔레포트처럼 모션 벡터를 지우면 2 오른다. 그래서 기록 시점보다 리비전이 정확히 1 늘었을 때만 유효로 본다. 갱신을 건너뛴 프레임, 한 엔진 프레임에 월드를 여러 번 진행한 경우(프리뷰 탐색), Leader Pose를 따르는 메시는 무효가 된다.
- 프레임 잘라내기: 이번 프레임의 판정 구간은 `[max(T0, Ts), min(T1, Te)]`이다. 구간 양 끝의 포즈는
  `alpha = (t - T0) / (T1 - T0)`로 두 샘플 사이를 보간해 구한다. 위치는 Lerp, 회전은 Slerp를 쓰며 서브스텝도 이 구간 안에서 나눈다.
- 시작 시점 판정: `bCheckOnStart`가 켜져 있으면 `Ts` 포즈에서 길이 0인 스윕(오버랩)을 한 번 한다.
- 종료 시점 판정: 정상 완료로 끝나면 태스크는 등록을 바로 지우지 않고 "종료 중"으로 표시한다. Subsystem은 그 프레임 Tick에서
  `Te`까지 잘라낸 마지막 스윕을 한 뒤 등록을 지운다. 애니메이션이 `Te`를 지나쳐도 판정 영역이 `Te` 포즈까지만 닿는다.
- 짧은 구간: `Ts`와 `Te`가 같은 프레임 안에 있으면 시작 오버랩 → `[Ts, Te]` 스윕 → 해제가 한 프레임에 모두 일어난다.
  구간이 아무리 짧아도 판정이 최소 한 번은 일어난다.
- 제한: `T0`와 `T1` 사이 포즈를 선형 보간하므로 한 프레임 안의 가감속과 호 궤적은 근사된다. 정확한 재현은 베이크 궤적(2차)이 필요하다.

### 프리뷰

- `UKataHitSubsystem`이 EditorPreview 월드에서도 생성되므로 판정과 처리기가 런타임과 같은 경로로 실행된다.
- 프리뷰 Target 더미가 맞으려면 `UKataHurtBoxComponent`가 붙어 있어야 한다. GE 처리기는 프리뷰 Target의 ASC에 적용된다.
- 프리뷰 월드가 Tick하는 동안에만 판정한다. 타임라인을 뒤로 스크럽할 때는 판정하지 않는다. 스크럽 중 동작은 실행 확인에서 정한다.
- 무기를 캐릭터 BP에 붙이고 `UKataHitBoxComponent`에 등록해 두면 프리뷰 액터에도 같은 구성이 나타나는지 확인이 필요하다.

### 디버그 시각화

- 코드 전체를 `#if ENABLE_DRAW_DEBUG`로 감싼다. Shipping 빌드에는 그리는 코드와 관련 데이터가 들어가지 않는다.
- 그리는 대상: 현재 판정 영역(SocketTrace 칼날 선·소켓 점, ShapeSweep 도형), Detailed에서는 서브스텝 궤적(SocketTrace는 쓸고 간 삼각형 면).
  궤적은 애니메이션을 재샘플링한 프레임이면 청록, 선형 보간으로 대신한 프레임이면 파랑으로 그린다(HT-10).
- 히트 표시: 맞은 도형, 판정에 쓰인 첫 교차 삼각형, 히트 지점과 법선을 빨간색으로 1초 남긴다. Area 단계에서도 그린다.
  그 대상과 교차한 나머지 삼각형은 주황색으로 그린다(2026-09-25 사용자 승인). 맞은 뒤 판정에서 빠진 대상도 구간이 끝날 때까지 교차를 계속 그리며,
  이 계산은 디버그를 켰을 때만 한다.
- 프리뷰 토글 선택은 에디터 사용자 설정에 저장해 에디터를 다시 켜도 유지한다(2026-09-25 사용자 요청).
- 시작·종료 시점 칼날은 따로 그리지 않는다(2026-09-25 사용자 결정).
- 판정 결과에 영향을 주지 않는다. 시각화를 켜고 꺼도 같은 대상이 맞는다.
- 켜기: 게임은 CVar `Kata.HitTrace.Debug`, 프리뷰는 뷰포트 툴바 토글. CVar 등록도 `ENABLE_DRAW_DEBUG` 안에 둔다.
- 에디터 토글 UI는 `KataFrameworkEditor`(Editor 전용 모듈)에 둔다. 런타임 모듈에는 에디터 코드를 넣지 않는다.

### 대상 필터

- 히트 후보를 `FTargetingDefaultResultsSet`에 먼저 넣고 FilterPreset의 태스크를 차례로 `Init`·`Execute`해 남은 대상만 제출한다.
  UE 5.8 엔진 소스에서 즉시 실행 경로가 결과 집합을 비우지 않고 태스크를 순서대로 실행함을 확인했다.
- `UTargetingSubsystem`은 GameInstance Subsystem이라 프리뷰 월드에는 없다. 그래서 `ExecuteTargetingRequestWithHandle` 대신 같은 일을 직접 한다. 요청 핸들과 데이터 저장소는 정적 함수로 만들고 해제하므로 게임과 프리뷰가 같은 경로를 쓴다.
  태스크가 `GetTargetingSubsystem`을 쓰는 Blueprint 필터라면 프리뷰에서는 null을 받는다.
- 비동기 실행은 쓰지 않는다. 히트가 난 프레임에만 요청을 하나 만든다.

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| HT-1 | 높음 | 엔진 직전 본 버퍼의 유효 조건 확인 | 없음 | 이전 포즈 무효 판정 조건이 엔진 근거와 함께 정해진다(위 "구간 경계와 짧은 구간 보정"에 기록) |
| HT-3 | 높음 | KataFramework 의존 추가(`TargetingSystem`), `UKataHitBoxComponent`, `UKataHitBoxPreset` | HT-1 | 캐릭터 BP에 컴포넌트를 붙이고 무기 메시를 등록할 수 있다. 프리셋 에셋을 만들 수 있다 |
| HT-4 | 높음 | `UKataHitSubsystem`(EditorPreview 월드 포함), `FKataHitRecord`, `UKataHitHandler`와 기본 처리기 두 종 | HT-3 | 제출된 기록이 같은 프레임에 순서대로 처리기로 전달된다 |
| HT-5 | 높음 | `UKataTask_HitTrace`와 태스크 인스턴스, 서브스텝 스윕, 시작·종료 시점 판정과 구간 잘라내기, 대상 1회, FilterPreset 필터 | HT-1, HT-4 | 두 방식 모두 구간 동안 대상을 한 번씩 찾아 처리기를 호출한다. 한 프레임보다 짧은 구간도 판정한다. 취소 시 마지막 판정 없이 등록이 해제된다 |
| HT-6 | 높음 | 디버그 시각화(`ENABLE_DRAW_DEBUG`), CVar, `KataFrameworkEditor` 모듈과 프리뷰 툴바 토글, 코어 메뉴 이름 공개 | HT-5 | 게임은 CVar로, 프리뷰는 툴바 토글로 판정 영역·서브스텝 궤적·히트 지점을 켜고 끌 수 있다. Shipping 빌드에 그리는 코드가 포함되지 않는다 |
| HT-7 | 보통 | 프리뷰 판정 확인과 보완 | HT-5 | 프리뷰 Target 더미가 맞고 처리기가 실행된다 |
| HT-8 | 보통 | 샘플 프로젝트 설정: `KataHurtBox` Object Channel·프로필, `HurtBoxCollisionProfile`, 샘플 캐릭터 HurtBox (2026-09-26 `KataHit` Trace Channel에서 변경) | HT-11 | 샘플 캐릭터로 실행 확인을 할 수 있다 |
| HT-9 | 높음 | SocketTrace 면 판정: 소켓 목록, 삼각형 띠, 후보 수집, 삼각형-구·캡슐·상자 교차, 디버그 표시(삼각형 면) | 없음 | 소켓 경로 사이를 지나가는 대상도 맞는다. 부위와 접촉점이 결과에 들어간다 |
| HT-10 | 높음 | 프레임 사이 포즈 보정: 애니메이션 원본 재샘플링과 차이값 보간 | HT-9 | 저프레임에서도 서브스텝 궤적이 실제 애니메이션의 호를 따른다 |
| HT-11 | 높음 | HurtBox: `UKataHurtBoxComponent`, HurtBox 전용 Object Type 판정, 태그 조건·전달, `UKataHitTraceSettings`, 진단 로그 | HT-9 | HurtBox만 맞고 `BoneName`에 부착 소켓이 들어간다. HurtBox가 없는 부위는 맞지 않는다 |

## 영향과 제한

- 모듈 경계: KataFramework가 엔진 `TargetingSystem` 플러그인(UE 5.8 Beta, 모듈 `TargetingSystem`)에 의존한다. Hit Trace 코드는 KataTargeting의 API를 쓰지 않으므로 KataTargeting 의존은 팩션 필터 태스크가 생겨 실제로 필요할 때 추가한다. 추가하더라도 통합 → 위성 방향이라 [플러그인 분리 계획](Plugin-Modularization-Plan.md)과 맞는다.
  KataFramework에 Editor 모듈 `KataFrameworkEditor`가 새로 생긴다(`KataFramework`, `KataEditor`, `ToolMenus`, `UnrealEd`에 의존).
  코어 변경은 `KataEditor`의 프리뷰 툴바 메뉴 이름을 Public 헤더로 공개하는 확장 지점 하나뿐이다.
- Beta 의존: `TargetingSystem` API가 바뀔 수 있다. 사용 범위를 Preset 즉시 실행과 결과 집합 조작으로 한정한다.
- 기존 에셋: 새 타입만 추가하므로 직렬화된 에셋과 Redirect 영향은 없다.
- 자원 수명: 태스크 인스턴스가 끝나면 Subsystem 등록을 해제한다. 소유자 파괴 뒤 남은 등록은 약참조 검사로 Subsystem이 정리한다. 큐의 기록은 해당 프레임에만 산다.
- 처리 지연: 판정과 처리 사이에 대상이 파괴되면 해당 기록을 건너뛴다. 태스크가 같은 프레임에 끝나도 처리기는 에셋 소유이므로 기록을 처리한다.
- 프리뷰 시각 차이: 프리뷰는 월드 Tick 뒤에 Kata를 진행하는 경로가 있어, Subsystem이 읽는 Kata 시각이 포즈보다 한 프레임 늦을 수 있다. 판정 구간 잘라내기가 한 프레임만큼 어긋날 수 있으며 실행 확인 항목이다.
- 알려진 제한: 서브스텝은 두 프레임 사이의 호를 직선으로 자른다. HT-10이 이 제한을 줄인다.
- 기존 에셋: HT-9에서 `BaseSocket`·`TipSocket`·`SampleCount`를 `Sockets`로 바꾸면 이미 만든 프리셋의 값이 사라진다. 테스트 에셋(`Content/KataTest`)만 있으므로 Redirect 없이 다시 설정한다.
- 대상 판정: HurtBox만 판정하고 `HitResult.BoneName`(부착 소켓)과 HurtBox 태그로 부위·속성을 구분한다(HT-11). Physics Asset이나 캐릭터 캡슐은 판정하지 않는다.
- 설정 반영 시점: HurtBox 기본 콜리전은 클래스 기본값을 만들 때 설정을 읽으므로, `HurtBoxCollisionProfile`을 바꾸면 에디터를 다시 켜야 새 기본값에 반영된다.
- 기존 에셋: HT-11에서 프리셋의 `TraceChannel`·`bTraceComplex`를 없앴다. 테스트 에셋만 있어 Redirect 없이 둔다.

## 사용자 확인 항목

- 구현 완료 조건: HT-3~HT-7, HT-9~HT-11의 코드가 작성된다.
- 실행 확인 조건(사용자)
  - 에디터·게임·Shipping 빌드가 성공한다.
  - SocketTrace·ShapeSweep이 샘플 캐릭터와 프리뷰에서 대상을 한 번씩 맞힌다.
  - 낮은 프레임(`t.MaxFPS 15` 등)에서도 빠른 공격과 한 프레임보다 짧은 구간이 대상을 놓치지 않는다.
  - 종료 시점 이후 애니메이션이 지나간 자리의 대상은 맞지 않는다.
  - 적대가 아닌 대상이 FilterPreset으로 걸러지고, 처리기(이벤트·GE)가 실행된다.
  - 액션 취소 뒤 판정이 남지 않는다.
  - 디버그 시각화를 게임에서는 CVar로, 프리뷰에서는 툴바 토글로 켜고 끌 수 있다.
- 이 목록만으로 에이전트가 빌드·테스트·별도 검사를 수행하지 않는다.

## 완료 시 갱신할 문서

- [현재 구현 상태](../devlog/Implementation-Status.md): KataFramework의 Hit Trace, 의존 추가, 남은 제한.
- 새 매뉴얼 `docs/manual/Hit-Trace.md`: 컴포넌트·HurtBox·프로젝트 설정·프리셋·태스크·처리기·필터 설정 방법.
- 새 devlog: 결과 처리 구조(Subsystem·Handler), 스윕 실행 시점, 구간 경계 보정, 기준 메시 제공 방식의 결정 이유.
- [기본 태스크 확장 계획](Base-Task-Plan.md): Hit Trace 요점을 이 문서 또는 devlog 링크로 정리.
- [문서 목록](../README.md): 이 plan과 새 manual·devlog 링크.
