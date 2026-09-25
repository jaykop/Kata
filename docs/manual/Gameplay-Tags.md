# 게임플레이 태그 사용법

갱신: 2026-09-25
대상: 프로젝트에 게임플레이 태그를 추가하고 C++에서 참조하는 사용자  
적용 기준: [게임플레이 태그 생성 구현 기록](../devlog/2026-09-24-Gameplay-Tag-Generation.md)  
확인 상태: 2026-09-24 사용자 확인(Rider 빌드, 에디터 Gameplay Tag Manager). Game 타깃 빌드와 패키징은 미확인

## 목적과 준비

프로젝트의 게임플레이 태그는 `Config/Tags` 아래에 카테고리별 ini로 나누어 관리한다.
C++에서 참조할 태그는 `Config/Tags/Native`에 두며, 빌드할 때 `KataTag.A.B` 형태로 접근하는 코드가 자동으로 생성된다.
Kata 플러그인은 게임용 태그를 정의하지 않는다. 플러그인의 API와 에셋 프로퍼티는 프로젝트가 정한 `FGameplayTag` 값을 받는다.
조건 자동화 테스트의 정적 테스트 태그는 예외이며 개발 테스트 빌드에서만 사용한다.

| 위치 | 용도 | C++ 참조 |
|---|---|---|
| `Config/Tags/Native/*.ini` | C++에서 참조하는 태그 | `KataTag.A.B`로 참조 가능 |
| `Config/Tags/*.ini` | 에디터와 에셋에서만 쓰는 태그 | 생성하지 않음 |

별도 준비는 필요 없다. `ProjectKata.uproject`의 PreBuildSteps가 모든 빌드(Rider, Visual Studio, `Scripts/Build.ps1`) 직전에
`Scripts/Generate-NativeGameplayTags.ps1`을 실행한다.

이 자동 연결은 ProjectKata 샘플의 설정이다. 다른 프로젝트에 플러그인만 복사하면 생성 단계가 추가되지는 않는다.
재사용 코드에서는 샘플의 KataTags.h나 KataTag 전역을 참조하지 않고 태그 값을 매개변수·설정으로 받는다.

## 사용 순서

### 코드 태그 추가

1. 에디터를 닫는다. 새 태그는 전역 정의를 추가하므로 Live Coding 대신 일반 빌드를 사용한다.
2. `Config/Tags/Native`에 카테고리 이름의 ini를 만들거나 기존 파일을 연다. 형식은 다음과 같다.
   ```ini
   [/Script/GameplayTags.GameplayTagsList]
   GameplayTagList=(Tag="Combat.Hit.Light",DevComment="Light hit reaction")
   ```
3. 빌드한다. 빌드 출력에 `[KataTags] N tag(s) from M file(s), K node(s): updated.`가 나오면 `Source/ProjectKata/KataTags.h`와
   `KataTags.cpp`가 갱신된 것이다. 태그를 바꾸지 않았으면 `unchanged`가 나오고 파일을 다시 쓰지 않는다.
4. C++에서 다음처럼 사용한다.
   ```cpp
   #include "KataTags.h"

   // 잎 태그와 중간 노드 모두 FGameplayTag가 필요한 자리에 넘길 수 있다.
   const bool bIsHit = Tag.MatchesTag(KataTag.Combat.Hit);
   Container.AddTag(KataTag.Combat.Hit.Light);
   ```

에디터의 Project Settings → GameplayTags → `Manage Gameplay Tags...`에서 태그를 추가할 때 Source로 `Native` 폴더의 ini를 고를 수도 있다.
추가한 태그는 다음 빌드부터 C++에서 쓸 수 있다.

### 에디터 전용 태그 추가

`Config/Tags`에 카테고리별 ini를 두거나 `Manage Gameplay Tags...`에서 해당 ini를 Source로 골라 추가한다. 코드는 생성되지 않는다.

## 주요 설정과 규칙

| 항목 | 의미 | 주의 |
|---|---|---|
| `GameplayTagList=(Tag="...",DevComment="...")` | 태그 한 줄 | **`+`, `-` 등 ini 명령 기호를 붙이지 않는다.** 붙이면 빌드가 실패한다 |
| ini 파일 이름 | 엔진의 태그 소스 이름 | `Config/Tags` 전체(하위 폴더 포함)에서 유일해야 한다. `Native`와 에디터 쪽에 같은 이름을 쓰지 않는다. 접두사 규약은 두지 않는다 |
| 태그 조각 | `A.B.C`의 각 부분 | 영문자로 시작하고 영문자·숫자·밑줄만 쓴다. 밑줄로 끝나거나 연속 밑줄을 쓸 수 없다. C++ 키워드와 `check`, `TEXT` 같은 엔진·Windows 매크로 이름은 쓸 수 없다 |
| DevComment | 태그 설명 | 생성 헤더의 멤버 주석과 네이티브 태그 설명으로 전달된다. 한국어를 쓸 수 있다 |
| `KataTag.A` | 중간 노드 | 구조체이며 `FGameplayTag`로 암시적으로 변환된다. `auto X = KataTag.A`는 태그가 아니라 구조체가 되므로 `FGameplayTag X = KataTag.A`로 쓴다 |

선언하지 않은 중간 부모도 네이티브 태그로 정의된다. `Combat.Hit.Light`만 적어도 `Combat`과 `Combat.Hit`이 함께 생긴다.
`A_B.C`와 `A.B_C`처럼 C++ 이름이 같아지는 태그, 대소문자만 다른 중복 태그도 빌드 오류가 된다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 빌드 출력에 `Remove the '+' prefix` 오류 | 엔진은 `Config/Tags`의 개별 ini를 ini 명령 기호 없이 읽는다. `+GameplayTagList`는 무시되고, 에디터가 같은 파일에 태그를 추가하면 그 줄이 지워진다 | 줄 앞의 기호를 지운다 |
| `Tag ini file name ... is used more than once` 오류 | 엔진은 태그 소스를 파일 이름으로만 구분해 같은 이름의 파일을 하나로 합친다 | 한쪽 파일 이름을 바꾼다 |
| `KataTags.h`를 찾을 수 없음 | 생성 파일은 커밋하지 않으며 첫 빌드 전에는 없다 | 한 번 빌드한다 |
| 새 태그가 자동 완성에 없음 | 생성은 빌드할 때 일어난다 | 빌드 후 IDE 인덱싱을 기다린다 |
| Tag Manager에서 코드 태그의 Source가 ini가 아니라 `ProjectKata`로 보임 | 엔진이 네이티브 태그를 먼저 등록하고 처음 등록된 소스를 표시한다 | 정상이다. 코드 태그는 에디터에서 삭제·이름 변경이 제한된다 |
| 코드 태그 이름을 바꾼 뒤 컴파일 오류 | 생성 멤버 이름이 바뀌었다 | 참조 코드를 고친다. 저장된 에셋의 태그 값은 `GameplayTagRedirects`로 따로 옮긴다 |

생성 스크립트는 Win64 PowerShell 기준이다. 다른 플랫폼용 스크립트는 없다.

## 확인 상태와 근거

2026-09-24 사용자가 Rider 빌드와 에디터에서 다음을 확인했다: 태그가 없을 때의 빈 생성, `Native` ini로부터의 계층 생성과
DevComment 전달, Gameplay Tag Manager 표시, `Native` 폴더 ini에 에디터로 태그를 추가해도 기존 줄이 유지됨.
Game 타깃 빌드, 오류 입력에 대한 빌드 실패 출력, 패키징은 확인하지 않았다.

- [Scripts/Generate-NativeGameplayTags.ps1](../../Scripts/Generate-NativeGameplayTags.ps1): 생성 규칙과 오류 조건.
- [게임플레이 태그 생성 구현 기록](../devlog/2026-09-24-Gameplay-Tag-Generation.md): 결정 이유와 `+` 접두사 문제.
- [현재 구현 상태](../devlog/Implementation-Status.md).
