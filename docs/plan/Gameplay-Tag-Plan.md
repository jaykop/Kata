# 게임플레이 태그 체계와 컴포넌트 태그 계획

작성: 2026-09-24  
갱신: 2026-09-24  
문서 상태: 일부 완료  
현재 상태 근거: [현재 구현 상태](../devlog/Implementation-Status.md) · [Kata 공통 요청 메모](Kata.md) 4번  
대체 관계: 없음

## 목적과 현재 상태

Hit Trace는 부착된 무기 메시를 판정 기준으로 선택해야 한다. 현재 컴포넌트 지정은 `FName`으로 한다.
예를 들어 `UKataCondition_Distance`의 Location에 있는 `ComponentTag`가 `FName`이다.
이름을 손으로 입력하므로 오타를 막을 수 없고, 태그 계층을 이용한 조회도 할 수 없다.
그래서 Hit Trace를 시작하기 전에 태그 체계를 먼저 정리하기로 했다.

현재 프로젝트에는 태그 기반이 없다. `Config/Tags`와 네이티브 태그 선언이 모두 없고,
`UE_DEFINE_GAMEPLAY_TAG_STATIC`은 조건 테스트 코드에서만 쓴다.

목표는 다음과 같다.

1. 태그는 코드 태그든 에디터 태그든 카테고리별 ini 여러 개로 나누어 관리한다. C++에서 참조하는 태그는
   `Tags/Native/` 아래 ini들을 원본으로 두고, 빌드할 때 `KataTag.Mesh.Character` 형태로 계층을 따라 접근하는 헤더를 생성한다.
2. 컴포넌트 지정을 `FGameplayTag`로 바꾸고, `FName` 입력 단계를 없앤다.

## 범위

- 포함: 카테고리별 태그 ini 구조와 `Native` 폴더, 생성 스크립트와 PreBuildSteps 연결, 생성 코드 형태, 태그 이름 규약,
  `UKataFL_Component` 공용 함수, 기존 `FName ComponentTag`의 전환, 프리뷰에서 무기를 부착하는 설정.
- 제외: 장비·장착 시스템과 컴포넌트 태그 부여용 데이터 에셋. 장착 시스템을 설계할 때 함께 다룬다.
  Hit Trace 본체는 별도 계획에서 다룬다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| ini 분할 | 확정 | 코드 태그와 에디터 태그 모두 카테고리별 ini 여러 개로 나눈다. 하나의 ini에 모으지 않는다. |
| 원본과 생성 방식 | 확정 | C++ 참조 태그의 원본은 `Config/Tags/Native/*.ini`다. 빌드할 때마다 PreBuildSteps가 헤더와 소스를 생성한다. 생성 파일은 커밋하지 않는다. |
| 생성 코드 형태 | 확정 | 값은 `FNativeGameplayTag`에 저장하고, 계층은 중첩 구조체의 참조 멤버로 표현한다. 아래 "생성 코드 형태"를 따른다. |
| 에디터 전용 태그 | 확정 | `Config/Tags/*.ini`에 카테고리별로 둔다. `Native` 폴더와 같은 위치다. |
| ini 파일 이름 | 확정 | 엔진은 태그 소스를 파일 이름으로만 구분한다. 폴더가 달라도 이름이 같으면 한 소스로 합쳐지므로, 모든 태그 ini의 파일 이름은 전역에서 유일해야 한다. |
| 스크립트 | 확정 | PowerShell. 프로젝트 `Scripts/`에 두고 `ProjectKata.uproject`의 PreBuildSteps에서 호출한다. |
| 태그 소유 | 확정 | 태그는 프로젝트에만 둔다. 루트 이름은 `KataTag`다. Kata 플러그인은 태그를 정의하지 않고, `Config/Tags`와 생성 코드도 갖지 않는다. 플러그인 API와 에셋 프로퍼티는 임의의 `FGameplayTag`를 받으며 `KataTag`를 참조하지 않는다. 플러그인이 프로젝트 모듈을 참조하면 의존 방향이 역전되기 때문이다. |
| 컴포넌트 태그 저장소 | 확정 | 엔진의 `UActorComponent::ComponentTags`를 그대로 사용한다. 별도 레지스트리를 두지 않는다. 쓰기와 조회는 `FGameplayTag` API로만 한다. |
| 캐릭터 메시 | 확정 | Avatar의 기본 메시는 태그 없이 찾는다. 태그는 무기처럼 부착·탈착되는 컴포넌트에만 사용한다. |
| 컴포넌트 태그 정의 | 확정 | 이번 범위에서 컴포넌트 태그를 만들지 않는다. 부착 위치(주 손·보조 손 등)를 태그로 표현하지 않는다. API와 프로퍼티는 임의의 `FGameplayTag`를 받는다. 태그 정의는 장비 설계에서 정한다. |
| 파일 이름 규약 | 확정 | 접두사를 붙이지 않는다. `Native`와 에디터 쪽이 같은 이름의 파일을 갖지 않도록 카테고리 이름을 나누어 짓고, 겹치면 생성기가 빌드를 실패시킨다. |
| Cost SetByCaller 태그 | 결정 필요 | Cost 정책을 재개할 때 이 체계 위에서 이름을 정한다. 이번 범위에서는 정하지 않는다. |

### 입력 ini 형식과 위치

| 코드 태그 입력 | 에디터 태그 | 생성 위치 | 루트 |
|---|---|---|---|
| `Config/Tags/Native/*.ini` | `Config/Tags/*.ini` | `ProjectKata` 모듈의 `KataTags.h`·`KataTags.cpp` | `KataTag` |

- 형식은 엔진이 태그 ini를 저장하는 형식과 같다: `[/Script/GameplayTags.GameplayTagsList]` 섹션과
  `GameplayTagList=(Tag="...",DevComment="...")` 줄. **`+` 접두사를 붙이지 않는다.** 엔진은 `Config/Tags`의 개별 ini를
  `FConfigFile::Read`(명령 기호 처리 없음)로 읽으므로 `+GameplayTagList`는 다른 키로 인식되어 무시된다.
  2026-09-24 사용자 확인에서, `+` 줄만 있던 파일에 에디터로 태그를 추가하자 기존 줄이 지워졌다. 생성기는 명령 기호가 붙은 줄을 오류로 처리한다.
  생성기는 `Native` 폴더의 모든 ini를 읽어 한 루트로 합친다. 파일 사이의 태그 중복은 오류로 처리한다.
- 엔진은 `Config/Tags`를 하위 폴더까지 재귀로 읽는다(`AddTagIniSearchPath`의 `FindFilesRecursive`).
  따라서 `Native` 폴더의 태그는 ini 소스와 네이티브 코드에 모두 등록된다. 엔진은 네이티브 태그를 먼저 추가하고,
  소스 충돌은 Restricted 태그에만 검사하므로 일반 태그의 중복 등록은 문제가 되지 않는다.
- 이 방식을 쓰면 Project Settings에서 태그를 추가할 때 `Native` 폴더의 ini를 소스로 고를 수 있다. 추가한 태그는
  다음 빌드부터 C++에서 쓸 수 있다. 에디터에서 코드 태그의 이름을 바꾸거나 지우면 다음 빌드에서 생성 멤버가 바뀌고,
  이를 참조하는 코드가 컴파일 오류로 드러난다.
- 플러그인은 태그를 갖지 않으므로 플러그인 `Config/Tags` 경로를 등록하지 않는다.
  프로젝트의 `Config/Tags`는 엔진이 기본 경로로 읽는다.

### 생성 코드 형태

```cpp
// KataTags.h (생성)
namespace KataTagPrivate
{
    extern PROJECTKATA_API FNativeGameplayTag Mesh;
    extern PROJECTKATA_API FNativeGameplayTag Mesh_Character;
}

struct FKataTag_Mesh
{
    const FNativeGameplayTag& Character = KataTagPrivate::Mesh_Character;
    operator FGameplayTag() const { return KataTagPrivate::Mesh; }
};

struct FKataTagRoot
{
    FKataTag_Mesh Mesh;
};

inline const FKataTagRoot KataTag;
```

```cpp
// KataTags.cpp (생성)
namespace KataTagPrivate
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mesh, "Mesh", "");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mesh_Character, "Mesh.Character", "<DevComment>");
}
```

위 코드는 형태를 보여 주는 예시다. 실제 태그 이름은 필요할 때 정한다.

엔진 확인 결과에 따라 다음 규칙을 따른다.

- 부모 노드의 변환 연산자는 **값을 반환한다**. `FNativeGameplayTag`의 `operator FGameplayTag() const`가 값을 반환하므로,
  `const FGameplayTag&`를 반환하면 임시 객체를 가리키게 된다. `FGameplayTag`는 `FName` 하나이므로 복사 비용은 무시할 수 있다.
- 정의 매크로는 `.cpp` 파일에서만 쓸 수 있다. 매크로 안의 `static_assert(HasFileExtension(__FILE__))`가 이를 검사한다.
  따라서 정의는 `.inl` 등에 넣지 않고, 생성된 `.cpp`에 둔다.
- 모듈 경계를 넘어 참조하므로 `extern` 선언에 모듈 API 매크로를 붙인다. 엔진의 `UE_DECLARE_GAMEPLAY_TAG_EXTERN`은
  API 매크로를 붙이지 않으므로 생성기가 선언을 직접 출력한다.
- UBT는 소스 목록을 모으기 전에 PreBuildSteps를 실행한다(`BuildMode.cs`의 `LoadMakefile`과 makefile 생성 경로).
  처음 생성된 `.cpp`도 같은 빌드에 포함된다.
- 사용 예: `Tag.MatchesTag(KataTag.Mesh)`, `Container.HasTag(KataTag.Mesh.Character)`.
  `auto X = KataTag.Mesh`는 태그가 아니라 노드 구조체가 된다. 태그 값이 필요하면 `FGameplayTag X = KataTag.Mesh`로 쓴다.

### 생성 스크립트 규칙

- 위치: `Scripts/Generate-NativeGameplayTags.ps1`.
  인수: `-TagsDir`, `-NativeDir`, `-OutputHeader`, `-OutputSource`, `-RootName`, `-ApiMacro`. 생략하면 이 프로젝트의 기본 경로와
  `KataTag`·`PROJECTKATA_API`를 쓴다. 구조체 접두사는 루트 이름에서 만든다(`FKataTag_`, `FKataTagRoot`).
- 선언하지 않은 중간 부모도 네이티브 태그로 정의한다. 부모 노드를 태그로 변환하려면 정의가 필요하기 때문이다.
- Windows PowerShell 5.1이 한국어 주석을 읽을 수 있도록 스크립트만 UTF-8 BOM으로 저장한다.
- 호출: `ProjectKata.uproject`의 `PreBuildSteps`(Win64). Editor와 Game 타깃에서 모두 실행된다.
- 결과가 기존 파일과 같으면 파일을 쓰지 않는다. 헤더를 매번 다시 쓰면 이 헤더를 포함한 파일이 매 빌드마다 재컴파일된다.
- 다음 경우에는 0이 아닌 코드로 끝나 빌드를 실패시킨다: 태그 조각이 C++ 식별자로 쓸 수 없음(숫자로 시작, 허용되지 않는 문자, C++ 예약어),
  태그 중복, 형제 노드 이름 충돌, 구조체 멤버 이름과 겹치는 조각, `+`·`-` 등 ini 명령 기호가 붙은 줄,
  `Config/Tags` 전체(하위 폴더 포함)에서 같은 이름을 가진 ini 파일.
- 입력이 비어 있으면 빈 루트 구조체를 생성한다.
- `.gitignore`에 생성 파일 두 개를 추가한다.

### 컴포넌트 태그 공용 함수

`KataConditions` 모듈의 `KataFL_Component.h/.cpp`에 `UKataFL_Component`를 추가하고 Blueprint에도 공개한다.
[KataFL 규약](../devlog/Function-Library-and-Blueprint-Task-Diagnosis.md)에 따라 매개변수로 Kata 전용 구조체를 받지 않는다.

- `AddComponentGameplayTag(UActorComponent*, FGameplayTag)`와 `RemoveComponentGameplayTag`: `ComponentTags`에 태그 이름을 쓰고 지운다.
- `GetComponentGameplayTags(UActorComponent*)`: `ComponentTags` 중 등록된 태그로 변환되는 이름만 모아 `FGameplayTagContainer`로 반환한다.
- `FindComponentByGameplayTag(AActor*, FGameplayTag, bExactMatch, bIncludeAttachedActors)`와 배열 반환 버전.
  기본값은 계층 매칭(`MatchesTag`)이다. `bIncludeAttachedActors`는 부착된 무기 액터의 컴포넌트까지 검색한다.
- 태그로 변환되지 않는 `ComponentTags` 이름은 조회에서 무시하고 Verbose 로그를 남긴다.
  조회 함수는 조건 평가에서도 쓰므로 로그 외의 부작용을 만들지 않는다.

### 기존 FName ComponentTag 전환

- 대상: `UKataCondition_Distance`의 Location에 있는 `ComponentTag`(`FName`).
  Hit Trace가 쓸 "비어 있으면 Character Mesh, 지정하면 해당 태그의 SceneComponent가 정확히 하나" 규칙과 같다.
- 프로퍼티 이름은 유지하고 타입만 `FGameplayTag`로 바꾼다. `FGameplayTag::SerializeFromMismatchedTag`가 저장된 `NameProperty`를 읽으므로
  등록된 태그와 이름이 같은 값은 그대로 이어진다. 등록되지 않은 이름은 유효하지 않은 태그가 된다.
  기존 에셋은 전환 뒤 사용자가 확인한다.
- 조회는 `UKataFL_Component`를 사용하도록 바꾼다. 조건 테스트의 `ComponentTags.Add` 준비 코드도 태그를 기준으로 맞춘다.

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 상태 | 선행 조건 | 완료 조건 | 결과 기록 |
|---|---|---|---|---|---|---|
| T01 | 높음 | 생성 스크립트와 PreBuildSteps 연결 | 확인 완료 | 없음 | `Config/Tags/Native`의 ini들로부터 헤더·소스가 생성되고 변경이 없으면 파일을 쓰지 않음. 잘못된 입력이면 빌드 실패 | 미완료 |
| T02 | 높음 | `Config/Tags`와 `Native` 폴더 구조 추가, `ProjectKata` 모듈의 `GameplayTags` 의존성 추가 | 확인 완료 | T01 | `KataTag` 루트 생성. 카테고리별 ini가 Project Settings에 소스로 표시 | 미완료 |
| T03 | 높음 | `UKataFL_Component` 추가 | 보류 | T02 | 태그 부여·조회·계층 매칭·부착 액터 검색 함수를 C++과 BP에서 호출 가능 | 미완료 |
| T04 | 보통 | `UKataCondition_Distance`의 `ComponentTag` 전환 | 보류 | T03 | 태그 피커로 지정하고, 기존 에셋 값이 유지되거나 무효로 표시됨 | 미완료 |
| T05 | 보통 | 프리뷰 무기 부착 설정 | 보류 | T03 | 프리뷰 캐릭터에 메시·부착 소켓·사용자가 지정한 컴포넌트 태그로 무기가 붙고 태그로 조회됨 | 미완료 |
| T06 | 보통 | 문서 반영 | 보류 | T01~T05 | 구현 상태·설명서에 태그 추가 방법과 컴포넌트 지정 방법이 반영됨 | 미완료 |

T03~T06은 2026-09-24 사용자 결정으로 보류했다. 다른 세션에서 플러그인·모듈 정리를 진행하므로, 그 결과로 모듈 경계가
확정된 뒤 `UKataFL_Component`의 위치를 다시 확인하고 재개한다. T01·T02 결과는
[구현 기록](../devlog/2026-09-24-Gameplay-Tag-Generation.md)과 [사용법](../manual/Gameplay-Tags.md)에 있다.

T05의 설정 위치와 형태는 착수할 때 멀티 타겟 프리뷰 배치 계획([다음 작업 계획](Next-Work-Plan.md#멀티-타겟-액터-배치))과 함께 정한다.
두 작업 모두 프리뷰 액터 사양을 바꾸기 때문이다.

## 영향과 제한

- 빌드 흐름: 모든 빌드가 PowerShell 스크립트 실행에 의존한다. Win64 이외의 플랫폼용 스크립트는 만들지 않는다.
- 모듈 경계: 생성 코드는 `ProjectKata` 모듈에 있고 `GameplayTags` 모듈만 사용한다. 현재 `ProjectKata.Build.cs`에는
  `GameplayTags` 의존성이 없어 추가한다. 에디터 의존성은 추가하지 않는다.
  `UKataFL_Component`는 태그 값을 인수로 받으므로 플러그인에 둬도 프로젝트 태그에 의존하지 않는다.
- 직렬화: T04에서 프로퍼티 타입이 바뀐다. 이름은 유지하므로 Redirect는 필요 없다.
- 코드 태그를 추가하거나 이름을 바꾸려면 다시 빌드해야 C++에서 쓸 수 있다. 이는 네이티브 태그가 원래 가진 제약이다.
- 태그 이름 변경: 생성된 멤버 이름이 바뀌어 참조하는 코드는 컴파일 오류가 난다. 오타를 잡는 효과가 있지만, 저장된 에셋의 태그 값에는
  `GameplayTagRedirects`가 별도로 필요하다.

## 사용자 확인 항목

2026-09-24 사용자 확인(Rider 빌드, 에디터): 빈 입력 생성, `Test.ini`로부터 `KataTag.Test.A.B`·`KataTag.Test.C` 생성,
Gameplay Tag Manager 표시(네이티브 소스 `ProjectKata`로 표시), `Native` 폴더 ini에 에디터로 태그를 추가해도 기존 줄 유지,
한국어 DevComment 전달. 이 확인은 `+` 접두사 금지 수정 이후 기준이다.

- Editor와 Game 빌드에서 생성 스크립트가 실행되고, 태그를 바꾸지 않은 두 번째 빌드에서 불필요한 재컴파일이 없다.
- 잘못된 태그 이름을 넣었을 때 빌드가 원인을 출력하고 실패한다.
- Project Settings에서 `Native` 폴더의 ini를 소스로 골라 태그를 추가하면 다음 빌드에서 생성 멤버가 생긴다.
- 기존 Distance 조건 에셋의 Component Tag 값이 전환 뒤 어떻게 표시되는지 확인한다.

## 완료 시 갱신할 문서

- [현재 구현 상태](../devlog/Implementation-Status.md): 태그 체계, 생성 경로, `UKataFL_Component`, 전환한 프로퍼티.
- 태그 사용 설명서(새 manual): 코드 태그와 에디터 태그를 추가하는 방법, 컴포넌트 태그를 부여하는 방법.
- [Kata 공통 요청 메모](Kata.md): 4번 질문에 결과를 연결한다.
- [다음 작업 계획](Next-Work-Plan.md): 진행 상태를 반영하고 Cost 태그 규약 결정을 연결한다.
- [문서 목록](../README.md): 새 manual과 devlog 링크.
