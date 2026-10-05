# 락온 카메라와 MainHUD 연결

작성: 2026-10-04  
갱신: 2026-10-04  
유형: 구현 기록  
대상: KataCamera CAM-5, KataTargeting 타겟 지점, KataFramework 컨트롤러·MainHUD  
기준: [#20](https://github.com/jaykop/Kata/issues/20), [#33](https://github.com/jaykop/Kata/issues/33) 작업 트리. 이전 IN-4 변경은 유지하며 무관한 변경은 포함하지 않는다.

> 이 기록의 카메라 구현(구도 구조체 항목, CameraData 락온 기본값, 블렌드 방식)은 [락온 카메라 블렌드·정렬 재작성](2026-10-05-Lock-On-Camera-Rework.md)으로 대체되었다.

## 배경과 결론

락온 입력과 지점 선택 API만으로는 화면 구도와 마커가 바뀌지 않았다.
사용자가 카메라 락온과 MainHUD 작업을 요청했고, HUD 범위는 기반과 락온 마커로 확정했다.

## 변경 내용과 이유

- KataCamera에 공통 구도 구조체, 별도 LockOnData와 값별 오버라이드, CameraData의 선택적 락온 기본값을 추가했다.
- 매니저 기본 Rotation·Framing Feature가 궤도 회전을 추적하고 대상 화면 위치·거리·옆 오프셋을 보정한다. 배치는 기존 레일과 Boom Arm을 유지한다.
- 지점 전환은 현재 초점·설정에서 블렌드하고 획득·해제는 구도 보정 가중치를 섞는다. 락온 데이터의 배치 참조는 StateTree보다 우선하고 해제 후 최신 상태로 돌아간다.
- TargetPoint는 타입을 제한한 UDataAsset 참조만 가진다. Framework가 카메라 타입으로 변환해 전달하므로 위성 간 직접 의존을 만들지 않는다.
- 컨트롤러가 폰 변경에 맞춰 타게팅 구독을 옮기고 종료 시 해제한다. HUD와 카메라는 같은 변경을 받는다.
- MainHUD는 기본 도형을 제공하는 UMG 기반이다. 기본 표시를 끄고 Widget Blueprint로 디자인을 바꿀 수 있다. 지점 약한 참조, DPI 보정, 화면 밖 숨김을 적용했다.
- 수동 시점 입력만 락온 중 무시한다. 공격·이동·점프를 함께 막는 것은 이번 요구가 아니므로 유지한다.
- 컨트롤 회전은 궤도 입력으로 보관한다. 최종 시선 Pitch를 저장하면 Spline 위치 매개변수가 바뀌므로 최종 구도 회전과 구분했다.

## 근거

- [매니저](../../Plugins/KataCamera/Source/KataCamera/Private/KataPlayerCameraManager.cpp): 초점·블렌드·StateTree 데이터 선택.
- [락온 Feature](../../Plugins/KataCamera/Source/KataCamera/Private/KataCameraFeature_LockOn.cpp): 회전과 화면 좌표 보정.
- [컨트롤러](../../Plugins/KataFramework/Source/KataFramework/Private/Player/KataPlayerController.cpp): 구독·HUD 생성·카메라 연결.
- [HUD](../../Plugins/KataFramework/Source/KataFramework/Private/UI/KataMainHUD.cpp): 투영과 기본 도형.

## 확인 범위와 결과

소스 작성과 현재 샘플 설정 읽기만 수행했다. 에이전트 빌드·테스트·lint·별도 코드 검사는 실행하지 않았다.
사용자 빌드·PIE·UI 확인 전이다. 과거 카메라 확인 결과를 새 변경에 적용하지 않는다.
현재 PC는 BP_BlackKnight, 사용 컨트롤러의 카메라 매니저 참조는 BP_KataTestCameraManager다.
이전 BP_SamplePC를 현재 기준으로 사용하지 않는다.

## 남은 제한과 후속 작업

2026-10-04 사용자 락온 실행에서 MainHUD의 `Points.Add(Points[0])`가 중단된 스크린샷을 받았다.
배열 내부 원소 참조를 같은 TArray의 Add에 넘겨 주소 검사에 걸리는 코드였으며, 첫 좌표를 다시 계산해 추가하도록 수정했다.
이 수정 후 재빌드·실행은 미확인이다. 화면 좌표 투영 이후 기본 도형 생성 중 중단된 결과를 락온 전체의 실행 성공으로 보지 않는다.

- 사용자 빌드 후 획득·전환·해제·대상 파괴·폰 교체와 DPI 변경을 확인한다.
- 지점별 데이터가 없으면 기본 구도로 작동한다. 새 데이터 타입의 샘플 에셋 생성은 빌드 후 가능하다.
- Shrink는 위치만 당기므로 장애물 근처의 정확한 화면 위치는 달라질 수 있다.
- 기본 원근 뷰포트 외의 강제 종횡비·레터박스·직교 카메라, 화면 밖 안내, 자동 거리 확대는 제공하지 않는다.
- CAM-4 다중 상태 확인은 실제 Status가 생길 때 수행한다는 기존 보류 결정을 유지한다.

## 연관 문서 반영

### 2026-10-05 HUD 확인과 계획 정리

사용자가 락온 마커 표시·전환·해제를 모두 확인했다. DPI·화면 밖 처리·폰 교체·대상 파괴는 별도 확인 결과가 없다. 위의 수정 직후 미확인 기록은 당시 상태이며, 마커 표시 경로는 이번 사용자 확인으로 갱신한다.
MainHUD 설계는 컨트롤러의 생성·정리와 타게팅 구독, 기본 마커와 Blueprint 교체, 약한 지점 참조, 화면 밖·뒤쪽 숨김으로 확정했다. 전역 라우터를 추가하지 않고 Framework의 UMG·SlateCore 의존성으로 구현했다.
#33 종료에 맞춰 MainHUD-Plan을 삭제하고 현재 계약은 HUD manual, 결정 이유는 이 기록으로 보존한다.

Camera·Targeting·Input manual, 새 HUD manual, Camera·MainHUD plan, README와 로컬 인계 기록에 반영했다.
#33 생성은 사용자 승인 후 수행했다. 구현 결과 댓글·라벨 변경·커밋은 아직 수행하지 않았다.
