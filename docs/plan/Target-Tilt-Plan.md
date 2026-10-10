# 대상 방향 Tilt 계획

작성: 2026-10-10  
갱신: 2026-10-10  
연결 이슈: [#14 대상 방향 Tilt 보정](https://github.com/jaykop/Kata/issues/14)  
현재 상태 근거: [#14](https://github.com/jaykop/Kata/issues/14), [대상 방향 Tilt 사용법](../manual/Target-Tilt.md), [대상 방향 Tilt 구현 기록](../devlog/2026-10-10-Target-Tilt.md), [타게팅 사용법](../manual/Targeting.md), [애니메이션 레이어 사용법](../manual/Animation-Layers.md), [오토 대시 기록](../devlog/2026-10-10-Auto-Dash.md)  
대체 관계: 없음. [기본 태스크 확장 계획](Base-Task-Plan.md)의 Face Target(회전 보정) 항목과는 별개다. 몸 전체의 Yaw 회전은 기존 회전 태스크가 맡는다

## 목적과 현재 상태

액션 중 캐릭터의 상체를 대상 쪽으로 기울여 공격 궤적이 대상에 닿게 한다.
원래 애니메이션 위에 "애니메이션이 가정한 조준 방향"과 "실제 대상 방향"의 Pitch 차이만 더한다.
PC와 AI가 같은 경로를 쓰고, 경사면·락온 부위·체격 차이를 반영한다.

- 이미 있는 것
  - `UKataAnimInstance`(KataFramework)는 게임 스레드에서 컴포넌트 값을 스냅샷으로 복사하고, Thread Safe 업데이트에서 파생값을 계산한다.
  - 메인 Template ABP는 스켈레톤 없이 상태 머신만 갖는다. 캐릭터(스켈레톤)마다 자식 ABP가 스켈레톤을 지정한다([애니메이션 레이어 사용법](../manual/Animation-Layers.md)).
  - 대상은 Resolve Target Command가 Context Target에 넣는다. PC는 `ResolveApproachLocation`을 재정의해, 락온 지점의 소유 액터가 액션 대상이면 그 지점을 돌려준다.
    `UKataAITargetingComponent`는 위치·방향 함수를 재정의하지 않으므로 기반 구현(대상 위치)을 쓴다.
  - 몸 전체의 Yaw 회전은 Resolve Facing Command와 `UKataTask_RotateToFacing`이 맡는다.
  - 태스크가 컴포넌트에 요청을 등록하고 핸들로 해제하는 구조는 `UKataRootMotionCurveComponent::BeginDistanceCorrection`·`EndDistanceCorrection`이 이미 쓰고 있다.
- 없는 것
  - 조준 높이를 정하는 타게팅 API.
  - 실행 중 계산한 각도를 포즈에 더하는 경로(컴포넌트 → Anim Instance → AnimGraph 노드).
  - Kata 플러그인의 AnimGraph 노드와, 편집기 노드 클래스를 둘 UncookedOnly 모듈.

### 진단 요점

- 엔진 `FAnimNode_LookAt`은 본 하나의 축을 목표점에 정렬한다. 원래 휘두르는 자세를 덮어쓰고 척추 여러 마디에 각도를 나누지 못하므로 이 용도에 맞지 않는다.
  Animation Warping 플러그인의 노드는 이동 방향·보폭 보정용이다.
- Template ABP에는 스켈레톤이 없고, 샘플의 DS3·Sekiro 추출 스켈레톤은 본 이름이 서로 다르다. 노드에 본을 직접 지정할 수 없으므로 체인은 스켈레톤별 데이터에서 이름으로 받아야 한다.
- `ACharacter`는 메시 Tick이 CharacterMovement 뒤에 오도록 선행 관계를 건다. `UKataActionComponent`도 `TG_PrePhysics`라 Kata Tick과 메시 애니메이션 갱신의 순서는 정해져 있지 않다.
  선행 관계를 걸지 않으면 Tilt 값이 한 프레임 늦게 반영될 수 있다.
- `FAnimInstanceProxy`는 액터와 컴포넌트의 Transform을 캐시한다(`GetActorTransform`, `GetComponentTransform`). 노드는 워커 스레드에서도 액터 기준 축을 컴포넌트 공간으로 바꿀 수 있다.
- Hit Trace는 메시 소켓 위치로 판정하므로 Tilt가 적용된 포즈를 따라간다.

## 범위

- 포함: 조준 위치 API, Tilt 요청·블렌드 컴포넌트, Anim Instance 값과 스켈레톤별 설정, Tilt AnimGraph 노드와 UncookedOnly 모듈, 액션 타임라인 태스크, 관련 manual·devlog.
- 제외
  - 몸 전체의 Yaw 회전. 기존 Resolve Facing·Rotate To Facing이 맡는다.
  - 상체 Yaw 비틀기.
  - 이동·대기 중 상시 LookAt(머리·목, Yaw·Pitch). [#51](https://github.com/jaykop/Kata/issues/51)에서 다루고 이 계획은 아래 "LookAt과의 경계"만 정한다.
  - 히트 판정([#6](https://github.com/jaykop/Kata/issues/6)), 발 IK와 Body 레이어 예제.

## 처리 구조

| 단계 | 위치 | 하는 일 |
|---|---|---|
| 1. 조준점 | `UKataTargetingComponent::ResolveAimLocation`(KataTargeting) | 액션 대상의 조준 위치를 돌려준다. PC는 그 대상에 락온 중이면 락온 지점이다 |
| 2. 각도 계산 | `UKataTask_TargetTilt` 인스턴스(KataFramework) | 구간 동안 목표 Pitch를 구해 컴포넌트에 넘긴다. 추적 시간이 지나면 마지막 값을 유지한다 |
| 3. 병합·블렌드 | `UKataTiltComponent`(KataFramework) | 요청을 쌓아 가장 최근 요청을 따른다. 각속도 제한, 블렌드 인·아웃, 요청 해제 뒤 페이드를 맡는다 |
| 4. 스냅샷 | `UKataAnimInstance` | 게임 스레드에서 `TiltPitch`·`TiltAlpha`를 복사한다 |
| 5. 포즈 적용 | `FAnimNode_KataTilt`(KataFramework 런타임), 편집기 노드(새 UncookedOnly 모듈) | Slot 뒤에서 척추 체인을 액터의 오른쪽 축으로 나눠 회전한다 |

### 각도 계산

내 발 높이를 F(캡슐 바닥, 캡슐이 없으면 액터 위치), 대상까지의 수평 거리를 d라 한다.

- 원점 O: 내 수평 위치, 높이 F + Aim Origin Height.
- 가정 조준점 E: 대상의 수평 위치, 높이 F + Expected Target Height. 애니메이션이 가정한 키의 대상이 나와 같은 지면에 섰을 때의 조준점이다.
- 실제 조준점 A
  - `ResolveAimLocation`이 부위 지점(락온 지점)을 돌려주면 그 위치.
  - 아니면 대상 캡슐의 세로 구간(바닥~꼭대기)에서 "대상 발 높이 + Expected Target Height"에 가장 가까운 점. 캡슐이 없으면 대상 위치.
- d는 실제 수평 거리와 Min Horizontal Distance 중 큰 값이다.
- 목표 Pitch = atan2(A.z − O.z, d) − atan2(E.z − O.z, d). 위쪽이 양수이며 −Max Pitch Down~Max Pitch Up으로 제한한다.
- 대상이 몸 정면에서 수평 90도 넘게 벗어나 있으면 목표 Pitch는 0이다. Pitch는 앞뒤를 구분하지 않으므로 등 뒤 대상 쪽으로 기울이면 반대 방향으로 보인다.

| 상황 | 결과 |
|---|---|
| 평지, 같은 체격 | A와 E가 같아 0이다 |
| 경사면 | 대상 발 높이 차이만큼 기운다 |
| PC 락온 부위 | 부위 지점 쪽으로 각도 제한까지 기운다 |
| 락온하지 않은 거대한 대상 | 기준 키 높이(다리 쪽)를 겨누므로 평지에서 0이다 |
| 작은 대상 | 대상 꼭대기를 겨누므로 아래로 기운다 |
| 거대 AI → PC | Expected Target Height를 사람 키로 두면 평지에서 0, PC가 높은 곳에 있으면 위로 기운다 |

### 블렌드와 요청 수명

- 태스크는 시작할 때 요청을 등록하고, 어떤 사유로 끝나든 해제한다. 해제 뒤에도 컴포넌트가 마지막 Pitch를 유지한 채 그 요청의 Blend Out Time 동안 Alpha를 0으로 내린다.
  콤보 전이로 액션이 취소돼도 기울기가 한 프레임에 풀리지 않는다.
- 요청이 겹치면 가장 최근 요청을 따른다. 그 요청이 해제되면 남은 요청 중 가장 최근 것을 따른다.
- 출력 Pitch는 Max Pitch Speed로 목표에 다가간다. Alpha가 0인 상태에서 새 요청이 오면 Pitch를 목표로 바로 맞춰, 이전 값에서 쓸고 지나가지 않게 한다.
- 컴포넌트는 요청이 있거나 Alpha가 0보다 클 때만 Tick한다. Tick 선행 관계는 Kata 액션 컴포넌트 → Tilt 컴포넌트 → 메시 순서로 건다. 구체적인 연결 위치는 구현 때 확인한다.

### 포즈 적용

- 노드는 Template ABP의 Slot 뒤에 하나 둔다. Pitch 핀은 `TiltPitch`, Alpha 핀은 `TiltAlpha`에 바인딩한다.
- 체인은 노드 초기화 때 소유 인스턴스(`UKataAnimInstance`)의 `TiltBoneChain`(본 이름과 가중치 목록)을 읽어 이름으로 해석한다. Class Defaults 값이라 실행 중에는 바뀌지 않는다.
- 회전 축은 액터의 오른쪽 축을 컴포넌트 공간으로 바꾼 것이다. 메시의 회전 오프셋과 무관하다.
- 루트 쪽 본부터 차례로, 정규화한 가중치만큼 각 본의 위치를 중심으로 돌린다. 자식은 부모를 따라가므로 체인 끝 본은 Pitch 전체만큼 돈다.
- 체인은 골반 위 첫 척추 본부터 팔이 붙은 본까지로 잡는다. 골반을 넣으면 다리가 같이 돌고, 팔이 붙은 본까지 가지 않으면 무기가 Pitch 일부만 받는다. 이 규칙은 manual에 적는다.
- 체인이 비었거나 본을 찾지 못하면 입력 포즈를 그대로 내보낸다.

## LookAt과의 경계

상시 LookAt은 연출용 기능으로 Yaw·Pitch를 모두 다루며 [#51](https://github.com/jaykop/Kata/issues/51)에서 설계한다. 이 계획은 다음을 LookAt과 공유할 수 있게 만든다.

- `ResolveAimLocation`: LookAt도 바라볼 위치로 쓴다.
- 체인 회전: 축과 각도를 받아 체인에 나눠 돌리는 계산을 노드 밖 공용 함수로 둔다. LookAt 노드가 Yaw 축으로 재사용한다.
- 값 전달: 컴포넌트 → Anim Instance 스냅샷 → 노드 핀 구조. 조준 포즈가 있는 캐릭터는 같은 값을 AimOffset에 넣을 수 있다.
- 그래프 순서는 Slot → Tilt → LookAt으로 한다. 액션 중 LookAt을 어떻게 억제할지는 #51에서 정한다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| D1 Tilt 정의 | 확정 | 원래 동작 위에 가정 조준 방향과 실제 대상 방향의 Pitch 차이만 더한다(2026-10-10 사용자) |
| D2 적용 대상 | 확정 | PC·AI 공통. PC 락온, 경사면, AI와 PC의 상대 위치를 반영한다(2026-10-10 사용자) |
| D3 회전 축 | 확정 | Pitch만 다룬다. 몸 Yaw는 기존 회전 태스크가 맡는다(2026-10-10 사용자) |
| D4 조준 위치 API | 확정 | 기반 `UKataTargetingComponent`에 `ResolveAimLocation`을 새로 둔다(2026-10-10 사용자). 시그니처: `bool ResolveAimLocation(AActor* ActionTarget, FVector& OutLocation, bool& bOutIsTargetPoint) const`(BlueprintNativeEvent). 지금 돌려주는 값은 `ResolveApproachLocation`과 같지만, 거리 기준과 조준 높이는 의미가 달라 나눈다 |
| D5 상시 LookAt | 확정 | 별도 이슈 [#51](https://github.com/jaykop/Kata/issues/51)로 나눈다(2026-10-10 사용자). 이 계획은 경계만 정한다 |
| D6 포즈 적용 | 확정 | C++ AnimNode를 Template ABP의 Slot 뒤에 하나 둔다(2026-10-10 사용자) |
| D7 체인 위치 | 확정 | 자식 ABP Class Defaults(2026-10-10 사용자) |
| D8 각도 계산과 실제 조준점 | 확정 | 위 "각도 계산". 락온하지 않은 대상은 캡슐 세로 구간에서 기준 키에 가장 가까운 점을 겨눈다. 대안인 대상 중심 조준은 거대한 대상 앞에서 크게 위로 기운다 (2026-10-10 사용자, 제안대로) |
| D9 원점 높이·기준 키 위치 | 확정 | 체인과 같은 자식 ABP Class Defaults(`AimOriginHeight`, `ExpectedTargetHeight`). 0 이하면 자기 캡슐 절반 높이를 쓴다. 애니메이션 세트에 묶인 값이고 캐릭터 행이 이미 Anim Class로 ABP를 고르므로 행 데이터 에셋을 따로 두지 않는다. 액션 편집기 프리뷰도 같은 ABP를 쓴다. 기준 키는 태스크에서 덮어쓸 수 있다. 대안: 캐릭터 행 데이터 에셋 (2026-10-10 사용자, 제안대로) |
| D10 추적과 고정 | 확정 | Track Duration 동안만 목표를 다시 구하고 이후 고정한다. AI가 휘두르는 중에도 따라오면 PC의 회피가 성립하지 않는다 (2026-10-10 사용자, 제안대로) |
| D11 중단 처리와 겹침 | 확정 | 위 "블렌드와 요청 수명" (2026-10-10 사용자, 제안대로) |
| D12 모듈 | 확정 | KataFramework에 `AnimGraphRuntime`을 Public 의존으로 더한다(노드 헤더가 `FAnimNode_SkeletalControlBase`를 노출). 편집기 노드는 새 UncookedOnly 모듈(`KataFrameworkAnimGraph`, 의존 `AnimGraph`·`BlueprintGraph`)에 둔다. Editor 모듈은 쿠킹하지 않은 `-game` 실행에서 로드되지 않아 ABP의 노드 클래스를 찾지 못할 수 있다. 엔진 Animation Warping 플러그인도 런타임 모듈과 UncookedOnly 모듈로 나눈다 (2026-10-10 사용자, 제안대로) |
| D13 이름 | 확정 | 태스크 `UKataTask_TargetTilt`("Kata Task: Target Tilt", `Animation` 단계·묶음). 컴포넌트 `UKataTiltComponent`(`AKataCharacter` 기본 서브오브젝트, 실행 상태만 갖는다). 노드 `FAnimNode_KataTilt`("Kata Tilt") (2026-10-10 사용자, 제안대로) |
| D14 대상이 없을 때 | 확정 | Context Target이 없거나 자신이면 요청하지 않고 끝난다. 타게팅 컴포넌트가 없으면 대상 액터 위치로 계산한다 (2026-10-10 사용자, 제안대로) |
| D15 기본값 | 확정 | 아래 두 표 (2026-10-10 사용자, 제안대로). 실행 확인 뒤 조정한다 |
| D16 캡슐이 몸보다 큰 대상의 조준점 | 확정 | 구현 뒤 StarvedHound(사람 크기 캡슐) 위로 휘두르는 것을 보고 검토했다. 락온 없이 겨누기용 Target Point를 쓰는 안은 소프트 타겟이 활성화되어 있으면 의미가 없다는 이유로 제외했다. 캡슐을 몸 크기에 맞추는 안, 작은 대상은 중심을 겨누는 안도 채택하지 않고 D8 규칙을 유지한다(2026-10-10 사용자) |
| D17 디버그 | 확정 | `Kata.Tilt.Debug`(조준 원점·가정·실제 조준점과 각도 표시), `Kata.Tilt.ForcePitch`(대상과 무관하게 각도 강제)를 KataFramework에 `ENABLE_DRAW_DEBUG` 조건으로 둔다. 적용 여부를 구분하기 어렵다는 사용자 피드백으로 추가했다(2026-10-10 사용자) |
| D18 샘플 적용 범위 | 확정 | BlackKnight만 설정한다. SilverKnight는 필요하면 같은 체인(`Spine`·`Spine1`)을 쓰고, StarvedHound는 Tilt 대상이 아니다(2026-10-10 사용자) |

### 태스크 설정

| 항목 | 의미 | 기본값 |
|---|---|---|
| Duration | Tilt를 유지할 구간. Single Frame과 0은 설정 오류 | 0.5초 |
| Blend In Time | 구간 시작부터 Alpha가 1이 되기까지 | 0.1초 |
| Blend Out Time | 구간이 끝나거나 취소된 뒤 Alpha가 0이 되기까지 | 0.2초 |
| Track Until End | 켜면 구간 끝까지 목표를 다시 구한다 | 켬 |
| Track Duration | Track Until End를 끄면 구간 시작부터 이 시간 동안만 다시 구하고 이후 고정한다. 0이면 시작 때 한 번만 구한다 | 0.2초 |
| Max Pitch Speed | 출력 Pitch의 초당 최대 변화. 0이면 제한하지 않는다 | 360°/s |
| Max Pitch Up / Max Pitch Down | 목표 Pitch의 한도 | 30° / 30° |
| Min Horizontal Distance | 각도 계산에 쓰는 최소 수평 거리 | 50cm |
| Override Expected Target Height | 켜면 이 액션에서만 기준 키를 덮어쓴다(내려찍기 등) | 끔 |

### 자식 ABP 설정(`Kata|Tilt`)

| 항목 | 의미 | 기본값 |
|---|---|---|
| Tilt Bone Chain | 본 이름과 가중치 목록. 골반 위 첫 척추 본부터 팔이 붙은 본까지 | 비어 있음(Tilt 없음) |
| Aim Origin Height | 발에서 조준 원점까지의 높이 | 0(캡슐 절반 높이) |
| Expected Target Height | 애니메이션이 가정한 대상의 조준 높이(대상 발 기준) | 0(자기 캡슐 절반 높이) |

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| T1 | 높음 | `ResolveAimLocation` 기반 구현과 PC 재정의 | D4 | 기반은 대상 위치, PC는 그 대상의 락온 지점을 부위 지점으로 돌려준다 |
| T2 | 높음 | `UKataTiltComponent` 요청 API(등록·갱신·해제 핸들), 병합·블렌드·페이드, `AKataCharacter` 기본 서브오브젝트와 Tick 선행 관계 | D11·D13 | 요청이 없으면 Tick하지 않고 출력 Alpha가 0이다. 해제 뒤 Blend Out Time 안에 0이 된다 |
| T3 | 높음 | `UKataAnimInstance`의 Tilt 값 스냅샷과 설정(체인·원점 높이·기준 키), 데이터 검증 | D7·D9 | ABP에서 `Tilt Pitch`·`Tilt Alpha`를 읽을 수 있다. 가중치가 음수이거나 합이 0이면 검증 오류 |
| T4 | 높음 | `FAnimNode_KataTilt`, 체인 회전 공용 함수, UncookedOnly 모듈과 편집기 노드, Build.cs·uplugin | D6·D12 | Template ABP에 노드를 놓고 컴파일할 수 있다. Alpha가 0이면 입력 포즈와 같다 |
| T5 | 높음 | `UKataTask_TargetTilt` 정의·인스턴스 | T1~T3, D8·D10·D14·D15 | 시작에 등록하고, 추적 뒤 고정하며, 완료·취소·중단·소유자 파괴에 해제한다. Single Frame과 Duration 0은 설정 오류 |
| T6 | 보통 | 샘플 설정: Template ABP 노드 배치, 샘플 캐릭터 자식 ABP의 체인·높이, 공격 액션의 태스크 | T4·T5 | PIE와 액션 편집기 프리뷰에서 기울기가 보인다 |
| T7 | 보통 | manual·devlog, AGENTS.md 모듈 목록, 문서 목록 | T5 | 문서가 현재 동작과 맞다 |

## 영향과 제한

- 모듈: 태스크·컴포넌트·노드는 KataFramework, 조준 위치 API는 KataTargeting에 둔다. KataFramework → KataTargeting 방향이라 의존 규칙에 맞고 코어 Kata는 바뀌지 않는다.
  새 엔진 플러그인 의존은 없다. 엔진 모듈 `AnimGraphRuntime`(런타임)과 `AnimGraph`·`BlueprintGraph`(UncookedOnly 모듈)가 늘어난다. 새 모듈은 AGENTS.md의 모듈 목록에 적는다.
- 공개 API: `UKataTargetingComponent`에 가상 함수 하나, `UKataAnimInstance`에 읽기 전용 값과 설정이 늘어난다. 기존 호출은 바뀌지 않는다.
- 실행 상태: 태스크 정의에는 설정만 둔다. 요청(목표 Pitch, 블렌드 시간)은 컴포넌트가 숫자로만 갖고 UObject를 참조하지 않는다. 태스크 인스턴스는 컴포넌트와 대상을 약한 참조로 둔다.
- 수명: 컴포넌트 `OnUnregister`에서 요청을 비운다. 대상이 파괴되면 마지막 Pitch를 유지하고 구간이 끝날 때 블렌드 아웃한다.
- 에셋: `AKataCharacter`에 기본 서브오브젝트가 하나 늘어난다. 기존 BP는 새 컴포넌트를 받고, 직렬화된 값과 충돌하지 않는다. Template ABP에 노드를 추가해야 효과가 보인다.
- 스레드: 노드는 핀 값과 Proxy의 Transform만 읽는다. 체인은 초기화 때 Class Defaults에서 복사하며, 이 시점의 스레드 안전성은 구현 때 확인한다.
- 프리뷰: 액션 편집기 프리뷰의 Self가 `AKataCharacter`이고 Context에 대상이 있으면 동작한다. 대상을 위아래로 옮겨 조정할 수 있다. 시간 탐색은 실제 실행을 고정 간격으로 진행하므로 블렌드도 재생과 같다.
- Hit Trace: 기울어진 포즈로 판정하므로 경사면의 대상에도 공격이 닿는다. 판정이 직전에 평가된 포즈를 쓰는 지금 순서는 바꾸지 않는다.
- 정확도: 방향 차이를 맞추는 보정이다. 각 척추 본을 중심으로 돌리므로 어깨 위치도 조금 움직이며, 무기 끝이 대상 지점을 정확히 지나도록 위치를 맞추지는 않는다.

## 사용자 확인 항목

- 구현 완료: T1~T5 코드와 T7 문서. T6 샘플 에셋 설정은 사용자와 함께 한다.
- 실행 확인(사용자)
  - PC: 평지의 같은 체격(기울지 않음), 경사면 위·아래 대상, 락온 부위(머리·다리), 락온하지 않은 거대한 대상, 작은 대상.
  - AI: 계단·경사면 위의 PC, 거대 AI의 기준 키 설정.
  - 공통: 추적 고정 뒤 회피, 콤보 전이·취소 때 블렌드 아웃, 등 뒤 대상, 아주 가까운 대상, 액션 편집기 프리뷰와 시간 탐색.
- 미확인으로 남을 범위: 네발·비인간형 스켈레톤의 체인, 재생 속도 변경, 히트스톱(시계 정책 미정).

## 완료 시 갱신할 문서

- [연결 이슈](https://github.com/jaykop/Kata/issues/14): 구현·확인 상태.
- [대상 방향 Tilt 사용법](../manual/Target-Tilt.md): 노드 배치, 체인 규칙, 태스크 설정. [애니메이션 레이어 사용법](../manual/Animation-Layers.md)에서 링크한다.
- [타게팅 사용법](../manual/Targeting.md): `ResolveAimLocation` 행.
- devlog: 결정과 실제 결과. 이 plan은 이슈를 닫을 때 삭제한다.
- [AGENTS.md](../../AGENTS.md): KataFramework 모듈 목록.
- [문서 목록](../README.md): 이 plan 링크 추가, 종료 시 제거.
