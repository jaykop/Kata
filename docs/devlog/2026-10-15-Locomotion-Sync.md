# 로코모션 Sync Group과 발 Sync Marker

작성: 2026-10-15  
갱신: 2026-10-15  
유형: 구현 기록, 진단  
대상: 샘플 로코모션 ABP(`ABP_CharacterBase`, `ABP_WeaponLayersBase`, `ALI_KataWeapon`, 레이어 자식 3개), 흑기사 이동 클립과 BlendSpace  
기준: `d08c3f0` 이후 작업 트리. 샘플 에셋은 Diversion 에서만 추적한다

## 배경과 결론

[애니메이션 시스템 계획](../plan/Animation-System-Plan.md) AS-2 는 샘플 PC 가 4방향 이동과 방향 전환에서 발 위상이 튀지 않게 하고, 로코모션 상태 머신에 공중 상태를 더하는 작업이다.
레이어 템플릿의 걷기·달리기 플레이어를 Sync Group 으로 묶고, 흑기사 클립에 발 Sync Marker 를 넣었다. BlendSpace 방향 표본은 약 0.23초에 걸쳐 섞이게 했고, `Airborne` 상태와 `FullBody_Fall` 레이어를 더했다.
PIE 에서 걷기↔달리기 전환, 아주 느린 걷기, 대각선 이동의 발 밀림이 드러났다. 원인은 클립이 가정한 속도와 실제 이동 속도가 다르기 때문이며, 사용자 결정으로 AS-2 에서는 고치지 않고 AS-11 로 넘겼다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| `ABP_WeaponLayersBase` 의 `FullBody_Walk` BlendSpace Player, `FullBody_Run` Sequence Player | Sync 없음(DoNotSync) | Sync Group `Locomotion`, Can Be Leader |
| 흑기사 Greatsword 걷기 4방향·`Run_F` 클립 | Sync Marker 없음 | `FootSyncMarkers` 트랙에 `L_FootDown`·`R_FootDown` 각 1개. 클립 하나가 걸음 한 주기다 |
| `BS_BlackKnight_Walk_Greatsword` `Sample Weight Speed` | 0 | 4.3. 방향 표본 가중치가 약 0.23초에 걸쳐 바뀐다(D10) |
| `ABP_CharacterBase` `Locomotion` | `Idle`·`Walk`·`Run` | `Airborne` 추가. State Alias(`Idle`·`Walk`·`Run`)에서 `Is Falling` 으로 들어가고 `Is On Ground` 로 `Idle` 에 돌아온다. 전이는 0.2초 Hermite Cubic |
| `ALI_KataWeapon`, 레이어 템플릿 | `FullBody_Idle`·`Walk`·`Run` | `FullBody_Fall` 과 변수 `Anim_Fall` 추가. 흑기사·은기사·사냥개 레이어의 `Anim_Fall` 은 각자의 Idle 클립 |

## 주요 결정과 이유

- **Sync Group 은 레이어 템플릿에 둔다.** 모든 캐릭터의 레이어가 같은 그래프를 물려받으므로 캐릭터마다 설정하지 않아도 된다. 마커가 없는 클립(은기사·사냥개)은 같은 그룹에서 길이 비율로 맞춰진다. 발 주기가 없는 Idle 과 Fall 은 그룹에 넣지 않았다.
- **발 Sync Marker 는 수동으로 맞춘다.** 먼저 엔진 `FootstepAnimEventsModifier` 의 `Foot Bone Reaches Ground` 방식을 써 봤다.
  기본 `Ground Threshold` 4 cm 에서는 클립마다 마커가 1개만 찍혔고, 8 cm 로 넓히자 달리기 1개를 빼고 하나도 찍히지 않았다.
  이 모디파이어는 발 높이가 cm 고정 범위의 밖에서 안으로 들어올 때 마커를 찍는다고 헤더에 설명되어 있다. 걷기 클립은 발목 높이 폭이 7~19 cm 로 작고 발마다 달라 범위를 거의 벗어나지 않는다고 본다. 런처판 엔진에는 구현 소스가 없어 이 원인은 추정이다.
  대안으로 에디터 Python 에서 발목·발끝 본 위치를 프레임마다 읽어 계산해 봤다. 각 본이 그 클립의 높이 폭 아래 15% 구간에 처음 들어오는 프레임 가운데 이른 쪽을 쓰는 방식이다. 결과는 클립마다 두 발 모두 한 번씩 찍혔다.
  하지만 사용자가 편집기에서 보고 맞춘 수동 마커를 쓰기로 했다. 계산값은 수동보다 0~6프레임(평균 약 3프레임) 이르다. Sync Group 에는 절대 프레임보다 클립끼리 같은 기준으로 찍혔는지가 중요하다.
- **낙하는 당분간 대기 포즈다.** 흑기사에 확정된 낙하 루프 클립이 없다. `Anim_Fall` 을 비워 두면 기준 포즈(T 포즈)가 나오므로 세 캐릭터 모두 Idle 클립을 넣었다(사용자 결정).
- **착지 전이는 `Airborne` → `Idle` 하나만 둔다.** 같은 프레임에 속도 전이가 이어서 걸리므로, 달리는 중에 착지하면 바로 `Run` 으로 섞인다.
- **채택하지 않은 대안**
  - Stride·Orientation Warping 을 지금 도입하는 방안: 계획의 AS-6 범위이고 IK 발 가상 본이 필요하다.
  - 속도 맞춤 BlendSpace 로 바로 고치는 방안: 상태 머신과 ALI 를 다시 바꿔야 해서 AS-11 로 넘겼다(사용자 결정).

## 진단: 발 밀림

디딘 발이 몸 기준으로 뒤로 밀리는 속도를 재서 각 클립이 가정하는 이동 속도를 구했다.

| 클립 | 가정 속도(cm/s) | 실제로 쓰이는 속도 구간(cm/s) |
|---|---|---|
| 걷기 F / B / L / R | 113 / 121 / 105 / 118 | `Walk` 상태 10~250 |
| `Run_F` | 약 440(디딘 프레임이 4~7개뿐이라 오차가 크다) | `Run` 상태 250~600 |

- 걷기↔달리기: 250 cm/s 에서 전환하면 걷기는 2배 넘게 빠른 이동을, 달리기는 절반 정도 느린 이동을 원래 속도로 재생한다. 0.2초 동안 섞이는 사이 발이 밀린다.
- 아주 느린 걷기: 10 cm/s 만 넘으면 걷기 클립을 원래 속도로 재생한다.
- 대각선: BlendSpace 축은 -1~1 인데 cm/s 를 그대로 넣어, 입력이 (1, 1) 모서리로 잘린 채 앞 걸음과 옆 걸음을 원래 속도로 섞는다.

## 근거

- [애니메이션 레이어 사용법](../manual/Animation-Layers.md): "로코모션 상태 머신과 Sync Group" 절과 제한 표.
- [애니메이션 시스템 계획](../plan/Animation-System-Plan.md): D2·D10, AS-2·AS-6·AS-11.
- [KataAnimInstance.h](../../Plugins/KataFramework/Source/KataFramework/Public/Animation/KataAnimInstance.h): 전이 조건에 쓰는 `GroundSpeed`·`bIsFalling`·`bIsOnGround`.
- 엔진 `AnimationModifierLibrary` 플러그인의 `FootstepAnimEventsModifier.h`: 감지 방식 설명과 기본값(`GroundThreshold` 4, `SampleRate` 60).

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 노드 Sync 설정, BlendSpace 값, 전이 설정, `Anim_Fall` 값, 컴파일 로그 | Unreal MCP 읽기(Claude, 2026-10-15) | 위 표와 같다. 마지막 컴파일에 경고 없음 | Property Access 경로, Alias 대상 상태, Sequence 바인딩 변수 이름은 MCP 로 읽지 못했다 |
| 흑기사 PC 이동·낙하 | 사용자 PIE(2026-10-15) | 발 밀림 3건을 보고했고 그 밖의 이상은 보고하지 않았다 | 은기사·사냥개 이동, Game 타깃 |
| 빌드 | 해당 없음 | C++ 변경 없음 | — |

## 남은 제한과 후속 작업

- 발 밀림 3건은 계획 AS-11(속도 맞춤 BlendSpace)에서 고친다. 그 뒤에도 남으면 AS-6 에서 Stride Warping 을 함께 본다.
- 은기사·사냥개 클립에는 Sync Marker 가 없다.
- 짧은 턱에서 `Airborne` 이 깜빡일 가능성은 계획 단계의 미결 항목이었고, PIE 에서는 보고되지 않았다.

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [#89](https://github.com/jaykop/Kata/issues/89) | AS-2 결과 댓글. AS-3 이후가 남아 열어 둔다 |
| [애니메이션 레이어 사용법](../manual/Animation-Layers.md) | 로코모션 절, 발 밀림 제한, 확인 상태 |
| [애니메이션 시스템 계획](../plan/Animation-System-Plan.md) | AS-11 추가, AS-6 에 Stride Warping 메모 |
