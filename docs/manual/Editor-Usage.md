# Kata 전용 에디터 사용법

갱신: 2026-09-20 · 소스 작성 완료, 빌드·UI 실행 미검증

## 에셋 만들기

1. 새 에디터 모듈을 반영한 뒤 Content Browser의 에셋 생성 메뉴에서 Kata를 선택한다.
2. 생성한 에셋을 더블클릭하면 전용 Kata Editor가 열린다.
3. Kata Action Details에서 태그, 시작 조건, 차단, 쿨다운, 루프 등을 지정한다. 쿨다운은 Enabled, Duration, Start Time만 설정하면 된다.
4. Timeline 영역에서 마우스 오른쪽 버튼을 누르고 Add Task에서 태스크 타입을 고른다. 기본 제공 타입은 Play Montage다.
5. 태스크 행을 선택하고 Timeline Details에서 Montage, Start Time, Duration 등 값을 입력한다.
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
- Snap을 켜면 드래그를 Interval (s) 눈금과 다른 태스크의 시작·끝에 맞춘다. 끄면 자유롭게 움직인다.
- Interval (s)는 타임라인 눈금 간격이자 스냅 간격이다. 기본값은 0.5초이며 0.001초까지 줄일 수 있다.
  눈금이 너무 촘촘해지면 화면에는 배수 간격으로 그린다. Details에서 직접 값을 입력할 수도 있다.
- Length는 표시하는 시간축 범위다. 범위 밖의 태스크는 Length 값을 늘리거나 툴바의 Resize를 누른다.
  직접 입력하거나 Resize로 바꾼 값은 프로젝트별 에디터 사용자 설정에 저장하고 다음 세션에서 복원한다.
- Current Time은 재생 헤드의 현재 시각이다. 값을 직접 입력하거나 시간 눈금 및 태스크 클립이 없는 빈 영역을
  클릭·드래그해 이동할 수 있다. 재생 헤드는 프리뷰 실행 가능 여부와 무관하게 지정 시각으로 이동하고,
  실행 가능한 프리뷰만 내부 상태를 해당 시각까지 따라가게 한다. 시간 탐색 영역은 십자 커서,
  태스크 양쪽 끝은 좌우 크기 조절 커서를 쓴다.
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
- 타임라인 상단의 재생·정지·반복 아이콘이 프리뷰 실행을 제어한다. 상태는 버튼 옆에,
  현재 시각은 Current Time에 표시한다.
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
- 그래프 편집기의 Create Comment 명령으로 설명 영역을 추가할 수 있다.
- Incoming/Outgoing Connection Limit Type은 편집기에서 한 노드에 허용할 연결 수를 정하는 고급 저작 제약이다.
  Unlimited는 제한하지 않고 Limited는 해당 Limit 값을 적용한다. 런타임 전이 우선순위와는 무관하다.

## 프리뷰

Preview Details 탭에서 Preview Actor Class, Preview Target Class, 각 Transform과 조명을 지정한다.
이 탭은 Preview 월드 화면과 분리되어 있으며 기본 배치에서 Kata Action Details 옆의 탭으로 열린다.
이 항목은 Kata Action Details에 표시하지 않는다. 설정은 에디터 전용이며 게임 빌드 데이터에서 제외된다. 자식 생성 시 복사하지만 ParentAction의 런타임 정책처럼 계속 상속하지 않는다.

기본 Self는 바닥 위 Z 100cm에 놓인다. 기본 Target은 Top View 화면에서 Self 위쪽인 -X 200cm, Z 100cm에 놓이며 Yaw 0도로 Self를 바라본다.

- 별도 GamePreview 월드에서 클래스의 액터를 생성한다. 현재 편집 중인 레벨의 액터는 사용하지 않는다.
- 클래스를 지정하지 않으면 위치 확인용 구체를 생성한다. 몽타주 프리뷰에는 메시와 AnimInstance를 갖춘 캐릭터 클래스가 필요하다.
- 월드 원점을 윗면으로 하는 충돌 바닥과 앞·왼쪽 기준 벽을 생성한다. 오른쪽에 있던 측면 벽은 왼쪽으로 옮겼다.
  바닥과 벽에는 M_ProcGrid의 Object Aligned 인스턴스를 사용해 세로 면에서도 정사각형 격자를 유지한다.
- Preview Environment의 Background Color와 Environment Size로 배경과 공간 크기를 조절한다.
  Environment Size의 X/Y는 바닥과 벽 폭, Z는 벽 높이에 함께 적용된다.
- Preview Debug의 Show Debug Shape로 측정 표시를 켜고 Debug Shape에서 Grid 또는 Sphere를 선택한다.
  Grid Cell Size는 간격, Debug Color는 색, Debug Thickness는 선 두께를 조절한다.
  Grid와 Sphere의 최대 범위는 Environment Size를 따르며 Sphere의 마지막 구는 최대 범위에 정확히 맞는다.
  Show Debug Shape는 기본적으로 꺼져 있고 Debug Thickness 기본값은 2다.
- 기존 ASC가 있으면 사용하고, 없으면 프리뷰 액터에 임시 ASC를 추가한다.
- AttributeSet이나 프로젝트 초기화가 필요한 조건은 프리뷰 캐릭터가 해당 설정을 제공해야 한다.
- 재생·정지·반복 버튼은 Preview 탭이 아니라 Timeline 탭 상단에 있다.
- 재생과 일시정지는 같은 버튼이 맡는다. 재생 중에는 아이콘이 일시정지 모양으로 바뀌며, 누르면 그 자리에서 멈춘다.
- 재생은 게임과 같은 PlayKataAction 경로로 활성화 조건·태그·쿨다운을 적용하고 실제 태스크를 실행한다.
- 일시정지는 프리뷰 월드의 시간 진행을 멈춘다.
- 정지는 실행을 종료하고 프리뷰 액터를 다시 생성한다.
- 정지 오른쪽의 반복 버튼을 켜면 액션이 끝날 때마다 프리뷰를 처음부터 다시 실행한다.
  이 버튼은 프리뷰에만 적용하는 편집기 설정이므로 에셋의 Loop Policy를 바꾸지 않는다.
  설정은 프로젝트별로 저장하며 다음 에디터 세션에서도 유지한다.
- Preview 탭 상단의 Perspective, Top, Right, Back 버튼이 카메라를 전환한다.
  Perspective 이외의 버튼은 직교 투영으로 해당 방향에서 장면을 본다.
  직교 카메라는 Self와 Target을 모두 담도록 중심과 확대 배율을 맞춘다.
  Back은 Self Actor가 바라보는 방향에 맞춰 축을 고르므로 Self Actor의 등 뒤에서 보는 구도가 된다.
  Self Actor를 회전한 뒤 Back을 다시 누르면 새 방향에 맞춰 축을 다시 고른다. 현재 선택한 버튼은 강조 색으로 표시한다.
  각 버튼은 그 구도에서 마지막으로 보던 카메라를 기억하므로 버튼을 오갈 때 시점이 초기화되지 않는다.
  기본 구도로 되돌리려면 이미 켜져 있는 버튼을 다시 누른다.
- Preview Lighting의 Rotation, Brightness, Color가 프리뷰 월드의 Directional Light를 조정한다.
  Rotation 기본값은 Pitch -40, Yaw 157.5, Roll 0이다. 이 값은 프리뷰에만 적용한다.
- 타임라인 눈금이나 빈 영역을 클릭하면 재생 헤드와 Current Time은 즉시 목표 시각으로 이동한다.
  프리뷰 액터 상태는 내부적으로 1/60초씩 목표 시각까지 재생해 따라가며 긴 구간은 프레임마다 나눠 진행한다.
  임의 시각으로 상태를 직접 역산하는 Sequencer 방식의 스크러빙은 아니다.
- 태스크 편집, 부모 변경, Undo/Redo 시 기존 프리뷰를 정리하고 새 설정을 표시한다.

에디터 툴바의 Save, Browse 오른쪽에 두 버튼이 있다.

- Select Target: 켜면 Target Actor에 Unreal 네이티브 트랜스폼 위젯을 표시한다.
  이 프리뷰는 ITF 자동 기즈모 대신 엔진의 FWidget 렌더링과 히트 프록시 입력 경로를 사용한다.
  위젯 입력은 Target Actor의 Transform에 직접 적용한다.
  측정 도형은 호버 판정에서 제외해 이동 손잡이를 가리지 않는다.
  조작 방법은 두 가지다.
  - 위젯의 축이나 평면 손잡이를 잡고 드래그한다.
  - 방향키로 X·Y를, PageUp·PageDown으로 Z를 조금씩 옮긴다. Shift를 누르면 큰 단위로 움직인다.
    회전 모드에서는 같은 키가 각도를 바꾼다.
  Q, W, E로 선택·이동·회전 모드를 전환하며 축은 항상 월드 기준이다. 크기 조절은 지원하지 않는다.
  조작을 끝낼 때 결과를 Preview Target Transform에 기록하며 Ctrl+Z로 되돌릴 수 있다.
  끄면 위젯이 사라지고 카메라 조작만 남는다.
- Resize: 가장 늦게 끝나는 태스크에 타임라인의 Length 범위를 맞춘다. 태스크가 없으면 5초를 사용한다.

이 월드는 PIE 세션이 아니다. GameInstance, PlayerController, PlayerState, 네트워크 및 게임 레벨 초기화를 자동 구성하지 않는다.
그것들에 의존하는 사용자 캐릭터·태스크는 프리뷰 월드에서도 실행할 수 있도록 작성해야 한다.
프리뷰는 호출 Gameplay Ability 없이 실행하므로 OwningAbility를 요구하는 프로젝트 태스크는 별도 대응이 필요하다.
Play Montage는 이 경우 AnimInstance 경로를 사용한다.

## 부모·자식과 변경분

Create Child로 현재 에셋을 부모로 참조하는 새 Kata 에셋을 만든다.
또는 Kata Action Details의 Parent Kata에서 부모를 지정한다.

부모의 현재 값과 태스크가 자식 에디터에 표시된다. 상속된 태스크도 행을 선택해 바로 편집하며 GUID를 직접 입력할 필요가 없다.
자식에서 고친 프로퍼티만 저장하므로 부모의 다른 수정과 태스크 추가는 계속 따라온다.

- Reset Override: 선택한 Kata 설정의 변경분을 제거하고 부모 값을 다시 따른다.
- Reset Task Override: 선택한 상속 태스크의 프로퍼티 하나 또는 전체 변경분을 제거한다.
- 상속 행을 삭제하면 Remove 변경분이 생긴다. 같은 메뉴의 Restore 항목으로 되돌린다.
- 비활성화는 Timeline Details의 Enabled를 끈다.
- 조건 객체와 배열은 하나의 프로퍼티 단위로 변경분을 보관한다.
- 부모를 바꿨을 때 대상이 사라진 태스크 오버라이드는 임의의 다른 태스크에 적용하지 않고 진단을 남긴다.

## 한 프레임 태스크

Timeline Details의 Single Frame을 켜면 그 태스크는 시작한 프레임에서 Tick을 한 번만 받고 끝난다.
켜는 순간 Duration은 의미가 없어져 Details에서 숨겨진다.

Duration을 0으로 둔 순간 태스크와 다르다. 순간 태스크는 Tick을 한 번도 받지 않고 시작과 동시에 끝난다.
한 프레임만 도는 처리가 필요하면 Single Frame을, 시작 시점의 단발 처리만 필요하면 Duration 0을 쓴다.

타임라인에서는 길이를 차지하지 않는 짧은 주황색 표식으로 그려지고 길이 조절 손잡이가 사라진다.
드래그로 시작 시각은 옮길 수 있지만 길이는 바꿀 수 없다.

## 검증 상태

사용자가 Editor 빌드와 Kata Action 에셋 생성을 확인했다.
에디터 조작과 프리뷰 실행의 회귀 여부는 아직 확인하지 않았다.
에이전트는 빌드, UHT, 테스트, 에디터 실행, 정적 검사 또는 별도 리뷰를 수행하지 않았고 테스트 코드도 추가하지 않았다.
