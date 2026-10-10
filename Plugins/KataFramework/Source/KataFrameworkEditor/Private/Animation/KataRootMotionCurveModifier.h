#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "KataRootMotionCurveModifier.generated.h"

/**
 * 애니메이션 시퀀스의 루트 모션을 Kata 루트 모션 커브 네 개(X, Y, Z, Yaw)로 추출하는 수정자.
 *
 * 엔진 Animation Modifiers 창이나 콘텐츠 브라우저의 Add Modifiers로 시퀀스에 추가하고 Apply하면,
 * 첫 프레임 루트 기준 누적 이동과 누적 Yaw를 선형 키로 기록한다. 이미 커브가 있으면 덮어쓴다.
 * 실행 처리는 커브가 있는 시퀀스의 루트 모션을 이 커브 값으로 대체하므로, 추출 뒤 기획자가 커브를 고쳐 이동량을 조정한다.
 *
 * 추출은 사용자가 Apply할 때만 일어난다. 원본 애니메이션이 바뀌어도 자동으로 다시 추출하지 않는다.
 * Revert하면 네 커브를 지우므로 편집한 내용도 함께 사라진다.
 * Yaw 외 루트 회전(Pitch·Roll)은 담지 않으며, 원본에 그런 회전이 있으면 경고한다.
 */
UCLASS(meta = (DisplayName = "Kata Root Motion Curve"))
class UKataRootMotionCurveModifier : public UAnimationModifier
{
    GENERATED_BODY()

public:
    /** 켜면 시퀀스의 샘플링 프레임 레이트로 키를 만든다. 원본과 같은 보간 결과를 내려면 켜 둔다. */
    UPROPERTY(EditAnywhere, Category = "Kata|Root Motion Curve")
    bool bUseSequenceFrameRate = true;

    /** 시퀀스 프레임 레이트를 쓰지 않을 때의 초당 키 수. 프레임 레이트보다 낮으면 샘플 사이 오차가 생긴다. */
    UPROPERTY(EditAnywhere, Category = "Kata|Root Motion Curve", meta = (ClampMin = "1.0", EditCondition = "!bUseSequenceFrameRate"))
    float SampleRate = 30.0f;

    /** 샘플 사이 중간 시각에서 원본 루트 모션과의 위치 차이가 이 값을 넘으면 경고한다. */
    UPROPERTY(EditAnywhere, Category = "Kata|Root Motion Curve", meta = (ClampMin = "0.0", Units = "cm"))
    float PositionTolerance = 0.5f;

    /** Yaw 차이나 원본의 Pitch·Roll이 이 값을 넘으면 경고한다. */
    UPROPERTY(EditAnywhere, Category = "Kata|Root Motion Curve", meta = (ClampMin = "0.0", Units = "deg"))
    float RotationTolerance = 0.5f;

    virtual void OnApply_Implementation(UAnimSequence* Animation) override;
    virtual void OnRevert_Implementation(UAnimSequence* Animation) override;
};
