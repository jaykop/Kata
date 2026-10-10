# 루트 모션 커브 사용법

갱신: 2026-10-10  
대상: 기획자·애니메이션 작업자, KataFramework `UKataRootMotionCurveComponent`, KataFrameworkEditor `UKataRootMotionCurveModifier`·몽타주 굽기 명령  
적용 기준: UE 5.8, [루트 모션 이동량 커브 결정 기록](../devlog/2026-10-10-Root-Motion-Curve.md)  
확인 상태: 2026-10-10 사용자 확인 — 수정자 추출, Y 커브 편집 PIE, 몽타주 커브 굽기·우선 적용·덮어쓰기 확인·실행 취소, 자른·이어 붙인 몽타주와 섹션 반복의 원본 일치. 재생 속도, URO·LOD, 화면 밖 캐릭터, 코드 섹션 점프는 미확인

## 목적과 준비

애니메이션의 루트 모션을 커브로 뽑아 두고, 커브를 고쳐 원본 애니메이션을 다시 만들지 않고 이동 거리와 속도 분포를 바꾼다.
추출 직후의 커브는 원본 루트 모션과 같은 이동을 낸다.

- 커브를 넣을 애니메이션 시퀀스는 Enable Root Motion이 켜져 있어야 한다. 커브는 엔진 루트 모션 경로 안에서 원래 값을 대체한다.
- 캐릭터는 `AKataCharacter` 계열이어야 한다. 이 클래스가 `KataRootMotionCurve` 컴포넌트를 기본으로 갖는다.
  다른 Character는 `UKataRootMotionCurveComponent`를 추가하면 된다.
- 캐릭터 AnimInstance의 Root Motion Mode는 Root Motion From Montages Only(엔진 기본값)여야 한다.

## 사용 순서

1. 콘텐츠 브라우저에서 애니메이션 시퀀스를 우클릭하고 Animation Modifiers의 Add Modifiers로 `Kata Root Motion Curve`를 추가해 적용한다.
   시퀀스 에디터의 Window → Animation Data Modifiers에서 추가·Apply해도 된다.
2. 시퀀스 에디터의 Curves 트랙에 `Kata.RootMotion.X`, `Kata.RootMotion.Y`, `Kata.RootMotion.Z`, `Kata.RootMotion.Yaw`가 생겼는지 확인한다.
   Output Log의 `LogKataRootMotionCurve`에 키 수, 끝 이동량, 키 사이 최대 오차가 나온다.
3. 커브를 쓸 캐릭터 Blueprint에서 `KataRootMotionCurve` 컴포넌트의 Use Root Motion Curves를 켠다. 기본값은 꺼짐이다.
4. 필요한 구간의 커브 키를 고치고 시퀀스를 저장한다. 다음 재생부터 바뀐 이동이 적용된다.

### 커브 값 읽는 법

- 커브 값은 애니메이션 첫 프레임 루트를 원점으로 한 누적 이동(cm)과 누적 Yaw(도)다. 실행 중에는 두 시각의 값 차이만 이동으로 쓴다.
  모든 키를 같은 값으로 바꾸면 그 애니메이션 동안 이동하지 않는다.
- 축은 캐릭터 정면이 아니라 애니메이션 루트의 축이다. 흑기사 샘플은 앞 방향이 `Kata.RootMotion.Y`다.
  어느 축이 앞인지는 추출 로그의 끝 이동량에서 가장 큰 축으로 확인한다.
- 마지막 키 하나만 크게 바꾸면 그 프레임에 이동이 몰려 순간이동처럼 보인다. 거리를 늘리려면 구간의 키를 함께 늘린다.

## 주요 설정과 실행 규칙

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| `Kata Root Motion Curve` 수정자 Apply | 시퀀스 루트 모션을 네 커브로 기록 | 이미 커브가 있으면 덮어쓴다. 원본 애니메이션이 바뀌어도 자동으로 다시 추출하지 않는다 |
| 수정자 Revert | 네 커브를 지운다 | 편집한 내용도 함께 사라진다 |
| 몽타주 우클릭 Bake Kata Root Motion Curves | 몽타주의 현재 이동을 몽타주 커브로 기록 | 루트 모션이 없는 몽타주는 건너뛴다. 기존 몽타주 커브는 확인 후 덮어쓴다. 결과는 알림과 `LogKataRootMotionCurve`에 나온다 |
| Use Sequence Frame Rate | 시퀀스 프레임마다 키를 만든다 | 켜짐. 끄면 Sample Rate(초당 키 수)를 쓰며 키 사이 오차가 생길 수 있다 |
| Position Tolerance / Rotation Tolerance | 키 사이 오차, 원본의 Pitch·Roll 경고 기준 | 0.5cm / 0.5도 |
| Reapply Post Owner Change(엔진 수정자 설정) | 에셋이 바뀔 때마다 다시 적용 | 꺼짐 유지. 켜면 커브를 고칠 때마다 다시 추출해 편집이 사라지므로 적용 시 경고한다 |
| Use Root Motion Curves | 이 캐릭터에서 커브 대체를 쓸지 | 꺼짐. 실행 중에 바꾸면 다음 이동 갱신부터 적용된다 |
| `AKataCharacter::GetRootMotionCurveComponent` | 컴포넌트 접근 | 생성자에서 만들기 때문에 항상 유효하다 |
| `KataFL::EvaluateRootMotionCurves` | 애니메이션 시간축의 커브 값 읽기 | 네 커브 중 하나라도 없으면 false |

### 우선순위

1. 몽타주 자신에게 네 커브가 있으면 몽타주 트랙 시간으로 그 커브를 읽는다.
2. 없으면 몽타주 구간을 시퀀스별로 나눠, 커브가 있는 시퀀스는 커브를, 없는 시퀀스는 원래 루트 모션을 쓴다.
3. 커브를 하나도 쓰지 않은 갱신은 엔진 값을 그대로 둔다.

### 몽타주 커브 굽기

같은 시퀀스로 만든 몽타주만 이동량을 따로 바꾸려면 몽타주에 커브를 굽는다.

1. 콘텐츠 브라우저에서 몽타주를 우클릭하고 Bake Kata Root Motion Curves를 실행한다. 여러 몽타주를 함께 선택해도 된다.
2. 몽타주의 현재 이동(시퀀스 커브가 있으면 그 커브, 없으면 원래 루트 모션)이 몽타주 트랙 시간축의 네 커브로 기록된다.
   세그먼트 경계에는 반드시 키가 들어가고, 나머지 키는 구성 시퀀스 중 가장 높은 프레임 레이트로 고르게 나뉜다.
3. 몽타주 에디터의 Curves 트랙에서 커브를 고친다. 이 몽타주는 시퀀스 커브 대신 몽타주 커브를 쓴다.

이미 몽타주에 커브가 있으면 덮어쓰기 확인 창이 뜬다. 굽기는 실행 취소(Ctrl+Z)할 수 있다.
몽타주 커브를 지우면 다시 시퀀스 커브나 원래 루트 모션을 쓴다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| Z 커브를 바꿔도 위아래로 움직이지 않음 | 엔진 루트 모션과 같은 제약이다. Walking은 바닥을 따르고 Falling은 중력 속도를 쓴다 | Z 이동이 필요한 구간은 Flying 같은 이동 모드로 바꾼다 |
| 커브를 바꿔도 이동이 그대로 | Use Root Motion Curves가 꺼져 있거나, 시퀀스의 Enable Root Motion이 꺼져 있거나, 네 커브 중 일부가 없다 | 컴포넌트 설정과 시퀀스 설정, 커브 이름을 확인한다 |
| 로그에 "already bound" 경고 | Motion Warping 같은 다른 시스템이 같은 루트 모션 델리게이트를 먼저 바인딩했다 | 그 캐릭터에서는 커브 대체가 꺼진다. 다른 시스템을 제거하거나 Kata 커브를 쓰지 않는다 |
| 원본의 Pitch·Roll 경고 | 커브는 Yaw만 담는다 | 루트 회전이 Yaw뿐이 되도록 애니메이션을 정리하거나 그 회전을 포기한다 |
| 역재생, 갱신 도중 노티파이가 위치를 바꾼 경우 | 그 갱신은 엔진 값을 쓴다 | 해당 프레임은 원래 루트 모션으로 움직인다 |
| Root Motion From Everything | 몽타주 밖 루트 모션도 섞이는 모드라 대체하지 않는다 | Root Motion From Montages Only를 쓴다 |

## 확인 상태와 근거

- 2026-10-10 사용자 확인: `AS_BlackKnight_Roll_F`에 수정자를 적용해 커브 4개 생성. 로그상 키 51개, 끝 이동 Y=526.77cm, 키 사이 최대 오차 0.002cm.
  `BP_BlackKnight_PC`의 PIE 앞 구르기로 `Kata.RootMotion.Y`의 모든 키를 300으로 바꾼 테스트를 통과했다(사용자 보고). Z 커브 변경은 Walking 중 이동에 반영되지 않았다.
- 2026-10-10 사용자 확인: `AM_BlackKnight_Roll_F`에 몽타주 우클릭 굽기로 커브 4개 생성, 몽타주 커브가 시퀀스 커브보다 우선 적용, 재굽기 시 덮어쓰기 확인 창, 실행 취소.
- 2026-10-10 사용자 PIE 확인(위치는 에디터 연결로 측정): 앞 0.3초를 자른 `Roll_F`에 `Backstep`을 이어 붙이고 `Roll_F`에만 커브가 있는 몽타주에서 커브 켬 67.79cm·끔 67.67cm.
  `Default` 섹션을 반복하는 `Roll_F` 몽타주 3초 재생에서 켬·끔 모두 1059.5cm(차이 0.013cm), 반복 경계에서 튐 없음.
- 소스 기준(미확인): 재생 속도 변경, URO·LOD, 화면 밖 캐릭터, 코드의 `Montage_JumpToSection`.
- [KataRootMotionCurveComponent.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Animation/KataRootMotionCurveComponent.cpp): 대체 처리와 섹션 경로 복원.
- [KataRootMotionCurveModifier.cpp](../../Plugins/KataFramework/Source/KataFrameworkEditor/Private/Animation/KataRootMotionCurveModifier.cpp): 추출과 검사.
- [KataRootMotionCurveBake.cpp](../../Plugins/KataFramework/Source/KataFrameworkEditor/Private/Animation/KataRootMotionCurveBake.cpp): 몽타주 커브 굽기와 메뉴.
- [KataFL_RootMotionCurve.h](../../Plugins/KataFramework/Source/KataFramework/Public/Animation/KataFL_RootMotionCurve.h): 커브 이름과 변화량 계산.
- [작업 상태](https://github.com/jaykop/Kata/issues/36).
