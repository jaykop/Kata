#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include <atomic>
#include <memory>

class FRunnableThread;
class ISonyGamepad;

/**
 * 패드 하나의 HID 입력 리포트를 전용 스레드에서 읽는다.
 *
 * 블로킹 읽기로 리포트가 도착할 때마다 ISonyGamepad::UpdateInput을 호출해 입력 버퍼를 갱신한다.
 * 게임 스레드는 FDeviceContext::CopyInputState로 최신 상태를 복사해 쓴다.
 * 라이브러리를 shared_ptr로 보유하므로, 레지스트리가 라이브러리를 지워도 스레드가 끝날 때까지 객체가 살아 있다.
 *
 * 생성하면 스레드가 바로 시작한다. 라이브러리를 정리하기 전에 게임 스레드에서 Shutdown을 호출해야 하며,
 * 소멸자도 Shutdown을 호출한다.
 */
class FSonyGamepadReader final : public FRunnable
{
public:
    FSonyGamepadReader(std::shared_ptr<ISonyGamepad> InGamepad, int32 InDeviceId);
    virtual ~FSonyGamepadReader() override;

    /**
     * 스레드에 정지를 요청하고 끝날 때까지 기다린다.
     * 대기 중인 HID 읽기는 취소한다. 여러 번 호출해도 된다.
     */
    void Shutdown();

    //~ FRunnable
    virtual uint32 Run() override;
    virtual void Stop() override;
    //~ FRunnable

private:
    /** 읽기 대상 라이브러리. 스레드가 끝날 때까지 수명을 유지한다. */
    std::shared_ptr<ISonyGamepad> Gamepad;

    FRunnableThread* Thread = nullptr;

    std::atomic<bool> bStopRequested{false};

    /** Run이 반환하기 직전에 true가 된다. Shutdown이 읽기 취소를 반복할지 판단하는 데 쓴다. */
    std::atomic<bool> bFinished{false};
};
