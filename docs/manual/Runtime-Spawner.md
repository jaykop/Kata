# 런타임 스포너 도구 사용법

갱신: 2026-10-10  
대상: 샘플 프로젝트 Editor 모듈 `ProjectKataEditor`의 SlateIM 도킹 탭  
적용 기준: [#49](https://github.com/jaykop/Kata/issues/49)  
확인 상태: 2026-10-10 사용자가 Editor 빌드 통과와 첫 구현의 기능 동작을 보고했다. 선택 메뉴 교체와 줄 정렬 수정 후에도 빌드 통과와 화면 확인을 보고했다. 수정 후의 개별 시나리오 결과는 보고되지 않았다.

## 목적과 준비

PIE 중 플레이어 정면에 NPC를 즉석으로 생성해 전투와 AI 동작을 확인한다. 생성한 NPC가 기본 AI를 실행하게 하거나, 아무것도 하지 않게 하거나, 고른 Action·Graph를 플레이어를 대상으로 반복하게 할 수 있다.
이 도구는 샘플 프로젝트 전용이며 Kata 플러그인에는 들어 있지 않다. Editor 타깃에서만 빌드되고 Game 빌드에는 포함되지 않는다.
샘플 `.uproject`가 Experimental 플러그인인 SlateIM을 Editor 타깃에서 켠다. 에디터를 열 때 실험 플러그인 경고가 나올 수 있다.

## 사용 순서

1. Project Settings > Editor > Kata Runtime Spawner의 `NPC Tables`에 행을 고를 NPC 테이블을 넣는다. 샘플은 `DT_NPC_Enemies`, `DT_NPC_Bosses`를 기본으로 넣어 두었다(DefaultEditor.ini).
2. PIE를 시작한다.
3. Tools 메뉴의 Kata 섹션에서 `Kata Runtime Spawner`를 연다. 콘솔 명령 `Kata.RuntimeSpawner`로도 열고 닫을 수 있다.
4. Character에서 테이블과 행을 고른다. 행 버튼을 누르면 검색창이 있는 선택 메뉴가 열린다.
5. Placement에서 거리, Yaw Offset, 개체 수, 패턴, 간격을 정한다.
6. AI에서 모드를 고른다. 반복 모드면 Action 또는 Graph를 고르고 반복 간격을 정한다. Graph는 필요하면 Entry Trigger를 고른다.
7. `Spawn`을 누른다. 이 도구로 만든 NPC는 `Despawn All`로 한꺼번에 지운다.

## 주요 설정과 실행 계약

| UI 항목 | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| NPC Table | 설정에 넣은 테이블 중 하나 | 행 구조가 `FKataNPCCharacterRow` 계열이 아닌 테이블은 경고 로그와 함께 목록에서 뺀다. 데이터 컬렉션에 등록되지 않은 테이블도 쓸 수 있다 |
| Settings | Project Settings의 도구 설정 화면을 연다 | - |
| Row | 고른 테이블의 행 | 저장된 행이 테이블에 없으면 첫 행을 고른다 |
| Distance (cm) | 플레이어에서 정면 기준점까지의 거리. Circle에서는 반지름 | 기본 300 |
| Yaw Offset (deg) | 플레이어를 바라보는 방향에 더할 회전 | 기본 0, -180~180 |
| Count | 한 번에 만들 개체 수 | 기본 1, 1~50 |
| Pattern | Line: 정면 기준점을 중심으로 플레이어 정면에 수직인 선 위에 놓는다. Circle: 거리를 반지름으로 플레이어를 둘러싼다 | 기본 Line |
| Spacing | 개체 사이 간격, cm. Circle에서는 원 위의 호 길이 | 기본 150. Circle은 정면을 가운데로 좌우에 펼치고, 개체 수 × 간격이 둘레 이상이거나 간격이 0이면 360도에 균등하게 놓는다 |
| Mode | Default StateTree / Idle / Repeat Action / Repeat Graph | Default는 행의 AI Data를 그대로 쓴다. 나머지는 이번 요청의 행 사본에서 AI Data를 비워 기본 AI를 끈다. 원본 테이블은 바뀌지 않는다 |
| Character folder only | Action·Graph 목록을 행의 Character Class가 있는 폴더와 그 하위 폴더로 좁힌다 | 기본 켜짐. 옆에 적용 중인 폴더를 보인다. 결과가 없으면 필터를 끄라는 안내가 나온다 |
| Refresh | Action·Graph 목록과 Trigger 태그를 다시 모은다 | 에디터 시작 직후 에셋 레지스트리가 다 읽히지 않았으면 누른다 |
| Action / Graph | 반복할 에셋. 버튼을 누르면 검색창과 폴더 묶음이 있는 메뉴가 열린다 | 필터에서 빠지면 첫 항목을 고르고, 남은 항목이 없으면 Spawn이 비활성화된다 |
| Entry Trigger | 자동 진입이 없는 Graph에 보낼 `Trigger` 루트 아래 태그 | 기본 (None). 시작 후에도 진입 대기에 머무는 Graph는 정지하고 경고를 한 번 남긴다 |
| Repeat Interval (s) | 한 번의 실행이 끝난 뒤 다시 시작할 때까지 기다리는 시간 | 기본 1. 첫 실행은 NPC의 ASC가 준비되는 즉시 한다 |
| Spawn | 플레이어 정면에 생성을 요청한다 | PIE 월드와 플레이어 Pawn, 테이블·행, 반복 모드의 에셋이 모두 있어야 활성화된다 |
| Despawn All | 이 도구로 만든 NPC와 그 AI Controller를 지운다 | 다른 스포너가 만든 NPC는 건드리지 않는다 |

생성은 `UKataCharacterSpawnSubsystem::RequestSpawnFromRow`로 요청하므로 행 에셋을 비동기로 로드한 뒤 생성한다.
위치마다 아래로 바닥을 찾고(World Static), 행 Character Class의 캡슐 절반 높이만큼 올린다. 바닥을 찾지 못하면 플레이어 중심 높이를 쓰고 엔진의 충돌 조정에 맡긴다.
반복 모드는 NPC가 Action·Graph를 실행하지 않는 상태로 반복 간격 이상 지나면 다시 시작한다. 대상은 실행할 때마다 첫 플레이어 Pawn으로 다시 고른다. 피격 반응처럼 다른 Kata가 실행 중이면 끝날 때까지 기다린다. 조건 때문에 시작이 거절되면 다음 간격에 다시 시도한다.
입력값은 사용자별 설정(EditorPerProjectUserSettings)에 저장해 에디터를 다시 열어도 유지된다. 저장은 Spawn, 탭 닫기, PIE 종료 때 한다. NPC 테이블 목록은 팀이 공유하는 DefaultEditor.ini에 저장한다.
PIE가 끝나면 이 도구의 생성 기록과 반복 기록을 비우고, 그 뒤에 도착한 이전 세션의 생성 결과는 버린다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| NPC Table 목록이 비어 있다 | 설정에 테이블이 없거나 NPC 행 구조가 아니다 | Settings로 `NPC Tables`를 채우고 `LogKataRuntimeSpawner` 경고를 확인한다 |
| Action·Graph 목록이 비어 있다 | 캐릭터 폴더에 에셋이 없거나 에셋 레지스트리를 읽는 중이다 | `Character folder only`를 끄거나 Refresh를 누른다 |
| 공용 BP를 쓰는 행에서 목록이 넓다 | Character Class가 상위 폴더에 있으면 그 아래가 모두 걸린다 | 검색창으로 좁힌다 |
| Graph 반복이 시작되지 않는다 | 자동 진입이 없는 Graph에 맞는 Entry Trigger가 없다 | 경고 로그를 보고 Entry Trigger를 고른다 |
| 반복 모드 NPC가 Action을 실행하지 않는다 | Action의 시작 조건·비용이 거절한다 | `LogKataRuntimeSpawner` Verbose 로그에서 시작 결과를 확인한다 |

이 도구는 다른 프로젝트에서 재사용하지 않는다. 일반 NPC 배치는 [스포너 사용법](Spawner.md)을 따른다.

## 확인 상태와 근거

에이전트는 빌드·테스트·실행을 하지 않았다. 사용자 확인 범위는 문서 머리의 확인 상태를 따른다.

- [탭 UI와 생성](../../Source/ProjectKataEditor/KataRuntimeSpawnerTab.h): 입력, 생성 요청, 일괄 제거, PIE 종료 처리.
- [배치 계산](../../Source/ProjectKataEditor/KataRuntimeSpawnLayout.h): Line·Circle 위치와 회전.
- [반복 실행](../../Source/ProjectKataEditor/KataRuntimeSpawnerLoop.h): Action·Graph 반복과 Entry Trigger 처리.
- [선택 메뉴](../../Source/ProjectKataEditor/KataRuntimeSpawnerPicker.h): 검색·묶음 메뉴.
- [설정](../../Source/ProjectKataEditor/KataRuntimeSpawnerSettings.h), [사용자 설정](../../Source/ProjectKataEditor/KataRuntimeSpawnerUserSettings.h).
- [결정 기록](../devlog/2026-10-10-Runtime-Spawner-Tool.md).
- [작업 상태](https://github.com/jaykop/Kata/issues/49).
