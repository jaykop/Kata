#include "HitTrace/KataHitPoseSampler.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "BoneIndices.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "HitTrace/KataHitBoxComponent.h"
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

        /** 두 소켓 트랜스폼 사이를 위치는 Lerp, 회전은 Slerp로 보간한다. 경로에는 스케일을 쓰지 않는다. */
        FTransform BlendTransform(const FTransform& From, const FTransform& To, float Alpha)
        {
            return FTransform(
                FQuat::Slerp(From.GetRotation(), To.GetRotation(), Alpha).GetNormalized(),
                FMath::Lerp(From.GetLocation(), To.GetLocation(), Alpha));
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
        // AnimInstance도 Leader Pose도 없는 스켈레탈 메시는 본이 레퍼런스 포즈에 머문다. 이런 무기는 스태틱 무기처럼
        // 부착된 부모 본을 따라 강체로 움직이므로 부모의 몽타주를 샘플링한다. 자기 자신을 고르면 몽타주가 없어 재샘플링이 꺼진다.
        const USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(&SocketMesh);
        const bool bAnimatesItself = SkeletalMesh != nullptr
            && (SkeletalMesh->GetAnimInstance() != nullptr || SkeletalMesh->LeaderPoseComponent.IsValid());

        // 무기처럼 캐릭터 스켈레탈 메시에 직접 붙은 메시는 부착된 본의 움직임을 따른다.
        const USkeletalMeshComponent* Parent = bAnimatesItself ? nullptr : Cast<USkeletalMeshComponent>(SocketMesh.GetAttachParent());
        if (Parent != nullptr)
        {
            const FName AttachSocket = SocketMesh.GetAttachSocketName();
            OutBoneName = AttachSocket.IsNone() ? Parent->GetBoneName(0) : Parent->GetSocketBoneName(AttachSocket);
            return Parent->GetBoneIndex(OutBoneName) != INDEX_NONE ? Parent : nullptr;
        }

        if (SkeletalMesh != nullptr)
        {
            // 스스로 애니메이션되거나 스켈레탈 부모가 없으면 자기 자신을 샘플링한다. 몽타주가 없으면 호출자가 선형 보간으로 대신한다.
            OutBoneName = SkeletalMesh->GetSocketBoneName(SocketName);
            return SkeletalMesh->GetBoneIndex(OutBoneName) != INDEX_NONE ? SkeletalMesh : nullptr;
        }
        return nullptr;
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

    void FSampler::SamplePose(float Alpha, FSocketPose& OutSockets) const
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

    void FFramePath::Build(const FFramePathInput& Input, const FKataSocketPathHistory& History)
    {
        const int32 SocketCount = Input.SocketNames.Num();
        CurrentTime = Input.CurrentTime;
        PreviousTime = Input.CurrentTime;
        bHasRealPreviousPose = false;
        bSampled = false;
        SamplingMesh = nullptr;
        CurrentAnimation = FAnimationState();
        bHasCurrentAnimation = false;
        CurrentPose.Reset();
        PreviousPose.Reset();
        if (Input.Mesh == nullptr)
        {
            return;
        }
        const UMeshComponent& Mesh = *Input.Mesh;

        CurrentPose.SetNum(SocketCount);
        for (int32 Index = 0; Index < SocketCount; ++Index)
        {
            CurrentPose[Index] = Mesh.GetSocketTransform(Input.SocketNames[Index], RTS_World);
        }

        // 루프 경계에서 Kata 시각이 되돌아가면 이전 기록은 다른 회차의 것이다.
        const bool bUseHistory = History.bHasPreviousPose
            && CurrentTime >= History.PreviousTime && History.PreviousSockets.Num() == SocketCount;

        PreviousPose = CurrentPose;
        if (bUseHistory)
        {
            PreviousPose = FSocketPose(History.PreviousSockets);
            PreviousTime = History.PreviousTime;
            bHasRealPreviousPose = true;
        }
        else if (Input.HitBoxComponent != nullptr)
        {
            // 첫 프레임: 구간이 이 프레임 안에서 시작했으므로 직전 프레임 포즈가 있어야 시작 시각부터 쓸 수 있다.
            FSocketPose ComponentPose;
            ComponentPose.SetNum(SocketCount);
            bool bHasAll = true;
            for (int32 Index = 0; Index < SocketCount && bHasAll; ++Index)
            {
                bHasAll = Input.HitBoxComponent->GetPreviousSocketTransform(&Mesh, Input.SocketNames[Index], Input.TickIndex, ComponentPose[Index]);
            }
            if (bHasAll)
            {
                bHasRealPreviousPose = true;
                PreviousPose = MoveTemp(ComponentPose);
                // Kata 시각은 실행 기준 액터의 시간 배율이 적용된 DeltaTime으로 진행하므로 직전 프레임 시각도 같은 배율로 되짚는다.
                PreviousTime = CurrentTime - Input.DeltaTime * Input.KataTimeDilation;
            }
        }

        // 프레임 사이 포즈 재샘플링(HT-10). 모든 소켓이 같은 스켈레탈 메시를 따를 때만 쓴다.
        FName SamplingBone;
        TArray<FName, TInlineAllocator<8>> SamplingBones;
        for (int32 Index = 0; Index < SocketCount; ++Index)
        {
            const USkeletalMeshComponent* SocketSamplingMesh = FindSamplingMesh(Mesh, Input.SocketNames[Index], SamplingBone);
            if (SocketSamplingMesh == nullptr || (SamplingMesh != nullptr && SocketSamplingMesh != SamplingMesh))
            {
                SamplingMesh = nullptr;
                break;
            }
            SamplingMesh = SocketSamplingMesh;
            SamplingBones.Add(SamplingBone);
        }
        bHasCurrentAnimation = SamplingMesh != nullptr && GetAnimationState(*SamplingMesh, CurrentAnimation);

        if (Input.bSampleAnimation && bHasCurrentAnimation && bHasRealPreviousPose && CurrentTime - PreviousTime > UE_KINDA_SMALL_NUMBER)
        {
            const UAnimMontage* PreviousMontage = nullptr;
            float PreviousPosition = 0.0f;
            FTransform PreviousMeshWorld;
            bool bHasPreviousAnimation = false;
            if (bUseHistory)
            {
                PreviousMontage = History.PreviousMontage.Get();
                PreviousPosition = History.PreviousMontagePosition;
                PreviousMeshWorld = History.PreviousSamplingMeshTransform;
                bHasPreviousAnimation = PreviousMontage != nullptr;
            }
            else if (Input.HitBoxComponent != nullptr)
            {
                bHasPreviousAnimation = Input.HitBoxComponent->GetPreviousAnimationState(SamplingMesh, Input.TickIndex, PreviousMontage, PreviousPosition, PreviousMeshWorld);
            }

            // 섹션 이동·루프·몽타주 교체로 재생 위치가 끊긴 프레임은 중간 포즈를 이을 수 없다. 예상 진행량과 크게 다르면 선형으로 대신한다.
            const float Progress = CurrentAnimation.Position - PreviousPosition;
            const float ExpectedProgress = CurrentAnimation.PlayRate * Input.DeltaTime * Input.AnimationTimeDilation;
            const bool bContinuous = bHasPreviousAnimation && PreviousMontage == CurrentAnimation.Montage && Progress >= 0.0f
                && FMath::Abs(Progress - ExpectedProgress) <= FMath::Max(0.05f, FMath::Abs(ExpectedProgress) * 0.5f);
            if (bContinuous)
            {
                bSampled = Sampler.Initialize(*SamplingMesh, *CurrentAnimation.Montage, PreviousPosition, CurrentAnimation.Position,
                    SamplingBones, PreviousPose, CurrentPose, PreviousMeshWorld, SamplingMesh->GetComponentTransform());
            }
        }
        else if (Input.bSampleAnimation && bHasCurrentAnimation && !bHasRealPreviousPose)
        {
            // 구간 첫 Tick에 직전 포즈 기록이 없으면(프리뷰처럼 엔진 직전 본 버퍼를 믿을 수 없는 경우) 한 프레임 분량을 통째로 잃는다.
            // 저프레임에서는 그 양이 판정 창의 절반을 넘을 수 있으므로, 재생 위치를 한 프레임 되짚어 원본 포즈로 직전 포즈를 만든다.
            // 재생 위치는 몽타주의 배율로, Kata 시각은 실행 기준 액터의 배율로 되짚는다. 두 액터가 다르면 배율도 다를 수 있다.
            const float AnimationFrameDelta = Input.DeltaTime * Input.AnimationTimeDilation;
            const float StartPosition = CurrentAnimation.Position - CurrentAnimation.PlayRate * AnimationFrameDelta;
            if (AnimationFrameDelta > UE_KINDA_SMALL_NUMBER && StartPosition >= 0.0f
                && Sampler.InitializeFromCurrent(*SamplingMesh, *CurrentAnimation.Montage, StartPosition, CurrentAnimation.Position,
                    SamplingBones, CurrentPose, SamplingMesh->GetComponentTransform()))
            {
                bSampled = true;
                bHasRealPreviousPose = true;
                Sampler.SamplePose(0.0f, PreviousPose);
                PreviousTime = CurrentTime - Input.DeltaTime * Input.KataTimeDilation;
            }
        }
    }

    void FFramePath::Commit(FKataSocketPathHistory& History) const
    {
        History.PreviousSockets = TArray<FTransform>(CurrentPose);
        History.PreviousTime = CurrentTime;
        History.bHasPreviousPose = true;
        History.PreviousMontage = bHasCurrentAnimation ? CurrentAnimation.Montage : nullptr;
        History.PreviousMontagePosition = bHasCurrentAnimation ? CurrentAnimation.Position : 0.0f;
        History.PreviousSamplingMeshTransform = SamplingMesh != nullptr ? SamplingMesh->GetComponentTransform() : FTransform::Identity;
    }

    float FFramePath::AlphaAt(float Time) const
    {
        const float FrameSpan = GetFrameSpan();
        return FrameSpan > UE_KINDA_SMALL_NUMBER ? FMath::Clamp((Time - PreviousTime) / FrameSpan, 0.0f, 1.0f) : 1.0f;
    }

    void FFramePath::PoseAt(float Alpha, FSocketPose& OutPose) const
    {
        if (bSampled)
        {
            Sampler.SamplePose(Alpha, OutPose);
            return;
        }
        OutPose.SetNum(CurrentPose.Num());
        for (int32 Index = 0; Index < CurrentPose.Num(); ++Index)
        {
            OutPose[Index] = BlendTransform(PreviousPose[Index], CurrentPose[Index], Alpha);
        }
    }
}
