# 듀얼센스 지원 계획

작성: 2026-10-10  
갱신: 2026-10-10  
연결 이슈: [#44 듀얼센스 입력 지원 (WindowsDualsense_ds5w 도입과 수정)](https://github.com/jaykop/Kata/issues/44)  
현재 상태 근거: 연결 이슈, 원본 플러그인 v2.0.3 소스와 UE 5.8 엔진 소스 대조  
대체 관계: 없음

## 목적과 현재 상태

Windows의 엔진 기본 게임패드 경로는 XInput이라서 듀얼센스를 인식하지 못한다. 듀얼센스를 쓰려면 HID를 직접 읽는 입력 장치 플러그인이 필요하다.
후보는 서드파티 플러그인 [WindowsDualsense_ds5w](https://github.com/rafaelvaloto/Unreal-Dualsense) v2.0.3(MIT)이다.

이 플러그인은 `IInputDeviceModule`로 등록되고 버튼·스틱을 엔진 표준 `FGamepadKeyNames`로 보낸다. 그래서 Enhanced Input의 `Gamepad_*` 키가 Xbox 패드와 같은 방식으로 동작한다.
DualSense·DualSense Edge·DualShock 4를 지원하고 자이로, 터치패드, 적응형 트리거, 라이트바, 오디오 햅틱 API도 제공한다.

현재 프로젝트에는 듀얼센스 입력 경로가 없다. 샘플 IMC(`Content/KataSample/Input/IMC_Default`)에도 게임패드 키 매핑이 없다. 플러그인만 들여오면 패드는 인식되지만 게임에는 반응이 없다.

원본 소스와 UE 5.8 엔진 소스를 대조한 결과, 그대로 들여오면 안 되는 결함이 있다.

### 원본 결함 진단

| 구분 | 원본 동작 | 영향 |
|---|---|---|
| 다른 장치의 끊김 처리 | `FDeviceRegistry::GetLibraryInstance()`는 조회에 실패하면 `RemoveLibraryInstance()`로 `DisconnectDevice()`를 호출한다. `DeviceManager::Tick`, `SendControllerEvents`, `SonyGamepadProxyHelpers::GetGamepad`는 연결된 모든 장치를 이 함수로 조회한다. | KBM(DeviceId 0)과 XInput 패드가 Disconnected로 바뀌고 `OnInputDeviceConnectionChange`가 브로드캐스트된다. 이 플러그인을 쓰던 기존 프로젝트의 에디터 로그에서 KBM Disconnected 기록을 확인했다. |
| 장치 탐색 | `TBasicDeviceRegistry::PlugAndPlay`는 1초마다 게임 스레드에서 모든 HID 장치를 `CreateFileW`로 연다. 이미 연결된 패드에도 `CreateHandle`과 `HidD_GetFeature`를 다시 호출하고, 그 Context를 버린다. | 1초 주기로 히치가 생기고 패드마다 HID 핸들이 매초 누수된다. |
| 읽기 스레드 | 폴링마다 `AsyncTask`로 HID를 읽고, 게임 스레드는 같은 입력 상태를 바로 읽는다. 백그라운드 람다도 `GetLibraryInstance`를 호출한다. | 데이터 레이스, 폴링 한 주기의 입력 지연, 패드 분리 시 크래시 가능성. 스레드 안전하지 않은 `IPlatformInputDeviceMapper`를 백그라운드에서 호출할 수 있다. |
| 폴링 주기 | `PollAccumulator`가 간격(0.0166초)을 넘으면 0으로 리셋한다. | 144fps에서 약 48Hz로 떨어지고, 60fps에서 프레임 시간이 흔들리면 해당 프레임은 30Hz가 된다. 입력 버퍼와 판정 창의 체감에 직접 영향을 준다. |
| 읽기 실패 | 읽기에 실패하면 `InvalidateHandle`로 핸들만 닫고 라이브러리 인스턴스는 남긴다. 장치 경로가 그대로 감지되므로 다시 생성되지 않는다. | Bluetooth 일시 끊김 뒤 입력이 멈춘다. |
| UE 5.8 장치 식별 | `FInputDeviceScope`를 사용한다. 5.8에서 이 클래스는 deprecated이고, 생성자는 `FInputDeviceRegistry`에 장치를 등록하지 않는다. | 빌드 경고가 생기고, `UInputDeviceSubsystem`이 듀얼센스 입력을 식별하지 못한다. 입력 장치별 UI 아이콘 전환 같은 기능이 동작하지 않는다. |
| 메시지 핸들러 | `MessageHandler`를 `const TSharedRef&` 멤버로 보관하고 `SetMessageHandler`를 비워 둔다. | 엔진 쪽 멤버를 참조해서 우연히 동작한다. 댕글링 위험이 있다. |
| 키 중복 발행 | 같은 물리 버튼을 표준 키와 PS 전용 키로 두 번 보낸다(`LeftThumb`과 `PS_PushLeftStick`, `SpecialRight`와 `PS_Menu`, `SpecialLeft`와 `PS_Share`). | 두 키를 모두 매핑하면 입력이 두 번 들어온다. |
| 기타 | `IsGamepadAttached()`가 항상 `true`를 반환한다. `GetHapticFrequencyRange`가 출력값을 초기화하지 않는다. `CheckButtonInput`은 버튼마다, 폴링마다 FName을 `std::string`으로 변환한다. | 패드가 없어도 연결된 것으로 판단하고, 불필요한 할당이 생긴다. |
| 로그와 디버그 | 자체 로그는 대부분 `ds.*` 콘솔 명령 안에 있다. 콘솔 명령은 Shipping에서도 등록된다. `HapticsRegistry`는 재등록마다 Warning을 내고, `PrintBufferAsHex`는 `LogTemp`로 버퍼를 덤프한다. | 디버그 기능을 Shipping에서 빼는 규칙에 어긋난다. 반복 경고가 생길 수 있다. |

## 범위

- 포함: 플러그인 도입과 출처 기록, 위 결함 수정, UE 5.8 장치 등록, 로그와 디버그 정리, 샘플 IMC 게임패드 매핑과 스틱 시점 감도 보정.
- 제외: 적응형 트리거·라이트바·오디오 햅틱을 Kata 액션 태스크와 연결하는 작업, Windows 외 플랫폼, 엔진 `GameInput` 플러그인 경로.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| Kata 플러그인과의 의존 | 확정 | 코어와 위성 플러그인은 이 플러그인을 참조하지 않는다. 코어는 엔진과 GAS에만 의존한다는 [AGENTS.md](../../AGENTS.md) 모듈 경계를 따른다. 기본 입력은 표준 게임패드 키라서 연결 코드가 필요 없다. |
| 배치 위치 | 확정(2026-10-10) | `Plugins/WindowsDualsense_ds5w/`에 독립 벤더 플러그인으로 두고 `ProjectKata.uproject`에서만 활성화한다. 폴더 이름은 `.uplugin` 이름에 맞춘다. |
| 원본 관리 방식 | 확정(2026-10-10) | 원본 디렉터리 구조를 유지한 채 패치한다. 수정은 레지스트리·DeviceManager·읽기 경로에 몰려 있어 원본 업데이트를 파일 단위로 비교할 수 있다. 원본 주석은 그대로 두고, 고치거나 새로 쓴 부분에만 한국어 주석을 단다. 출처, 버전, 변경 목록은 `UPSTREAM.md`에 적는다. |
| 범위 축소 | 확정(2026-10-10) | Linux·Mac SDL 경로와 `Legacy/` 프록시(모든 함수가 `(LEGACY)` 표기된 BP API)를 삭제한다. `API/` 프록시는 라이트바·적응형 트리거의 진입점으로 남긴다. `PlatformAllowList`를 Win64로 한정하고 `"Installed": true`를 `false`로 바꾼다. |
| HID 읽기 구조 | 확정(2026-10-10) | 장치별 리더 스레드가 블로킹 읽기로 최신 리포트를 잠금 보호 버퍼에 두고, 게임 스레드는 Tick마다 한 번 복사해 입력 이벤트를 보낸다. 게임 스레드 overlapped I/O 방식은 이벤트 핸들·취소·부분 완료 처리가 늘어나서 택하지 않았다. |
| PS 전용 키 | 확정(2026-10-10) | 표준 키와 겹치는 PS 키(`PS_PushLeftStick`, `PS_PushRightStick`, `PS_Menu`, `PS_Share`)는 등록과 발행을 중단한다. Mic, 터치패드 버튼, PS 버튼, Edge의 Fn·Paddle만 남긴다. |

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| DS-1 | 높음 | 원본 소스 도입과 범위 축소, `.uplugin` 수정, `UPSTREAM.md`·LICENSE 보존, uproject 활성화 | 없음 | Editor 빌드에서 플러그인이 로드되고 듀얼센스 입력이 엔진 게임패드 키로 들어온다. |
| DS-2 | 높음 | 장치 레지스트리 수정: 조회 함수의 부작용 제거, 알려진 경로의 재오픈 금지, VID 필터, 읽기 실패 시 라이브러리 제거 | DS-1 | KBM·XInput 장치의 연결 상태가 바뀌지 않는다. 핸들 수가 늘지 않는다. 패드 분리·재연결 뒤 입력이 복구된다. |
| DS-3 | 높음 | HID 읽기 구조 교체와 폴링 주기 수정, 메시지 핸들러 보관 방식 수정 | DS-1 | 프레임레이트와 관계없이 매 Tick 최신 리포트로 입력을 보낸다. 백그라운드 스레드에서 입력 장치 매퍼를 호출하지 않는다. |
| DS-4 | 보통 | `FInputDeviceRegistry::RegisterDevice`로 장치 등록, `FInputDeviceScope` 제거 | DS-2 | deprecated 경고가 없어지고, `UInputDeviceSubsystem`이 듀얼센스를 최근 사용 장치로 보고한다. |
| DS-5 | 보통 | 정리: 키 중복 발행 제거, 버튼 상태 비트마스크화, `IsGamepadAttached`·`GetHapticFrequencyRange` 수정, `ds.*` 명령의 `!UE_BUILD_SHIPPING` 처리, 로그 수준 조정 | DS-1 | Game 빌드에 `ds.*` 명령이 없고, 정상 사용 중 `LogDualSense` Warning이 반복되지 않는다. |
| DS-6 | 높음 | 샘플 IMC 게임패드 매핑(이동·시점·공격·회피·락온), 시점 매핑에 `Scale By Delta Time`과 `Scalar` Modifier 추가 | DS-1 | PIE에서 듀얼센스로 이동·시점·공격·회피·락온이 동작하고, 시점 회전 속도가 프레임레이트와 무관하다. |

항목별 진행 상태는 연결 이슈의 체크리스트와 댓글로 관리한다.

## 영향과 제한

- 모듈 경계: 새 플러그인은 Kata 코어·위성·통합 플러그인과 의존 관계가 없다. 샘플 프로젝트만 활성화한다.
- 공개 API: 원본 BP 프록시 중 `Legacy/` 계열을 삭제한다. 이 프로젝트에는 사용처가 없다.
- 에셋: 샘플 IMC에 매핑을 추가한다. `UKataInputHandlerComponent::Look`은 축 값을 그대로 회전 입력으로 쓰므로, 스틱 감도는 코드가 아니라 IMC Modifier로 보정한다.
- 자원 수명: HID 핸들과 리더 스레드는 장치 제거와 모듈 종료 시 정리해야 한다.
- 충돌: Steam Input이나 DS4Windows가 패드를 XInput으로 변환하고 있으면 입력이 중복된다. 사용 설명서에 적는다.
- 저장소: 원본 LICENSE 두 개(플러그인, GamepadCore)와 `miniaudio.h`를 함께 추적한다.

## 사용자 확인 항목

- 구현 완료: DS-1부터 DS-6까지의 코드와 에셋 반영.
- 실행 확인(사용자):
  - Editor·Game 빌드
  - USB와 Bluetooth 연결 각각에서 입력
  - 키보드·마우스와 번갈아 사용할 때 연결 변경 로그가 생기지 않음
  - 패드 분리·재연결 뒤 입력 복구
  - 고주사율(144fps 이상)에서 입력 반응
  - Steam Input을 끈 상태에서 중복 입력 없음
- 아직 확인되지 않은 범위: 원본의 오디오 햅틱과 적응형 트리거는 이번 확인 대상이 아니다.

## 완료 시 갱신할 문서

- [연결 이슈](https://github.com/jaykop/Kata/issues/44): 구현·확인 상태.
- [입력 사용법](../manual/Input.md): 게임패드 매핑, 듀얼센스 사용 조건, Steam Input 충돌.
- devlog: 원본 결함과 수정 결정 기록(새 문서).
- [문서 목록](../README.md): 이 계획 링크 추가, 이슈를 닫으면 삭제.
