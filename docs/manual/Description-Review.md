# 설명 변경 검토 사용법

갱신: 2026-10-13  
대상: Kata에서 주석·manual·태그 설명을 작성하는 Claude와 부분 교정을 맡는 Codex  
적용 기준: [ko-review 플러그인](https://github.com/jaykop/claude-plugins/tree/main/plugins/ko-review)(`review_queue.py`), `.claude/ko-review/profile.md`, AGENTS.md "설명 변경의 공유 검토"  
확인 상태: 플러그인 도구로 이 문서를 고친 작업(#94)의 목록을 capture·register했다. 플러그인 설치와 스킬 호출, Codex 쪽 검토 실행은 확인하지 않았다.

## 목적과 준비

작업마다 바꾼 설명과 로직 변경에 영향을 받는 기존 설명을 공유 목록에 남긴다. Claude가 기록하고 Codex가 요청받은 대기 항목을 읽어 교정한다. 전체 프로젝트를 매번 읽는 대신 해당 문단·주석과 직접 관련된 구현을 대조한다.

기록 도구와 스킬은 ko-review 플러그인이 제공한다. 설치는 AGENTS.md "AI 작업 플러그인"을 따른다. Python 3.8 이상과 Git이 필요하고, 프로젝트 루트에서 실행한다. 목록 형식, 명령, 위치 지정 방법의 원문은 플러그인의 [목록 형식](https://github.com/jaykop/claude-plugins/blob/main/plugins/ko-review/shared/queue-format.md)이다. 이 문서는 Kata에서 쓰는 값과 절차만 적는다.

| Kata 값 | 내용 |
|---|---|
| 목록 위치 | `docs/localdocs/threads/<task>.descriptions.json`. Git에서 제외하고 Diversion에서 추적한다 |
| 작업 ID | 이슈 번호가 있으면 `<번호>-<slug>`, 없으면 `<slug>`. 인계 메모 파일 이름과 같게 쓴다 |
| 대상 | 자체 소스의 주석, `docs/manual/`, 태그 ini(`Config/Tags/*.ini`, `Config/DefaultGameplayTags.ini`)의 `DevComment`. 벤더 플러그인과 생성 파일은 제외한다 |
| 작성·검토 | 작성은 Claude, 검토는 Codex. 검토 요청은 사용자가 전달한다 |

GitHub Issue는 작업 상태, 이 목록은 설명 검토 상태를 관리한다. 빌드·실행 확인과 서로 대신하지 않는다.

## 사용 순서

### Claude: 시작과 종료 기록

ko-review의 `record` 스킬이 아래 순서로 도구를 실행한다. workflow의 `feature-impl`·`feature-doc-sync`도 같은 단계에서 이 스킬을 쓴다.

1. `git status --short`와 기존 작업 메모를 읽고 작업 ID를 정한다. 같은 작업의 목록이 있으면 다시 만들지 않는다.
2. 파일을 고치기 전에 명시적 파일 목록으로 `start`한다. 디렉터리 전체를 지정하지 않는다. 작업 중 새로 고칠 파일은 고치기 전에 `scope`로 더한다. 이미 고친 뒤 더했다면 그 사실을 작업 메모와 항목의 reason에 적는다.
3. 구현과 문서 반영 뒤 `capture`한다. 빌드 확인이나 후속 질문을 기다리느라 기록을 미루지 않는다.
4. 바꾼 설명마다 항목을 JSON 배열로 써서 `register`한다. 입력 JSON도 `docs/localdocs/threads/`에 둔다. 아래는 태그 설명 항목의 예다.

   ```json
   [
     {
       "id": "super-armor-description",
       "kind": "data-comment",
       "reason": "슈퍼아머와 그로기 판정 우선순위를 설명한다",
       "selector": {
         "path": "Config/Tags/Status.ini",
         "anchor": "Tag=\"Status.SuperArmor\""
       },
       "evidence": [
         {
           "path": "Plugins/KataFramework/Source/KataFramework/Private/Attributes/KataHitReactionEffectComponent.cpp",
           "anchor": "if (Stance->GetMaxGroggy()",
           "mode": "span",
           "endAnchor": "UE_LOG(LogKataFramework, Verbose"
         }
       ]
     }
   ]
   ```

   kind는 코드 주석이 `comment`, manual이 `doc`, 태그 `DevComment`가 `data-comment`다. 설명을 고치지 않았어도 구현 변경으로 기존 설명이 실제 동작과 달라졌다면 등록한다.
5. 설명에 영향을 주지 않는 변경은 `no-impact`로 이유를 남긴다. `pending` 출력의 미등록 파일과 실제 변경 구간을 비교해 빠진 기록을 채운다.

### Codex: 대기 항목 교정

ko-review의 `review` 스킬이 아래 순서로 진행한다. 교정 기준은 같은 플러그인의 `polish`(에디터 주석은 언리얼 기준 포함)다.

1. 지정한 작업의 `pending`을 읽는다. 작업을 지정하지 않았다면 `*.descriptions.json`에서 대기 항목을 고른다. 파일 단위의 미등록 알림은 기록 누락으로 다루고 전체 프로젝트 검토로 넓히지 않는다.
2. 항목마다 `begin`으로 스냅샷과 token을 받고, 설명과 근거 구현을 대조해 교정한다.
3. 구현 결함이나 정하지 않은 동작이면 `needs-decision`으로 기록한다. 설명에 맞추려고 구현을 바꾸지 않는다.
4. `finish`로 결과를 기록한다. 직접 설명을 고쳤다면 최종 본문을 읽고 `snapshot`이 출력한 해시를 `--description-hash`로 넘긴다.

## 주요 설정과 실행 계약

| 항목 | 의미·조건 |
|---|---|
| 프로필 `queue_dir` | 목록 폴더. Kata는 `docs/localdocs/threads` |
| 프로필 `writer`, `reviewer` | 검토 기록에 남는 이름. Kata는 Claude, Codex |
| 프로필 `terms_file` | Kata 전용 금지어 목록 `.claude/ko-review/terms.json`. `doc_check.py`가 기본 목록과 함께 읽는다 |
| `status` | pending·reviewed·needs-decision. 설명이나 근거 내용이 바뀌면 다음 pending·capture·begin에서 다시 pending |

anchor는 파일 안의 한 줄에만 맞는 문자열로 지정한다. 선택 범위(`lines`·`heading`·`span`)와 목록의 칸은 플러그인의 목록 형식을 따른다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| Windows 콘솔에서 `UnicodeEncodeError` | 콘솔 인코딩 | `python -X utf8`로 실행한다. 출력 오류가 났다고 기록까지 저장됐다고 가정하지 말고 목록 파일을 확인한다 |
| `Queue changed …`로 거절 | 다른 쓰기나 편집이 겹쳤다 | 목록을 다시 읽고 이어서 실행한다 |
| `.lock` 파일이 남음 | 비정상 종료 | 다른 처리가 실행 중이 아닌지 확인한 뒤 그 잠금 파일만 지운다 |
| 예전 목록의 kind가 `manual`·`tag-devcomment` | 2026-10-12에 만든 Kata 도구의 이름 | 그대로 둔다. 도구가 읽을 수 있고, 다시 등록할 때만 새 이름으로 바뀐다 |

- 자동 캡처는 작업 파일의 변경·diff 구간까지만 찾고 의미상의 영향은 판정하지 않는다. `unassignedChanges`는 파일 단위 알림이므로 같은 파일 안의 미등록 구간은 Claude가 대응시킨다.
- 시작 전에 변경돼 있던 파일은 해시와 상태만 기록된다. 이전 변경과 이번 변경의 구분이 모호하면 확인하지 못한 부분을 완료 처리하지 않는다.
- 해시 일치는 문장의 품질을 판정하지 않는다. `Docs-Reviewed` 트레일러는 문서 반영 절차를 뜻하며 이 목록을 대신하지 않는다. AI 자동 호출·메시지 전송은 없다.

## 확인 상태와 근거

- [ko-review 플러그인](https://github.com/jaykop/claude-plugins/tree/main/plugins/ko-review): `review_queue.py`, `record`·`review`·`polish` 스킬.
- [채택 이유와 처음 구현](../devlog/2026-10-12-Description-Review-Pipeline.md). 이 기록의 도구는 Kata `Scripts/DescriptionReview.py`였고 #94에서 플러그인으로 옮겼다.
- [작업 상태](https://github.com/jaykop/Kata/issues/94).
- 2026-10-12에 이전 Kata 도구로 start·register·capture를 실행했다. 2026-10-13 #94 작업의 목록은 이전 도구로 start하고 플러그인 도구로 capture·register했다. 플러그인 설치 경로에서의 실행과 Codex 쪽 begin·finish는 확인하지 않았다. 빌드·테스트·lint는 실행하지 않았다.
