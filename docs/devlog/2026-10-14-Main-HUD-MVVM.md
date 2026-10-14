# MainHUD MVVM 전환

작성: 2026-10-14  
갱신: 2026-10-14  
유형: 결정 기록  
대상: `UKataHUDSubsystem`, `UKataHUDViewModelResolver`, `UKataVM_PlayerStatus`, `UKataVM_EnemyHealth`·`UKataVM_EnemyHealthList`, `UKataGaugeWidget`, `UKataEnemyHealthBarLayer`, `UKataCombatEventSubsystem`, `UKataHUDSettings`, 샘플 `ProjectKataEditor`의 `UKataMVVMAuthoringToolset`  
기준: [#61](https://github.com/jaykop/Kata/issues/61) 종료 시점. 하위 [#67](https://github.com/jaykop/Kata/issues/67) HUD-0, [#68](https://github.com/jaykop/Kata/issues/68) HUD-1, [#69](https://github.com/jaykop/Kata/issues/69) HUD-2. 삭제한 `docs/plan/Main-HUD-MVVM-Plan.md`(2026-10-11 작성, 2026-10-13 갱신)의 결정을 옮긴 기록

현재 사용법은 [MainHUD 사용법](../manual/HUD.md)을 따른다.

## 배경과 결론

`UKataMainHUD`는 락온 마커만 그리고 있었다([#33](https://github.com/jaykop/Kata/issues/33)). HP·스태미나와 적 체력바를 더하면서 HUD의 로직과 위젯을 UMG Viewmodel(엔진 `ModelViewViewModel`)로 분리했다.
HUD-0에서 MVVM 기반과 저작 도구, HUD-1에서 PC 상태 바, HUD-2에서 적 체력바를 구현했다. 보스 UI와 지역 입장 UI는 범위에서 뺐다.

## 계층과 구성

| 계층 | 담당 |
|---|---|
| Model | GAS Attribute, `UKataDeathComponent`, 락온 지점, 피해 알림(`UKataCombatEventSubsystem`) |
| ViewModel | FieldNotify 상태. 모델 구독과 표시 규칙(시간·대상 필터)은 `UKataHUDSubsystem`(ULocalPlayerSubsystem)이 맡는다 |
| View | WBP 레이아웃과 바인딩. 보간(`UKataGaugeWidget`)·투영과 풀링(`UKataEnemyHealthBarLayer`)만 얇은 C++ 위젯 기반 클래스에 둔다 |

위젯은 GAS·캐릭터 타입을 직접 참조하지 않는다. 적 체력바 위치처럼 화면에 필요한 월드 좌표는 ViewModel이 `FVector`로 제공한다.

## 주요 결정과 이유

| 결정 | 시점·근거 | 이유 |
|---|---|---|
| UMG Viewmodel을 쓴다 | 2026-10-11 사용자 요청 | 엔진 기본 바인딩 체계로 로직과 위젯을 나눈다. `KataFramework`가 `ModelViewViewModel`(Beta) 런타임에 의존하며 코어·위성 플러그인은 영향이 없다 |
| ViewModel은 HUD 서브시스템이 소유 | 2026-10-12 사용자 결정 | 위젯을 다시 만들어도 같은 인스턴스에 바인딩되고, 모델 구독을 한곳에서 정리한다. 폰 교체·락온 변경은 `AKataPlayerController`가 전달한다 |
| 고정 ViewModel은 Resolver, 적 항목은 Manual | HUD-0·1 구현, HUD-2 구현 | PC 상태처럼 하나뿐인 값은 Resolver로 찾는다. 수가 바뀌는 적 항목은 레이어가 위젯마다 `UMVVMView::SetViewModelByClass`로 넣는다 |
| 평평한 필드와 비율 FieldNotify 함수 | 2026-10-13 HUD-1 계획 사용자 승인 | 중첩 ViewModel 없이 `Health`·`MaxHealth`와 `GetHealthRatio`처럼 둔다. Beta 도구에서 다단 경로 바인딩의 위험을 줄인다 |
| 메뉴까지 포함한 UI 레이어는 CommonUI | 2026-10-12 사용자 결정 | HUD-0~2 범위는 바꾸지 않는다. 루트 레이아웃과 레이어 스택은 [#82](https://github.com/jaykop/Kata/issues/82)에서 넣는다 |
| 위젯 저작은 MCP 툴셋, 부족분은 보조 툴셋 | HUD-0 | UMGToolSet과 엔진 MVVMToolset을 쓰되, 엔진 `CreateViewBinding`은 양쪽을 프로퍼티로만 찾아 함수 바인딩(FieldNotify 함수 → 위젯 설정 함수)과 생성 방식 지정을 못 한다. 이를 샘플 Editor 모듈의 `UKataMVVMAuthoringToolset`이 맡는다 |
| 피해 알림은 타입이 정해진 월드 서브시스템 | HUD-2 | [전역 메시지 라우터 검토](2026-09-24-Gameplay-Message-Router-Review.md)의 재검토 조건(HUD에 전역 알림 필요)에 해당했지만, 범용 라우터 대신 델리게이트 하나로 해결했다. 라우터를 들이더라도 이 델리게이트를 연결 지점으로 쓴다 |
| 실제로 줄어든 Health만 알린다 | HUD-2 | `UKataAttributeSet_Base`가 Health를 깎은 뒤 감소량만 알린다. 대미지 무시로 줄지 않았거나 이미 0이면 알리지 않아, HUD가 무적 대상에 바를 띄우지 않는다 |
| 적 체력바 대상과 출처 | 2026-10-11, 2026-10-14 사용자 결정 | 피해를 입은 적대 캐릭터와 락온 대상. 출처는 설정(Player Only 기본 / Any)으로 고른다. 광역기·몬스터끼리의 공격을 쓰는 게임에서 바꿀 수 있게 했다 |
| 락온 대상은 맞지 않아도 표시 | 2026-10-14 사용자 결정 | 락온 중 만료하지 않고 해제 시점부터 표시 시간을 잰다. 대상을 직접 고른 것이라 거리 제한도 두지 않는다 |
| 위치는 캡슐 꼭대기 + 설정 오프셋 | 2026-10-14 사용자 결정 | 모든 캐릭터에 바로 동작한다. 캐릭터별 오프셋은 머리가 캡슐보다 훨씬 높은 대상이 생기면 추가한다 |
| 표시 수치는 DeveloperSettings | 2026-10-14 사용자 결정 | Kata HUD 설정에 표시 5초, 최대 8개, 30m, 사망 뒤 1초, 높이 30cm. 개수 상한은 하한만 두고 프로젝트가 올릴 수 있다 |
| 만료는 실시간 | HUD-2 | 히트스톱·시간 배율로 바가 오래 남지 않게 한다. 게이지 지연 바도 위젯 Tick 시간이라 같은 성질이다 |
| 상한 초과 시 가장 먼저 사라질 항목 제거 | HUD-2 | 최근에 맞은 대상의 바가 남는다. 락온 항목은 지우지 않는다 |

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| HUD-0 MVVM 기반 | 사용자 빌드, MCP 시험 WBP 컴파일 | Resolver ViewModel 바인딩으로 컴파일 | - |
| HUD-1 상태 바 | 사용자 PIE(2026-10-13) | 부활 후 HP 반영, 스태미나 소모, HP 지연 감소 | 최대값 변경, HP·마나 회복 GE |
| HUD-2 적 체력바 | 사용자 빌드·PIE(2026-10-14) | 피해 표시·만료, 락온 표시·유지·해제, 여러 적, 사망 숨김, 화면 밖·뒤쪽 숨김, 거리 제거, 출처 설정, 재시작 | 많은 수의 바를 띄운 상태의 성능 |

## 남은 제한과 후속 작업

- 캐릭터별 바 높이는 없다.
- 보스 UI, 지역 입장 UI, 락온 마커의 MVVM 이전, 적 이름·피해 숫자 표시는 범위 밖이다.
- CommonUI 루트 레이아웃은 [#82](https://github.com/jaykop/Kata/issues/82).

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [#61](https://github.com/jaykop/Kata/issues/61), [#69](https://github.com/jaykop/Kata/issues/69) | 결과 댓글과 종료는 사용자 확인 후 |
| [MainHUD 사용법](../manual/HUD.md) | 적 체력바 단락, Kata HUD 설정, 문제 해결, 확인 상태 |
| `docs/plan/Main-HUD-MVVM-Plan.md` | #61 종료와 함께 삭제. 결정은 이 기록으로 옮겼다 |
