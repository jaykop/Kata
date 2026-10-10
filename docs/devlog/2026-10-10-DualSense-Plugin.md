# 듀얼센스 플러그인 도입과 원본 결함 수정

작성: 2026-10-10  
갱신: 2026-10-10  
유형: 결정 기록 / 진단  
대상: `Plugins/WindowsDualsense_ds5w`, 샘플 `IMC_Default`  
기준: [#44](https://github.com/jaykop/Kata/issues/44)의 DS-1부터 DS-6까지 반영한 작업 트리

## 배경과 결론

Windows에서 엔진의 기본 게임패드 경로는 XInput이라 듀얼센스를 인식하지 못한다.
서드파티 플러그인 WindowsDualsense_ds5w v2.0.3(MIT)을 독립 벤더 플러그인으로 들여왔다. 이 플러그인은 버튼과 스틱을 엔진 표준 게임패드 키로 보내므로
Kata 쪽에는 연결 코드가 없고, 샘플 IMC에 게임패드 키를 매핑하는 것만으로 조작할 수 있다.

원본 소스를 UE 5.8 엔진 소스와 대조해 보니, 그대로 쓰면 다른 입력 장치를 끊김으로 처리하고 HID 핸들을 누수하는 등의 결함이 있었다.
들여오면서 Linux·Mac 경로와 Legacy API를 삭제했고, 장치 레지스트리와 입력 읽기 구조를 고쳤다.
실행 확인 중에는 패드를 뽑으면 입력이 끝까지 밀린 채 고정되는 원본 결함을 추가로 발견해 수정했다.
이어서 UE 5.8의 장치 식별 방식(`FInputDeviceRegistry`)에 맞춰 장치를 등록하고, 중복 키 발행과 디버그 명령·로그를 정리했다.
변경 목록 전체는 [UPSTREAM.md](../../Plugins/WindowsDualsense_ds5w/UPSTREAM.md)에 있다.

## 변경 또는 진단 내용

| 항목 | 원본 동작 | 반영 결과 |
|---|---|---|
| 다른 장치의 끊김 처리 | `FDeviceRegistry::GetLibraryInstance`가 조회에 실패하면 `RemoveLibraryInstance`로 장치를 끊김 처리했다. 호출자가 연결된 모든 장치를 조회하므로 KBM(DeviceId 0)과 XInput 패드가 Disconnected로 바뀌었다. 기존 사용 프로젝트의 에디터 로그에서 `Input device connection change: NewState=Disconnected, DeviceId=0 ... KBM`을 확인했다 | 순수 조회로 바꿨다. 장치 제거는 장치 감지 루프에서만 한다 |
| 장치 감지 | 1초마다 게임 스레드에서 모든 HID 장치를 열었다. 이미 연결된 패드에도 `CreateHandle`과 `HidD_GetFeature`를 다시 호출한 뒤 결과를 버려 핸들이 매초 누수되었다 | 경로에 Sony VID(`054c`)가 없으면 열지 않고, 이미 연결된 경로는 건너뛴다 |
| 읽기 실패 뒤 복구 | 핸들만 닫고 라이브러리는 남겨, 장치 경로가 계속 감지되면 다시 만들지 않았다 | 핸들이 무효화된 라이브러리를 제거하고 같은 DeviceId로 다시 만든다 |
| 입력 읽기 구조 | 0.0166초마다 `AsyncTask`로 읽고 누산기를 0으로 리셋했다. 144fps에서는 약 48Hz로 떨어졌고, 백그라운드 작업이 원시 포인터로 라이브러리를 쓰는 동안 게임 스레드가 같은 상태를 읽었다 | 장치별 리더 스레드가 리포트마다 읽고, 게임 스레드는 매 Tick 잠금 아래 복사본으로 이벤트를 보낸다 |
| 분리 시 입력 고정 | 읽기에 실패하면 0으로 지운 버퍼를 그대로 파싱했다. 스틱 원시값 0이 (-1, +1)로 정규화되어, PIE 중 패드를 뽑으면 캐릭터가 혼자 전진하고 카메라가 올라갔다. 끊긴 장치에는 중립 이벤트도 보내지 않았다 | 읽기 실패 시 중립 상태를 게시하고, 장치가 Disconnected로 바뀌면 중립 입력을 한 번 보낸다 |
| 메시지 핸들러 | const 참조 멤버로 보관하고 `SetMessageHandler`가 비어 있었다 | 값으로 보관하고 `SetMessageHandler`를 구현했다 |
| 범위 | Linux·Mac SDL 경로, `Legacy/` BP 프록시, 원본 README를 포함했다 | 삭제했다. `API/` 프록시는 라이트바·적응형 트리거의 진입점으로 남겼다 |
| 장치 식별 | deprecated `FInputDeviceScope`만 썼다. UE 5.8에서 이 생성자는 장치를 등록하지 않아 `UInputDeviceSubsystem`이 듀얼센스 입력을 최근 사용 장치로 식별하지 못했다 | 연결 알림 전에 `FInputDeviceRegistry`에 등록하고, 하드웨어 식별자(`DualSense`·`DualSenseEdge`·`DualShock4`)를 `UInputPlatformSettings`에 Gamepad 유형으로 추가한다 |
| 키 중복 발행 | 스틱 누르기·Options·Create 버튼을 표준 키와 PS 전용 키로 두 번 보냈다 | 표준 키만 보내고 겹치는 PS 키 4개는 등록하지 않는다 |
| 버튼 상태 기록 | 매 이벤트 발송마다 FName을 `std::string`으로 변환해 맵을 조회했다 | 장치별 FName 집합에 눌린 버튼만 기록한다 |
| 기타 인터페이스 | `IsGamepadAttached`가 항상 true였고 `GetHapticFrequencyRange`가 출력값을 채우지 않았다 | 듀얼센스가 있을 때만 true를 돌려주고, 범위는 0~1로 채운다 |
| 디버그 명령·로그 | `ds.*` 콘솔 명령을 Shipping에도 등록했고, 재등록 경고와 `LogTemp` 버퍼 덤프가 있었다 | 콘솔 명령은 Shipping에서 빼고, 경고는 Verbose, 버퍼 덤프는 `LogDualSense` VeryVerbose로 옮겼다 |
| 샘플 IMC | 게임패드 매핑이 없었다 | 이동·시점·공격·회피·락온 매핑을 추가했다. 아래 사용법은 [입력 사용법](../manual/Input.md#게임패드와-듀얼센스)에 있다 |

## 주요 결정과 이유

- **독립 벤더 플러그인으로 둔다.** 코어는 엔진과 GAS에만 의존해야 하고, 기본 입력은 표준 게임패드 키라 KataFramework와 연결할 코드가 없다.
  샘플 프로젝트만 플러그인을 활성화한다.
- **원본 구조를 유지한 채 패치한다.** 수정은 레지스트리, `DeviceManager`, 읽기 경로에 몰려 있어서 원본이 갱신되면 파일 단위로 비교할 수 있다.
  다시 쓰면 HID 리포트 파싱과 보정 같은 검증된 부분까지 새로 만들어야 한다. 원본 파일의 BOM·CRLF와 원문 주석은 유지하고 수정한 부분에만 한국어 주석을 단다.
- **HID 읽기는 장치별 리더 스레드로 한다.** 리포트가 오는 즉시 읽으므로 지연이 가장 짧고, 경합은 입력 상태 복사 한 곳의 잠금으로 막는다.
  게임 스레드에서 overlapped I/O로 논블로킹 읽기를 하는 방식은 이벤트 핸들, 취소, 부분 완료 처리가 늘어나서 택하지 않았다.
  리더 시작·정지는 라이브러리 생성·정리와 같은 곳(레지스트리 정책)에서 하고, 리더가 라이브러리를 `shared_ptr`로 보유해 정리 순서와 수명을 맞춘다.
  정지할 때는 `CancelSynchronousIo`를 스레드가 끝날 때까지 반복한다. 리포트가 끊긴 장치에서 블로킹 읽기가 돌아오지 않을 수 있기 때문이다.
- **분리 시 중립 입력을 두 곳에서 보낸다.** 읽기 실패 시점에 중립 상태를 게시해도, 같은 Tick에 장치 감지가 먼저 라이브러리를 지우면 그 상태가 이벤트로 나가지 않는다.
  그래서 장치가 Disconnected로 바뀌는 연결 변경 델리게이트에서도 한 번 보낸다. 레지스트리가 라이브러리를 정리하기 직전에 끊김을 알리므로, 이 시점에는 라이브러리와 눌린 버튼 기록이 아직 남아 있어 해제 이벤트를 만들 수 있다.
- **하드웨어 식별자는 런타임에 추가한다.** XInput 플러그인은 `Config/Input.ini`로 `UInputPlatformSettings`의 HardwareDevices에 식별자를 넣는다.
  프로젝트 플러그인의 설정 파일이 같은 방식으로 병합되는지 확인하지 못해, 장치를 처음 감지할 때(엔진 초기화 이후) 목록에 없으면 추가한다.
  지원 기능은 플러그인이 실제로 제공하는 것만 표시한다.
- **버튼 상태는 비트마스크 대신 FName 집합으로 기록한다.** 계획에는 비트마스크로 적었지만, 목적은 이벤트마다 생기던 문자열 할당을 없애는 것이라 기존 호출 구조를 유지하는 FName 집합을 택했다.
- **샘플 IMC의 스틱 시점은 프레임 시간으로 보정한다.** `UKataInputHandlerComponent::Look`은 축 값을 그대로 회전 입력에 더하므로,
  코드를 바꾸지 않고 게임패드 매핑에만 Scale By Delta Time과 Scalar를 붙였다. 스틱 상하는 사용자 요청으로 반전하지 않는다.

## 시행착오

- DS-4 빌드에서 unity 파일 `Module.WindowsDualsense_ds5w.cpp`가 `AudioMixerQuantizedCommands.h`의 `EQuartzCommandType::PlaySound`에서 실패했다(`PlaySoundW`).
  처음에는 리더 스레드 파일의 `<windows.h>` 직접 포함을 의심해 `Windows/WindowsHWrapper.h`로 바꿨지만 같은 에러가 났다.
  실제 원인은 GamepadCore의 `miniaudio_impl.cpp`다. `MINIAUDIO_IMPLEMENTATION`으로 `miniaudio.h`를 포함하면 그 안에서 `<windows.h>`를 lean 설정 없이 포함해
  `PlaySound` 매크로가 정의되고, 같은 unity 묶음의 `HapticsRegistry.cpp`가 포함하는 오디오 헤더를 깨뜨린다.
  플러그인 파일이 미추적·수정 상태일 때는 adaptive unity가 따로 컴파일해 드러나지 않았고, 첫 커밋 뒤 unity로 묶이면서 나타난 것으로 본다.
  miniaudio 구현부를 lean 설정으로 감싸면 WinMM 같은 오디오 백엔드 구성이 바뀔 수 있어, 이 모듈만 `bUseUnity = false`로 했다.

## 근거

- [UPSTREAM.md](../../Plugins/WindowsDualsense_ds5w/UPSTREAM.md): 출처, 라이선스, 삭제·변경 목록.
- [DeviceRegistry.cpp](../../Plugins/WindowsDualsense_ds5w/Source/WindowsDualsense_ds5w/Private/Implementations/Adapters/DeviceRegistry.cpp): `GetLibraryInstance`의 순수 조회, 모듈 종료 시 레지스트리 파괴.
- [TBasicDeviceRegistry.h](../../Plugins/WindowsDualsense_ds5w/Source/WindowsDualsense_ds5w/Private/GamepadCore/Source/Public/GCore/Templates/TBasicDeviceRegistry.h): 장치 감지 루프, 리더 시작·정지 시점.
- [SonyGamepadReader.cpp](../../Plugins/WindowsDualsense_ds5w/Source/WindowsDualsense_ds5w/Private/Implementations/Managers/SonyGamepadReader.cpp): 리더 스레드와 정지 절차.
- [DeviceManager.cpp](../../Plugins/WindowsDualsense_ds5w/Source/WindowsDualsense_ds5w/Private/DeviceManager.cpp): 매 Tick 이벤트 발송, Disconnected 시 중립 입력, 버튼 상태 기록.
- [DeviceRegistryPolicy.cpp](../../Plugins/WindowsDualsense_ds5w/Source/WindowsDualsense_ds5w/Private/Implementations/Adapters/DeviceRegistryPolicy.cpp): 장치 등록과 하드웨어 식별자 추가, 리더 시작·정지.
- UE 5.8 `InputDeviceSubsystem.cpp`: 입력 전처리기가 `FInputDeviceRegistry::FindDescriptor`로 장치를 찾고 `UInputPlatformSettings::GetHardwareDeviceForClassName`으로 유형을 정한다.
- UE 5.8 `GenericPlatformInputDeviceMapper.cpp`: `Internal_SetInputDeviceConnectionState`는 상태가 바뀔 때만 `OnInputDeviceConnectionChange`를 브로드캐스트한다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 원본 결함 | 원본 소스와 UE 5.8 엔진 소스 대조, 기존 사용 프로젝트 로그 확인(에이전트) | KBM 끊김 로그 1회 확인. 나머지는 코드 대조로 판단 | 사용자가 말한 "입력 장치 변경 이벤트가 계속 발생"은 로그로 재현되지 않았다 |
| DS-1 도입 | 사용자 Editor 빌드 | 성공 | 없음(Game 빌드는 아래 전체 행) |
| DS-2 레지스트리 | 사용자 Editor 빌드 | 성공 | KBM 끊김 로그가 사라졌는지, 핸들 수, 1초 주기 히치 |
| DS-3 리더 스레드와 분리 수정 | 사용자 Editor 빌드, PIE | 빌드 성공. PIE 중 패드를 뽑아도 캐릭터와 카메라가 움직이지 않음 | 고주사율 반응, 에디터 종료 시 정지, 재연결 복구, Bluetooth 연결 |
| DS-4 장치 식별 | 사용자 Editor 빌드(unity 수정 후), PIE | 빌드 성공. 키보드·마우스와 듀얼센스 사이 장치 전환이 동작함 | `SetMostRecentlyUsedHardwareDevice` Verbose 로그는 사용자 환경에서 보이지 않았다. `RegisterDevice` 로그 확인 보고 없음 |
| DS-5 정리 | 사용자 Editor 빌드, PIE | 빌드 성공. 버튼, `ds.*` 콘솔 명령(에디터), 장치 전환, 이동·시점 이전과 동일 | Game 타깃에서 `ds.*` 명령 제외 |
| DS-1~DS-6 전체 | 사용자 Game 타깃 빌드(DS-5 반영 후) | 성공 | 패키지 실행, Game에서 `ds.*` 명령이 빠졌는지 |
| DS-6 IMC 매핑 | MCP로 저장 후 에셋 값 다시 읽기(에이전트) | 매핑과 모디파이어 값이 저장됨 | 매핑별 동작과 시점 감도는 사용자 확인 보고 없음 |

## 남은 제한과 후속 작업

- 버튼을 누른 채 패드를 다시 연결하면 첫 리포트의 눌림이 새 입력으로 들어가 액션이 나간다(사용자 확인). XInput 패드와 같은 동작이다.
  연결 직후 이미 눌려 있던 버튼을 뗄 때까지 무시하는 처리는 요청 범위 밖이라 넣지 않았다.
- 원본에 남아 있는 경합: 마이크 버튼 토글 상태(`static bLastMuteState`)를 장치 사이에 공유하고, `IsConnected`를 잠금 없이 읽는다.
- Steam Input이나 DS4Windows가 패드를 XInput으로 바꾸는 중이면 입력이 중복된다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [연결 이슈](https://github.com/jaykop/Kata/issues/44) | DS-1부터 DS-6까지 구현하고 사용자 확인 뒤 닫았다 |
| [입력 사용법](../manual/Input.md#게임패드와-듀얼센스) | 게임패드 매핑, 듀얼센스 사용 조건, PS 전용 키, 재연결 시 동작, Steam Input 충돌 |
| 듀얼센스 지원 계획(`docs/plan/DualSense-Plan.md`) | 이슈를 닫으며 삭제했다. 확정 사항과 원본 결함 진단은 이 문서의 변경 표와 결정 절로 옮겼다 |
