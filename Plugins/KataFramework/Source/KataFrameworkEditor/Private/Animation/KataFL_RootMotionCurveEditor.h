#pragma once

#include "Animation/KataFL_RootMotionCurve.h"
#include "CoreMinimal.h"

class UAnimSequenceBase;

/** 루트 모션 커브 추출·굽기 결과와 경고를 기록한다. */
DECLARE_LOG_CATEGORY_EXTERN(LogKataRootMotionCurve, Log, All);

namespace KataFL
{
    /** 이전 값에 이어지도록 Yaw를 펼친다. Rotator의 Yaw는 ±180도에서 끊기기 때문이다. */
    double UnwrapRootMotionYaw(double RawYaw, double PreviousYaw);

    /**
     * 첫 키부터의 누적 루트 Transform을 커브 값으로 바꾼다.
     * Previous가 있으면 Yaw를 그 값에 이어 붙이고, 없으면 Rotator의 Yaw를 그대로 쓴다.
     */
    FKataRootMotionCurveValue MakeRootMotionCurveValue(const FTransform& RootFromStart, const FKataRootMotionCurveValue* Previous);

    /** 애니메이션 데이터 모델에 Kata 루트 모션 커브가 하나라도 있으면 true를 반환한다. 덮어쓰기 확인에 쓴다. */
    bool HasAnyRootMotionCurveInModel(const UAnimSequenceBase& Animation);

    /**
     * 시각과 값 목록으로 Kata 루트 모션 커브 네 개를 선형 키로 쓴다. 없는 커브는 만들고 있는 커브는 키 전체를 바꾼다.
     * 데이터 컨트롤러 브래킷 하나로 묶어 실행 취소 단위가 하나가 되게 한다. Times와 Values의 길이가 다르면 아무것도 하지 않는다.
     */
    void WriteRootMotionCurves(UAnimSequenceBase& Animation, TConstArrayView<double> Times, TConstArrayView<FKataRootMotionCurveValue> Values,
        const FText& Description);
}
