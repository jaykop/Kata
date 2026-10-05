# GAS Inspector 설계

작성: 2026-10-05  
갱신: 2026-10-05  
연결 이슈: 없음. 사용자 요청으로 이슈를 게시하지 않는다.  
현재 상태 근거: [사용법](../manual/GAS-Inspector.md) · [구현 기록](../devlog/2026-10-05-GAS-Inspector.md)  
대체 관계: 없음

## 목적과 현재 상태

싱글플레이 PIE/SIE에서 ASC의 태그·Attribute·Ability·Gameplay Effect를 관찰하고, 콘텐츠의 Ability Trigger 설정을 태그로 찾는다. 게임 코드를 수정하지 않고 관측할 수 있는 Editor 전용 도구로 구성한다. 사용 계약은 manual을 따르며, 아래는 책임 분리와 확인 기준이다.

## 범위

- 포함: Editor/PIE 월드 선택, ASC 검색, 읽기 전용 GAS 데이터, 주기 갱신·Freeze·Refresh, 행 검색·필터·정렬·에셋 열기, Trigger CDO 인덱스.
- 제외: 게임 상태 편집, Ability 강제 실행, 이벤트 주입, 원격 프로세스 접속·복제·RPC·클라이언트 예측, 종료된 GE/Ability 이벤트 이력, GAS 계산 재구현.
- 별도 범위: KataGraph 실행 디버그(#29)와 각 게임플레이 모듈의 디버그. 이 도구는 엔진 GAS를 관찰한다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 |
|---|---|---|
| 도구 구현 | 확정 | 2026-10-05 사용자 요청, 이름 KataGASInspector |
| 이슈 미게시 | 확정 | 2026-10-05 사용자 선택 |
| 모듈 구성 | 구현 선택 | KataGASInspectorEditor 단일 Editor 모듈, 게임플레이 플러그인 의존 없음 |
| 상태 표시 | 구현 선택 | 시각·대상이 있는 값 snapshot, read-only |
| 갱신 주기 | 구현 선택 | 기본 0.2초, 0.1~1초 사용자 설정 |
| 활성화 가능 여부 | 구현 선택 | 자동 CanActivateAbility 호출 없음. 관측 가능한 input/tag block과 active 상태만 표시 |
| Trigger 검색 | 구현 선택 | 기본 exact, 선택적으로 query tag의 자식 trigger 포함. 빈 query는 전체 |
| 콘텐츠 범위 | 구현 선택 | 기본 /Game, 루트 비우면 전체 콘텐츠. native는 로드된 클래스만 포함 |
| 후속 개선 | 미확정 | 수동 활성화 가능 여부 점검, 추가 이벤트 이력, 상태 강제 변경은 별도 요청 때 설계 |

## 구조와 수명

| 책임 | 구현 타입 | 계약 |
|---|---|---|
| 탭·메뉴·콘솔 | FKataGASInspectorEditorModule | Window 메뉴와 콘솔 명령 등록. 종료 시 탭 콘텐츠·서비스·등록 자원 정리 |
| 월드·대상·갱신 | FKataGASInspectionSession | 창별 weak World/ASC. 초기 선택과 대상 소멸을 구분하며 소멸 후 임의 대상에 붙지 않음 |
| 값 수집 | FKataGASSnapshotCollector | 게임 스레드에서 API를 읽고 값으로 반환. 화면은 spec/container/world 포인터를 보관하지 않음 |
| 설정·Task 읽기 | FKataGASAbilityMetadata | protected metadata는 reflected inner 타입을 확인. FProperty 주소는 캐시하지 않음 |
| Trigger 검색 | FKataGASAbilityTriggerIndex | 레지스트리 후보+현재 로드 클래스, CDO 설정 판정, 중복 제거·취소·변경 감지 |
| 화면 | SKataGASInspector | snapshot 값만 렌더링. 검색은 source 목록 필터, key 기반 행 재사용 |

Editor 모듈은 Game 타깃에 포함되지 않는다. 프로젝트 모듈과 코어·위성·Framework는 이 도구에 의존하지 않는다. 내부 헤더와 UI 의존은 Private에 둔다. 게임 에셋을 추가하거나 변환하지 않는다.

Freeze는 태그 count·remaining time·tooltip를 포함한 snapshot 값을 고정한다. Refresh는 frozen 상태에서도 한 번 수집한다. 자동 월드 전환 뒤 frozen 화면은 이전 시각·대상을 계속 명시하며 Resume/Refresh로 새 데이터를 얻는다. 대상이 사라진 live 화면은 비우고 다시 선택하도록 안내한다.

태그·Attribute·Ability·GE 값 수집과 에셋 검색을 분리한다. Trigger 페이지에서는 runtime 갱신을 쉬고 검색 작업만 진행하며, 탭이 foreground가 아니면 두 작업 모두 쉰다. 서비스는 프로세스 전역 대상 캐시를 사용하지 않는다.

## 데이터 의미

| 데이터 | 표시 계약·제약 |
|---|---|
| Owned tags | owned count, 주석·정의 출처. Loose/GE/Ability 제공자 전체의 추적 결과는 아님 |
| Blocked tags | Ability 차단 여부. owned count를 차단 횟수로 쓰지 않음 |
| Attributes | Set 클래스와 속성 이름, base/current. 엔진 지원 속성 열거 API 사용 |
| Abilities | Spec active/input/tag block, level/input/count, 인스턴스·Task, 기본·동적 Trigger. Idle은 활성화 가능의 보증이 아님 |
| Active GE | 기간·remaining·period·스택·level·inhibition·source·tags와 evaluated modifier. Instant/종료 GE 이력 없음 |
| Trigger 검색 | CDO의 TriggerTag/Source 설정. event 발생·활성화 성공을 뜻하지 않음. native는 source 에셋 링크 없음 |

보호된 Trigger/Task 데이터는 ArrayProperty와 inner struct/object class가 예상 스키마인지 확인한 뒤 property accessor로 읽는다. schema 불일치는 해당 정보의 지원 불가로 표시하고 다른 데이터는 계속 보여 준다. Task의 GetDebugString과 cooldown getter는 해당 객체의 읽기 API를 호출하므로 순수 조회 계약을 지켜야 한다.

Trigger 인덱스는 GeneratedClass 파생 관계로 후보를 모으고 SearchableName 참조로 보강한다. CDO 로드는 게임 스레드에서 틱당 최대 두 에셋, 배치 기준 약 4ms로 나눈다. 개별 GetAsset 동기 로드 시간까지 제한하는 것은 아니다. 취소는 다음 에셋 로드를 멈추며 이미 읽은 partial 결과를 명시한다.

에셋 추가·삭제·갱신·rename·Blueprint compile는 인덱스를 stale로 만든다. 자동 저장/컴파일/전체 재로드를 하지 않으며 사용자가 Scan으로 다시 구축한다. 검색 결과는 soft asset path와 값만 보관하며 click 때 에셋을 재조회한다.

## 작업 순서와 완료 조건

| ID | 작업 | 완료 조건 |
|---|---|---|
| GI-1 | Editor 모듈과 세션 | 게임플레이 모듈의 역참조 없음, 월드/대상 소멸 처리와 등록 해제 |
| GI-2 | snapshot와 네 데이터 영역 | frozen 시점 일치, 객체 수명에 의존하지 않는 표시, missing 값 구분 |
| GI-3 | metadata와 Ability 확장 | inner schema 확인, task/spec/instance 구분, 자동 활성화 검사 없음 |
| GI-4 | 검색·필터·정렬·navigation | key 기반 선택/확장 유지, 동명 항목 정렬, source asset 재조회 |
| GI-5 | Trigger 인덱스 | exact/자식 범위·중복 제거·취소·stale·실패/지원 불가 안내 |
| GI-6 | 프로젝트와 문서 | Editor 활성화, 사용법·제약·실제 확인 여부 기록 |
| GI-7 | 사용자 확인 | Editor/Game 빌드와 아래 수명/표시 항목 확인 |

## 사용자 확인 항목

빌드·실행은 사용자가 담당한다. 자동 테스트·스트레스 코드·별도 검사 환경을 만들지 않는다.

- Editor와 Game 빌드, Window 메뉴·콘솔 명령으로 창 열기.
- 창을 연 채 PIE 시작/종료/재시작, actor 생성/파괴, 여러 ASC·동명 actor, 초기화 전 ActorInfo.
- Freeze/Refresh/Resume의 태그 count·GE remaining·tooltip 시점 일치와 종료된 PIE snapshot.
- Attribute base/current, blocked tag 의미, GE inhibited·nullable source·modifier 값.
- Spec/인스턴스/Task와 기본·동적 Trigger 표시.
- 콘텐츠 범위·다중 태그·query 부모와 trigger 자식, Blueprint 로드 실패·GC·재컴파일·rename/delete, Scan 취소.
- 실제 콘텐츠 규모에서 UI 반응성과 선택/확장·scroll 유지.

## 완료 시 갱신할 문서

- [사용법](../manual/GAS-Inspector.md): 현재 UI·데이터 의미·지원 범위.
- [구현 기록](../devlog/2026-10-05-GAS-Inspector.md): 결정 이유와 실제 확인 결과.
- [문서 목록](../README.md): 진입점.
- AGENTS.md·[모듈화 계획](Plugin-Modularization-Plan.md): 독립 Editor 도구의 경계.
- 사용자 요청으로 이슈를 게시하지 않으며, 이후 추적 방식을 바꾸면 해당 요청을 따른다.
