# 런타임 스포너 사용법

갱신: 2026-10-17  
대상: 샘플 Editor 모듈 `ProjectKataEditor`의 Kata Debug 탭 Spawner 패널  
적용 기준: [#49](https://github.com/jaykop/Kata/issues/49)(첫 구현), [#55](https://github.com/jaykop/Kata/issues/55)(Kata Debug 탭의 패널로 통합), [#113](https://github.com/jaykop/Kata/issues/113)(반복·보호를 더미 컴포넌트로 통합)  
확인 상태: 2026-10-10 단독 탭 시절의 기능 동작을 사용자가 확인했다. 2026-10-11 Kata Debug 탭의 패널로 옮기고 Circle 배치를 바꾼 뒤에도 사용자가 Editor 빌드·PIE에서 동작을 확인했다. 2026-10-12 추가한 Protection(Ignore Damage)도 사용자가 PIE에서 확인했다. 2026-10-15 반복과 보호를 `UKataDummyComponent`로 옮기고 Return To Start를 더했다(#113). 2026-10-15 사용자가 Editor 빌드와 PIE에서 패널의 반복·보호와 Return To Start를 확인했다.

## 목적과 준비

PIE 중 플레이어 정면에 NPC를 즉석으로 생성해 전투와 AI 동작을 확인한다. 생성한 NPC가 기본 AI를 실행하게 하거나, 아무것도 하지 않게 하거나, 고른 Action·Graph를 플레이어를 대상으로 반복하게 할 수 있다.
[Kata Debug 탭](Debug-Tab.md)의 패널이다. 샘플 프로젝트 전용이며 Kata 플러그인에는 들어 있지 않다. Editor 타깃에서만 빌드되고 Game 빌드에는 포함되지 않는다.
샘플 `.uproject`가 Experimental 플러그인인 SlateIM을 Editor 타깃에서 켠다. 에디터를 열 때 실험 플러그인 경고가 나올 수 있다.

## 사용 순서

1. Project Settings > Editor > Kata Runtime Spawner의 `NPC Tables`에 행을 고를 NPC 테이블을 넣는다. 샘플은 `DT_NPC_Enemies`, `DT_NPC_Bosses`를 기본으로 넣어 두었다(DefaultEditor.ini).
2. PIE를 시작한다.
3. Tools 메뉴의 Kata 섹션에서 `Kata Debug`를 열고 `Panel` 드롭다운에서 `Spawner`를 고른다. 콘솔 명령 `Kata.RuntimeSpawner`는 탭을 이 패널로 바로 열고, 이미 열려 있으면 닫는다.
4. Character에서 테이블과 행을 고른다. 행 버튼을 누르면 검색창이 있는 선택 메뉴가 열린다.
5. Placement에서 거리, Yaw Offset, 개체 수, 패턴을 정한다. Line이면 간격도 정한다.
6. AI에서 모드를 고른다. 반복 모드면 Action 또는 Graph를 고르고 반복 간격과 `Return To Start`를 정한다. Graph를 반복할 때는 필요하면 Entry Trigger도 고른다.
7. 필요하면 Spawn의 `Protection`에서 생성한 NPC에 무적 또는 대미지 무시 보호를 건다.
8. `Spawn`을 누른다. 이 패널로 만든 NPC는 `Despawn All`로 한꺼번에 지운다.

## 주요 설정과 실행 계약

| UI 항목 | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| NPC Table | 설정에 넣은 테이블 중 하나 | 행 구조가 `FKataNPCCharacterRow` 계열이 아닌 테이블은 경고 로그와 함께 목록에서 뺀다. 데이터 컬렉션에 등록되지 않은 테이블도 쓸 수 있다 |
| Settings | Project Settings의 도구 설정 화면을 연다 | - |
| Row | 고른 테이블의 행 | 저장된 행이 테이블에 없으면 첫 행을 고른다 |
| Distance (cm) | 플레이어에서 정면 기준점까지의 거리. Circle에서는 반지름 | 기본 300 |
| Yaw Offset (deg) | 플레이어를 바라보는 방향에 더할 회전 | 기본 0, -180~180 |
| Count | 한 번에 만들 개체 수 | 기본 1, 1~50. 입력칸은 소수를 받아도 반올림한다 |
| Pattern | Line: 정면 기준점을 중심으로 플레이어 정면에 수직인 선 위에 놓는다. Circle: 거리를 반지름으로 플레이어를 둘러싸며 정면의 개체부터 360도에 균등하게 놓는다 | 기본 Line |
| Spacing | Line의 개체 사이 간격, cm | 기본 150. Circle에서는 쓰지 않아 숨긴다(2026-10-11 변경. 이전에는 호 길이로 쓰고 둘레를 넘을 때만 원 전체에 놓았다) |
| Mode | Default StateTree / Idle / Repeat Action / Repeat Graph | Default는 행의 AI Data를 그대로 쓴다. 나머지는 이번 요청의 행 사본에서 AI Data를 비워 기본 AI를 끈다. 원본 테이블은 바뀌지 않는다 |
| Character folder only | Action·Graph 목록을 행의 Character Class가 있는 폴더와 그 하위 폴더로 좁힌다 | 기본 켜짐. 옆에 적용 중인 폴더를 보인다. 결과가 없으면 필터를 끄라는 안내가 나온다 |
| Refresh | Action·Graph 목록과 Trigger 태그를 다시 모은다 | 에디터 시작 직후 에셋 레지스트리가 다 읽히지 않았으면 누른다 |
| Action / Graph | 반복할 에셋. 버튼을 누르면 검색창과 폴더 묶음이 있는 메뉴가 열린다 | 필터에서 빠지면 첫 항목을 고르고, 남은 항목이 없으면 Spawn이 비활성화된다 |
| Entry Trigger | 시작 후 진입 대기 중인 Graph에 보낼 `Trigger` 루트 아래 태그 | 기본 (None). 시작 후에도 진입 대기에 머무는 Graph는 정지하고 경고를 한 번 남긴다 |
| Repeat Interval (s) | 한 번의 실행이 끝난 뒤(Return To Start를 켜면 복귀한 뒤) 다시 시작할 때까지 기다리는 시간 | 기본 1. NPC의 ASC가 준비되고 다른 Kata가 실행 중이지 않으면 첫 실행은 간격을 기다리지 않는다. 다른 Kata가 실행 중이면 종료 후 간격을 기다린다 |
| Return To Start | 한 번의 실행이 끝날 때마다 생성 위치·회전으로 순간이동한다 | 기본 켜짐. 피격 등으로 실행이 중단돼도 끝난 것으로 본다 |
| Protection | None / Invincible / Ignore Damage. 생성 직후 NPC에 더미 보호를 건다. 동작과 한계는 [스포너 사용법의 더미 보호](Spawner.md#더미로-쓰기)와 같다 | 기본 None. 치트 패널은 별도의 치트 태그를 조작하므로 이 보호를 해제하지 못한다. Kata Combat에 보호 태그가 비어 있거나 NPC에 ASC가 없으면 경고 로그를 남기고 보호를 적용하지 않는다. 이미 더미 컴포넌트가 있으면 새 설정을 적용하지 않는다 |
| Spawn | 플레이어 정면에 생성을 요청한다 | PIE 월드와 플레이어 Pawn, 테이블·행, 반복 모드의 에셋이 모두 있어야 활성화된다 |
| Despawn All | 이 패널로 만든 NPC와 그 AI Controller를 지운다 | 다른 스포너가 만든 NPC는 건드리지 않는다 |

생성은 `UKataCharacterSpawnSubsystem::RequestSpawnFromRow`로 요청하므로 행 에셋을 비동기로 로드한 뒤 생성한다.
위치마다 아래로 바닥을 찾고(World Static), 행 Character Class의 캡슐 절반 높이만큼 올린다. 바닥을 찾지 못하면 플레이어 중심 높이를 쓰고 엔진의 충돌 조정에 맡긴다.
반복 모드나 보호를 고르면 생성한 NPC에 `Kata Dummy` 컴포넌트(`UKataDummyComponent`)를 붙인다. 배치 스포너의 [Dummy 설정](Spawner.md#더미로-쓰기)과 같은 구현이며, 스포너가 없으므로 사망 뒤 재생성은 쓰지 않는다. 피격 정보 표시는 기본값(켬, 3초)으로 보인다.
반복 규칙은 스포너 사용법의 더미 반복 규칙과 같다. Graph 실행에는 캐릭터에 Kata Graph Component와 Kata Action Component가 모두 있어야 한다. 대상은 실행할 때마다 첫 플레이어 Pawn으로 다시 고르고, 피격 반응처럼 다른 Kata가 실행 중이면 끝날 때까지 기다린다. 조건 때문에 시작이 거절되거나 플레이어 Pawn이 없으면 다음 간격에 다시 시도한다.
입력값은 사용자별 설정(EditorPerProjectUserSettings)에 저장해 에디터를 다시 열어도 유지된다. 저장은 Spawn, 다른 패널로 전환, 탭 닫기, PIE 종료 때 한다. NPC 테이블 목록은 팀이 공유하는 DefaultEditor.ini에 저장한다.
PIE가 끝나면 이 패널의 생성 기록을 비우고, 그 뒤에 도착한 이전 세션의 생성 결과는 버린다. 반복은 NPC의 더미 컴포넌트와 함께 사라진다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| NPC Table 목록이 비어 있다 | 설정에 테이블이 없거나 NPC 행 구조가 아니다 | Settings로 `NPC Tables`를 채우고 `LogKataRuntimeSpawner` 경고를 확인한다 |
| Action·Graph 목록이 비어 있다 | 캐릭터 폴더에 에셋이 없거나 에셋 레지스트리를 읽는 중이다 | `Character folder only`를 끄거나 Refresh를 누른다 |
| 공용 BP를 쓰는 행에서 목록이 넓다 | Character Class가 상위 폴더에 있으면 그 아래가 모두 걸린다 | 검색창으로 좁힌다 |
| 저장된 입력값이 기본값으로 돌아갔다 | 범위를 벗어난 저장값(예: 이전 버전에서 깨진 값)은 패널을 처음 그릴 때 기본값으로 되돌린다 | 값을 다시 입력한다 |
| Graph 반복이 시작되지 않는다 | 자동 진입이 없는 Graph에 맞는 Entry Trigger가 없다 | 경고 로그를 보고 Entry Trigger를 고른다 |
| 반복 모드 NPC가 Action을 실행하지 않는다 | 조건·비용 때문에 Action 시작이 거절된다 | `LogKataFramework` Verbose 로그에서 시작 결과를 확인한다 |
| 패널로 만든 NPC의 보호를 치트 패널에서 풀 수 없다 | 2026-10-15부터 보호를 치트 태그가 아니라 게임플레이 태그로 건다 | 보호 없이 다시 생성한다 |

이 패널은 다른 프로젝트에서 재사용하지 않는다. 일반 NPC 배치는 [스포너 사용법](Spawner.md)을 따른다.

## 확인 상태와 근거

사용자 확인 범위는 문서 머리의 확인 상태를 따른다.

- [패널 UI와 생성](../../Source/ProjectKataEditor/KataDebugPanel_Spawner.h): 입력, 생성 요청, 일괄 제거, PIE 종료 처리.
- [배치 계산](../../Source/ProjectKataEditor/KataRuntimeSpawnLayout.h): Line·Circle 위치와 회전.
- [더미 컴포넌트](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataDummyComponent.h): 보호, Action·Graph 반복, Entry Trigger 처리, 시작 위치 복귀.
- [선택 메뉴](../../Source/ProjectKataEditor/KataRuntimeSpawnerPicker.h): 검색·묶음 메뉴.
- [설정](../../Source/ProjectKataEditor/KataRuntimeSpawnerSettings.h), [사용자 설정](../../Source/ProjectKataEditor/KataRuntimeSpawnerUserSettings.h).
- [결정 기록](../devlog/2026-10-10-Runtime-Spawner-Tool.md)(단독 탭 시절), [Kata Debug 탭 결정](../devlog/2026-10-11-Debug-Tab-Editor-Return.md), [더미 설정 기록](../devlog/2026-10-15-Spawner-Dummy.md)(반복·보호 통합).
- 작업 상태: [#49](https://github.com/jaykop/Kata/issues/49), [#55](https://github.com/jaykop/Kata/issues/55).
