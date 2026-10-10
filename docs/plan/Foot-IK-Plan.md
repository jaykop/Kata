# 발 IK 계획

작성: 2026-10-10  
갱신: 2026-10-11  
연결 이슈: [#52 발 IK와 경사면 골반 보정 (Body 레이어)](https://github.com/jaykop/Kata/issues/52)  
현재 상태 근거: [#52](https://github.com/jaykop/Kata/issues/52), [애니메이션 레이어 사용법](../manual/Animation-Layers.md), [애니메이션 레이어 구조 결정](../devlog/2026-10-04-Anim-Layer-Structure.md), [장비·무기 계획](Equipment-Plan.md)의 Body 레이어 결정  
대체 관계: 없음. [대상 방향 Tilt 계획](Target-Tilt-Plan.md)이 범위에서 뺀 "발 IK와 Body 레이어 예제"를 이 계획이 다룬다

## 목적과 현재 상태

경사면과 계단에서 발이 지면 위에 뜨거나 지면에 파묻히지 않게 다리를 IK로 맞추고, 낮은 쪽 발이 지면에 닿도록 골반 높이를 내린다.
발 IK는 무기와 무관하고 스켈레톤에 따라 정해지므로 스켈레톤별 Body 레이어에 둔다. 처리는 Control Rig로 구성한다.

- 이미 있는 것
  - `UKataAnimLayerSetup::BodyLayer`를 `LinkLayers`가 메시에 링크한다. 무기 레이어를 바꿔도 Body 레이어는 유지된다.
    Anim Blueprint 편집기 프리뷰에서도 `UKataAnimInstance::PreviewAnimLayerSetup`으로 링크한다.
  - 레이어 ABP는 `UKataAnimLayerInstance::GetMainAnimInstance`로 메인 인스턴스의 C++ 값(`Is On Ground`, `Is Falling`, `Ground Speed` 등)을 읽는다.
  - 메인 Template ABP(`ABP_CharacterBase`)는 스켈레톤이 없고 무기 ALI(`ALI_KataWeapon`)만 구현한다. [#14](https://github.com/jaykop/Kata/issues/14)가 Slot 뒤에 Kata Tilt 노드를 둔다.
- 없는 것
  - Body ALI와 메인 ABP의 Body 레이어 호출 지점.
  - Body 레이어 예제와 발 IK Control Rig.

### 진단 요점

- 엔진 Control Rig(UE 5.8)
  - Control Rig 플러그인은 엔진 기본 활성이고 Experimental이 아니다. 샘플 프로젝트 설정을 바꾸지 않아도 쓸 수 있다.
  - 월드 충돌 노드(`Sphere Trace By Trace Channel`, `Line Trace By Trace Channel` 등)가 있다. 실행 중인 소유 액터와 컴포넌트를 자동으로 무시한다.
    다른 액터는 지정한 채널의 충돌 응답에 따라 맞는다.
  - `Two Bone IK`는 Control Rig 기본 노드다. Full Body IK는 별도 Full Body IK 플러그인(기본 비활성)에 있다.
  - Control Rig 노드는 본을 직접 고르지 않고 리그가 본 이름으로 동작한다. 리그의 공개 변수는 AnimGraph 노드의 입력 핀이 된다.
- 엔진 Animation Warping 플러그인의 Foot Placement 노드는 검토 후 채택하지 않았다. IK 발 본이 필수이고 골반 본 하나를 위아래로만 옮기므로 4족 몸통 정렬을 할 수 없다. 비교는 아래 "구현 수단 선택지"에 남긴다.
- 샘플 DS3 스켈레톤(BlackKnight, SilverKnight, LothricKnight, DS3Player, StarvedHound)
  - 최상위 본은 `Master`다. 그 아래 `Root`(DS3Player는 `RootPos`)에서 `Pelvis`(다리)와 `Spine`(팔·목)이 형제로 갈라진다.
    따라서 `Pelvis`만 내리면 상체가 따라 내려오지 않는다. BlackKnight 계층은 #14 작업에서 에디터로 확인했고, 나머지는 스켈레톤 에셋의 본 순서로 추정했다.
  - 엔진 샘플식 `ik_foot_*` 본은 없다. Control Rig는 FK 발 위치에서 목표를 계산하므로 IK 발 본이 필요 없다.
  - 2족 다리는 `L_Thigh` → `L_Calf` → `L_Foot` → `L_Toe0`이다. Twist 본은 곁가지라 IK 체인에 들어가지 않는다.
  - 사냥개 뒷다리는 `L_Thigh` → `L_Calf` → `L_Foot` → `L_Toe0`, 앞다리는 `Spine2` → `L_Clavicle` → `L_UpperArm` → `L_Forearm` → `L_Hand` → `L_Finger0`이다.
- 레이어 ABP는 메인 ABP의 Blueprint 변수를 읽지 못한다([애니메이션 레이어 사용법](../manual/Animation-Layers.md)의 제한). 레이어가 읽을 수 있는 값은 `UKataAnimInstance`의 C++ 값과 포즈의 커브뿐이다.
- 몽타주 루트 모션은 AnimGraph 평가 전에 추출된다. AnimGraph에서 골반 쪽 본을 옮겨도 이동량은 바뀌지 않는다. 샘플 애니메이션의 루트 모션이 `Master`에 실려 있는지는 구현 때 확인한다.

## 범위

- 포함
  - Body ALI와 메인 Template ABP의 호출 지점.
  - 2족 발 IK Control Rig와 BlackKnight Body 레이어.
  - 공중·다운처럼 발 IK를 끄거나 줄여야 하는 상황의 처리 기준.
  - 사냥개 4다리 정렬과 경사면 몸통 Pitch(F9).
  - 관련 manual·devlog.
- 제외
  - 발 고정(이동 중 발 미끄러짐 방지).
  - 손 IK, 물리 기반 반응, 상시 LookAt([#51](https://github.com/jaykop/Kata/issues/51)).
  - 레이어 설정이 없는 캐릭터(SilverKnight 등)의 레이어 구성.

## 처리 구조

| 단계 | 위치 | 하는 일 |
|---|---|---|
| 1. 호출 | 메인 Template ABP | Slot 뒤에서 Body ALI 함수에 포즈를 넘긴다. 링크된 Body 레이어가 없으면 기본 구현이 입력 포즈를 그대로 돌려준다 |
| 2. 입력 | Body 레이어 ABP | Linked Input Pose를 Control Rig 노드에 넣는다. 메인 인스턴스의 `Is On Ground`를 Property Access로 리그 입력 핀에 연결한다 |
| 3. 보정 계산·적용 | 발 IK Control Rig | 다리별 트레이스, 발 높이 보정, 골반 보정, 보간, Two Bone IK |

메인 그래프 순서: State Machine → Slot → `Body_PostProcess`(Control Rig) → Kata Tilt → (LookAt, #51) → Output Pose.

### 2족 리그의 Forward Solve

1. 다리마다 입력 포즈의 FK 발 본 위치에서 아래로 스윕 트레이스를 한다. 시작 높이는 컴포넌트 원점 위 Trace Up, 끝은 원점 아래 Trace Down이다.
2. 발 높이 보정은 "맞은 지면 높이 − 컴포넌트 원점 높이"다. 애니메이션의 발 높이(원점 기준)는 그대로 두고 지면 차이만큼만 위아래로 옮긴다. 들어 올린 발은 지면 위 같은 높이를 유지한다.
   맞지 않았거나 공중이면 0이다. 보정 범위는 설정값으로 제한한다.
3. 골반 보정은 두 다리 보정 중 낮은 값이다. 내리기만 하며(0 이하) Max Pelvis Drop으로 제한한다. 낮은 쪽 발이 닿도록 몸 전체를 내리고, 높은 쪽 다리는 IK로 더 굽힌다.
4. 다리 보정과 골반 보정을 각각 시간 기반으로 보간한다. 계단과 경사 변화에서 튀지 않게 하기 위해서다. 보간 상태는 캐릭터별 리그 인스턴스에 남는다.
5. 골반 보정 본(F5)에 수직 오프셋을 더한다.
6. 다리마다 Two Bone IK(허벅지·종아리·발)로 발을 목표에 맞춘다. 무릎 방향은 입력 포즈의 무릎 방향을 쓴다. 발 회전은 지면 법선 쪽으로 Max Foot Angle까지 기울인다.
7. 전체 결과를 Alpha로 섞는다. Alpha는 노드 Alpha, `Is On Ground`, 애니메이션 커브(F8)의 곱이다.

4족 리그는 같은 방식으로 네 다리를 다룬다. 뒷다리는 `Thigh`·`Calf`·`Foot`, 앞다리는 `UpperArm`·`Forearm`·`Hand`를 Two Bone IK 체인으로 쓰고 접지점은 `Toe0`·`Finger0`이다.
몸통 Pitch는 앞다리 평균 보정과 뒷다리 평균 보정의 차이와 앞뒤 다리 간격으로 각도를 구해, 골반 보정 본을 액터의 오른쪽 축으로 돌린다. 각도는 Max Body Pitch로 제한하고 골반 보정과 같은 방식으로 보간한다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| F1 위치 | 확정 | 스켈레톤별 Body 레이어(Body ALI, `UKataAnimLayerSetup::BodyLayer`)에 둔다. 2026-10-04 사용자 결정([장비·무기 계획](Equipment-Plan.md)) |
| F2 구현 수단 | 확정 | Control Rig로 구현한다(2026-10-10 사용자). IK 발 본이 필요 없고, 4족 몸통 Pitch를 같은 그래프에서 다룰 수 있으며, 엔진 기본 활성 플러그인만 쓴다. Kata 플러그인 C++ 변경은 없다 |
| F3 Body ALI 형태 | 확정 | `ALI_KataBody`에 입력 포즈 하나를 받는 함수 `Body_PostProcess` 하나를 둔다. 메인 Template ABP가 이 ALI를 구현하고 기본 구현은 입력을 그대로 돌려준다. 무기 레이어 함수는 입력 없이 둔다는 결정의 예외다. Body 레이어는 포즈를 만들지 않고 받은 포즈를 고치는 후처리이기 때문이다 (2026-10-11 사용자, 제안대로) |
| F4 그래프 순서 | 확정 | Slot → Body → Kata Tilt → LookAt. Tilt 체인(`Spine`~`Spine1`)은 다리와 골반 보정 본을 포함하지 않는다. 골반 보정 본의 수직 이동은 Tilt 체인을 강체로 옮길 뿐이라 순서를 바꿔도 결과가 거의 같다. Body를 먼저 두면 Tilt·LookAt이 최종 몸 높이에서 회전 중심을 잡는다 (2026-10-11 사용자, 제안대로) |
| F5 골반 보정 본 | 확정 | `Pelvis`와 `Spine`의 공통 부모에 오프셋을 준다. 기사·사냥개는 `Root`, DS3Player는 `RootPos`다. 리그 변수 Pelvis Bone으로 받아 기본값을 `Root`로 둔다. `Pelvis`만 내리면 다리만 내려가고 상체가 제자리에 남는다 (2026-10-11 사용자, 제안대로) |
| F6 IK 발 본 | 해소 | F2에서 Control Rig를 택해 필요 없다. 스켈레톤에 가상 본을 추가하지 않는다 |
| F7 착지 판정과 발 고정 | 확정 | 첫 단계는 발을 고정하지 않고 위 Forward Solve처럼 항상 지면 높이에 맞춘다. 발 고정은 발 속도 판정과 고정 위치 유지가 필요해 후속으로 둔다 (2026-10-11 사용자, 제안대로) |
| F8 예외 처리 | 확정 | 공중은 메인 인스턴스의 `Is On Ground`를 리그 입력으로 받아 보정을 페이드한다. 다운·구르기처럼 다리를 지면에 맞추면 안 되는 애니메이션에는 커브 `DisableFootIK`(1이면 끔)를 넣고 리그가 읽는다. Kata 액션 타임라인에서 끄는 경로는 `UKataAnimInstance`에 C++ 값을 더해야 하므로 커브로 부족할 때 따로 정한다 (2026-10-11 사용자, 제안대로) |
| F9 4족 범위 | 확정 | 사냥개 네 다리 정렬과 경사면 몸통 Pitch까지 이 작업에 넣는다(2026-10-10 사용자). Control Rig에서는 같은 리그 안의 추가 단계다. 대안이던 네 다리 정렬만 하는 안과 2족만 하는 안은 채택하지 않았다 |
| F10 플러그인 | 확정 | Control Rig는 엔진 기본 활성이므로 샘플 프로젝트 설정을 바꾸지 않는다. Kata 플러그인은 Control Rig를 참조하지 않는다. Full Body IK가 필요해지면 그때 샘플 전용 활성화를 정한다 (2026-10-11 사용자, 제안대로) |
| F11 트레이스 채널과 비용 | 구현 때 확인 | 기본 채널(Visibility)이 다른 캐릭터의 캡슐·메시에 맞아 발이 적 위에 올라가지 않는지 본다. 맞으면 지형만 막는 채널을 쓴다. 먼 캐릭터는 Control Rig 노드의 LOD Threshold로 끈다 |
| F12 IK 솔버 | 확정 | 다리마다 Two Bone IK를 쓰고 골반·몸통 보정은 리그에서 직접 계산한다. 추가 플러그인이 필요 없고 결과를 예측하기 쉽다. 대안: Full Body IK(플러그인 활성화 필요, 골반·척추를 함께 풀지만 조정 항목이 많다) (2026-10-11 사용자, 제안대로) |
| F13 리그·레이어 공유 | 확정 | 리그는 2족용 `CR_FootIK_Biped`와 4족용 `CR_FootIK_Quadruped` 둘로 둔다. 본 이름은 리그 변수로 받는다. Body 레이어 ABP는 스켈레톤 없는 Template으로 만들어 같은 리그를 쓰는 스켈레톤끼리 공유하는 것을 먼저 시도하고, Template에서 Control Rig 노드가 동작하지 않으면 스켈레톤별 ABP로 만든다 (2026-10-11 사용자, 제안대로) |

### 구현 수단 선택지

F2의 원래 질문과 비교 결과다. C를 택했다(2026-10-10 사용자).

| 안 | 내용 | 장점 | 단점 |
|---|---|---|---|
| A. 엔진 Foot Placement + Leg IK | Animation Warping 플러그인의 노드를 Body 레이어 ABP에 둔다 | 골반 보간, 착지 판정, 발 고정, 공중 처리가 내장되어 있다. 노드 하나라 비용이 작다 | Experimental 노드이고 플러그인(기본 비활성)을 켜야 한다. IK 발 본이 필수인데 DS3 스켈레톤에 없다. 골반 본 하나를 위아래로만 옮겨 4족 몸통 정렬을 할 수 없다 |
| B. Kata C++ AnimNode | KataFramework에 트레이스·두 본 IK·골반 보정 노드를 둔다 | 추가 플러그인이 없다 | 보간, 계단, 다리 수, 공중 처리를 C++로 직접 구현·조정해야 하고 엔진 기능과 겹친다 |
| C. Control Rig (채택) | 리그 안에서 트레이스, Two Bone IK, 골반·몸통 보정을 조립하고 Body 레이어 ABP의 Control Rig 노드로 실행한다 | 엔진 기본 활성이고 Experimental이 아니다. IK 발 본이 필요 없다. 4족 몸통 Pitch를 같은 그래프에서 다룬다. 본 이름이 같은 스켈레톤끼리 리그를 공유할 수 있다 | 보간·공중 처리를 직접 만들고 조정해야 한다. RigVM 실행으로 A보다 비용이 크다(측정 전). 리그 작성이 에디터 작업 위주다 |

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| FI-0 | 높음 | 에디터에서 공통 부모 계층과 루트 모션 본을 확인 | 없음 | 확인 결과가 F5와 맞다. 다르면 F5를 다시 정한다 |
| FI-1 | 높음 | `ALI_KataBody`(`Body_PostProcess`, 입력 포즈 하나), 메인 Template ABP의 ALI 구현과 호출 지점 | #14 Template ABP 설정 저장 | Body 레이어가 없는 캐릭터의 포즈가 이전과 같다 |
| FI-2 | 높음 | `CR_FootIK_Biped`, BlackKnight Body 레이어 ABP, `DA_AnimLayerSetup_BlackKnight`의 Body Layer 지정 | FI-1 | 경사면과 계단에서 Idle·이동·공격 몽타주 중 발이 지면에 닿고 상체가 골반과 함께 내려간다. 평지에서는 원래 포즈와 같다. Tilt와 함께 써도 어긋나지 않는다 |
| FI-3 | 보통 | 공중 페이드와 `DisableFootIK` 커브(다운·구르기 등) | FI-2 | 공중과 다운 중 다리가 지면 쪽으로 당겨지지 않는다 |
| FI-4 | 보통 | `CR_FootIK_Quadruped`와 사냥개 Body 레이어 | FI-2 | 경사면에서 네 발이 지면에 닿고 몸통이 경사를 따라 기운다 |
| FI-5 | 보통 | manual과 devlog | FI-2 | Body 레이어 예제, 발 IK 리그 설정과 제한이 manual에 있다 |

## 영향과 제한

- 모듈 경계: Kata 플러그인의 소스와 의존을 바꾸지 않는다. Control Rig는 샘플 콘텐츠에서만 쓴다.
- Control Rig 노드는 본을 직접 고르지 않지만 2족과 4족은 리그가 다르므로 확정대로 Body 레이어에서 리그를 고른다.
- 콘텐츠 충돌: `ABP_CharacterBase`는 #14 샘플 설정에서도 고친다. #14의 Template ABP 설정을 저장한 뒤 FI-1을 한다.
- 실행 상태: 보간 상태는 캐릭터마다 생기는 리그 인스턴스에 있고 리그 에셋에는 설정값만 있다.
- 이동과의 관계: 발 IK는 포즈만 바꾸고 이동량(루트 모션, CharacterMovement)은 바꾸지 않는다.
  루트 모션 커브([#36](https://github.com/jaykop/Kata/issues/36)), 오토 대시([#37](https://github.com/jaykop/Kata/issues/37)), 전진 제한([#38](https://github.com/jaykop/Kata/issues/38))과 독립이다.
  Hit Trace는 메시 소켓 위치를 쓰므로 보정된 포즈를 따른다.
- 비용: 다리마다 매 프레임 스윕 트레이스를 하고 RigVM 그래프를 실행한다. 먼 캐릭터는 LOD Threshold로 끈다. 실제 비용은 구현 후 측정한다.
- 프리뷰: Anim Blueprint 편집기 프리뷰는 `PreviewAnimLayerSetup`으로 Body 레이어를 링크한다. 프리뷰에는 CharacterMovement가 없어 `Is On Ground`가 기본값(false)이므로 공중 페이드에 걸려 보정이 보이지 않을 수 있다. 프리뷰 처리는 구현 때 정한다.

## 사용자 확인 항목

- 구현 전 에디터 확인(FI-0): BlackKnight 외 스켈레톤의 공통 부모 계층, 루트 모션이 `Master`에 있는지.
- 구현 중 확인: 레이어 안 Control Rig 노드에서 트레이스가 동작하는지, Template Body 레이어에서 Control Rig 노드가 동작하는지(F13).
- 실행 확인: 경사면·계단이 있는 맵에서 BlackKnight PC와 AI의 Idle·Walk·Run·공격 몽타주·회피, Tilt 태스크가 있는 액션, 사냥개.
- 이 목록만으로 에이전트가 빌드·테스트·별도 검사를 하지 않는다.

## 완료 시 갱신할 문서

- 연결 이슈: 구현·확인 상태.
- [애니메이션 레이어 사용법](../manual/Animation-Layers.md): Body ALI와 입력 포즈 규칙, Body 레이어 예제, 발 IK 리그 설정과 제한. "Body 레이어 예제는 아직 없다" 문장을 고친다.
- devlog: 구현 수단(Control Rig 채택 이유), 골반 보정 본, 리그 구조 결정과 실제 결과.
- [문서 목록](../README.md): 이 계획의 링크(작성 시 추가), devlog 링크.
