# 설명 변경 검토 사용법

갱신: 2026-10-12  
대상: Kata에서 주석·manual·태그 설명을 작성하는 Claude와 부분 교정을 맡는 Codex  
적용 기준: `Scripts/DescriptionReview.py`, 프로젝트 AGENTS.md와 로컬 스킬  
확인 상태: 소스 작성과 이번 작업의 start·register·capture 실행. 자동화 테스트·lint·스킬 검증은 실행하지 않았다.

## 목적과 준비

작업마다 바꾼 설명과 로직 변경에 영향을 받는 기존 설명을 공유 목록에 남긴다. Claude가 기록하고 Codex가 요청받은 대기 항목을 읽어 교정한다. 전체 프로젝트를 매번 읽는 대신 해당 문단·주석과 직접 관련된 구현을 대조한다.

Python 3.10 이상과 Git이 필요하다. 프로젝트 루트에서 실행하며 외부 패키지는 사용하지 않는다. 실제 기록은 `docs/localdocs/threads/<task>.descriptions.json`에 저장된다. Git에서는 제외하고 Diversion에서 추적하며, 스킬 자체는 기존 정책대로 양쪽에서 제외된다.

GitHub Issue는 작업 상태, 이 목록은 설명 검토 상태를 관리한다. 빌드·실행 확인과 서로 대신하지 않는다. 두 AI는 같은 프로젝트의 파일을 공유하며 검토 요청은 사용자가 전달한다.

## 사용 순서

### Claude: 시작과 종료 기록

1. `git status --short`와 기존 작업 메모를 읽는다. 작업 ID는 이슈 번호가 있으면 `<번호>-<slug>`, 없으면 `<slug>`로 정한다.
2. 파일을 수정하기 전에 명시적 파일 목록으로 시작 상태를 기록한다. 디렉터리 전체를 지정하지 않는다. 아래는 새 작업의 호출 형식이다.

   ```powershell
   python Scripts/DescriptionReview.py start 123-feature --issue 123 --paths Plugins/Kata/Source/KataRuntime/Public/Tasks/KataTask.h docs/manual/Task-Authoring.md
   ```

   같은 작업의 목록이 있으면 다시 만들지 않는다. 추가 파일은 수정 전에 `scope <task> --paths <파일들>`로 등록한다. 이미 수정한 뒤 등록하면 시작 전 차이를 복원할 수 없으므로 그 사실을 작업 메모와 항목의 reason에 적는다.
3. 구현과 관련 문서 반영 후 `capture <task>`를 실행한다. 시작 상태와 다른 작업 파일, Git 기준 diff의 구간과 현재 HEAD를 기록한다. 시작부터 수정되어 있던 파일은 `initiallyDirty=true`로 표시되므로 기존 변경과 이번 변경을 사람이 구분한다. 빌드 확인이나 문서화 후속 질문을 기다리느라 기록을 미루지 않는다.
4. 이번 설명과 관련 구현의 위치를 JSON 배열로 작성하고 `register <task> --items <상대경로.json>`으로 등록한다. 입력 JSON도 같은 localdocs 작업 폴더에 둔다. 다음 예시의 위치 지정 방식을 실제 작업의 파일·태그·함수에 맞춰 사용한다.

   ```json
   [
     {
       "id": "super-armor-description",
       "kind": "tag-devcomment",
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

   설명 파일은 taskPaths에 포함한다. 근거 구현은 읽기만 한다면 범위 밖의 파일도 참조할 수 있다. 설명을 추가하지 않았더라도 구현 변경으로 기존 설명이 실제 동작과 달라졌다면 등록한다. 같은 id를 다시 등록하면 해당 항목을 pending으로 돌린다.
5. 설명에 영향을 주지 않는 변경은 `no-impact <task> --path <작업파일> --note "이유"`로 기록한다. 파일 내용이 바뀌면 해당 기록을 재사용하지 않는다. `pending <task>` 출력과 실제 변경 구간을 읽고, 설명·영향·영향 없음의 기록이 빠지지 않았는지 작업 범위에서 확인한다.

### Codex: 대기 항목 교정

1. 지정한 task의 `pending <task>`를 읽고 항목과 주변 문맥·근거 구현을 읽는다. task를 지정하지 않았다면 `*.descriptions.json` 목록에서 대기 항목을 선택한다. 파일 단위의 미등록 알림은 기록 누락으로 다루며 전체 프로젝트 검토로 확대하지 않는다.
2. `begin <task> --id <항목id>`로 현재 설명·근거의 스냅샷과 token을 받는다. 자연스러운 한국어인지, 설정 조건·단위·실패 조건이 구현에 맞는지 검토한다. 교정 기준은 `doc-polish`와 `editor-comment-polish`를 따른다.
3. 뜻이 분명한 설명은 직접 수정한다. 구현 결함이나 미확정 계약이면 status를 `needs-decision`으로 기록하고 근거를 남긴다. 설명에 맞추려고 구현을 바꾸지 않는다.
4. `finish <task> --id <항목id> --token <token> --note "결과"`로 기록한다. 직접 설명을 고쳤다면 최종 본문을 읽은 뒤 `snapshot <task> --id <항목id>`의 description.sha256을 `--description-hash <hash>`로 지정한다. snapshot은 해시를 계산할 뿐 검토 완료로 기록하지 않는다. begin 이후 근거가 바뀌었다면 finish가 거절하므로 다시 begin하고 읽는다.

## 주요 기록과 위치 지정

| 항목 | 의미·조건 |
|---|---|
| `schemaVersion`, `taskId`, `issue` | 현재 형식은 1. 이슈가 없으면 null |
| `baseCommit`, `headCommit` | 시작·캡처 시점의 Git 커밋 |
| `taskPaths`, `initialChanges`, `initialFiles` | 명시적 파일 범위, 시작 시 Git 상태와 파일 해시. 범위를 추가한 시각도 기록 |
| `changes`, `noImpact` | 시작 상태와 다른 파일·diff 구간, 설명에 영향이 없는 이유 |
| `entries[].id`, `kind`, `reason` | 작업 안에서 유지하는 id. kind는 comment·manual·tag-devcomment |
| `selector`, `evidence[]` | 설명·근거의 상대 경로, anchor와 블록 선택 방식 |
| `snapshot`, `review`, `ticket` | 내용의 SHA-256과 줄 위치 힌트, 검토 결과, 검토 시작 시 token |
| `status` | pending·reviewed·needs-decision. 위치가 사라지거나 내용이 바뀌면 pending |

anchor는 **파일 안의 한 줄에만 맞는 문자열**로 지정한다. 타입·함수 이름만 쓰면 여러 호출과 맞을 수 있으므로 선언이나 제목 등 구별되는 내용을 쓴다.

| mode | 선택 범위 |
|---|---|
| `lines`(기본) | anchor의 한 줄. before·after로 앞뒤 줄 수를 더한다. 태그 한 줄이나 짧은 주석에 사용 |
| `heading` | Markdown 제목부터 다음 같은 수준·상위 제목 직전까지. manual의 절에 사용 |
| `span` | anchor부터 뒤쪽에서 한 번만 찾는 endAnchor 직전까지. includeEnd=true면 끝 줄도 포함. 주석·구현 블록에 사용 |

무관한 줄 이동만 있으면 검토를 유지한다. 설명이나 근거 내용이 바뀌면 다음 pending·capture·begin에서 다시 pending으로 돌린다. 설명은 문맥을 포함하고, 근거는 기본값·해당 분기·종료 처리처럼 설명을 뒷받침하는 범위를 선택한다.

## 제한과 문제 해결

- Windows 콘솔에서 유니코드 출력에 `UnicodeEncodeError`가 나면 `python -X utf8 Scripts/DescriptionReview.py <명령> <task>`로 실행한다. 출력 중 오류가 났다고 목록 기록까지 저장됐다고 가정하지 말고, 파일을 읽은 뒤 명령을 다시 실행한다.
- 자동 캡처는 taskPaths의 변경 파일·Git diff 구간까지만 찾으며, 주석 구문이나 의미상의 영향을 자동으로 판정하지 않는다. `unassignedChanges`는 항목·근거·영향 없음 기록이 없는 **파일 단위**의 알림이다. 같은 파일 안의 미등록 구간을 모두 찾지는 않으므로 Claude가 변경 구간과 등록 항목을 대응시킨다.
- 시작 전에 변경되어 있던 파일은 해시와 상태만 기록한다. 파일 전체를 복사하지 않으며 Git diff에는 이전 변경이 섞일 수 있다. 구분이 모호하면 항목별 이유·위치를 적고 확인하지 못한 부분을 완료 처리하지 않는다.
- anchor가 삭제되거나 중복되면 locationError를 남기고 대기한다. 설명 자체가 삭제됐다면 삭제 이유와 남은 관련 설명을 기록해 위치를 갱신한다. 생성·벤더 코드는 대상에서 제외한다.
- 목록을 기록하는 동안 잠근다. 다른 목록 쓰기나 내용 변경이 있으면 덮어쓰지 않고 중지한다. Windows의 일시적인 파일 교체 거절은 내용을 다시 확인하며 최대 4번 시도한다. 계속 거절되면 오류를 보고하고 종료하며 사용자 프로세스를 강제로 닫지 않는다. 비정상 종료 후 .lock이 남았다면 다른 처리가 실행 중인지 확인하고 그 잠금 파일만 제거한다.
- Codex의 자체 교정은 최종 본문 해시로 기록한다. 이후 Claude가 설명·근거를 고치면 다시 검토한다. 스냅샷 일치는 문장의 품질을 판정하지 않는다.
- 기존 `Docs-Reviewed`는 문서 반영 절차를 뜻하며 설명 검토 목록을 대신하지 않는다. AI 자동 호출·메시지 전송·커밋 차단은 추가하지 않았다.

## 확인 상태와 근거

- [기록 도구](../../Scripts/DescriptionReview.py): 명시적 파일 범위와 블록 SHA-256, 대기 전환, 시작·완료 기록, 동시 쓰기 방지.
- [채택 이유와 구현 기록](../devlog/2026-10-12-Description-Review-Pipeline.md).
- 이번 작업에서 start로 실제 목록을 생성하고 register·capture로 항목과 현재 스냅샷을 기록했다. 파일 교체 거절 후 제한된 재시도를 추가했고 이후 capture가 완료됐다. 재시도 횟수와 모든 처리 경로의 동작을 별도로 확인한 것은 아니다. 빌드·테스트·lint·스킬 검증은 실행하지 않았다.
