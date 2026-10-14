# Blockout Tools 도입과 기획 문서 분리

작성: 2026-10-14  
갱신: 2026-10-14  
유형: 결정 기록  
대상: 샘플 프로젝트 플러그인 구성, 저장소별 추적 범위, 문서 분류  
기준: d012e57 위의 #74 작업 트리. 다른 작업의 미커밋 변경(`MVVMToolset` 검사 접두사)은 포함하지 않는다.

## 배경과 결론

#74 전투 샌드박스 맵을 만들기 전에, 테스트 레벨을 다시 짓지 않고 계획적으로 만드는 절차를 정하기로 했다. 그 과정에서 두 가지를 결정했다.

- 레벨 블록아웃에 Fab 상품인 Blockout Tools 플러그인(v1.52)을 샘플 프로젝트에서만 쓴다. 마켓플레이스 라이선스 때문에 공개 저장소(Git)에서 제외하고 Diversion에서만 추적한다.
- 레벨 제작 절차, 레벨 메트릭, 레벨별 도안은 Kata 사용법이 아니라 샘플 게임의 기획 문서로 본다. `docs/design/`에 두고 Git에서 제외하며 Diversion에서만 추적한다.

첫 적용으로 샘플 맵 `LV_CombatSandbox`를 블록아웃했다. 맵은 `/Content/`에 있어 Git에는 올라가지 않는다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| Blockout Tools | 없음 | `Plugins/Blockout`에 두고 샘플 전용으로 쓴다. `.uproject`에는 등록하지 않는다 |
| `.gitignore`·`.dvignore` | `docs/design`, `Plugins/Blockout` 규칙 없음 | Git에서 제외하고 Diversion에서 재포함한다. 플러그인의 `Binaries`·`Intermediate`는 Diversion에서도 제외한다 |
| AGENTS.md | 해당 항목 없음 | 현재 범위에 샘플 전용 벤더 플러그인 항목, 저장소별 추적 범위에 두 경로, 문서 작성 절에 기획 문서 위치를 더했다 |
| 규칙 검사 | 샘플 전용 접두사에 Blockout 없음 | `SAMPLE_ONLY_PREFIXES`에 `BlockoutTools`를 더하고 [규칙 검사 사용법](../manual/Agents-Rules-Check.md)을 고쳤다 |
| 문서 목록 | 기획 문서 안내 없음 | [문서 목록](../README.md)에 기획 문서 절을 더했다. Git에 없는 파일이라 링크하지 않는다 |

## 주요 결정과 이유

- **`.uproject`에 등록하지 않는다.** Git 사본에는 플러그인 폴더가 없으므로 등록하면 GitHub 클론에서 누락 경고가 난다. 프로젝트 플러그인은 기본으로 켜지므로 등록하지 않아도 쓸 수 있다. 블록아웃 액터는 Runtime 모듈 클래스라 Editor 타깃으로만 제한할 수도 없다.
- **규칙 검사는 접두사로 판정한다.** GitHub Actions의 Git 사본에는 플러그인이 없어 엔진 플러그인과 같은 방식으로 이름 접두사만 본다.
- **기획 문서를 manual·plan과 분리한다.** manual은 Kata 기능의 현재 사용법, plan은 이슈를 닫으면 지우는 설계 문서다. 레벨 도안은 레벨이 남아 있는 동안 유지해야 하고, 장르를 정한 뒤에는 게임 기획 문서가 늘어날 것이라 별도 폴더로 뒀다. 공개 저장소에 올리지 않는 것은 사용자 결정이다.
- **도안은 문서의 좌표 표를 원본으로 해서 MCP로 블록아웃한다.** 첫 블록아웃 뒤에는 레벨이 지오메트리의 원본이 되고, 문서는 존의 의도와 캡처만 갱신한다. 손으로 그린 이미지를 보고 배치하는 방식은 치수 오차가 생겨 택하지 않았다.
- 아트 컨셉, 레벨 연출, 조명은 장르를 정한 뒤(#81) 절차에 붙인다.

## 근거

- [AGENTS.md](../../AGENTS.md): "프로젝트와 현재 범위", "커밋과 저장소별 추적 범위", "문서 작성과 동기화".
- [규칙 검사 스크립트](../../Scripts/CheckAgentsRules.py): `SAMPLE_ONLY_PREFIXES`.
- [.gitignore](../../.gitignore), [.dvignore](../../.dvignore).

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| Git 제외 | `git status`, Claude | `docs/design`, `Plugins/Blockout`이 추적 대상에서 빠졌다 | — |
| Diversion 포함 | `dv status`, Claude | 두 경로가 새 항목으로 잡히고 플러그인 `Binaries`·`Intermediate`는 잡히지 않았다 | 다른 PC에서 받아 빌드 |
| 플러그인 로드 | 에디터 MCP, Claude | `/BlockoutToolsPlugin/` 콘텐츠 경로가 보이고 블록 액터를 배치할 수 있었다 | — |
| Kata 플러그인 참조 | .uplugin·Build.cs 텍스트 검색, Claude | Blockout 참조 없음 | 규칙 검사 스크립트 실행 |
| `LV_CombatSandbox` | 사용자 PIE (2026-10-14) | 시작, 적 생성, NavMesh 추격, 카메라, 보스 생성, 사망 뒤 재시작 확인 | 재시작 뒤 보스 체력 회복 |

## 남은 제한과 후속 작업

- `.uplugin`의 `"Installed": true`가 `Binaries`가 없는 PC에서 소스 빌드를 막는지 확인하지 않았다.
- 사전 플레이에서 나온 문제는 #74에서 이어서 다룬다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#74](https://github.com/jaykop/Kata/issues/74) | 맵 구성 완료와 추가 범위를 댓글로 남긴다. 게시는 사용자 확인 후 |
| [규칙 검사 사용법](../manual/Agents-Rules-Check.md) | 접두사 표와 판정 방식을 고쳤다 |
| [플러그인 분리 결정 기록](2026-10-10-Plugin-Modularization-Decisions.md) | 영향 없음. Kata 플러그인 구성만 다룬다 |
