# 궤적(Trail) 태스크와 공용 소켓 경로 샘플러

작성: 2026-10-15  
갱신: 2026-10-15  
유형: 구현 기록 / 결정 기록 / 진단  
대상: `KataFramework`의 `KataHitPoseSampler`·`UKataHitSubsystem`·`UKataTask_Trail`, [연출 태스크 사용법](../manual/FX-Tasks.md)  
기준: #108 커밋 3e4fac8 이후 작업 트리. #109 변경은 이 기록과 같은 커밋에 들어간다

## 배경과 결론

칼날·팔다리가 지나간 자리를 Niagara 리본으로 그리되, 저프레임에서도 판정과 같은 호를 따르고 한 프레임보다 짧은 구간도 정확히 자르기를 원했다.
조사해 보니 Hit Trace가 이미 같은 문제를 풀고 있었다. 프레임 사이 포즈를 몽타주로 재샘플링하고, 직전 포즈 기록과 구간 경계 클리핑을 쓴다.
다만 이 흐름이 `UKataHitSubsystem::ProcessHitBox` 안에 묶여 있었다. 그래서 두 단계로 나눴다.

1. [#108](https://github.com/jaykop/Kata/issues/108): 소켓 경로 계산을 공용 샘플러로 분리하고, 같은 조사에서 찾은 Hit 판정 진단 두 건을 고쳤다.
2. [#109](https://github.com/jaykop/Kata/issues/109): 공용 샘플러로 샘플을 만들어 Niagara User 파라미터 배열로 넘기는 `UKataTask_Trail`을 추가했다.

구현 뒤 확인 단계에서 액션 편집기 프리뷰에서만 궤적이 그려지지 않는 문제가 나왔다.
원인은 Niagara 배열 함수 라이브러리의 에디터 전용 동작이었다. 게임 월드가 아닐 때만 값을 쓰는 경로를 바꿔 해결했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 |
|---|---|---|
| 소켓 경로 계산 | `ProcessHitBox` 안의 람다(`PoseAt`, `AlphaAt`)라 다른 기능이 쓸 수 없었다 | `KataHitPoseSampler::FFramePath`(Build·Commit·AlphaAt·PoseAt)와 `FKataSocketPathHistory`로 분리. 판정과 궤적이 같은 입력으로 경로를 만든다(#108) |
| 스켈레탈 무기의 재샘플링 | 애님 인스턴스가 없는 스켈레탈 무기는 자기 자신을 샘플링 메시로 골라 몽타주가 없었다 | 애님 인스턴스·리더 포즈가 없으면 부착 부모의 스켈레탈 메시로 올라간다(#108) |
| 첫 프레임 되짚기의 배율 | `InitializeFromCurrent` 분기만 Avatar 배율로 직전 시각을 계산했다 | 다른 분기와 같이 Kata 실행 배율을 쓴다(#108) |
| 궤적 연출 | 없었다 | `UKataTask_Trail`(표시 이름 `Kata Task: Trail`). Subsystem이 판정 뒤·포즈 기록 전에 궤적을 처리한다(#109) |
| 프리뷰에서 궤적 미표시 | PIE에서는 그려지고 프리뷰에서는 입자가 0개였다 | 진단 아래. 게임 월드가 아니면 배열을 컴포넌트의 DI에 바로 쓴다(#109) |

시험 중 콤보 액션의 앞부분 판정이 20fps에서 빠지는 문제도 나왔다. 이 문제는 Kata 시각이 몽타주보다 한 프레임 늦던 별도 원인이라 [#115](https://github.com/jaykop/Kata/issues/115)로 나눠 고쳤다([기록](2026-10-15-Start-Frame-Montage-Sync.md)).

## 주요 결정과 이유

- **C++가 샘플을 만들고 Niagara는 그리기만 한다.** Niagara 단독(부착 + 보간 스폰 + 리본 Tessellation)은 프레임 사이를 직선으로 잇는 추정이라 저프레임 호를 따르지 못한다.
  엔진 `AnimNotifyState_Trail`은 Cascade 기반이고 Kata 타임라인 밖이라 쓰지 않았다.
- **샘플 전달은 User 파라미터 배열 계약으로 한다.** 계약 이름은 `KataTrail` 접두사를 쓴다. 출력은 Count와 위치·폭·방향·리본 번호·연결 순서·나이·칼날 비율 배열이고, 입력은 샘플 간격 두 값이다.
  입자 하나는 한 샘플 포즈의 이웃한 소켓 구간 하나다. 그래서 칼날의 중점과 길이, 방향만으로 리본 폭과 Custom Side Vector가 정해진다.
  에셋에 없는 출력은 등록할 때 한 번 조사해 건너뛰고, 데이터 검증에서 경고한다.
- **소켓은 태스크가 이름 배열로 받는다**(2개 이상, 사용자 제안). Hit Box Preset을 참조하지 않아 판정과 다른 소켓(예: 칼등)으로도 그릴 수 있다.
  기준 메시는 Spawn VFX와 같은 규칙(Weapon·Character)으로 고른다. 몸 소켓에도 쓰므로 이름을 Weapon Trail에서 Trail로 바꿨다(사용자 결정).
- **샘플 수는 거리와 각도 기준 중 큰 쪽**이다. 기본 10cm·10°, 한 프레임 최대 64개다. 간격은 Niagara 에셋 User 파라미터로 이펙터가 정한다. 0 이하는 기본값으로 본다. 새로 추가한 파라미터의 기본값 0이 최소 간격 1로 바뀌어 매 프레임 상한까지 샘플을 만들던 문제를 막기 위해서다.
- **실행 위치는 `UKataHitSubsystem` Tick의 판정 뒤·포즈 기록 전**이다. 직전 포즈 기록이 Subsystem Tick 번호에 묶여 있어 별도 Subsystem으로 나누면 기록 순서가 어긋난다.
- **컴포넌트는 Solo 모드, 마지막 틱 그룹, Avatar 시간 배율로 실행한다.** Solo여야 컴포넌트별 `SetCustomTimeDilation`이 적용돼 히트스톱 동안 궤적도 멈춘다. 마지막 틱 그룹이어야 같은 프레임에 넘긴 샘플을 그 프레임에 그린다.
  기준 메시에 붙여 생성해 Avatar가 소유하게 하고, 풀링은 Spawn VFX와 같은 이유로 쓰지 않는다.
- **정상 완료만 종료 시각까지 그린다.** 중단·취소·Kata 종료는 더 넘기지 않고 방출을 멈춘다. 그리기에 실패해도 태스크를 일찍 끝내지 않는다(연출 태스크 규칙).

## 프리뷰 미표시 진단

| 단계 | 확인한 것 | 결과 |
|---|---|---|
| C++ 경로 | 프리뷰 월드에서 등록·샘플 전달·컴포넌트 상태(등록·활성·렌더) | 모두 정상. 같은 태스크에 User 파라미터가 없는 다른 Niagara 에셋을 넣으면 프리뷰에서도 그려졌다 |
| 실행 방식 | Solo·마지막 틱 그룹·월드 생성과 부착 생성을 각각 바꿈 | 변화 없음 |
| 에셋 | 배열 읽기 모듈 끄기, 재질 지정, 새로 만든 궤적 에셋 | 변화 없음. User 배열을 쓰는 궤적 에셋은 모두 프리뷰에서 틱 0 |
| 인스턴스 | 값을 쓰기 전과 후의 시스템 인스턴스 비교 | 쓰기 전에는 한 번 틱했지만, 쓰는 순간 인스턴스가 매 프레임 새로 만들어졌다 |
| 호출별 | 쓰기 호출마다 인스턴스 비교 | `SetNiagaraArray*` 배열 7개 호출 하나하나가 인스턴스를 바꿨다. `SetVariableInt`는 바꾸지 않았다 |

엔진 소스로 원인을 확인했다. `NiagaraDataInterfaceArrayFunctionLibrary.cpp`의 `SetNiagaraArray`는 에디터 빌드에서 `UNiagaraDataInterfaceArray::CreateAndSetVariant`도 부른다.
이 함수는 월드가 게임 월드가 아니면 DI를 복제해 컴포넌트의 편집용 오버라이드로 저장하고, 컴포넌트가 활성이면 `ReinitializeSystem()`을 호출한다.
레벨에 배치한 컴포넌트에 값을 남기기 위한 동작인데, 액션 편집기 프리뷰도 게임 월드가 아니어서 매 프레임 쓰는 궤적은 인스턴스가 이어서 돌지 못했다. PIE는 게임 월드라 이 경로를 타지 않는다.

해결 방법으로 다음을 검토했다.

- **채택**: 게임 월드에서는 라이브러리를 그대로 쓴다. 게임 월드가 아니면 `UNiagaraFunctionLibrary::GetDataInterface`로 컴포넌트의 배열 DI를 찾아 `GetArrayReference()`에 바로 쓴다. 위치는 라이브러리처럼 LWC 시뮬레이션 좌표로 바꾼다.
  컴포넌트는 활성화할 때 에셋의 DI를 복제해 가지므로(`UNiagaraComponent::CopyParametersFromAsset`) 에셋 값은 바뀌지 않는다.
- **채택하지 않음**: 라이브러리의 실행 쓰기 부분(`FProxyType::SetArrayData`)만 직접 호출하는 방법. 형별 도우미 특수화가 Niagara 모듈의 Private 헤더에 있어 다른 모듈에서 컴파일되지 않았다.

## 근거

- [KataHitSubsystemTrail.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/HitTrace/KataHitSubsystemTrail.cpp): `WriteArray`의 월드 분기, `ProcessTrail`의 구간 클리핑·샘플 수·입자 배열, 방출 정지.
- [KataTask_Trail.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Tasks/KataTask_Trail.cpp): 설정 오류, 계약 검증, 부착 생성과 Solo·틱 그룹·배율 설정, 정상 완료만 끝까지 그리는 닫기.
- [KataTrailParameters.h](../../Plugins/KataFramework/Source/KataFramework/Private/HitTrace/KataTrailParameters.h): User 파라미터 이름, 기본 간격, 샘플 상한.
- [KataHitPoseSampler.h](../../Plugins/KataFramework/Source/KataFramework/Private/HitTrace/KataHitPoseSampler.h): `FFramePath`와 샘플링 메시 선택.
- UE 5.8 엔진 소스 `Plugins/FX/Niagara/Source/Niagara/Private`: `NiagaraDataInterfaceArrayFunctionLibrary.cpp`의 `SetNiagaraArray`, `NiagaraDataInterfaceArray.cpp`의 `CreateAndSetVariant`, `NiagaraComponent.cpp`의 `SetVariableInt`·`SetParameterOverride`·`CopyParametersFromAsset`.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| #108 공용 샘플러와 진단 1·3 | 2026-10-15 사용자 Editor 빌드와 Hit Trace 회귀 확인 | 정상·20fps 판정 유지 | 애님 인스턴스 없는 스켈레탈 무기 실물(샘플 대검은 스태틱) |
| Trail 표시 | 2026-10-15 사용자 Editor 빌드, 샘플 `KA_BlackKnight_Attack01`과 확인용 에셋 `NS_Test_WeaponTrail` | 프리뷰·PIE 모두 표시 | GPU 이미터 |
| Trail 동작 | 같은 날 사용자 실행 | `t.MaxFPS 20`의 호, 한 프레임보다 짧은 구간, 회피 캔슬 시점 정지, 히트스톱 정지, `slomo 0.2`의 앞끝·꼬리, `KataTrailBladeRanges` 누락 경고 | 소켓 3개 이상, Character 메시 소켓, 쿠킹 빌드 |
| 프리뷰 원인 | 진단 로그(확인 뒤 삭제)와 엔진 소스 대조 | 위 진단표 | 없음 |

## 남은 제한과 후속 작업

- 프리뷰 경로는 라이브러리의 인스턴스 잠금과 GPU 동기화 표시를 거치지 않는다. 그래서 프리뷰에서는 CPU 이미터만 갱신되고, 이미터 스크립트가 배열에 쓰지 않는다고 가정한다. 사용법 문서의 제한 표에 적었다.
- 확인용 에셋은 칼날 비율 배열이 없고 리본 번호를 렌더러 Ribbon ID에 연결하지 않았다. 소켓 2개 기준으로만 확인했다. 이펙터용 템플릿과 칼날 폭 방향 UV는 [#110](https://github.com/jaykop/Kata/issues/110)에서 다룬다.
- 같은 서브스텝에서 여러 HurtBox가 닿을 때 시간 순서로 부위를 고르는 Hit 판정 진단 2는 [#111](https://github.com/jaykop/Kata/issues/111)로 남겼다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#109](https://github.com/jaykop/Kata/issues/109) | 구현·사용자 확인 결과와 후속 이슈. 게시는 사용자 확인 후 |
| [연출 태스크 사용법](../manual/FX-Tasks.md) | Trail 설정·실행 규칙·Niagara 계약 표, 프리뷰 동작, 프리뷰 GPU 이미터 제한, 확인 상태 |
| [Hit Trace 사용법](../manual/Hit-Trace.md) | #108 커밋에서 공용 샘플러 반영. 이번 변경은 영향 없음 |
| [런타임 사용법](../manual/Runtime-Usage.md) | 연출 태스크 목록에 Trail 언급 |
