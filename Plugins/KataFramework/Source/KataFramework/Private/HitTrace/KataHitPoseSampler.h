#pragma once

#include "CoreMinimal.h"
#include "HitTrace/KataHitTraceTypes.h"

class UAnimMontage;
class UAnimSequence;
class UKataHitBoxComponent;
class UMeshComponent;
class USkeletalMeshComponent;

/**
 * 두 프레임 사이의 소켓 포즈를 애니메이션 원본에서 다시 샘플링한다(HT-10).
 *
 * 게임은 프레임마다 한 번 포즈를 계산하므로 두 프레임 사이 칼끝의 호를 모른다. 활성 몽타주의 재생 위치를 중간 시각으로 나눠
 * 원본 본 트랜스폼을 추출하면 호를 따라갈 수 있다. 원본에는 블렌드·IK·애디티브가 빠져 있으므로 양 끝 프레임에서
 * 실제 포즈와의 차이를 구해 중간에서 보간해 적용한다. 양 끝은 실제 포즈와 정확히 일치한다.
 */
namespace KataHitPoseSampler
{
    /** 소켓 순서대로 담은 한 시점의 소켓 월드 트랜스폼. */
    using FSocketPose = TArray<FTransform, TInlineAllocator<8>>;

    /** 한 시점의 활성 몽타주 재생 상태. */
    struct FAnimationState
    {
        const UAnimMontage* Montage = nullptr;
        float Position = 0.0f;
        float PlayRate = 1.0f;
    };

    /** 스켈레탈 메시의 활성 몽타주 상태를 읽는다. 몽타주가 없으면 false. */
    bool GetAnimationState(const USkeletalMeshComponent& Mesh, FAnimationState& OutState);

    /**
     * 소켓을 읽는 메시에서 샘플링할 스켈레탈 메시와 소켓의 부모 본을 찾는다.
     * 스스로 애니메이션되는 스켈레탈 메시면 자기 자신을 돌려준다.
     * 스스로 애니메이션되지 않는 메시(스태틱 무기, AnimInstance와 Leader Pose가 없는 스켈레탈 무기)가
     * 캐릭터 스켈레탈 메시에 직접 붙어 있으면 그 부모 메시와 부착 본을 돌려준다.
     * 찾지 못하면 nullptr이다.
     */
    const USkeletalMeshComponent* FindSamplingMesh(const UMeshComponent& SocketMesh, FName SocketName, FName& OutBoneName);

    /** 한 프레임 구간의 재샘플링기. Initialize가 실패하면 호출자는 선형 서브스텝으로 대체한다. */
    class FSampler
    {
    public:
        /**
         * @param BoneNames 소켓마다 샘플링할 부모 본.
         * @param SocketsWorld0/1 직전·현재 프레임의 실제 소켓 월드 트랜스폼.
         * @param MeshWorld0/1 직전·현재 프레임의 샘플링 메시 컴포넌트 월드 트랜스폼.
         * @return 두 재생 위치가 같은 몽타주의 연속 구간이고 원본 추출이 가능하면 true.
         */
        bool Initialize(const USkeletalMeshComponent& Mesh, const UAnimMontage& Montage, float Position0, float Position1,
            TConstArrayView<FName> BoneNames, TConstArrayView<FTransform> SocketsWorld0, TConstArrayView<FTransform> SocketsWorld1,
            const FTransform& MeshWorld0, const FTransform& MeshWorld1);

        /**
         * 직전 프레임 기록이 없을 때 현재 프레임만으로 준비한다. 직전 재생 위치 Position0의 원본 포즈에 현재 프레임의 차이값을 그대로 적용한다.
         * 메시 이동은 한 프레임 동안 없다고 본다. 구간 첫 프레임에서 잃어버릴 한 프레임 분량을 되살리는 데 쓴다.
         */
        bool InitializeFromCurrent(const USkeletalMeshComponent& Mesh, const UAnimMontage& Montage, float Position0, float Position1,
            TConstArrayView<FName> BoneNames, TConstArrayView<FTransform> SocketsWorld1, const FTransform& MeshWorld1);

        /** 두 프레임 사이 Alpha(0~1) 시점의 소켓 월드 트랜스폼. Initialize가 성공한 뒤에만 부른다. */
        void SamplePose(float Alpha, FSocketPose& OutSockets) const;

    private:
        struct FSocketChain
        {
            /** 소켓 부모 본부터 루트까지의 메시 본 번호. */
            TArray<int32, TInlineAllocator<16>> MeshBones;
            /** 부모 본 기준 소켓의 고정 오프셋. 무기 부착 위치까지 포함한다. */
            FTransform Offset;
            /** 양 끝에서 원본 포즈를 실제 포즈로 맞추는 컴포넌트 공간 차이값. */
            FTransform Correction0;
            FTransform Correction1;
            /** 중간 추출이 실패할 때 쓰는 양 끝 실제 소켓 월드 트랜스폼. */
            FTransform World0;
            FTransform World1;
        };

        /** 재생 위치 Position의 원본 포즈로 소켓의 컴포넌트 공간 트랜스폼을 만든다. */
        bool SampleSocket(float Position, const FSocketChain& Chain, FTransform& OutComponentSpace) const;

        const USkeletalMeshComponent* Mesh = nullptr;
        const UAnimMontage* Montage = nullptr;
        float Position0 = 0.0f;
        float Position1 = 0.0f;
        FTransform MeshWorld0;
        FTransform MeshWorld1;
        /** 루트 모션 몽타주는 루트 본을 레퍼런스 포즈로 고정한다. 액터 이동은 메시 월드 트랜스폼 보간이 맡는다. */
        bool bLockRoot = false;
        TArray<FSocketChain, TInlineAllocator<8>> Chains;
    };

    /** FFramePath::Build에 넘기는 이번 프레임의 입력. 포인터는 Build 호출 동안만 쓴다. */
    struct FFramePathInput
    {
        /** 소켓을 읽는 메시. nullptr이면 Build는 빈 경로를 만든다. */
        const UMeshComponent* Mesh = nullptr;
        TConstArrayView<FName> SocketNames;
        /** 구간 첫 프레임의 직전 포즈를 제공한다. 없으면 재샘플링으로 한 프레임을 되짚거나 직전 포즈 없이 시작한다. */
        const UKataHitBoxComponent* HitBoxComponent = nullptr;
        /** UKataHitSubsystem의 현재 Tick 번호. HitBoxComponent의 기록이 바로 앞 Tick의 것인지 가린다. */
        uint64 TickIndex = 0;
        /** 이번 프레임의 Kata 시각과 Tick DeltaTime. */
        float CurrentTime = 0.0f;
        float DeltaTime = 0.0f;
        /** Kata 시각이 진행하는 배율. 직전 Kata 시각을 되짚을 때 쓴다. */
        float KataTimeDilation = 1.0f;
        /** 몽타주 재생 위치가 진행하는 배율. 재생 위치의 연속성 판정과 되짚기에 쓴다. */
        float AnimationTimeDilation = 1.0f;
        /** false면 재샘플링하지 않고 두 프레임 사이를 선형으로 보간한다. */
        bool bSampleAnimation = true;
    };

    /**
     * 한 프레임 구간 [직전 시각, 현재 시각]의 소켓 경로.
     *
     * 직전 포즈는 History, 없으면 HitBoxComponent의 직전 프레임 기록, 그것도 없으면 몽타주 재생 위치를 한 프레임 되짚은 원본 포즈에서 얻는다.
     * 활성 몽타주의 재생 위치가 두 프레임 사이에서 이어지면 중간 포즈를 재샘플링하고, 아니면 선형으로 보간한다.
     * Build는 History를 읽기만 한다. 경로를 다 쓴 뒤 Commit으로 이번 프레임을 History에 남긴다.
     */
    class FFramePath
    {
    public:
        void Build(const FFramePathInput& Input, const FKataSocketPathHistory& History);

        /** 이번 프레임의 현재 포즈와 몽타주 상태를 다음 프레임의 직전 기록으로 남긴다. */
        void Commit(FKataSocketPathHistory& History) const;

        /** Kata 시각 Time을 이 구간의 Alpha(0~1)로 바꾼다. 구간 길이가 0이면 1이다. */
        float AlphaAt(float Time) const;

        /** Alpha 시점의 소켓 월드 트랜스폼. 재샘플링할 수 있으면 애니메이션의 호를, 아니면 직선을 따른다. */
        void PoseAt(float Alpha, FSocketPose& OutPose) const;

        const FSocketPose& GetCurrentPose() const { return CurrentPose; }
        float GetPreviousTime() const { return PreviousTime; }
        float GetCurrentTime() const { return CurrentTime; }
        float GetFrameSpan() const { return CurrentTime - PreviousTime; }
        /** 직전 포즈가 실제 기록이나 재샘플링에서 왔으면 true. false면 직전 포즈는 현재 포즈의 복사본이고 구간 길이는 0이다. */
        bool HasRealPreviousPose() const { return bHasRealPreviousPose; }
        bool IsSampled() const { return bSampled; }
        /** 진단 로그용 현재 몽타주 재생 위치. 몽타주가 없으면 -1이다. */
        float GetMontagePosition() const { return bHasCurrentAnimation ? CurrentAnimation.Position : -1.0f; }

    private:
        FSocketPose CurrentPose;
        FSocketPose PreviousPose;
        float PreviousTime = 0.0f;
        float CurrentTime = 0.0f;
        bool bHasRealPreviousPose = false;
        bool bSampled = false;

        FSampler Sampler;
        const USkeletalMeshComponent* SamplingMesh = nullptr;
        FAnimationState CurrentAnimation;
        bool bHasCurrentAnimation = false;
    };
}
