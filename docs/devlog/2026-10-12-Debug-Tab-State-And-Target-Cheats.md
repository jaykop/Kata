# Kata Debug 탭 상태 저장과 대상 무적·대미지 무시

작성: 2026-10-12  
갱신: 2026-10-12  
유형: 구현 기록  
대상: 샘플 `ProjectKataEditor`의 Kata Debug 탭, KataFramework 치트 확장  
기준: 이 기록과 같은 커밋의 작업 트리. [#55](https://github.com/jaykop/Kata/issues/55)의 후속이며 별도 이슈는 없다

## 배경과 결론

사용자는 두 가지를 요청했다. 하나는 Kata Debug 탭이 마지막 상태를 기억하는 것이다. 다른 하나는 NPC를 무적이나 피격 가능한 대미지 무시 상태로 스폰하거나, 치트를 선택한 대상에게도 걸 수 있게 하는 것이다.
탭은 마지막 패널과 플레이어 치트 토글을 사용자별 설정에 저장한다. 대상 보호는 치트 패널의 버튼과 스포너의 Protection 옵션 두 가지로 제공한다.
작업 중 PIE를 끈 뒤 패널 안내 문구가 단어마다 줄바꿈되는 SlateIM 문제를 찾아 우회했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| 마지막 패널 | 탭을 열 때마다 정렬상 첫 패널(Cheats)로 열렸다 | `UKataDebugUserSettings::ActivePanelClass`에 저장하고 다음에 그 패널로 연다 |
| 플레이어 치트 토글 | PIE마다 PlayerController와 치트 확장이 새로 만들어져 꺼졌다 | 치트 패널이 보이는 동안 값을 저장하고, `FEditorDelegates::PostPIEStarted`에서 탭이 닫혀 있어도 다시 건다 |
| 대상 무적·대미지 무시 | `KataGod`·`KataNoDamage`는 PC 전용이었다 | `UKataFrameworkCheatExtension::SetActorInvincible`·`SetActorIgnoreDamage`를 추가했다. 치트 패널 Selected Targets 버튼과 스포너 Protection이 쓴다 |
| 대상 버튼 배치 | 버튼이 두 줄로 나뉘었다 | Selected Targets의 버튼 다섯 개를 한 줄로 합쳤다 |
| PIE 종료 뒤 안내 문구 | "Start PIE to use cheats."가 단어마다 줄바꿈되었다. 에디터를 처음 켰을 때는 정상이었다 | 패널이나 PIE 여부가 바뀐 프레임에 내용 컨테이너 종류를 바꿔 패널 영역을 새로 만든다 |

## 주요 결정과 이유

- 대상 보호는 기존 치트 태그(`Cheat.Invincible`·`Cheat.IgnoreDamage`)를 대상 ASC에 Loose Tag로 건다. 히트 판정과 AttributeSet이 이미 대상의 ASC로 이 태그를 확인하므로 판정 쪽은 바꿀 필요가 없다.
- 대상이 이 PC의 Pawn이면 태그 대신 PC 토글을 바꾼다. PC Pawn의 태그는 빙의가 바뀔 때마다 토글 값으로 다시 맞추므로 태그만 바꾸면 토글과 어긋난다.
- 스포너 Protection은 치트 패널과 같은 함수를 쓴다. 생성 뒤 치트 패널에서 같은 대상을 끄고 켤 수 있다.
- 플레이어 토글 복원은 탭이 아니라 `UKataDebugPanel::OnPlaySessionStarted` 훅으로 패널에 맡긴다. 저장 상태를 다시 거는 일은 그 상태를 만든 패널이 아는 일이다.
- 대상 보호를 콘솔 명령으로는 추가하지 않았다. 요청 범위가 Kata Debug 탭이었다.

### SlateIM 슬롯 정렬 갱신 누락

SlateIM은 슬롯 정렬이 바뀌었는지를 루트 전체 위젯의 일련번호로 저장한 해시와 비교한다. 그런데 위젯과 슬롯은 컨테이너 안의 순번으로 재사용한다.
PIE 중에는 위쪽 Target 목록의 위젯 수만큼 패널 위젯의 일련번호가 밀린다. PIE를 끄면 안내 문구는 패널 컨테이너의 첫 슬롯을 재사용한다. 그 슬롯에는 `Player` 제목의 좁은 정렬이 있었다.
그런데 안내 문구의 일련번호에는 지난 프레임에 Target 영역의 "(none)" 문구가 있었고, 둘은 정렬값(Padding 4·2, HAlign Fill)이 같았다. 그래서 정렬 갱신을 건너뛰고 좁은 정렬이 남았다.

엔진 플러그인 소스를 고치지 않고 탭에서 우회했다. 패널 인덱스와 PIE 여부로 만든 키가 바뀐 프레임에는 내용 영역 컨테이너를 세로 스택과 Border 사이에서 바꾼다.
컨테이너 종류가 다르면 SlateIM이 하위 위젯을 모두 새로 만들고 슬롯 정렬도 다시 적용한다. 대안으로 문구의 정렬값을 일부러 다르게 하는 방법이 있었지만, 다른 패널에서 같은 문제가 다시 생길 수 있어 택하지 않았다.

## 근거

- [KataFrameworkCheatExtension.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Cheats/KataFrameworkCheatExtension.cpp): `SetActorCheatTag`, PC Pawn 분기.
- [KataDebugTab.cpp](../../Source/ProjectKataEditor/KataDebugTab.cpp): 패널 저장·복원, `HandlePostPIEStarted`, 내용 컨테이너 교체.
- [KataDebugPanel_Cheats.cpp](../../Source/ProjectKataEditor/KataDebugPanel_Cheats.cpp): 토글 저장·복원, 대상 보호 버튼.
- [KataDebugPanel_Spawner.cpp](../../Source/ProjectKataEditor/KataDebugPanel_Spawner.cpp): `ApplyProtection`.
- [KataDebugUserSettings.h](../../Source/ProjectKataEditor/KataDebugUserSettings.h): 저장 항목.
- 엔진 SlateIM `SlateIMWidgetScope.h`의 `bIsSlotDirty` 판정과 `SlateIMManager.cpp`의 `FRootNode::GetWidgetHash`: 일련번호 기준 해시 비교.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 상태 저장 | 사용자 Editor 빌드·PIE | God을 켠 뒤 PIE를 다시 시작해도 켜져 있었다. 에디터를 다시 열어도 마지막 패널로 열렸다 | Infinite Stamina·Hide From AI 복원은 항목별로 보고되지 않았다 |
| 대상 보호 | 사용자 PIE | Protection Ignore Damage로 스폰한 NPC가 피격 반응은 하고 Health는 줄지 않았다. 치트 패널에서 `Take Damage`로 보였다 | Protection Invincible은 항목별로 보고되지 않았다 |
| 버튼 배치, PIE 종료 뒤 문구 | 사용자 에디터 UI | 한 줄 배치와 PIE 종료 뒤 한 줄 문구를 확인했다 | 없음 |

## 남은 제한과 후속 작업

- 다른 액터에 건 보호 태그는 그 액터가 사라질 때까지 남고, 재생성 뒤 다시 걸지 않는다.
- SlateIM 우회는 패널 전환과 PIE 시작·종료에만 적용된다. 패널 안에서 내용이 통째로 바뀌는 경우에도 같은 증상이 나올 수 있다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#55](https://github.com/jaykop/Kata/issues/55) | 닫힌 이슈의 후속으로 커밋 메시지에서만 참조한다. 이슈 게시는 하지 않았다 |
| [Kata Debug 탭 사용법](../manual/Debug-Tab.md) | 패널 저장, 토글 복원, 대상 보호 버튼, `OnPlaySessionStarted`, SlateIM 주의 사항 |
| [치트 사용법](../manual/Cheats.md) | 다른 액터의 무적·대미지 무시, PIE 재시작 시 토글 복원 |
| [런타임 스포너 사용법](../manual/Runtime-Spawner.md) | Protection 항목 |
