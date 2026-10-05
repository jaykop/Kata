# MainHUD 사용법

갱신: 2026-10-04  
대상: KataFramework 플레이어 HUD를 설정하거나 확장하는 사용자  
적용 기준: [#33 MainHUD 기반과 락온 마커](https://github.com/jaykop/Kata/issues/33)  
확인 상태: 소스 작성. 사용자 빌드·실행·UI 확인 전

## 목적과 준비

`AKataPlayerController`가 로컬 플레이어마다 `UKataMainHUD`를 만들고 플레이어 화면에 추가한다.
이번 범위는 MainHUD 기반과 락온 마커다. HP·스태미나·적 체력바는 포함하지 않는다.
폰의 `UKataPlayerTargetingComponent`가 알리는 지점 변경을 컨트롤러가 HUD와 카메라에 전달한다.

## 사용 순서

1. Player Controller Class를 `AKataPlayerController` 파생으로 설정한다. Main HUD Class 기본값은 네이티브 `Kata Main HUD`다.
2. 폰에 PC용 타게팅 컴포넌트와 [락온 입력](Input.md#락온-입력), 대상에 [타겟 지점](Targeting.md)을 설정한다.
3. 락온을 획득하면 해당 지점에 흰 마름모가 표시되고, 전환하면 새 지점으로 옮겨지며 해제하면 사라진다.
4. 디자인을 바꾸려면 `UKataMainHUD` 파생 Widget Blueprint를 만들어 컨트롤러의 Main HUD Class로 지정한다.
   자체 마커를 만들 때는 Draw Default Lock Marker를 끄고 `GetLockMarkerPosition`의 좌표와 성공 여부를 사용한다.

별도 텍스처·Widget Blueprint 없이 기본 마커를 제공한다. 폰을 교체하면 이전 타게팅 구독을 해제하고 새 폰을 구독하며, 컨트롤러 종료 시 HUD를 화면에서 제거한다.

## 주요 설정과 실행 계약

| 항목 | 의미 | 빈 값·실패 시 동작 |
|---|---|---|
| Main HUD Class | 컨트롤러가 만들 위젯 클래스 | 비우면 HUD를 만들지 않는다 |
| Draw Default Lock Marker | 기본 마름모 표시 | 끄면 커스텀 위젯만 표시한다 |
| Lock Marker Radius / Color | DPI 보정 위젯 공간 반지름과 색 | 기본 반지름 10, 흰색 |
| Get Main HUD | 생성된 HUD 조회 | 생성 전이나 종료 후 null |
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
| 마커는 있지만 카메라가 추적하지 않는다 | 다른 매니저를 사용하거나 카메라 배치가 없음 | [카메라 사용법](Camera.md#락온-카메라) |

## 확인 상태와 근거

기본 마커, DPI·화면 밖 처리, 폰 교체, 해제·파괴는 사용자 실행 확인 전이다. 빌드·테스트·별도 검사는 실행하지 않았다.

- [HUD 기반](../../Plugins/KataFramework/Source/KataFramework/Public/UI/KataMainHUD.h).
- [컨트롤러 연결](../../Plugins/KataFramework/Source/KataFramework/Private/Player/KataPlayerController.cpp).
- [락온 카메라와 MainHUD 기록](../devlog/2026-10-04-Lock-On-Camera-HUD.md).
