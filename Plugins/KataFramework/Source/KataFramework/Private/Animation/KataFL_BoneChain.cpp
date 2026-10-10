#include "Animation/KataFL_BoneChain.h"

void KataFL::RotateBoneChain(FCSPose<FCompactPose>& Pose, TConstArrayView<FCompactPoseBoneIndex> Bones,
    TConstArrayView<float> Weights, const FVector& Axis, float AngleRadians, TArray<FBoneTransform>& OutBoneTransforms)
{
    if (Bones.Num() != Weights.Num() || FMath::IsNearlyZero(AngleRadians))
    {
        return;
    }

    // 앞선 본의 회전을 "원래 컴포넌트 공간 → 회전 후 컴포넌트 공간" 강체 변환으로 누적한다.
    // 각 본의 원래 Transform에 이 누적을 적용하면 부모 회전을 따라간 위치가 되고, 그 위치를 중심으로 자기 몫을 더 돈다.
    FTransform Accumulated = FTransform::Identity;
    for (int32 Index = 0; Index < Bones.Num(); ++Index)
    {
        const FTransform BoneTransform = Pose.GetComponentSpaceTransform(Bones[Index]);
        const FVector Pivot = Accumulated.TransformPosition(BoneTransform.GetLocation());

        // FTransform 곱은 왼쪽을 먼저 적용한다. Pivot을 원점으로 옮겨 돌린 뒤 되돌린다.
        const FTransform AroundPivot = FTransform(-Pivot) * FTransform(FQuat(Axis, AngleRadians * Weights[Index])) * FTransform(Pivot);
        Accumulated = Accumulated * AroundPivot;

        OutBoneTransforms.Add(FBoneTransform(Bones[Index], BoneTransform * Accumulated));
    }
}
