# MainHUD MVVM 계획

작성: 2026-10-11  
갱신: 2026-10-12  
연결 이슈: [#61 MainHUD MVVM 전환: PC 상태 바와 적 체력바](https://github.com/jaykop/Kata/issues/61)  
현재 상태 근거: [MainHUD 사용법](../manual/HUD.md), [락온 카메라와 MainHUD 연결](../devlog/2026-10-04-Lock-On-Camera-HUD.md)  
대체 관계: 없음

## 목적과 현재 상태

현재 `UKataMainHUD`는 락온 마커만 그린다([#33](https://github.com/jaykop/Kata/issues/33)). HP·스태미나와 적 체력바는 범위에서 빠져 있었다.
이번 계획은 UMG Viewmodel(엔진 `ModelViewViewModel` 플러그인)로 HUD의 로직과 위젯을 분리하고, PC 상태 바와 적 체력바를 추가한다.
Widget Blueprint는 에이전트가 MCP 툴셋으로 만들고, 엔진 툴셋에 없는 기능은 보조 툴셋으로 채운다.

## 범위

- 포함: MVVM 기반(ViewModel, Resolver, HUD 서브시스템), 위젯 저작 도구 보강, PC HP·마나·스태미나 바, 피해를 입은 적과 락온 대상의 체력바.
- 제외: 보스 UI와 지역 입장 UI. 레벨 이벤트 작업을 먼저 진행한 뒤 별도로 다룬다.
- 제외: 락온 마커의 MVVM 이전, 적 이름 표시, 피해 숫자 표시.

## 계층 구조

| 계층 | 담당 | 참조할 수 있는 대상 |
|---|---|---|
| Model | GAS Attribute, `UKataDeathComponent`, 락온 지점, 피해 알림 | 없음 |
| ViewModel | FieldNotify 상태. 모델 구독과 표시 규칙(시간·대상 필터)은 HUD 서브시스템이 맡는다 | Model |
| View | WBP 레이아웃과 바인딩. 보간·투영·풀링만 얇은 C++ 위젯 기반 클래스에 둔다 | ViewModel |

위젯은 GAS·캐릭터 타입을 직접 참조하지 않는다. 적 체력바 위치처럼 화면 표시에 필요한 월드 좌표는 ViewModel이 `FVector`로 제공한다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| MVVM 사용 | 확정 | 2026-10-11 사용자 요청. 엔진 `ModelViewViewModel`(Beta)을 쓴다 |
| 적 체력바 대상 | 확정 | 피해를 입은 모든 적대 캐릭터와 락온 대상. 2026-10-11 사용자 결정 |
| 적 체력바 위치 | 확정 | 화면 공간 오버레이. MainHUD 안의 레이어가 머리 위 위치를 투영한다. 2026-10-11 사용자 결정 |
| 보스·지역 UI | 확정 | 이번 계획에서 제외한다. 2026-10-11 사용자 결정 |
| UI 레이어 | 확정 | 메뉴까지 포함한 UI 레이어는 엔진 CommonUI로 구성한다. 2026-10-12 사용자 결정. HUD-0은 MVVM만 구현하고, 루트 레이아웃·레이어 스택·입력 데이터·뷰포트 클라이언트 설정은 [#82](https://github.com/jaykop/Kata/issues/82)에서 넣는다. MainHUD는 입력을 받지 않으므로 그때 Game 레이어로 옮긴다 |
| ViewModel 소유 | 확정 | `UKataHUDSubsystem`(ULocalPlayerSubsystem)이 소유하고 모델 구독을 맡는다. 2026-10-12 사용자 결정. HUD-0은 클래스별 ViewModel 생성·보관만 넣고 모델 구독은 HUD-1부터 추가한다. 폰 교체와 락온 변경은 `AKataPlayerController`가 지금처럼 전달한다 |
| ViewModel 공급 | 제안 | 고정 ViewModel은 Resolver(`UKataHUDViewModelResolver`)로 받는다. 적 체력바 항목 ViewModel은 Manual로 두고 레이어가 주입한다 |
| ViewModel 형태 | 제안 | 중첩 ViewModel 없이 평평한 필드(`Health`, `MaxHealth` 등)와 비율 FieldNotify 함수로 둔다. Beta 도구에서 다단 경로 바인딩의 위험을 줄인다 |
| 피해 알림 | 제안 | `UKataAttributeSet_Base`가 Health에 피해를 반영한 뒤 월드 서브시스템 `UKataCombatEventSubsystem`의 네이티브 델리게이트로 대상·가해자·피해량을 알린다. 대미지 무시 태그로 Health가 줄지 않았으면 알리지 않는다 |
| 피해 알림과 라우터 | 제안 | [전역 메시지 라우터 검토](../devlog/2026-09-24-Gameplay-Message-Router-Review.md)의 재검토 조건(HUD에 전역 알림 필요)에 해당한다. 범용 라우터를 들이지 않고 타입이 정해진 확장점 하나로 해결한다. 라우터를 나중에 들이더라도 이 델리게이트를 연결 지점으로 쓴다 |
| 적 판정 | 제안 | `UKataFL_Faction::IsHostile(관찰 폰, 대상)`, 생존, Base 세트 보유, 최대 거리 이내 |
| 표시 수치 | 제안 | `Kata HUD` DeveloperSettings에 둔다. 초기값 후보: 표시 5초, 최대 8개, 최대 거리 30m, 사망 뒤 1초 후 숨김, 캡슐 위 높이 오프셋 |
| 위젯 저작 | 제안 | UMGToolSet(트리·속성·컴파일)과 엔진 MVVMToolset(ViewModel 추가·바인딩)을 쓴다. 엔진 툴셋에 없는 ViewModel 생성 방식·Resolver 지정은 `ProjectKataEditor`의 보조 툴셋 `UKataMVVMAuthoringToolset`이 맡는다 |
| 위젯 에셋 위치 | 제안 | `Content/KataSample/UI/`. 플러그인은 기본 락온 마커처럼 C++ 기반 클래스만 제공한다 |

## 클래스 구성

| 대상 | 모듈 | 역할 |
|---|---|---|
| `UKataCombatEventSubsystem` | KataFramework | 피해 반영 알림 (대상, 가해자, 피해량) |
| `UKataVM_PlayerStatus` | KataFramework | HP·마나·스태미나의 현재·최대값, 비율 함수, 관찰 폰 유무 |
| `UKataVM_EnemyHealth` | KataFramework | 적 하나의 체력과 비율, 락온 여부, 머리 위 월드 위치 조회 |
| `UKataVM_EnemyHealthList` | KataFramework | 표시 중인 항목 목록 |
| `UKataHUDSubsystem` | KataFramework | ViewModel 소유, ASC·사망·피해 구독, 만료 처리 |
| `UKataHUDViewModelResolver` | KataFramework | 위젯의 로컬 플레이어에서 ViewModel 조회 |
| `UKataHUDSettings` | KataFramework | 적 체력바 표시 수치 |
| `UKataGaugeWidget` | KataFramework | 비율 설정 함수(바인딩 대상), 지연 바 보간 |
| `UKataEnemyHealthBarLayer` | KataFramework | 항목 위젯 풀링, 투영, 화면 밖·뒤쪽 숨김 |
| `UKataMVVMAuthoringToolset` | ProjectKataEditor | ViewModel 생성 방식·Resolver 지정 MCP 도구 |
| `WBP_KataGauge`, `WBP_PlayerStatus`, `WBP_EnemyHealthBar`, `WBP_MainHUD` | `Content/KataSample/UI` | 레이아웃과 바인딩 |

수정 대상: `AKataPlayerController`(서브시스템 전달), `UKataAttributeSet_Base`(피해 알림), KataFramework `Build.cs`·`.uplugin`(MVVM 의존),
`ProjectKata.uproject`(MVVMToolset, Editor 한정), `ProjectKataEditor`(툴셋 등록), 샘플 `BP_PlayerController`(Main HUD Class 지정).

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| HUD-0 | 높음 | MVVM 의존 추가, MVVMToolset 활성, 보조 툴셋, Resolver 골격과 ViewModel 보관만 하는 HUD 서브시스템. 시험 WBP로 생성 방식 지정·바인딩·컴파일을 확인한다 | 사용자 빌드·에디터 재시작 | 시험 WBP가 Resolver ViewModel 바인딩으로 컴파일된다 |
| HUD-1 | 높음 | PlayerStatus ViewModel, HUD 서브시스템의 모델 구독, 게이지 위젯, WBP 저작, 샘플 컨트롤러 설정 | HUD-0 | PIE에서 세 바가 Attribute 변화와 폰 교체를 따른다 |
| HUD-2 | 높음 | 피해 알림, 적 추적·만료·사망 제거, 락온 대상 고정, 레이어와 항목 WBP | HUD-1 | 적이 피해를 입으면 바가 뜨고 표시 시간 뒤 사라진다. 락온 중에는 유지된다 |

## 영향과 제한

- KataFramework가 `ModelViewViewModel`(Beta) 런타임에 의존하므로 Game 타깃에도 포함된다. 코어 `Kata`와 위성 플러그인은 영향이 없다.
- CommonUI 채택은 HUD-0~2의 구현 범위를 바꾸지 않는다. MainHUD는 지금처럼 `AKataPlayerController`가 화면에 추가하며, CommonUI 위젯도 `UUserWidget` 파생이므로 ViewModel 바인딩은 그대로 쓴다.
  CommonUI 의존은 #82에서 `KataFramework`에 추가하며, 소비 프로젝트는 `GameViewportClientClassName`을 `CommonGameViewportClient` 파생으로 지정해야 한다.
- MVVMToolset은 Experimental·Editor 전용이다. 보조 툴셋은 `ProjectKataEditor`에만 두고 Kata 플러그인은 참조하지 않는다.
- 저작 도구로 생성 방식 지정이나 바인딩이 막히면 View의 C++ 기반 클래스가 FieldNotify를 직접 구독하는 방식으로 바꾼다. ViewModel 계층은 그대로 유지한다.
- 피해 알림은 월드 범위다. 레벨을 이동하면 HUD 서브시스템이 새 월드의 알림을 다시 구독한다.
- 항목 ViewModel은 대상 액터를 약한 참조로 보관하고, 제거 시 ASC·사망 구독을 해제한다.

## 사용자 확인 항목

- 각 단계의 빌드와 PIE 확인은 사용자가 맡는다. 에이전트는 MCP 저작 결과(컴파일 성공 여부)만 보고한다.
- 확인 시나리오: 피해·회복·Max 변화, 스태미나 소모·회복, 폰 교체와 사망 뒤 재시작, 여러 적 동시 피격·만료·사망, 락온 전환·해제, 화면 밖·뒤쪽 대상.

## 완료 시 갱신할 문서

- 연결 이슈: 구현·확인 상태.
- [MainHUD 사용법](../manual/HUD.md): MVVM 구조, 위젯 교체 방법, 설정.
- devlog: MVVM 채택과 저작 도구 결정, 실제 결과.
- [문서 목록](../README.md): 이 plan과 새 devlog 링크.
