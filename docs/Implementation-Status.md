# Kata 구현 상태

갱신: 2026-09-19

## 확정 범위

- 플러그인: Kata. 엔진: UE 5.8.
- GAS 필수, 싱글플레이 전용.
- 현재 모듈: KataConditions, KataRuntime, KataEditor.
- KataAI, BT/StateTree 연동, 네트워크·예측은 현재 구현 범위에 포함하지 않는다.
- 프로젝트 루트는 개발·검증용 호스트이며 재사용 코드는 Plugins/Kata에 둔다.

## 기본 조건 구현 및 현재 작업 원칙

- `UKataCondition`과 Context·Pass/Fail/Invalid 결과, 공통 Invert, C++·Blueprint 확장 지점 구현.
- Tag: Self/Target, Any/All, Exact Match.
- Attribute: 절대값/Current-to-Max Ratio, 비교 연산과 같음 허용 오차.
- Distance: Self/Target별 Actor/Socket 위치, 컴포넌트 태그 선택, 2D/3D, 최소·최대 거리.
- Angle: 2D/3D, HalfAngle, 방향을 회전시키는 YawOffset.
- 사용자 지침 변경 전에 원본 프로젝트의 Editor 컴파일·링크는 성공했다. 기본 조건 자동화 테스트와 변경 후 Game 빌드는 실행하지 않았다.
- 테스트 코드는 사용자 실행용으로 추가되어 있다. 이후 테스트 작성·실행, 빌드, 별도 검사는 명시적으로 요청받을 때만 수행한다.
- 작업 경로는 `C:\Users\jeyko\Documents\Unreal Projects\ProjectKata`로 통일했다. 이전 ChatGPT 경로의 에이전트 생성 임시 작업 사본은 제거했다.

## 이번 초기 구성

- Kata.uplugin과 Runtime 2개 / Editor 1개 모듈의 Build.cs·공개 모듈 헤더·진입점.
- GameplayAbilities 필수 플러그인 의존성 및 호스트 프로젝트의 Kata/GAS 활성화.
- 호스트 게임 모듈에서 KataRuntime과 KataConditions를 사용하는 빌드 의존성.
- Editor/Game 빌드 스크립트, UE 생성 파일 제외 규칙, uasset/umap Git LFS 규칙.
- 공용 AGENTS.md와 이를 읽는 CLAUDE.md, 저장소 README.
- 공개 커밋에 포함하지 않도록 공유 설정의 생성된 Android 디버그 보안 토큰 제거.

## 아직 구현하지 않은 기능

Kata 에셋과 실행기, 태스크, AbilityTask 연결, 콤보 에셋·입력 버퍼, 그래프·타임라인 편집기, 프리뷰 시뮬레이션은 후속 작업이다. 기본 조건의 사용법은 `Conditions.md`를 참조한다.

콤보 전이의 데이터 소유권과 Ability 대응 단위는 기존 설계 문서에서 제안한 안이며 아직 구현으로 확정하지 않았다. GenericGraph는 추가하지 않았다.

## 초기 골격 당시 검증 기록

검증 환경: Windows, UE 5.8, Visual Studio 2022 MSVC 14.44, Windows SDK 10.0.22621.0.

| 확인 항목 | 결과 |
|---|---|
| 플러그인 JSON 및 PowerShell 빌드 스크립트 문법 | 통과 |
| ProjectKata Win64 Development (Game) | 원본 프로젝트에서 전체 컴파일·링크 성공 |
| ProjectKataEditor Win64 Development | 동일 소스의 별도 검증 사본에서 전체 컴파일·링크 성공 |
| 원본·검증 사본의 소스 및 descriptor 비교 | 16개 파일 SHA-256 일치 |
| Game 빌드의 Editor 모듈 분리 | Game build receipt에 KataEditor 없음 |
| 공개 커밋 후보 점검 | 30개 소스·설정·문서 파일, 점검한 민감 값 패턴 및 10MB 초과 파일 없음 |
| 생성 파일 제외 규칙 | 프로젝트·플러그인 Binaries/Intermediate, Saved, 솔루션, .env 제외 확인 |

원본 경로의 Editor 빌드는 실행 중인 Unreal Editor가 `UnrealEditor-ProjectKata.dll`을 잠가 마지막 링크가 실패했다. 플러그인 세 모듈은 원본에서도 컴파일·링크되었다. 사용자 에디터를 강제 종료하지 않고 `Saved/BuildValidation/InitialScaffold`에 소스를 복사해 Editor 전체 빌드를 성공시켰다. 이 검증 사본과 생성물은 Git에서 제외한다.

원본의 Editor 출력까지 갱신하려면 열려 있는 에디터에서 작업을 저장하고 종료한 다음 `Scripts/Build.ps1 -Target Editor`를 실행한다. 모듈·플러그인 구성 변경은 에디터를 다시 열어 반영한다.

에디터 UI 실행, 게임플레이 동작, 쿠킹/패키징은 이번 골격 작업의 검증에 포함하지 않았다. 컴파일 성공을 기능 구현 완료로 간주하지 않는다.

## 다음 작업 계획

다음 작업은 `Next-Work-Plan.md`에 정리한 단일 Kata 에셋·실행기·GAS AbilityTask·몽타주 태스크 연결이다. Claude Code가 공용 지침을 읽고 이어서 진행할 수 있도록 순서와 범위를 기록했다. 테스트·빌드·검사는 사용자가 담당하며 에이전트는 명시적인 요청 없이는 수행하지 않는다.
