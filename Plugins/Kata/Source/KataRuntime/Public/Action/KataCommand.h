#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KataCommand.generated.h"

class UKataActionInstance;

/**
 * 액션의 시작 또는 종료 시점에 한 번 실행하고 같은 프레임 안에 끝나는 로직.
 *
 * 언제 실행할지는 UKataAction의 PreCommands·PostCommands 목록이 정한다. 같은 클래스를 양쪽 목록에 넣을 수 있다.
 * 지속 시간이 있거나 효과를 유지해야 하는 로직은 Command가 아니라 타임라인 태스크로 만든다.
 * 실행되는 객체는 액션을 재생할 때마다 해석 결과가 새로 복제한 사본이며, 실행이 끝난 뒤 상태를 남기지 않는다.
 * C++에서는 Execute_Implementation을, Blueprint에서는 Execute 이벤트를 재정의한다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories)
class KATARUNTIME_API UKataCommand : public UObject
{
    GENERATED_BODY()

public:
    /** Run이 실행되는 동안에는 실행 중인 액션의 월드를 돌려준다. 그 밖에는 nullptr이다. */
    virtual UWorld* GetWorld() const override;

    /**
     * UKataActionInstance가 호출하는 실행 진입점. Execute를 한 번 호출하고 반환한다.
     * 실행하는 동안만 Instance를 월드 컨텍스트로 붙잡고, 반환 전에 놓는다.
     */
    void Run(UKataActionInstance* Instance);

protected:
    /**
     * 명령 로직. 같은 프레임 안에서 끝나야 한다.
     * Blueprint에서 Delay 같은 지연 노드로 이후 프레임에 작업을 예약하지 않는다. 그 시점에는 월드 컨텍스트가 없다.
     *
     * PreCommands에서는 Instance->SetTargetActor로 이번 실행의 대상을 바꿀 수 있다.
     * PostCommands에서는 Instance->GetEndReason으로 종료 사유를 읽을 수 있다.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Kata|Command")
    void Execute(UKataActionInstance* Instance);
    virtual void Execute_Implementation(UKataActionInstance* Instance);

private:
    /**
     * Run 동안만 유효한 실행 인스턴스. Blueprint의 월드 컨텍스트 노드가 GetWorld를 쓸 수 있게 한다.
     * 해석 결과의 사본에만 잠시 기록하며 공유 에셋에는 남지 않는다.
     */
    UPROPERTY(Transient)
    TObjectPtr<UKataActionInstance> RunningInstance;
};
