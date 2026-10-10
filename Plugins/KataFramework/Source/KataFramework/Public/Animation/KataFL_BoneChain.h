#pragma once

#include "BonePose.h"
#include "CoreMinimal.h"

namespace KataFL
{
    /**
     * 본 체인을 루트 쪽부터 차례로 같은 축으로 돌린 컴포넌트 공간 Transform을 OutBoneTransforms에 더한다.
     * 스켈레탈 컨트롤 노드가 결과를 FCSPose::LocalBlendCSBoneTransforms로 적용하는 것을 전제로 한다.
     *
     * 각 본은 자기 위치를 중심으로 AngleRadians × 가중치만큼 돈다. 자식 본은 부모의 회전을 따라가므로,
     * 가중치 합이 1이면 체인 끝 본과 그 아래 본(팔, 무기 등)은 AngleRadians 전체만큼 돈다.
     * 체인 사이에 목록에 없는 본이 있어도 부모를 따라 함께 돈다.
     *
     * @param Pose 회전 전 포즈. 읽기만 하며 바꾸지 않는다.
     * @param Bones 컴팩트 포즈 인덱스 오름차순(부모가 먼저)인 체인. Weights와 길이가 다르면 아무것도 더하지 않는다.
     * @param Axis 컴포넌트 공간의 단위 회전 축. 양의 각도의 방향은 FQuat(Axis, AngleRadians)를 따른다.
     */
    KATAFRAMEWORK_API void RotateBoneChain(FCSPose<FCompactPose>& Pose, TConstArrayView<FCompactPoseBoneIndex> Bones,
        TConstArrayView<float> Weights, const FVector& Axis, float AngleRadians, TArray<FBoneTransform>& OutBoneTransforms);
}
