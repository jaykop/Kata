# 루트 모션 이동량 커브

작성: 2026-10-10  
갱신: 2026-10-10  
유형: 결정 기록·구현 기록  
대상: KataFramework `UKataRootMotionCurveComponent`·`KataFL_RootMotionCurve`, KataFrameworkEditor `UKataRootMotionCurveModifier`·몽타주 굽기 명령  
기준: 커밋 d71b237(엔진 확인), 6adb07f(추출 수정자), 6e9935a(런타임 대체), 94beddf(몽타주 굽기)와 이 기록을 담은 커밋

## 배경과 결론

[#36](https://github.com/jaykop/Kata/issues/36)은 애니메이션의 루트 모션 이동량을 커브로 뽑아 편집하고, 실행 중에는 그 커브로 캐릭터를 움직이는 기능이다.
사용자는 이 기능을 "추출·편집은 에디터 도구, 런타임은 커브를 읽어 루트 모션 대신 이동"으로 정했고, 추출 직후에는 원본 이동량을 보존하며
LOD와 애니메이션 최적화에 대응해야 한다고 요구했다.

결론은 다음과 같다. 커브는 애니메이션 시퀀스와 몽타주에 두고, 실행 중에는 엔진 루트 모션 경로를 그대로 둔 채
`UCharacterMovementComponent::ProcessRootMotionPreConvertToWorld`에서 엔진 값을 커브 변화량으로 바꾼다. 사용법은 [루트 모션 커브 사용법](../manual/Root-Motion-Curve.md)을 따른다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 커브 추출 | 없음 | `UKataRootMotionCurveModifier`(Animation Modifier)가 `ExtractRootMotionFromRange(0, t)`로 시퀀스 프레임마다 누적 이동과 펼친 Yaw를 `Kata.RootMotion.X/Y/Z/Yaw` 선형 키로 기록하고, 키 사이 오차·Pitch/Roll·Enable Root Motion·자동 재적용을 검사한다 |
| 실행 시 이동 | 몽타주 루트 모션을 그대로 적용 | `UKataRootMotionCurveComponent`가 루트 모션 몽타주가 이번 갱신에 지나간 트랙 구간을 섹션·세그먼트 단위로 나눠 몽타주 커브 → 시퀀스 커브 → 원래 루트 모션 순서로 계산한다. `AKataCharacter`가 기본으로 갖고 `bUseRootMotionCurves`는 기본 꺼짐이다 |
| 몽타주 커브 | 없음 | 몽타주 우클릭 Bake Kata Root Motion Curves가 현재 이동을 몽타주 트랙 시간축 커브로 굽는다. 세그먼트 경계는 반드시 키로 넣는다 |
| 공용 계산 | 없음 | 커브 이름·평가·변화량 계산과 몽타주 구간 추출은 `KataFL_RootMotionCurve`, 커브 기록은 에디터의 `KataFL_RootMotionCurveEditor`에 둔다 |

## 주요 결정과 이유

| 결정 | 채택 | 검토한 대안과 제외 이유 |
|---|---|---|
| 커브 위치 | 애니메이션 시퀀스 커브 + 몽타주 커브(몽타주 우선) | 태스크 소유 커브: 몽타주 구성이 바뀌면 다시 구워야 하고, Kata 시계와 애니메이션 시계가 히트스톱·중단에서 어긋나며, 편집 UI를 새로 만들어야 한다. 시퀀스·몽타주 커브는 엔진 커브 편집 UI를 그대로 쓴다. 사용자 이전 프로젝트도 시퀀스에 커브를 두었다 |
| 액션별 이동량 차이 | 두지 않는다(사용자 결정) | 다른 이동이 필요하면 몽타주를 새로 만들고 몽타주 커브로 조정한다 |
| 런타임 적용 지점 | `ProcessRootMotionPreConvertToWorld` | Override 루트 모션 소스: 애니메이션 루트 모션이 있는 동안 Yaw를 바꿀 수 없다. `IgnoreRootMotion`으로 원본을 끄기: 몽타주 진행이 CharacterMovement의 `TickCharacterPose`에서 메시 Tick으로 돌아가 URO 예외를 잃는다 |
| LOD·최적화 대응 | 에셋 커브를 몽타주 위치로 직접 평가 | 평가된 포즈의 커브 값(`GetCurveValue`)은 URO 평가 주기와 LOD 커브 필터의 영향을 받는다. 루트 모션 몽타주 재생 중에는 엔진이 URO 업데이트 주기를 1로 고정하므로(`AnimUpdateRateSetParams`) 몽타주 위치 기록은 LOD와 무관하다 |
| 누적값과 변화량 | 커브는 누적값, 실행은 구간 변화량 누적 | 누적값이면 기획자가 "이 시각까지 몇 cm"를 읽기 쉽고 압축 오차가 쌓이지 않는다. 실행 중에는 엔진 추출과 같은 단계 순서로 변화량을 이어 붙여 세그먼트·섹션 경계에서 튀지 않는다 |
| 섹션 점프 | 1단계부터 지원(사용자 결정) | `Advance`는 시작 위치와 총 이동량만 남기므로, 시작 위치 + 이동량이 현재 위치와 다르면 `GetNextSectionID`로 경로를 다시 만든다. 노티파이가 갱신 중 위치를 바꾼 경우와 역재생은 그 갱신만 엔진 값을 쓴다 |
| 회전 | Yaw만 | 루트 회전의 Pitch·Roll은 캡슐을 기울인다. Yaw 하나면 편집이 직관적이다. 원본에 Pitch·Roll이 있으면 추출 시 경고한다 |
| 재추출 | 자동 재추출 없음, 커브가 있으면 항상 커브(사용자 결정) | 원본이 바뀌어도 편집한 커브를 지키기 위해서다. 다시 Apply하거나 굽는 것은 사용자의 명시적 실행으로 본다 |
| 컴포넌트 기본값 | 꺼짐(사용자 결정) | 커브를 쓸 캐릭터에서만 켠다 |
| Motion Warping | 도입하지 않는다(사용자 결정, [#37](https://github.com/jaykop/Kata/issues/37)) | Skew Warp가 남은 이동량을 원본 루트 트랙(`ExtractRootMotionFromAnimation`)에서 계산해 커브와 맞지 않고, 같은 단일 바인딩 델리게이트를 쓴다. #37 거리 보정과 #38 전진 제한은 Kata 처리 안에 이어 붙인다 |

### 바로잡은 진단

- 처음 엔진 확인에서는 몽타주 루트 모션을 "몽타주 블렌드 × 슬롯 가중치로 섞는다"고 기록했다. Root Motion From Montages Only(엔진 기본값)에서는
  루트 모션 몽타주 하나만 가중치 없이 루트 모션을 내며, 가중치 혼합은 Root Motion From Everything에서만 쓴다(`UAnimInstance::Montage_Advance`).
  런타임은 루트 모션 몽타주 하나만 계산하고, Root Motion From Everything에서는 엔진 값을 그대로 둔다.
- Z 이동은 Falling에서도 반영된다고 처음 적었으나, Walking은 바닥을 따르고(`MaintainHorizontalGroundVelocity`) Falling은 중력 속도를 쓴다(`ConstrainAnimRootMotionVelocity`).
  Flying 같은 이동 모드에서만 반영된다.

## 근거

- [KataRootMotionCurveComponent.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Animation/KataRootMotionCurveComponent.cpp): 섹션 경로 복원과 대체 처리.
- [KataFL_RootMotionCurve.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Animation/KataFL_RootMotionCurve.cpp): 커브 평가, 변화량, 몽타주 구간 추출.
- [KataRootMotionCurveModifier.cpp](../../Plugins/KataFramework/Source/KataFrameworkEditor/Private/Animation/KataRootMotionCurveModifier.cpp), [KataRootMotionCurveBake.cpp](../../Plugins/KataFramework/Source/KataFrameworkEditor/Private/Animation/KataRootMotionCurveBake.cpp): 추출과 굽기.
- UE 5.8 엔진 소스: `CharacterMovementComponent.cpp`(`ConvertLocalRootMotionToWorld`, `TickCharacterPose`), `AnimMontage.cpp`(`FAnimMontageInstance::Advance`), `AnimInstance.cpp`(`Montage_Advance`),
  `SkinnedMeshComponent.cpp`(`AnimUpdateRateSetParams`), `RootMotionModifier_SkewWarp.cpp`.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 추출 수정자 | 사용자 에디터 적용 | `AS_BlackKnight_Roll_F` 키 51개, 끝 이동 Y=526.77cm, 키 사이 최대 오차 0.002cm | — |
| 커브 편집 반영 | 사용자 PIE | Y 커브 편집이 앞 구르기 이동에 반영. Z는 Walking에서 반영되지 않음 | — |
| 몽타주 굽기 | 사용자 에디터·PIE | 메뉴, 굽기, 몽타주 커브 우선, 덮어쓰기 확인, 실행 취소 | — |
| 자르기·이어 붙이기·섞인 경우 | 사용자 PIE, 위치는 에디터 연결로 측정 | `Roll_F`(앞 0.3초 자름, 커브) + `Backstep`(원본) 몽타주에서 켬 67.79cm·끔 67.67cm | — |
| 섹션 반복 | 사용자 PIE, 위치는 에디터 연결로 측정 | `Default` 반복 3초 재생에서 켬 1059.51cm·끔 1059.50cm, 반복 경계 튐 없음 | — |
| 나머지 | 소스 기준 | 엔진 루트 모션 경로를 그대로 쓴다 | 재생 속도 변경, URO·LOD, 화면 밖 캐릭터, 코드의 `Montage_JumpToSection` |

테스트 몽타주는 `Content/KataSample/Test/RootMotionCurve/`에 남겼다.

## 남은 제한과 후속 작업

- 미확인 범위는 위 표와 같다. 문제가 보이면 새 이슈로 다룬다.
- 속도·방향 커브 같은 편집 보조와 액션 편집기 연동(이동 경로 표시)은 사용 후 필요하면 검토한다.
- #37 거리 보정과 #38 전진 제한은 같은 델리게이트 처리 안에 이어 붙인다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [연결 이슈 #36](https://github.com/jaykop/Kata/issues/36) | 결과 댓글 후 종료(게시는 사용자 확인 후) |
| [루트 모션 커브 사용법](../manual/Root-Motion-Curve.md) | 사용법·제한·확인 상태 |
| [런타임 사용법](../manual/Runtime-Usage.md) | `AKataCharacter` 기본 컴포넌트 안내 |
| [기본 태스크 확장 계획](../plan/Base-Task-Plan.md) | 루트 모션 선행 질문의 결과 링크를 이 기록으로 바꿈 |
| 루트 모션 이동량 커브 계획 | 이슈 종료로 삭제. 결정 이유는 이 기록으로 옮김 |
