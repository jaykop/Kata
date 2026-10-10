# 런타임 스포너 에디터 도구

작성: 2026-10-10  
갱신: 2026-10-10  
유형: 구현 기록, 결정 기록  
대상: 샘플 프로젝트 Editor 모듈 `ProjectKataEditor`, SlateIM  
기준: main 826a358 이후 미커밋 작업 트리. 같은 작업 트리에 이 작업과 무관한 KataRuntime·HitReaction 미커밋 변경이 함께 있다.

## 배경과 결론

전투와 AI를 확인할 때마다 레벨에 스포너를 배치하지 않고, PIE 중 플레이어 정면에 NPC를 바로 만들고 싶다는 요청이 있었다. 생성한 NPC는 기본 AI, 정지, 고른 Action·Graph 반복 중 하나로 동작해야 한다.
SlateIM 도킹 탭으로 런타임 스포너 도구를 만들었다. 샘플 프로젝트에 Editor 모듈 `ProjectKataEditor`를 새로 추가했으며, Kata 플러그인 코드는 바꾸지 않았다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 |
|---|---|---|
| 샘플 프로젝트 모듈 | Runtime 모듈 `ProjectKata`만 있었다 | Editor 모듈 `ProjectKataEditor`를 추가하고 Editor Target에 포함했다 |
| SlateIM | 엔진의 Experimental 플러그인이며 기본 비활성 | 샘플 `.uproject`에서 Editor 타깃에만 켰다 |
| 생성 경로 | 스포너와 같은 `UKataCharacterSpawnSubsystem::RequestSpawnFromRow`를 쓸 수 있다 | 행 사본으로 요청한다. AI를 끄는 모드는 행 사본의 `AIData`를 비운다(#48 경로) |
| 반복 실행 | KataAI StateTree Task가 쓰는 Action·Graph 시작 경로가 있다 | 같은 컴포넌트 API를 에디터 ticker에서 호출한다. 런타임 코드는 추가하지 않았다 |
| 선택 목록 | SlateIM 콤보박스의 검색 옵션(`bSearchable`)은 사용자 환경에서 검색창이 보이지 않았다. 원인은 확정하지 못했다 | Kata 액션 에디터 Add Task 메뉴와 같은 `SGraphActionMenu`를 `SlateIM::Widget`으로 넣었다 |
| 에디터 종료 시 탭 재생성 | 탭을 연 채 에디터를 끄면 탭 창이 잠깐 떴다 닫혔다. 메인 창이 탭째로 파괴될 때 `OnTabClosed`가 불리지 않아 SlateIM 그리기 Tick이 남고, `FSlateIMNomadTabBase`가 탭이 없다고 보고 `TryInvokeTab`으로 새 창을 연다 | 메인 창 파괴 전에 오는 `GEngine->OnEditorClose`에서 그리기 Tick만 멈춘다. 탭은 레이아웃에 남아 다음 실행 때 복원된다 |
| 설정 저장 | 없음 | 테이블 목록은 프로젝트 공유 설정(DefaultEditor.ini), 마지막 입력값은 사용자별 설정에 둔다 |

## 주요 결정과 이유

- **샘플 프로젝트 Editor 모듈에 둔다.** 처음에는 별도 도구 플러그인을 제안했다. AGENTS.md의 "재사용 코드는 플러그인에" 규칙을 따르고, KataFramework에 Experimental 의존을 넣지 않기 위해서였다. 사용자는 이 도구를 샘플 확인용으로 보고 샘플 Editor 모듈을 택했다. 두 방식의 추가 작업량은 비슷했고, 다른 프로젝트에서 재사용하지 않는다는 점이 결정 기준이었다.
- **원형 패턴은 플레이어를 둘러싼다.** 사용자가 정면 기준점 주변의 군집 대신 포위 배치를 택했고, 거리를 반지름으로 쓴다. 개체 간격은 호 길이이며, 둘레를 넘으면 360도에 균등하게 놓는다.
- **Action·Graph 1차 필터는 Character Class 폴더로 한다.** 사용자는 행 이름 기준을 검토했지만, 에셋 이름이 캐릭터 이름을 포함한다는 보장이 없었다. 샘플 콘텐츠는 캐릭터 BP와 Kata 에셋을 같은 캐릭터 폴더에 두므로 폴더 기준을 택했다. 체크박스로 끌 수 있다.
- **AI 정지·반복 모드는 행 사본의 AI Data를 비운다.** StateTree가 반복 실행과 같은 컴포넌트를 두고 다투지 않게 하려는 것이며, 런타임 코드 변경 없이 #48의 경로를 재사용한다.
- **반복은 에디터 ticker가 구동한다.** 반복용 StateTree 에셋이나 런타임 컴포넌트를 추가하지 않으려는 것이다. 에디터 전용 도구라서 PIE 종료 때 기록을 비우는 것으로 충분하다.
- **진입 대기에 머무는 Graph는 정지한다.** 실행 중으로 남으면 반복이 영원히 멈추므로, 정지시키고 Entry Trigger를 지정하라는 경고를 한 번 남긴다.
- **입력값 저장은 Spawn·탭 닫기·PIE 종료 때 한다.** SpinBox를 끄는 동안 매 프레임 설정 파일에 쓰지 않으려는 것이다.

## 근거

- [탭 구현](../../Source/ProjectKataEditor/KataRuntimeSpawnerTab.cpp): 목록 수집, 폴더 필터, 생성 요청, 일괄 제거.
- [반복 실행](../../Source/ProjectKataEditor/KataRuntimeSpawnerLoop.cpp): 쉬는 상태 판정, Entry Trigger 처리.
- [선택 메뉴](../../Source/ProjectKataEditor/KataRuntimeSpawnerPicker.cpp): `SGraphActionMenu` 사용.
- [배치 계산](../../Source/ProjectKataEditor/KataRuntimeSpawnLayout.cpp): Line·Circle 계산.
- [스포너 AI 설정 덮어쓰기 기록](2026-10-10-Spawner-AI-Override.md): 행 사본의 AI Data를 비우는 경로.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 첫 구현 | 사용자 빌드·실행 보고(2026-10-10) | 빌드 통과, 기능 모두 동작 | 개별 시나리오 결과 미보고 |
| 선택 메뉴 교체 | 사용자 빌드 보고와 스크린샷 | 빌드 통과, 메뉴 버튼 표시 | 검색·묶음 동작의 개별 결과 미보고 |
| 종료 시 탭 재생성 수정 | 사용자 빌드·실행 보고 | 빌드 통과, 종료 시 창이 뜨지 않음 | - |
| 줄 정렬 수정 | 사용자 빌드·화면 확인 보고 | 빌드 통과, 정렬 확인 | 개별 시나리오 결과 미보고 |

에이전트는 빌드·테스트·실행을 하지 않았다.

## 남은 제한과 후속 작업

- SlateIM 콤보박스의 검색 옵션이 보이지 않은 원인은 확정하지 못했다. 이 도구는 해당 옵션을 쓰지 않는다.
- Action·Graph 폴더 필터는 콘텐츠 폴더 규칙에 기댄다. 공용 BP를 쓰는 행은 필터가 넓어진다.
- 다른 프로젝트에서 재사용하려면 플러그인으로 옮겨야 한다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#49](https://github.com/jaykop/Kata/issues/49) | 구현 완료, 사용자 빌드·화면 확인 완료 |
| [런타임 스포너 도구 사용법](../manual/Runtime-Spawner.md) | 새로 작성 |
| [AGENTS.md](../../AGENTS.md) | 샘플 Editor 모듈과 SlateIM 샘플 전용 활성화를 추가 |
