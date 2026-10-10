#pragma once

#include "CoreMinimal.h"

class UAnimMontage;
class UAnimSequenceBase;

/**
 * 루트 모션 커브의 한 시각 값.
 * 애니메이션 첫 프레임 루트를 원점으로 한 누적 이동(cm)과 누적 Yaw(도)다.
 * Yaw는 ±180도에서 끊기지 않도록 이어 붙인 값이다.
 */
struct FKataRootMotionCurveValue
{
    FVector Translation = FVector::ZeroVector;
    double Yaw = 0.0;
};

namespace KataFL
{
    /** 루트 모션 X 이동 커브 이름. 애니메이션 시퀀스와 몽타주가 같은 이름을 쓴다. */
    KATAFRAMEWORK_API FName GetRootMotionCurveNameX();

    /** 루트 모션 Y 이동 커브 이름. */
    KATAFRAMEWORK_API FName GetRootMotionCurveNameY();

    /** 루트 모션 Z 이동 커브 이름. */
    KATAFRAMEWORK_API FName GetRootMotionCurveNameZ();

    /** 루트 모션 누적 Yaw 커브 이름. */
    KATAFRAMEWORK_API FName GetRootMotionCurveNameYaw();

    /** 루트 모션 커브 네 개의 이름을 X, Y, Z, Yaw 순서로 반환한다. */
    KATAFRAMEWORK_API TArray<FName> GetRootMotionCurveNames();

    /**
     * 애니메이션이 루트 모션 커브 네 개를 모두 갖고 있으면 true를 반환한다.
     * 일부만 있으면 false이며, 이 경우 실행 시 커브를 쓰지 않고 원래 루트 모션을 쓴다.
     */
    KATAFRAMEWORK_API bool HasRootMotionCurves(const UAnimSequenceBase& Animation, bool bForceUseRawData = false);

    /**
     * 애니메이션 자신의 시간축 Time에서 루트 모션 커브 값을 읽는다.
     * 시퀀스는 압축 커브, 몽타주는 원본 커브를 평가한다. bForceUseRawData가 true면 시퀀스도 원본 커브를 읽는다.
     * 커브가 하나라도 없으면 false를 반환하고 OutValue를 바꾸지 않는다.
     */
    KATAFRAMEWORK_API bool EvaluateRootMotionCurves(const UAnimSequenceBase& Animation, double Time, FKataRootMotionCurveValue& OutValue,
        bool bForceUseRawData = false);

    /**
     * 두 커브 값 사이의 루트 모션 변화량을 시작 시각 루트 기준 Transform으로 만든다.
     * UAnimSequence::ExtractRootMotionFromRange가 반환하는 변화량과 같은 공간이며, Yaw 외 회전은 담지 않는다.
     */
    KATAFRAMEWORK_API FTransform MakeRootMotionCurveDelta(const FKataRootMotionCurveValue& Start, const FKataRootMotionCurveValue& End);

    /**
     * 몽타주 첫 슬롯 트랙의 정방향 구간 [StartTrackPosition, EndTrackPosition]에서 나는 루트 모션 변화량을 만든다.
     * bUseMontageCurves가 true이고 몽타주에 커브가 있으면 몽타주 트랙 시각으로 커브를 읽는다.
     * 그렇지 않으면 엔진 추출(UAnimCompositeBase::ExtractRootMotionFromTrack)과 같은 단계로 나눠,
     * 커브가 있는 시퀀스는 커브를, 없는 시퀀스는 원래 루트 모션을 누적한다. Enable Root Motion이 꺼진 시퀀스는 건너뛴다.
     * bUseSequenceCurves가 false면 시퀀스 커브가 있어도 원래 루트 모션을 쓴다. 커브 대체를 끈 캐릭터의 이동량을 구할 때 쓴다.
     * 커브를 하나라도 썼으면 bOutUsedCurve를 true로 바꾸고, 쓰지 않았으면 값을 바꾸지 않는다.
     */
    KATAFRAMEWORK_API FTransform ExtractMontageRootMotion(const UAnimMontage& Montage, float StartTrackPosition, float EndTrackPosition,
        bool bUseMontageCurves, bool& bOutUsedCurve, bool bUseSequenceCurves = true);
}
