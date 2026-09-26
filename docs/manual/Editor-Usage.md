# Kata 전용 에디터 사용법

갱신: 2026-09-25 · 현재 소스 기준. 항목별 사용자 확인 범위는 문서 끝을 따른다.

## 에셋 만들기

1. 새 에디터 모듈을 반영한 뒤 Content Browser의 에셋 생성 메뉴에서 Kata를 선택한다.
2. 생성한 에셋을 더블클릭하면 전용 Kata Editor가 열린다.
3. Kata Action Details에서 태그, 시작 조건, 차단, 쿨다운, 루프 등을 지정한다. 쿨다운은 Enabled, Duration, Start Time만 설정하면 된다.
4. Timeline 영역에서 마우스 오른쪽 버튼을 누르고 Add Task에서 태스크 타입을 고른다. Play Montage·Send Gameplay Event·Apply Gameplay Effect·Apply Loose Tag·Transition Window를 제공한다.
5. 태스크 행을 선택하고 Timeline Details에서 Montage, Start Time, Duration 등 값을 입력한다.
   시작·종료 시점에 한 번 실행할 로직은 Kata Action Details의 Kata|Command 분류에 있는 Pre Commands·Post Commands 목록에 추가한다.
   Post Commands 항목의 End Reasons로 실행할 종료 사유를 고른다. 실행 규칙은 [런타임 사용법](Runtime-Usage.md#prepost-command)을 따른다.
6. 에디터의 Save로 uasset을 저장한다.

태스크의 추가·삭제·시간 변경·Details 수정·상속 변경분 복원에는 Undo/Redo가 연결되어 있다.

탭 배치는 에디터를 닫을 때 저장하고 다음에 열 때 복원한다. 닫은 탭은 Window 메뉴에서 다시 연다.
배치는 에디터가 정상 종료할 때 기록하므로 편집기가 비정상 종료하면 그 세션의 변경은 남지 않는다.
Kata Action Details, Timeline Details, Preview Details는 각 객체 타입을 처음 표시할 때 모든 필드를 펼친다.
그 뒤 사용자가 접거나 펼친 상태는 각 탭별로 저장하고 다음 세션에서 복원한다.

## 타임라인

- 각 행이 하나의 태스크다. 시작과 지속 시간을 막대로 표시한다.
- 위쪽의 시간 눈금 영역과 아래쪽의 태스크 배치 영역은 가로 경계선으로 나뉜다.
- 타임라인은 태스크가 적어도 Timeline 탭을 세로로 가득 채운다. 태스크가 많아지면 스크롤한다.
- 막대 가운데를 드래그하면 시작 시각을 변경한다. 커서는 이동 모양으로 바뀐다.
- 막대의 왼쪽 끝을 드래그하면 끝 시각을 고정한 채 시작 시각을, 오른쪽 끝을 드래그하면 지속 시간을 변경한다.
  양쪽 끝 6픽셀 안에서는 커서가 좌우 크기 조절 모양으로 바뀐다. 막대가 좁으면 절반씩 나눠 잡는다.
- Snap을 켜면 태스크를 끌 때 스냅 대상이 화면 8픽셀 안에 들어오면 그 대상에 붙고, 범위 밖에서는 자유롭게 움직인다.
  끄면 항상 자유롭게 움직인다. 자유롭게 움직일 때도 시각은 1ms 단위로 맞춘다.
- Snap 옆의 Snap To 드롭다운에서 스냅 대상을 여러 개 고를 수 있다. Tasks는 다른 태스크의 시작·끝, Playhead는 재생 헤드 위치,
  Interval은 Interval (s) 눈금이다. 기본값은 Tasks와 Playhead이며, 고른 대상은 프로젝트별 에디터 사용자 설정에 저장한다.
- Interval (s)는 타임라인 눈금 간격이자 스냅 간격이다. 기본값은 0.5초이며 0.001초까지 줄일 수 있다.
  눈금이 너무 촘촘해지면 화면에는 배수 간격으로 그린다. Details에서 직접 값을 입력할 수도 있다.
- Length는 표시하는 시간축 범위다. 범위 밖의 태스크는 Length 값을 늘리거나 툴바의 Resize를 누른다.
  직접 입력하거나 Resize로 바꾼 값은 프로젝트별 에디터 사용자 설정에 저장하고 다음 세션에서 복원한다.
- Current Time은 재생 헤드의 현재 시각이다. 값을 직접 입력하거나 위쪽 시간 눈금 영역을
  클릭·드래그해 이동할 수 있다. 태스크 배치 영역의 빈 곳을 클릭해도 재생 헤드는 움직이지 않는다.
  재생 헤드는 스냅 없이 어느 시각에나 놓인다. 태스크를 옮기거나 값을 바꿔 프리뷰 장면을 다시 만들어도 재생 헤드 시각은 유지되며,
  바뀐 에셋으로 그 시각까지 다시 탐색한다.
  재생 헤드는 프리뷰 실행 가능 여부와 무관하게 지정 시각으로 이동한다.
  해당 시각까지 액션을 실제로 실행한 상태를 표시한다. 시간 눈금 영역은 십자 커서,
  태스크 양쪽 끝은 좌우 크기 조절 커서를 쓴다.
- 탐색은 몽타주 포즈·블렌드·섹션 진행, 캐릭터 이동·충돌과 다른 태스크 효과를 실제 실행 경로로 진행한다.
  드래그하는 동안에도 다시 그리기를 요청한다. 프리뷰 환경의 한계와 미해결 사용자 보고는 아래 프리뷰 절을 따른다.
  태스크 시작 직후에는 Blend In 동안 이전 포즈와 섞여 보이므로 몽타주 에디터의 같은 시각 포즈와 다를 수 있다.
  탐색하는 동안에는 사운드가 재생되지 않는다.
- 앞으로 탐색하면 현재 시각에서 목표까지 차이만큼 진행한다. 뒤로 탐색하면 처음부터 다시 실행하므로 먼 시각일수록 느려지며,
  긴 액션을 뒤로 드래그하면 끊겨 보일 수 있다.
- 탐색·Pause 뒤 Play는 실행 인스턴스가 살아 있으면 남은 구간을 이어가고, 이미 끝났으면 처음부터 시작한다.
  Current Time의 같은 값을 다시 확정하는 동작은 탐색으로 처리하지 않는다.
- 태스크는 Task Id에 따라 안정적인 자동 색상을 사용하므로 새 태스크와 기존 태스크가 서로 다른 색으로 보인다.
  Timeline Details에서 Automatic Display Color를 끄면 Display Color로 직접 지정할 수 있다.
  회색 막대는 비활성 태스크, [P]는 부모에서 상속한 태스크다.
- Editor Comment는 클립 호버 툴팁에 표시한다. 타임라인 상단의 Comments를 켜면 클립 안에도 함께 표시한다.
- 선택한 태스크 클립은 주황색 외곽선으로 표시한다. Ctrl 또는 Shift를 누른 채 클릭하면 복수 선택한다.
  같은 태스크 클래스끼리 선택하면 Timeline Details에서 공통 프로퍼티를 한 번에 수정할 수 있다.
- 미완성 설정도 표시한다. 예를 들어 Montage가 비어 있는 새 태스크도 선택해 설정할 수 있다.
- 해석 진단은 타임라인 아래에 표시한다. 실행 오류가 있으면 프리뷰 시작도 거절될 수 있다.
- 진단은 태스크 표시 이름과 원인을 함께 적는다. 예: MissingMontage [Task KataTask_PlayMontage] - Starts at 0.000s and will not run: 'Montage' is not set
- 값을 아직 채우지 않은 태스크는 저장할 때 오류가 아니라 경고로 보고한다. 에셋 저장과 검사 통과에는 영향을 주지 않는다.
  다만 해당 태스크는 실행에서 제외되고, 오류가 남아 있는 Kata는 프리뷰 재생과 게임 실행이 거절된다.
  완성 전까지 실행해 보려면 값을 채우거나 Timeline Details의 Enabled를 꺼서 비활성 태스크로 둔다.
- 타임라인 상단의 재생·정지·반복 아이콘이 프리뷰 실행을 제어한다. 시작·액터 생성 실패만 오류 문자열로 표시하고,
  일반 상태 문자열은 숨긴다. 현재 시각은 Current Time에 표시한다.
- 타임라인에서 마우스 오른쪽 버튼을 누르면 Add Task, Delete Task, Copy, Paste, Undo, Redo 메뉴가 나온다.
  Add Task와 메뉴의 Paste는 우클릭한 시각을 시작 시각으로 사용한다.
- 타임라인에 포커스가 있으면 Delete로 선택한 태스크를 삭제하고 Ctrl+C·Ctrl+V로 복사·붙여넣기한다.
  단축키로 붙여넣으면 재생 헤드 위치에 들어간다. Ctrl+Z·Ctrl+Y는 에디터 어디에서나 동작한다.
- 복사한 태스크는 열려 있는 다른 Kata 에디터에도 붙여넣을 수 있다. 붙여넣은 항목은 새 Task Id를 받는다.
- 복수 태스크를 선택한 뒤 우클릭의 Group Selected Tasks로 편집기 전용 그룹을 만든다.
  이미 존재하는 그룹에 넣으려면 태스크를 선택하고 Move Selected Tasks to Group에서 대상 그룹을 고른다.
  그룹 머리글의 화살표를 클릭하면 접고 펼치며, 머리글 본문을 클릭하면 Timeline Details에서 이름·색상·주석을 편집한다.
  머리글 우클릭의 Add Task to Group은 새 태스크를 바로 그룹 안에 만들고, Add Selected Tasks to Group은 현재 선택한
  태스크를 기존 그룹으로 옮긴다. Edit Group과 Delete Group도 같은 메뉴에서 사용할 수 있다.
  그룹 색상은 머리글에 표시하고 주석은 호버 툴팁과 Comments 표시를 사용한다.
  그룹에 속한 태스크 행은 같은 그룹 색상의 세로 레일·가지선·배경색과 들여쓰기로 그룹 밖 태스크와 구분한다.
  Ungroup Selected Tasks는 선택 태스크를 그룹 밖으로 옮긴다. 그룹은 실행 순서에 영향을 주지 않고 자식 액션에
  상속되지 않으며, 접힘 상태만 프로젝트별 사용자 설정으로 보관한다.
- 의존성 연결선의 시각 편집은 후속 기능이다.

## 그래프 에디터

- 액션 노드의 제목은 할당한 UKataAction 에셋 이름을 항상 따른다. 노드 제목을 따로 편집하지 않는다.
- Kata Graph Details는 그래프 에셋 자체를, Selection Details는 현재 선택한 노드 또는 엣지를 표시한다.
- Trigger Event Tag는 전이를 요청한 사건이고 Required Action Window Tag는 현재 액션이 그 사건을 받을 수 있는
  시간 구간이다. 예를 들어 Input.Attack.Light 입력이 들어와도 Combo.Light 창이 열려 있을 때만 다음 액션으로 간다.
  Window를 비우면 액션 실행 중 언제든 받고, Trigger를 비우면 현재 액션의 정상 완료 뒤 평가하는 자동 전이다.
  자동 전이는 완료 시점에 평가하므로 Required Action Window Tag도 비워 둔다.
- 동일한 두 노드 사이에 엣지를 여러 개 연결할 수 있다. 각 엣지는 서로 다른 Trigger·Window·Condition을 가진다.
- 엣지의 Keep Target을 끄면 그 전이로 들어가는 액션은 대상 없이 시작한다. 기본값은 켬이며 진입 노드의 엣지에서는 무시한다.
- 그래프 편집기의 Create Comment 명령으로 설명 영역을 추가할 수 있다.
- Incoming/Outgoing Connection Limit Type은 편집기에서 한 노드에 허용할 연결 수를 정하는 고급 저작 제약이다.
  Unlimited는 제한하지 않고 Limited는 해당 Limit 값을 적용한다. 런타임 전이 우선순위와는 무관하다.

## 프리뷰

Preview Details에서 Preview Actor Class·Preview Target Class·Transform·조명·환경을 지정한다.
설정은 에디터 전용이며 자식 생성 시 복사하지만 ParentAction의 런타임 정책처럼 계속 상속하지 않는다.

### 캐릭터와 환경 준비

- 몽타주에는 Skeletal Mesh와 슬롯 출력이 연결된 Anim Blueprint가 필요하다. Use Animation Asset 모드는 몽타주 슬롯을 평가하지 않는다.
- KataFramework의 AKataCharacter·AKataPlayerCharacter 파생 BP를 사용할 수 있다. Kata 컴포넌트는 기본 클래스가 제공하므로 메시·AnimBP만 직접 지정한다. 구성은 [런타임 사용법](Runtime-Usage.md)을 따른다.
- 클래스가 비면 표시용 메시 없는 빈 액터를 만든다. 위치와 트랜스폼 위젯 조작에는 사용하지만 화면에 구체를 그리지 않는다.
- 기본 Self는 (0, 0, 100)cm·Yaw 180도, Target은 (-500, 0, 100)cm·Yaw 0도다. 기존 저장값은 유지된다.
- 별도 EditorPreview 월드를 사용한다. 현재 레벨의 액터나 PIE 초기화를 사용하지 않는다.
- 충돌 바닥의 윗면은 Z=0이다. Show Front Wall·Show Side Wall은 기본 꺼짐이며 벽은 각각 -X·+Y 쪽에 생긴다.
  Environment Size는 기본 10000×10000×1000cm이고 X/Y는 바닥·벽 폭, Z는 벽 높이를 정한다.
- Background Color와 Preview Lighting의 Rotation·Brightness·Color를 조절할 수 있다. 조명 기본 회전은 Pitch -40, Yaw 157.5, Roll 0이다.
- Show Debug Shape로 Grid·Sphere 측정을 표시한다. Grid Cell Size·Debug Color·Debug Thickness를 설정한다.
  기본 표시 꺼짐, 두께 2이며 범위는 Environment Size를 따른다.
- Self·Target은 기존 ASC를 사용하거나 임시 ASC를 받는다. 프리뷰는 AttributeSet이나 이벤트를 수신할 Ability를 자동 추가하지 않는다.

Character는 중력을 받아 착지한다. 저장한 Z는 최초 배치이며 착지 이후 유지되지 않을 수 있다.
캐릭터 이동을 위해 프리뷰가 bRunPhysicsWithNoController와 기본 MovementMode를 설정하고,
메시는 렌더 여부와 관계없이 포즈·본을 갱신하도록 구성한다. 이 설정은 프리뷰 액터에만 적용한다.

루트 모션은 몽타주와 AnimBP의 추출 설정, CharacterMovement의 이동·충돌 경로가 함께 맞아야 위치에 반영된다.
프리뷰는 몽타주 포즈만으로 임의 Actor의 위치를 강제로 옮기지 않는다.
루트 모션 미이동과 Pause 후 처음부터 재생되는 현상은 사용자 보고가 있으므로 아래 문제 해결·확인 범위를 함께 따른다.

### 재생·일시정지·시간 탐색

| 조작 | 현재 동작 |
|---|---|
| Play/Pause | Timeline 상단의 같은 버튼을 사용한다. 살아 있는 실행을 Pause하면 월드 진행을 멈추고 다시 Play하면 해당 인스턴스를 이어간다 |
| 실행이 없거나 종료된 뒤 Play | 장면을 초기화하고 처음부터 시작한다. 종료된 액션의 잔여 실행은 없다 |
| Stop·Reset | 현재 실행을 취소하고 프리뷰 액터를 다시 생성한다 |
| Repeat | 일반 재생이 진행된 뒤 끝났을 때 새로 시작한다. 꺼져 있으면 일반 재생 종료 시 장면을 초기화한다. 액션 Loop Policy와 별개의 사용자 설정이다 |
| Current Time·눈금 탐색 | 다음 프리뷰 Tick에서 마지막 요청을 실행한다. 앞으로는 실행을 이어가고 뒤로는 0초부터 다시 시뮬레이션한다 |

탐색은 1/60초 단계의 실제 실행이다. 다음 처리 프레임 안에 목표까지 진행하고, 살아 있는 인스턴스를 남겨 Play로 이어간다.
몽타주 섹션 연결·블렌드·충돌·루트 모션·다른 태스크 효과를 실행 경로로 계산한다. 몽타주 에셋의 단독 포즈를 직접 찍는 기능이 아니다.
탐색 도중에는 프리뷰 월드 소리를 끈다. 뒤로 멀리 이동하면 다시 계산할 양이 커져 편집이 끊길 수 있다.
재생 헤드는 액션 길이보다 뒤를 가리킬 수 있지만 시뮬레이션 목표는 타임라인 길이로 제한한다.

탐색으로 끝에 도달한 실행은 Repeat·자동 Reset 대상으로 표시하지 않는다. 다만 현재 코드에서 **실행 인스턴스가
종료되거나 없으면 대기 월드 Tick을 수행**하므로, 종료 후 Idle·중력까지 완전히 정지하는 화면은 보장하지 않는다.
일반 재생·대기와 달리 살아 있는 액션의 Pause에서는 월드가 진행하지 않는다.

### 카메라와 액터 조작

Preview 상단은 엔진 뷰포트 툴바다. Camera 메뉴는 Perspective와 Top·Left·Right·Front·Back 직교 뷰,
카메라 속도·FOV·클리핑 평면·Frame(F)·Reset Camera를 제공한다. 축은 월드 기준이며 Self 방향을 따르는 옛 Back 버튼은 없다.
뷰별 시점을 기억하고 Reset Camera는 현재 뷰만 기본 구도로 돌린다. F는 Self·Target을 함께 화면에 맞춘다.
View Mode는 Lit·Unlit·Wireframe·Lighting Only·Player Collision·Visibility Collision·Clay를 제공한다.
Snapping·Realtime/Performance·Asset Viewer Profile·LOD 메뉴는 제공하지 않는다.

Select Self 또는 Select Target으로 조작할 액터 하나를 선택한다. 다시 누르면 해제한다.
Q/W/E는 선택·이동·회전이며 축·평면 손잡이를 드래그하거나 방향키·PageUp/PageDown으로 조정한다.
Shift는 큰 간격으로 조정한다. 좌표는 월드 기준이고 크기 조절은 지원하지 않는다.
조작을 마치면 해당 Preview Transform에 기록하며 Undo로 되돌릴 수 있다.
Resize는 타임라인 표시 길이를 마지막 태스크 끝에 맞춘다. 태스크가 없으면 5초다.

### 제한과 문제 해결

| 증상 | 확인할 조건·현재 한계 |
|---|---|
| 몽타주가 화면에 보이지 않음 | Preview Actor Class의 Mesh·AnimBP·Slot 연결, Montage 설정, Duration과 종료 정책을 확인한다 |
| 루트 모션 몽타주인데 위치가 움직이지 않음 | 사용 캐릭터의 AnimBP 루트 모션 설정·CharacterMovement·충돌과 태스크 구간을 확인한다. 사용자 보고의 정확한 원인과 해소 여부는 이번에 실행으로 확인하지 않았다 |
| Pause 뒤 처음부터 시작됨 | 액션이 이미 종료됐는지, 편집·Undo·Stop으로 장면이 재생성됐는지, 탐색 요청이 있었는지 구분한다. 살아 있는 인스턴스에서도 발생한다는 사례의 재현·해소는 미확인이다 |
| 끝 시각에서 Idle이나 중력이 계속 진행함 | 종료된 인스턴스는 현재 대기 월드 Tick 경로로 들어간다 |
| 이동 후 뷰포트 클릭만으로 배치가 바뀜 | 탐색 뒤에는 기록 기준을 맞추지만 일반 재생의 루트 모션 이동 뒤에는 Preview Transform에 기록될 수 있는 제한이 남아 있다 |
| 프리뷰에서 GE·이벤트가 기대대로 동작하지 않음 | ASC만으로 충분하지 않다. 필요한 AttributeSet·수신 Ability·프로젝트 초기화를 캐릭터 쪽에서 제공한다 |

프리뷰에는 호출 Gameplay Ability, GameInstance·Controller·PlayerState 기반 게임 초기화가 없다.
OwningAbility를 요구하는 태스크는 이 환경을 고려한다. 게임 실행 안내는 [Runtime-Usage](Runtime-Usage.md)를 따른다.

## 부모·자식과 변경분

Create Child로 현재 에셋을 부모로 참조하는 새 Kata 에셋을 만든다.
또는 Kata Action Details의 Parent Kata에서 부모를 지정한다.

부모의 현재 값과 태스크가 자식 에디터에 표시된다. 상속된 태스크도 행을 선택해 바로 편집하며 GUID를 직접 입력할 필요가 없다.
자식에서 고친 프로퍼티만 저장하므로 부모의 다른 수정과 태스크 추가는 계속 따라온다.

- Reset Override: 선택한 Kata 설정의 변경분을 제거하고 부모 값을 다시 따른다.
- Reset Task Override: 선택한 상속 태스크의 프로퍼티 하나 또는 전체 변경분을 제거한다.
- 상속 행을 삭제하면 Remove 변경분이 생긴다. 같은 메뉴의 Restore 항목으로 되돌린다.
- 비활성화는 Timeline Details의 Enabled를 끈다.
- 조건 객체와 배열은 하나의 프로퍼티 단위로 변경분을 보관한다. Pre Commands·Post Commands도 목록 전체가 하나의 변경분이다.
- 부모를 바꿨을 때 대상이 사라진 태스크 오버라이드는 임의의 다른 태스크에 적용하지 않고 진단을 남긴다.

## 한 프레임 태스크

Timeline Details의 Single Frame을 켜면 그 태스크는 시작한 프레임에서 Tick을 한 번만 받고 끝난다.
켜는 순간 Duration은 의미가 없어져 Details에서 숨겨진다.

Duration을 0으로 둔 순간 태스크와 다르다. 순간 태스크는 Tick을 한 번도 받지 않고 시작과 동시에 끝난다.
한 프레임만 도는 처리가 필요하면 Single Frame을, 시작 시점의 단발 처리만 필요하면 Duration 0을 쓴다.

타임라인에서는 길이를 차지하지 않는 짧은 주황색 표식으로 그려지고 길이 조절 손잡이가 사라진다.
드래그로 시작 시각은 옮길 수 있지만 길이는 바꿀 수 없다.

## 확인 상태와 근거

2026-09-24 사용자가 프리뷰 시뮬레이션 변경의 빌드·탐색 동작을 확인했다는 기록이 있다.
클릭·양방향 드래그·Play 이어가기·Reset 뒤 Idle·소리의 항목별 결과는 따로 보고되지 않았다.
이 대화의 루트 모션 미이동·Pause 재시작 보고를 해당 과거 확인으로 해결됐다고 간주하지 않는다.
이번 문서 작업에서는 소스만 대조했으며 빌드·UHT·테스트·UI 실행을 수행하지 않았다.

- [KataActionEditor.cpp](../../Plugins/Kata/Source/KataEditor/Private/KataActionEditor.cpp): 버튼·입력·자동 초기화.
- [SKataPreviewViewport.cpp](../../Plugins/Kata/Source/KataEditor/Private/SKataPreviewViewport.cpp): 장면·Play/Pause·탐색·대기 Tick.
- [KataAction.h](../../Plugins/Kata/Source/KataRuntime/Public/Action/KataAction.h): 프리뷰 저장값·기본 배치.
- [실행 시뮬레이션 전환 기록](../devlog/2026-09-24-Preview-Scrub-Simulation.md): 결정과 사용자 확인 범위.
- [에셋 이전 안내](Asset-Migration.md): 이전 설정 처리.
