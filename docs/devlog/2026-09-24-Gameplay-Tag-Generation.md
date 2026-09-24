# 게임플레이 태그 코드 생성 구현

작성: 2026-09-24  
갱신: 2026-09-24  
유형: 구현 기록·결정 기록  
대상: `Scripts/Generate-NativeGameplayTags.ps1`, `ProjectKata.uproject`, `ProjectKata` 모듈, `Config/Tags`  
기준: 커밋 `06ede58` 위의 미커밋 작업 트리. 이 기록과 함께 커밋한다

## 배경과 결론

Hit Trace가 부착된 무기 메시를 판정 기준으로 고르려면 컴포넌트를 지정할 방법이 필요하다. 현재는 `FName` 컴포넌트 태그를 손으로 입력한다.
사용자는 Hit Trace보다 태그 정리를 먼저 하기로 했다. 과거 프로젝트에서 쓰던 방식을 요청했다: ini를 원본으로 두고
빌드 전 단계에서 헤더를 생성해 태그를 구조체 계층(`KataTag.Mesh.Character`)으로 참조하는 방식이다.

[#2](https://github.com/jaykop/Kata/issues/2)의 T01~T02를 구현했다. `Config/Tags/Native/*.ini`로부터 매 빌드마다
`Source/ProjectKata/KataTags.h/.cpp`를 생성하며, 사용자 확인을 마쳤다.
이후 계획했던 컴포넌트 태그 작업(T03~T05)은 취소하고, 대신 Distance 조건의 컴포넌트 선택을 제거했다(아래 "후속 결정").
#2를 닫으면서 계획 문서(`docs/plan/Gameplay-Tag-Plan.md`)는 삭제했다. 결정 이유는 이 기록에 남긴다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 태그 기반 | `Config/Tags`와 네이티브 태그 선언이 없음 | `Config/Tags`, `Config/Tags/Native` 폴더(`.gitkeep`) |
| 생성 스크립트 | 없음 | `Scripts/Generate-NativeGameplayTags.ps1` 신규 |
| 빌드 연결 | 없음 | `ProjectKata.uproject`의 `PreBuildSteps.Win64` |
| 모듈 의존성 | `GameplayTags` 없음 | `ProjectKata.Build.cs` Public 의존성에 추가 |
| 생성 파일 | 없음 | `KataTags.h/.cpp`, `.gitignore`로 커밋 제외 |

## 주요 결정과 이유

| 결정 | 대안 | 채택 이유 |
|---|---|---|
| 값은 `FNativeGameplayTag`, 계층은 참조 멤버를 가진 중첩 구조체 | `FGameplayTag` 멤버와 Init 호출(과거 Lyra 방식), 평면 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` | 사용자가 `KataTag.A.B` 문법을 선택했다. 엔진이 등록 시점을 관리하므로 Init 호출 순서를 신경 쓸 필요가 없다 |
| 원본을 `Config/Tags/Native`에 둠 | `Config/Tags` 밖의 별도 폴더 | 사용자 요청. 엔진이 이 ini도 읽으므로 Project Settings에서 코드 태그를 추가할 수 있다 |
| 태그는 프로젝트만 소유, 루트 `KataTag` | 플러그인 `KataTag` + 프로젝트 `GameTag` | 사용자 결정. 플러그인이 태그를 가질 이유가 없다. 플러그인은 `FGameplayTag` 값을 받는다 |
| ini 파일 이름 접두사 없음 | `Native` 접두사 | 사용자 결정. 충돌은 생성기가 검사한다 |
| 부착 위치 태그(`Component.Weapon.MainHand` 등)를 만들지 않음 | 계획 초안의 제안 | 사용자 결정. 태그 정의는 장비 설계에서 정한다 |
| 결과가 같으면 파일을 쓰지 않음 | 매번 쓰기 | 헤더를 매번 쓰면 이를 포함한 파일이 매번 재컴파일된다 |
| 스크립트만 UTF-8 BOM 저장 | BOM 없음(C++ 규칙) | Windows PowerShell 5.1은 BOM이 없는 파일을 시스템 코드 페이지로 읽어 한국어 주석이 깨진다 |

## 후속 결정: 컴포넌트 태그 작업 취소와 Distance 조건 단순화

계획에는 태그 생성 뒤의 작업으로 T03~T05가 있었다. T03은 컴포넌트를 게임플레이 태그로 부여·조회하는 `UKataFL_Component`,
T04는 Distance 조건의 `FName ComponentTag`를 `FGameplayTag`로 전환하는 작업, T05는 프리뷰 무기 부착 설정이다.
이 작업의 목적은 코어가 무기 메시를 태그로 찾게 하는 것이었다.

| 결정 | 대안 | 채택 이유 |
|---|---|---|
| Hit Trace 태스크는 KataFramework에 두고, 판정 기준 메시는 캐릭터가 제공한다([#6](https://github.com/jaykop/Kata/issues/6)). 장비 시스템은 범위에서 제외한다 | 코어에 두고 컴포넌트 태그로 무기를 찾는다 | 사용자 결정. 태스크가 반드시 코어에 있을 필요는 없다. 태그를 부여하는 단계가 없어지고 부착 위치를 태그로 표현하지 않는다는 방향과 맞는다 |
| T03~T05를 진행하지 않는다 | `UKataFL_Component`를 코어에 추가 | 코어에서 태그로 컴포넌트를 찾을 곳이 남지 않는다. 프리뷰 무기는 캐릭터 BP에 붙이면 그대로 나타난다 |
| Distance 조건에서 `Mode`·`ComponentTag`를 제거한다. `SocketName`이 비어 있으면 Actor 위치, 있으면 `ACharacter::GetMesh()`의 Socket을 쓴다 | `ComponentTag` 유지, 또는 `FGameplayTag`로 전환 | 사용자 결정. 코어 조건이 부착 컴포넌트를 고를 이유가 없다. 이 조건을 쓰는 에셋이 없어 이전 코드는 넣지 않았다 |

Distance 소켓 테스트는 소켓이 있는 스켈레탈 메시를 테스트에서 만들 수 없어 범위를 줄였다.
빈 `SocketName`의 Actor 거리와 비캐릭터의 `MissingCharacterMesh`만 자동화 테스트로 남기고, 소켓 위치의 양성 케이스는 에디터에서 확인한다.

## 시행착오

- **`+GameplayTagList` 줄이 무시되고 에디터 저장 시 지워졌다.** 처음 안내한 ini 예시에 `+`를 붙였다.
  엔진은 `Config/Tags`의 개별 ini를 `FConfigFile::Read`로 읽는데, 이 함수는 `FillFileFromDisk(..., false)`로 ini 명령 기호를 처리하지 않는다.
  그래서 키가 `+GameplayTagList`가 되어 소스 목록에 들어가지 않았다. 생성기는 이 줄을 받아들여 네이티브 태그만 만들었다.
  사용자가 Tag Manager로 같은 파일에 태그를 추가하자, 에디터가 소스 목록(추가한 한 줄)만 다시 기록해 기존 줄이 사라졌다.
  생성기가 명령 기호가 붙은 줄을 빌드 오류로 처리하도록 고쳤다.
- 부모 노드의 변환 연산자는 값을 반환해야 한다. `FNativeGameplayTag::operator FGameplayTag()`가 값을 반환하므로 참조를 반환하면 임시 객체를 가리킨다.
- `UE_DEFINE_GAMEPLAY_TAG*`는 `static_assert(HasFileExtension(__FILE__))` 때문에 `.cpp`에서만 쓸 수 있다. 생성 소스를 `.inl`로 만들 수 없다.
- 엔진은 태그 소스를 파일 이름으로만 구분한다(`GameplayTagsManager.cpp`의 `AddTagIniSearchPath`). 폴더가 달라도 이름이 같으면 한 소스로 합쳐진다.
- 일반 플러그인의 `Config/Tags`는 엔진이 자동 등록하지 않는다. 플러그인 태그를 두지 않기로 해 이번 구현에는 영향이 없다.

## 근거

- [Scripts/Generate-NativeGameplayTags.ps1](../../Scripts/Generate-NativeGameplayTags.ps1): 파싱, 트리 구성, 오류 조건, 쓰기 조건.
- UE 5.8 `Runtime/GameplayTags/Private/GameplayTagsManager.cpp`: 재귀 검색, 파일 이름 소스, 네이티브 태그 선등록, Restricted 태그 전용 충돌 검사.
- UE 5.8 `Runtime/Core/Private/Misc/ConfigCacheIni.cpp`: `FConfigFile::Read`의 명령 기호 비처리.
- UE 5.8 `Plugins/Editor/GameplayTagsEditor/.../GameplayTagsEditorModule.cpp`: `AddNewGameplayTagToINI`가 소스 목록을 다시 기록.
- UE 5.8 `UnrealBuildTool/Modes/BuildMode.cs`: 소스 목록 수집 전 PreBuildSteps 실행. `UEBuildTarget.cs`: 프로젝트 모듈의 `UE_PLUGIN_NAME`은 빈 문자열.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 빈 입력 생성 | 사용자 Rider 빌드, 생성 파일 대조 | 빈 루트 생성, 빌드 통과 | - |
| `Native` ini로부터 계층 생성 | 사용자 빌드, 생성 헤더 대조 | `KataTag.Test.A.B`, `KataTag.Test.C`, DevComment 주석 생성 | 자동 완성은 사용자 보고 |
| 에디터 인식과 추가 | 사용자 에디터 확인, ini 대조 | Tag Manager 표시, 에디터 추가 후 기존 줄 유지(`+` 금지 수정 이후) | - |
| Distance 조건 변경 | 사용자 빌드, 자동화 테스트 `Kata.Conditions.Distance.*` 2건, 에디터 확인 | 테스트 통과, 에디터에서 `SocketName`만 표시되고 캐릭터 Mesh 소켓 기준 판정 | - |
| 오류 입력 | 미실시 | - | `+` 접두사·잘못된 조각·파일 이름 충돌 시 빌드 실패 출력 |
| Game 타깃, 패키징 | 미실시 | - | 전체 |

에이전트는 빌드와 스크립트 실행을 하지 않았다. 테스트용 `Test.ini`는 확인 후 삭제했다.

## 남은 제한과 후속 작업

- 생성 스크립트는 Win64 전용이다.
- 태그를 추가·변경한 뒤 Live Coding으로 반영되는지는 확인하지 않았다. 에디터를 닫고 빌드하도록 안내한다.

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [현재 구현 상태](Implementation-Status.md) | 게임플레이 태그 항목 추가 |
| [게임플레이 태그 사용법](../manual/Gameplay-Tags.md) | 신규 |
| 게임플레이 태그 계획 | #2 종료와 함께 삭제. 결정은 이 기록으로 옮겼다 |
| [기본 조건](../manual/Conditions.md) | Distance 기준점 설명 갱신 |
| [문서 목록](../README.md) | manual·devlog 링크 추가 |
