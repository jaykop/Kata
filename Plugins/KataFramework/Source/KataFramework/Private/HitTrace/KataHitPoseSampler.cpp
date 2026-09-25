#include "HitTrace/KataHitPoseSampler.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "BoneIndices.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"

namespace KataHitPoseSampler
{
    namespace
    {
        /** 재생 위치의 몽타주 첫 슬롯 트랙에서 시퀀스와 시퀀스 안의 위치를 찾는다. 여러 슬롯은 첫 슬롯만 본다. */
        const UAnimSequence* FindSequence(const UAnimMontage& Montage, float Position, float& OutPositionInSequence)
        {
            if (Montage.SlotAnimTracks.IsEmpty())
            {
                return nullptr;
            }
            const FAnimSegment* Segment = Montage.SlotAnimTracks[0].AnimTrack.GetSegmentAtTime(Position);
            if (Segment == nullptr)
            {
                return nullptr;
            }
            return Cast<UAnimSequence>(Segment->GetAnimationData(Position, OutPositionInSequence));
        }
    }

    bool GetAnimationState(const USkeletalMeshComponent& Mesh, FAnimationState& OutState)
    {
        const UAnimInstance* AnimInstance = Mesh.GetAnimInstance();
        const FAnimMontageInstance* MontageInstance = AnimInstance != nullptr ? AnimInstance->GetActiveMontageInstance() : nullptr;
        if (MontageInstance == nullptr || MontageInstance->Montage == nullptr)
        {
            return false;
        }
        OutState.Montage = MontageInstance->Montage;
        OutState.Position = MontageInstance->GetPosition();
        OutState.PlayRate = MontageInstance->GetPlayRate();
        return true;
    }

    const USkeletalMeshComponent* FindSamplingMesh(const UMeshComponent& SocketMesh, FName SocketName, FName& OutBoneName)
    {
        if (const USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(&SocketMesh))
        {
            OutBoneName = SkeletalMesh->GetSocketBoneName(SocketName);
            return SkeletalMesh->GetBoneIndex(OutBoneName) != INDEX_NONE ? SkeletalMesh : nullptr;
        }

        // 무기처럼 캐릭터 스켈레탈 메시에 직접 붙은 메시는 부착된 본의 움직임을 따른다.
        const USkeletalMeshComponent* Parent = Cast<USkeletalMeshComponent>(SocketMesh.GetAttachParent());
        if (Parent == nullptr)
        {
            return nullptr;
        }
        const FName AttachSocket = SocketMesh.GetAttachSocketName();
        OutBoneName = AttachSocket.IsNone() ? Parent->GetBoneName(0) : Parent->GetSocketBoneName(AttachSocket);
        return Parent->GetBoneIndex(OutBoneName) != INDEX_NONE ? Parent : nullptr;
    }

    bool FSampler::Initialize(const USkeletalMeshComponent& InMesh, const UAnimMontage& InMontage, float InPosition0, float InPosition1,
        TConstArrayView<FName> BoneNames, TConstArrayView<FTransform> SocketsWorld0, TConstArrayView<FTransform> SocketsWorld1,
        const FTransform& InMeshWorld0, const FTransform& InMeshWorld1)
    {
        Mesh = &InMesh;
        Montage = &InMontage;
        Position0 = InPosition0;
        Position1 = InPosition1;
        MeshWorld0 = InMeshWorld0;
        MeshWorld1 = InMeshWorld1;
        bLockRoot = InMontage.HasRootMotion();
        Chains.Reset();

        const USkeletalMesh* SkeletalMeshAsset = InMesh.GetSkeletalMeshAsset();
        if (SkeletalMeshAsset == nullptr || BoneNames.Num() != SocketsWorld0.Num() || BoneNames.Num() != SocketsWorld1.Num())
        {
            return false;
        }
        const FReferenceSkeleton& RefSkeleton = SkeletalMeshAsset->GetRefSkeleton();

        for (int32 Index = 0; Index < BoneNames.Num(); ++Index)
        {
            FSocketChain& Chain = Chains.AddDefaulted_GetRef();
            for (int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneNames[Index]); BoneIndex != INDEX_NONE; BoneIndex = RefSkeleton.GetParentIndex(BoneIndex))
            {
                Chain.MeshBones.Add(BoneIndex);
            }
            if (Chain.MeshBones.IsEmpty())
            {
                return false;
            }

            // 소켓과 부모 본 사이는 강체로 본다. 무기의 부착 오프셋도 여기에 들어간다.
            const FTransform BoneWorld1 = InMesh.GetSocketTransform(BoneNames[Index], RTS_World);
            Chain.Offset = SocketsWorld1[Index].GetRelativeTransform(BoneWorld1);

            // 양 끝에서 원본을 실제에 맞추는 차이값. 실제 = 원본 · 차이이므로 차이 = 원본⁻¹ · 실제다.
            FTransform Raw0;
            FTransform Raw1;
            if (!SampleSocket(Position0, Chain, Raw0) || !SampleSocket(Position1, Chain, Raw1))
            {
                return false;
            }
            Chain.Correction0 = Raw0.Inverse() * SocketsWorld0[Index].GetRelativeTransform(InMeshWorld0);
            Chain.Correction1 = Raw1.Inverse() * SocketsWorld1[Index].GetRelativeTransform(InMeshWorld1);
            Chain.World0 = SocketsWorld0[Index];
            Chain.World1 = SocketsWorld1[Index];
        }
        return true;
    }

    bool FSampler::InitializeFromCurrent(const USkeletalMeshComponent& InMesh, const UAnimMontage& InMontage, float InPosition0, float InPosition1,
        TConstArrayView<FName> BoneNames, TConstArrayView<FTransform> SocketsWorld1, const FTransform& InMeshWorld1)
    {
        // 직전 실제 포즈가 없으므로 현재 포즈를 자리표시로 넘기고, 시작 쪽 차이값을 현재 쪽 차이값으로 덮어쓴다.
        if (!Initialize(InMesh, InMontage, InPosition0, InPosition1, BoneNames, SocketsWorld1, SocketsWorld1, InMeshWorld1, InMeshWorld1))
        {
            return false;
        }
        for (FSocketChain& Chain : Chains)
        {
            Chain.Correction0 = Chain.Correction1;
        }
        return true;
    }

    bool FSampler::SampleSocket(float Position, const FSocketChain& Chain, FTransform& OutComponentSpace) const
    {
        float PositionInSequence = 0.0f;
        const UAnimSequence* Sequence = FindSequence(*Montage, Position, PositionInSequence);
        const USkeletalMesh* SkeletalMeshAsset = Mesh->GetSkeletalMeshAsset();
        USkeleton* Skeleton = SkeletalMeshAsset != nullptr ? const_cast<USkeletalMesh*>(SkeletalMeshAsset)->GetSkeleton() : nullptr;
        // 애디티브는 차이 포즈라 그대로 합성할 수 없고, 다른 스켈레톤의 시퀀스는 본 번호가 맞지 않는다.
        if (Sequence == nullptr || Skeleton == nullptr || Sequence->GetSkeleton() != Skeleton || Sequence->IsValidAdditive())
        {
            return false;
        }

        const TArray<FTransform>& RefPose = SkeletalMeshAsset->GetRefSkeleton().GetRefBonePose();
        // 패키징 빌드에도 있는 압축 데이터에서 추출한다. Raw 데이터는 에디터에만 있다.
        const FAnimExtractContext Context(static_cast<double>(PositionInSequence));

        FTransform ComponentSpace = FTransform::Identity;
        for (const int32 MeshBone : Chain.MeshBones)
        {
            FTransform Local = RefPose.IsValidIndex(MeshBone) ? RefPose[MeshBone] : FTransform::Identity;
            const bool bRootBone = MeshBone == 0;
            if (!(bRootBone && bLockRoot))
            {
                const int32 SkeletonBone = Skeleton->GetSkeletonBoneIndexFromMeshBoneIndex(SkeletalMeshAsset, MeshBone);
                if (SkeletonBone != INDEX_NONE)
                {
                    Sequence->GetBoneTransform(Local, FSkeletonPoseBoneIndex(SkeletonBone), Context, false);
                }
            }
            // 부모 체인을 거슬러 오르며 누적한다. 자식 로컬 · 부모 로컬 · ... · 루트 로컬 순서다.
            ComponentSpace = ComponentSpace * Local;
        }
        OutComponentSpace = Chain.Offset * ComponentSpace;
        return true;
    }

    void FSampler::SamplePose(float Alpha, TArray<FTransform, TInlineAllocator<8>>& OutSockets) const
    {
        OutSockets.SetNum(Chains.Num());
        const float Position = FMath::Lerp(Position0, Position1, Alpha);
        FTransform MeshWorld;
        MeshWorld.Blend(MeshWorld0, MeshWorld1, Alpha);

        for (int32 Index = 0; Index < Chains.Num(); ++Index)
        {
            const FSocketChain& Chain = Chains[Index];
            FTransform Raw;
            FTransform Socket;
            if (SampleSocket(Position, Chain, Raw))
            {
                FTransform Correction;
                Correction.Blend(Chain.Correction0, Chain.Correction1, Alpha);
                Socket = Raw * Correction * MeshWorld;
            }
            else
            {
                // 양 끝은 Initialize에서 확인했으므로 중간 실패는 몽타주 구간 경계 같은 드문 경우다. 그 점만 선형으로 대신한다.
                Socket.Blend(Chain.World0, Chain.World1, Alpha);
            }
            Socket.SetScale3D(FVector::OneVector);
            OutSockets[Index] = Socket;
        }
    }
}
