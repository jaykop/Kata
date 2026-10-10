#include "Implementations/Managers/SonyGamepadReader.h"
#include "GCore/Interfaces/ISonyGamepad.h"
#include "HAL/PlatformProcess.h"
#include "HAL/RunnableThread.h"

#if PLATFORM_WINDOWS
// <windows.h>를 직접 포함하면 WIN32_LEAN_AND_MEAN 없이 mmsystem.h까지 들어와 PlaySound 같은 매크로가 정의되고,
// unity 빌드에서 같은 파일로 묶인 오디오 헤더(EQuartzCommandType::PlaySound)가 깨진다. 엔진 래퍼를 쓴다.
#include "Windows/WindowsHWrapper.h"
#endif

FSonyGamepadReader::FSonyGamepadReader(std::shared_ptr<ISonyGamepad> InGamepad, int32 InDeviceId)
    : Gamepad(MoveTemp(InGamepad))
{
    check(Gamepad);

    const FString ThreadName = FString::Printf(TEXT("SonyGamepadReader_%d"), InDeviceId);
    Thread = FRunnableThread::Create(this, *ThreadName, 0, TPri_AboveNormal);
    if (Thread == nullptr)
    {
        // 스레드를 만들지 못하면 Run이 실행되지 않으므로 Shutdown의 대기 루프가 끝나도록 완료 상태로 둔다.
        bFinished = true;
    }
}

FSonyGamepadReader::~FSonyGamepadReader()
{
    Shutdown();
}

void FSonyGamepadReader::Shutdown()
{
    if (Thread == nullptr)
    {
        return;
    }

    Stop();

#if PLATFORM_WINDOWS
    // 블로킹 ReadFile은 다음 리포트가 오면 돌아오지만, 리포트가 끊긴 장치에서는 계속 기다릴 수 있다.
    // CancelSynchronousIo는 호출 시점에 진행 중인 읽기만 취소하므로, 스레드가 끝날 때까지 반복한다.
    if (HANDLE ReaderThreadHandle = ::OpenThread(THREAD_TERMINATE, 0, Thread->GetThreadID()))
    {
        while (!bFinished.load())
        {
            ::CancelSynchronousIo(ReaderThreadHandle);
            FPlatformProcess::Sleep(0.001f);
        }
        ::CloseHandle(ReaderThreadHandle);
    }
#endif

    Thread->WaitForCompletion();
    delete Thread;
    Thread = nullptr;
}

uint32 FSonyGamepadReader::Run()
{
    while (!bStopRequested.load())
    {
        // 읽기에 실패해 핸들이 무효화되면 레지스트리가 다음 감지 주기에 라이브러리를 지우고 이 스레드를 멈춘다.
        // 그때까지 바쁜 대기를 피한다.
        if (!Gamepad->IsConnected())
        {
            FPlatformProcess::Sleep(0.005f);
            continue;
        }

        // 읽기는 다음 입력 리포트가 올 때까지 블로킹되므로 이 호출이 루프 주기를 정한다.
        // 라이브러리 구현은 Delta 인자를 쓰지 않는다.
        Gamepad->UpdateInput(0.0f);
    }

    bFinished = true;
    return 0;
}

void FSonyGamepadReader::Stop()
{
    bStopRequested = true;
}
