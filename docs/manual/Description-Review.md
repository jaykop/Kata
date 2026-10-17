# 설명 변경 검토 사용법

갱신: 2026-10-17  
대상: Kata에서 주석·manual·태그 설명을 작성하는 Claude와 부분 교정을 맡는 Codex  
적용 기준: [ko-review 플러그인](https://github.com/jaykop/claude-plugins/tree/main/plugins/ko-review)(`review_queue.py`), `.claude/ko-review/profile.md`, AGENTS.md "설명 변경의 공유 검토"  
확인 상태: 2026-10-13 Codex가 설치된 ko-review의 `review` 스킬 절차로 #94의 대기 항목 5개를 교정하고 `begin`·`snapshot`·`finish`로 결과를 기록했다. 훅 승인 상태와 커밋 차단 동작은 이번 검토에서 확인하지 않았다. 2026-10-16에 `codex exec` 넘기기를 처음 실행했다. Codex는 교정을 마쳤지만 당시 지시문이 목록 기록을 허용하지 않아 `finish`를 남기지 않았고, 이후 지시문을 고쳤다. 고친 지시문은 아직 실행해 보지 않았다.

## 목적과 준비

작업마다 바꾼 설명과 로직 변경에 영향을 받는 기존 설명을 공유 목록에 남긴다. Claude가 기록하고, 커밋 직전에 Claude가 넘긴 대기 항목을 Codex가 읽어 교정한다. 전체 프로젝트를 매번 읽는 대신 해당 문단·주석과 직접 관련된 구현을 대조한다.

기록 도구와 스킬은 ko-review 플러그인이 제공한다. 설치는 AGENTS.md "AI 작업 플러그인"을 따른다. Python 3.8 이상과 Git이 필요하고, 프로젝트 루트에서 실행한다. 목록 형식, 명령, 위치 지정 방법의 원문은 플러그인의 [목록 형식](https://github.com/jaykop/claude-plugins/blob/main/plugins/ko-review/shared/queue-format.md)이다. 이 문서는 Kata에서 쓰는 값과 절차만 적는다.

| Kata 값 | 내용 |
|---|---|
| 목록 위치 | `docs/localdocs/threads/<task>.descriptions.json`. Git에서 제외하고 Diversion에서 추적한다 |
| 작업 ID | 이슈 번호가 있으면 `<번호>-<slug>`, 없으면 `<slug>`. 인계 메모 파일 이름과 같게 쓴다 |
| 대상 | 자체 소스의 주석, `docs/manual/`, 태그 ini(`Config/Tags/*.ini`, `Config/DefaultGameplayTags.ini`)의 `DevComment`. 벤더 플러그인과 생성 파일은 제외한다 |
| 작성·검토 | 작성은 Claude, 검토는 Codex. 검토는 커밋 직전에 Claude가 `codex exec`로 넘긴다 |

GitHub Issue는 작업 상태, 이 목록은 설명 검토 상태를 관리한다. 빌드·실행 확인과 서로 대신하지 않는다.

## 사용 순서

### Claude: 시작과 종료 기록

ko-review의 `record` 스킬이 아래 순서로 도구를 실행한다. workflow의 `feature-impl`·`feature-doc-sync`도 같은 단계에서 이 스킬을 쓴다.

1. `git status --short`와 기존 작업 메모를 읽고 작업 ID를 정한다. 같은 작업의 목록이 있으면 다시 만들지 않는다.
2. 파일을 고치기 전에 명시적 파일 목록으로 `start`한다. 디렉터리 전체를 지정하지 않는다. 작업 중 새로 고칠 파일은 고치기 전에 `scope`로 더한다. 이미 고친 뒤 더했다면 그 사실을 작업 메모와 항목의 reason에 적는다.
3. 구현과 문서 반영 뒤 `capture`한다. 빌드 확인이나 후속 질문을 기다리느라 기록을 미루지 않는다.
4. 프로필 `queue_scope` 안에서 바꾼 설명마다 항목을 JSON 배열로 써서 `register`한다. `exclude`에 해당하는 파일은 제외한다. 입력 JSON도 `docs/localdocs/threads/`에 둔다. 아래는 태그 설명 항목의 예다.

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
5. 설명에 영향을 주지 않는 변경은 `no-impact`로 이유를 남긴다. 범위 밖 파일도 `register` 대신 `no-impact`로 범위 밖임을 기록한다. `pending` 출력의 미등록 파일과 실제 변경 구간을 비교해 빠진 기록을 채운다.

### Claude: 커밋 전 Codex 검토 넘기기

사용자가 커밋을 요청하면 `commit-doc-review`보다 먼저 이 단계를 거친다. Codex가 설명을 고칠 수 있으므로 문서 반영 검토는 그 뒤에 한다.

1. 커밋할 작업의 `pending`을 읽는다. `entries`에 검토할 항목이 없으면 Codex 호출을 건너뛴다. `outOfScopeEntries`는 검토하지 않고 목록 경로와 항목 ID를 사용자에게 보고한다.
2. 프로젝트 루트에서 그 작업의 목록 파일 하나만 지정해 Codex를 실행한다. 목록을 지정하지 않으면 `review` 스킬이 다른 작업의 목록까지 고를 수 있다.

   ```powershell
   codex exec -C . -s workspace-write "ko-review의 review 스킬로 docs/localdocs/threads/<task>.descriptions.json의 대기 항목만 검토해. outOfScopeEntries는 검토하지 말고 보고만 해. 이전 실행에서 begin만 하고 마치지 못한 항목이 있으면 begin부터 다시 해. 할 일은 문장을 자연스러운 한국어로 교정하는 것뿐이야. 코드 동작을 판단하거나 추정한 한계·동작을 설명에 새로 쓰지 마. 설명이 근거와 어긋나 보이면 설명을 고치지 말고 needs-decision 메모에 위치만 적어. 설명 교정은 그 목록의 taskPaths 안에서만 하고, 목록 파일에는 review_queue.py의 begin·finish로 결과를 기록해. 스테이징·커밋하지 마."
   ```

   목록 파일은 `taskPaths`에 들어 있지 않다. 설명 교정의 범위와 목록 기록의 허용을 구분해 적는다. 첫 실행에서는 이 구분이 없어 Codex가 목록 쓰기까지 금지로 해석했고, 교정만 하고 `finish`를 남기지 않았다.

3. Codex의 마지막 메시지, 목록의 검토 결과, Codex가 고친 파일의 diff를 사용자에게 보여 준다.
4. 검토 뒤에도 대기 항목이 남으면 Codex를 다시 부르지 않고 사용자에게 보여 준다. Codex가 고친 설명이 다른 항목의 근거이면 그 항목이 다시 대기로 돌아가는데, 이때 다시 부르면 넘기기가 되풀이될 수 있다.
5. 커밋할 diff에서 각 변경 구간이 이 작업에 속하는지 확인한 뒤 해당 경로만 지정해 스테이징한다. 같은 파일에 다른 작업의 변경이 섞여 있으면 이 작업의 변경 구간만 고른다. 비대화형으로 구간을 고를 때는 커밋 기준 내용에 이 작업의 구간만 적용한 내용을 `git hash-object -w`로 만들고 `git update-index --cacheinfo`로 올린다. 여러 세션이 같은 작업 트리를 쓰므로 `git add -A`처럼 전체를 담지 않는다.

### Codex: 대기 항목 교정

ko-review의 `review` 스킬이 아래 순서로 진행한다. 교정 기준은 같은 플러그인의 `polish`(에디터 주석은 언리얼 기준 포함)다.

Kata에서 Codex의 역할은 대기 항목의 주석·문서·`DevComment`를 자연스러운 한국어로 교정하는 것이다. 코드 기능 검수, 버그 판단, 동작 추정은 Codex의 역할이 아니다. 근거 구현은 교정이 원래 뜻을 바꾸지 않았는지 확인하는 데만 읽는다. 재현하지 않은 동작이나 한계를 설명에 새로 쓰지 않는다. 설명과 근거가 어긋나 보이면 설명을 고치지 않고 `finish` 메모에 어긋난 위치만 적는다. Claude는 사용자에게 확인을 제안한다. 2026-10-17에 Codex가 코드를 읽고 추정한 한계를 manual과 주석에 사실처럼 적은 일이 있어 이렇게 정했다.

1. 지정한 작업의 `pending`을 읽고 `entries`의 항목만 검토한다. `outOfScopeEntries`는 검토하거나 완료로 기록하지 않고, 목록 경로와 항목 ID만 보고한다. 작업을 지정하지 않았다면 `*.descriptions.json`에서 대기 항목을 고른다. 파일 단위의 미등록 알림은 기록 누락으로 다루고 전체 프로젝트 검토로 넓히지 않는다.
2. 항목마다 `begin`으로 스냅샷과 token을 받고, 문장을 교정한다. 이전 실행에서 `begin`만 하고 끝나지 않은 항목도 `begin`부터 다시 검토한다. 설명 교정은 목록의 `taskPaths` 안에서만 한다. `--reviewer`를 생략하면 프로필의 `reviewer` 값(Codex)이 검토자로 기록된다. ko-review 0.1.0은 프로필을 읽지 않고 `reviewer`로 기록했으므로 그 버전에서는 `--reviewer Codex`를 붙인다.
3. 설명이 근거와 어긋나 보이면 설명을 바꾸지 않고 `needs-decision`으로 기록한다. 메모에는 어긋난 위치와 근거 위치만 적고, 원인이나 결함을 추정하지 않는다. 구현은 바꾸지 않는다.
4. `finish`로 결과를 기록한다. 직접 설명을 고쳤다면 최종 본문을 읽고 `snapshot`이 출력한 해시를 `--description-hash`로 넘긴다.

## 주요 설정과 실행 계약

| 항목 | 의미·조건 |
|---|---|
| 프로필 `queue_dir` | 목록 폴더. Kata는 `docs/localdocs/threads` |
| 프로필 `queue_scope`, `exclude` | 설명 파일의 등록 범위와 제외 경로. Kata는 자체 소스·manual·태그 ini를 대상으로 하며 devlog·plan·README는 제외한다. `register`는 범위 밖 설명을 거절하고 `pending`은 범위 밖 대기 항목을 `outOfScopeEntries`로 따로 알린다. 근거 파일의 경로는 제한하지 않는다 |
| 프로필 `writer`, `reviewer` | 작성·검토 역할의 이름. Kata는 Claude, Codex. `begin`에 `--reviewer`를 주지 않으면 도구가 `reviewer` 값을 검토 기록에 남긴다(ko-review 0.1.1부터) |
| 프로필 `terms_file` | Kata 전용 금지어 목록 `.claude/ko-review/terms.json`. `doc_check.py`가 기본 목록과 함께 읽는다 |
| `status` | pending·reviewed·needs-decision. 설명이나 근거 내용이 바뀌면 다음 pending·capture·begin에서 다시 pending |

anchor는 파일 안의 한 줄에만 맞는 문자열로 지정한다. 선택 범위(`lines`·`heading`·`span`)와 목록의 칸은 플러그인의 목록 형식을 따른다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| Windows 콘솔에서 `UnicodeEncodeError` | 콘솔 인코딩 | `python -X utf8`로 실행한다. 출력 오류가 났다고 기록까지 저장됐다고 가정하지 말고 목록 파일을 확인한다 |
| `Queue changed …`로 거절 | 다른 쓰기나 편집이 겹쳤다 | 목록을 다시 읽고 이어서 실행한다 |
| `finish`가 `Implementation evidence changed during review; begin again`으로 거절 | `begin` 이후 선택한 근거 내용이 바뀌었다. 검토자 자신의 교정으로 바뀐 경우도 포함한다 | 변경 원인을 사용자에게 알리고, 같은 Codex 호출 안에서 현재 설명·근거를 읽고 `begin`부터 다시 검토한다 |
| `finish`가 `Description changed …`로 거절 | 최종 설명 해시가 전달한 해시와 다르다. `--description-hash`를 생략하면 `begin` 때의 해시와 비교한다 | 직접 고친 설명이면 최종 본문을 다시 읽고 `snapshot`의 해시를 `--description-hash`로 넘긴다. 다른 작업자가 고쳤다면 사용자에게 알리고, 같은 Codex 호출 안에서 `begin`부터 다시 검토한다 |
| `codex exec`가 `The '<모델>' model is not supported when using Codex with a ChatGPT account`로 실패 | 첫 실행에서는 PATH의 `codex`(npm 전역 CLI)가 오래돼 설정된 모델을 지원하지 않았다 | CLI 버전과 설정된 모델의 지원 여부를 확인한다. 같은 원인이면 CLI를 업데이트하거나, 해당 모델을 지원하는 Codex 앱의 `codex.exe` 경로로 실행한다 |
| `.lock` 파일이 남음 | 비정상 종료 | 다른 처리가 실행 중이 아닌지 확인한 뒤 그 잠금 파일만 지운다 |
| 예전 목록의 kind가 `manual`·`tag-devcomment` | 2026-10-12에 만든 Kata 도구의 이름 | 그대로 둔다. 도구가 읽을 수 있고, 다시 등록할 때만 새 이름으로 바뀐다 |

- 자동 캡처는 작업 파일의 변경·diff 구간까지만 찾고 의미상의 영향은 판정하지 않는다. `unassignedChanges`는 파일 단위 알림이므로 같은 파일 안의 미등록 구간은 Claude가 대응시킨다.
- 시작 전에 변경돼 있던 파일은 해시와 상태만 기록된다. 이전 변경과 이번 변경의 구분이 모호하면 확인하지 못한 부분을 완료 처리하지 않는다.
- 해시 일치는 문장의 품질을 판정하지 않는다. `Docs-Reviewed` 트레일러는 문서 반영 절차를 뜻하며 이 목록을 대신하지 않는다. Codex 호출은 커밋 직전의 넘기기와 사용자가 요청한 경우에만 한다. ko-review 스킬의 검토 요청 절차가 다르더라도 Kata에서는 AGENTS.md의 규칙이 우선한다.
- `begin`·`finish`의 토큰과 스냅샷은 검토 기록만 지킨다. Codex가 고친 파일 자체는 다른 세션의 수정과 겹쳐도 막지 않으므로, 같은 파일을 두 세션이 동시에 고치지 않는다.

## 확인 상태와 근거

- [ko-review 플러그인](https://github.com/jaykop/claude-plugins/tree/main/plugins/ko-review): `review_queue.py`, `record`·`review`·`polish` 스킬.
- [채택 이유와 처음 구현](../devlog/2026-10-12-Description-Review-Pipeline.md). 이 기록의 도구는 Kata `Scripts/DescriptionReview.py`였고 #94에서 플러그인으로 옮겼다.
- [커밋 직전 Codex 넘기기로 바꾼 이유](../devlog/2026-10-16-Codex-Review-Handoff.md).
- [작업 상태](https://github.com/jaykop/Kata/issues/94).
- 2026-10-12에 이전 Kata 도구로 start·register·capture를 실행했다. 2026-10-13 #94 작업의 목록은 이전 도구로 start하고 플러그인 도구로 capture·register했다. 같은 날 Codex가 설치된 ko-review 0.1.0의 도구로 대기 항목 5개를 검토하고 begin·snapshot·finish를 실행해 reviewed로 기록했다. 빌드·테스트·lint와 이슈 우선순위 도구는 이번 검토에서 실행하지 않았다.
- 2026-10-17에는 이 문서를 Kata 지침·프로필과 설치된 ko-review 0.1.2의 `review_queue.py`에 대조했다. 등록 범위, 범위 밖 항목 보고, 미완료 항목의 재검토 안내를 보완했다. `codex exec` 넘기기 예문 자체는 실행하지 않았다.
