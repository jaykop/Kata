#include "Animation/AnimNode_KataTilt.h"

#include "Animation/AnimInstanceProxy.h"
#include "Animation/KataAnimInstance.h"
#include "Animation/KataFL_BoneChain.h"
#include "Components/SkeletalMeshComponent.h"

void FAnimNode_KataTilt::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
    FAnimNode_SkeletalControlBase::Initialize_AnyThread(Context);

    ConfiguredBoneNames.Reset();
    ConfiguredBoneWeights.Reset();

    // 체인 설정은 Class Defaults 값이라 실행 중 바뀌지 않으므로 초기화 때 한 번만 복사한다.
    const UKataAnimInstance* KataInstance = Cast<UKataAnimInstance>(Context.AnimInstanceProxy->GetAnimInstanceObject());
    if (KataInstance == nullptr)
    {
        // Linked Anim Layer 안에 둔 노드는 레이어 인스턴스를 받으므로 메시의 메인 인스턴스에서 찾는다.
        const USkeletalMeshComponent* Mesh = Context.AnimInstanceProxy->GetSkelMeshComponent();
        KataInstance = Mesh != nullptr ? Cast<UKataAnimInstance>(Mesh->GetAnimInstance()) : nullptr;
    }
    if (KataInstance == nullptr)
    {
        return;
    }

    for (const FKataTiltBone& Bone : KataInstance->TiltBoneChain)
    {
        ConfiguredBoneNames.Add(Bone.BoneName);
        ConfiguredBoneWeights.Add(Bone.Weight);
    }
}

void FAnimNode_KataTilt::GatherDebugData(FNodeDebugData& DebugData)
{
    FString DebugLine = DebugData.GetNodeName(this);
    DebugLine += TEXT("(");
    AddDebugNodeData(DebugLine);
    DebugLine += FString::Printf(TEXT(" Pitch: %.1f, Bones: %d)"), Pitch, ChainBones.Num());
    DebugData.AddDebugItem(DebugLine);

    ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_KataTilt::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
    // Proxy가 게임 스레드에서 캐시한 Transform만 읽으므로 워커 스레드에서도 안전하다.
    const FTransform& ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
    const FTransform& ActorTransform = Output.AnimInstanceProxy->GetActorTransform();
    const FVector RightAxis = ComponentTransform.InverseTransformVectorNoScale(ActorTransform.GetUnitAxis(EAxis::Y)).GetSafeNormal();
    if (RightAxis.IsNearlyZero())
    {
        return;
    }

    // UE 쿼터니언은 오른쪽(+Y) 축으로 양의 각도만큼 돌리면 전방이 아래로 숙여진다(FRotator의 양의 Pitch와 반대).
    // 위쪽이 양수인 Pitch를 그대로 쓰기 위해 각도의 부호를 뒤집는다.
    KataFL::RotateBoneChain(Output.Pose, ChainBones, ChainWeights, RightAxis, -FMath::DegreesToRadians(Pitch), OutBoneTransforms);
}

bool FAnimNode_KataTilt::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
    return !ChainBones.IsEmpty();
}

void FAnimNode_KataTilt::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
    ChainBones.Reset();
    ChainWeights.Reset();

    TArray<TPair<FCompactPoseBoneIndex, float>, TInlineAllocator<8>> Found;
    float TotalWeight = 0.0f;
    for (int32 Index = 0; Index < ConfiguredBoneNames.Num(); ++Index)
    {
        const float Weight = ConfiguredBoneWeights[Index];
        FBoneReference Reference(ConfiguredBoneNames[Index]);
        if (!(Weight > 0.0f) || !Reference.Initialize(RequiredBones))
        {
            continue;
        }

        // 현재 LOD에서 빠진 본은 건너뛰고 남은 본으로 회전을 나눈다.
        const FCompactPoseBoneIndex CompactIndex = Reference.GetCompactPoseIndex(RequiredBones);
        if (CompactIndex.IsValid())
        {
            Found.Emplace(CompactIndex, Weight);
            TotalWeight += Weight;
        }
    }
    if (!(TotalWeight > 0.0f))
    {
        return;
    }

    // 부모가 먼저 오도록 정렬한다. 컴팩트 포즈 인덱스는 부모가 자식보다 작다.
    Found.Sort([](const TPair<FCompactPoseBoneIndex, float>& A, const TPair<FCompactPoseBoneIndex, float>& B)
    {
        return A.Key.GetInt() < B.Key.GetInt();
    });
    for (const TPair<FCompactPoseBoneIndex, float>& Entry : Found)
    {
        ChainBones.Add(Entry.Key);
        ChainWeights.Add(Entry.Value / TotalWeight);
    }
}
