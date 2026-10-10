# WindowsDualsense_ds5w 도입 기록

이 플러그인은 아래 오픈소스 플러그인을 가져와 Kata 샘플 프로젝트에 맞게 고친 것이다.
Kata 코어·위성·통합 플러그인은 이 플러그인을 참조하지 않는다. 샘플 프로젝트만 활성화한다.

- 출처: https://github.com/rafaelvaloto/Unreal-Dualsense
- 버전: 2.0.3 (`.uplugin`의 `VersionName`)
- 라이선스
  - 플러그인: MIT ([LICENSE](LICENSE))
  - 포함된 GamepadCore 라이브러리: MIT ([LICENSE](Source/WindowsDualsense_ds5w/Private/GamepadCore/LICENSE))
  - GamepadCore에 포함된 `miniaudio.h`: 퍼블릭 도메인 또는 MIT-0 (파일 끝의 라이선스 문구)
- 연결 이슈: [#44](https://github.com/jaykop/Kata/issues/44), 계획: [듀얼센스 지원 계획](../../docs/plan/DualSense-Plan.md)

## 삭제한 내용

Windows 전용 싱글플레이 샘플에서 쓰지 않는 코드를 뺐다.

- Linux·Mac 경로: `Implementations/Platforms/Commons/`(`CommonsDeviceInfo`, `LinuxHardwarePolicy.h`),
  `Subsystems/SonyInputProcessor`, 모듈 진입점과 `HapticsRegistry.cpp`의 `PLATFORM_LINUX`·`PLATFORM_MAC` 분기,
  `Build.cs`의 SDL2 의존.
- `Legacy/` 프록시(`UDualSenseProxy`, `USonyGamepadProxy`): 원본에서 이미 `(LEGACY)`로 표시한 Blueprint API다.
  `API/` 프록시는 라이트바·적응형 트리거의 진입점으로 남겼다.
- 플러그인 루트의 `README.md`, `README-pt_br.md`: 사용 안내는 원본 저장소를 따른다.

## 변경한 내용

- `.uplugin`: `"Installed"`를 `false`로 바꿨다. 마켓 설치본 표시라서 프로젝트 플러그인에는 맞지 않는다.
  모듈의 `PlatformAllowList`에서 `Linux`를 뺐다.
- `Build.cs`: UE 5.8 기본값과 같은 `CppStandard = Cpp20` 지정을 뺐다.
- `Build.cs`: `bUseUnity = false`. GamepadCore의 `miniaudio_impl.cpp`가 `<windows.h>`를 lean 설정 없이 포함해,
  unity 빌드로 묶이면 `PlaySound` 매크로가 같은 묶음의 오디오 헤더를 깨뜨린다.

### 장치 레지스트리 (DS-2)

- `FDeviceRegistry::GetLibraryInstance`를 순수 조회로 바꿨다. 원본은 조회에 실패하면 `RemoveLibraryInstance`로
  끊김 처리를 했다. 호출자가 연결된 모든 장치를 조회하므로 KBM(DeviceId 0)과 XInput 패드가 Disconnected로 바뀌었다.
- `TBasicDeviceRegistry::PlugAndPlay`(GamepadCore)
  - 이미 라이브러리가 있는 장치 경로는 다시 열지 않는다. 원본은 감지 주기(1초)마다 `CreateHandle`을 호출하고
    결과 Context를 버려 패드마다 HID 핸들이 누수되었다.
  - 핸들이 무효화된 라이브러리(`IsConnected()`가 false)를 제거해 다음 감지에서 같은 DeviceId로 다시 만든다.
    원본은 읽기 실패 뒤 장치가 계속 감지되면 라이브러리를 그대로 두어 입력이 멈췄다.
- `FWindowsDeviceInfo::Detect`: 경로에 Sony VID(`054c`)가 없는 HID 장치는 열지 않는다. 원본은 감지 주기마다
  키보드·마우스를 포함한 모든 HID 장치를 게임 스레드에서 열었다.

### 입력 읽기 구조 (DS-3)

- 장치마다 리더 스레드(`FSonyGamepadReader`, 신규)를 둔다. 스레드는 블로킹 HID 읽기로 리포트가 도착할 때마다
  `UpdateInput`을 호출한다. 원본은 `DeviceManager::Tick`이 `PollInterval`(0.0166초)마다 `AsyncTask`로 읽고
  누산기를 0으로 리셋해, 고주사율에서 폴링이 프레임 단위로 떨어지고(144fps에서 약 48Hz) 백그라운드 작업이
  원시 포인터로 라이브러리를 쓰는 동안 게임 스레드가 같은 상태를 읽었다.
- 리더의 시작과 정지는 `FDeviceRegistryPolicy`의 `StartReader`·`StopReader`가 맡고,
  `TBasicDeviceRegistry`(GamepadCore)가 라이브러리 생성 직후와 정리 직전에 호출한다. 리더는 라이브러리를
  `shared_ptr`로 보유한다. 정지할 때는 `CancelSynchronousIo`로 대기 중인 읽기를 취소한다.
- `DeviceManager::Tick`은 매 Tick `SendControllerEvents(DeltaTime)`를 호출한다. `PluginSettings::PollInterval`과
  `USonyGamepadSettingsProxy::PollInterval`은 더 이상 입력 주기에 영향을 주지 않는다.
- `FDeviceContext::CopyInputState`(GamepadCore, 신규): 게임 스레드가 입력 상태를 잠금 아래에서 복사한다.
  원본의 `GetInputState`는 잠금을 푼 뒤 포인터를 돌려주어, 리더의 버퍼 교체와 동시에 읽을 수 있었다.
- `DeviceManager::MessageHandler`를 값으로 보관하고 `SetMessageHandler`를 구현했다. 원본은 const 참조 멤버였고
  `SetMessageHandler`가 비어 있었다.
- `FDeviceRegistry::Shutdown`이 `RegistryImplementation`도 파괴하고, 모듈의 `ShutdownModule`이 이를 호출한다.
  원본은 레지스트리를 정적 소멸 시점까지 남겼다.

### 분리 시 입력 고정 (DS-3 확인 중 발견)

- `FDualSenseLibrary::UpdateInput`, `FDualShockLibrary::UpdateInput`(GamepadCore): 읽기에 실패해 핸들이 무효화되면
  파싱하지 않고 중립 입력 상태를 게시한다. 원본은 `InvalidateHandle`이 0으로 지운 버퍼를 그대로 파싱해,
  스틱 원시값 0이 (-1, +1)로 정규화되면서 패드를 뽑은 순간 이동·시점 입력이 끝까지 밀린 채 고정되었다.
- `DeviceManager::HandleInputDeviceConnectionChange`(신규): 이 플러그인의 장치가 Disconnected로 바뀌면 중립 입력을
  한 번 보낸다. 원본은 끊긴 장치에 이벤트를 더 보내지 않아 마지막 스틱·버튼 값이 엔진에 남았다.

### 장치 식별 등록 (DS-4)

- `FDeviceRegistryPolicy::RegisterDeviceDescriptor`(신규): 장치를 연결 알림 전에 `FInputDeviceRegistry::RegisterDevice`로 등록한다.
  입력 클래스 이름은 `WindowsDualsense`, 하드웨어 식별자는 `DualSense`·`DualSenseEdge`·`DualShock4`다.
  식별자가 `UInputPlatformSettings`의 HardwareDevices에 없으면 Gamepad 유형과 지원 기능으로 런타임에 추가한다.
- `FDeviceRegistryPolicy::DispatchNewGamepad`가 라이브러리를 함께 받는다. `TBasicDeviceRegistry`의 호출과 정책 concept도 바꿨다.
- `DeviceManager::SendControllerEvents`의 `FInputDeviceScope`를 제거했다. UE 5.8에서 deprecated이고, 생성자가 장치를 등록하지 않아
  `UInputDeviceSubsystem`이 듀얼센스 입력을 최근 사용 장치로 식별하지 못했다.

### 정리 (DS-5)

- PS 전용 키 중 표준 게임패드 키와 겹치는 `PS_PushLeftStick`, `PS_PushRightStick`, `PS_Menu`, `PS_Share`를 등록하지도 발행하지도 않는다.
  원본은 같은 버튼을 두 키로 보내 두 키를 모두 매핑하면 입력이 중복되었다. `PS_Mic`, `PS_TouchButtom`, `PS_Button`과 Edge의 Fn·Paddle 키는 유지한다.
- `DeviceManager::CheckButtonInput`: 장치별 FName 집합(`PressedButtons`)으로 눌린 버튼을 기록한다. 원본은 매 이벤트 발송마다
  FName을 `std::string`으로 변환해 `FDeviceContext::ButtonStates`를 조회했다.
- `DeviceManager::IsGamepadAttached`: 라이브러리가 있을 때만 true를 돌려준다(`FDeviceRegistry::HasAnyDevice`, `TBasicDeviceRegistry::HasLibraries` 신규). 원본은 항상 true였다.
- `DeviceManager::GetHapticFrequencyRange`: 출력 인자를 0~1로 채운다. 원본은 비워 두었다.
- `ds.*` 콘솔 명령을 `!UE_BUILD_SHIPPING`에서만 등록한다.
- 로그: `HapticsRegistry`의 재등록 경고를 Verbose로 낮췄고, `FValidateHelpers::PrintBufferAsHex`를 `LogTemp`(Log)에서 `LogDualSense`(VeryVerbose)로 옮겼다.
- `Build.cs`: Linux·Mac 입력 전처리기용이던 `Slate`·`SlateCore` 의존을 뺐다.

## 주석 규칙

벤더 코드의 원문 주석과 파일 인코딩(BOM, CRLF)은 유지한다. 우리가 고치거나 새로 쓴 부분에만 한국어 주석을 단다.
