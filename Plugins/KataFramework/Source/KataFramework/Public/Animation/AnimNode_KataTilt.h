#pragma once

#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "CoreMinimal.h"
#include "AnimNode_KataTilt.generated.h"

/**
 * 대상 방향 Tilt를 포즈에 더하는 스켈레탈 컨트롤 노드.
 *
 * 원래 포즈 위에 각도 차이만 더한다. 소유 Anim Instance(Linked Layer 안이면 메시의 메인 인스턴스)가 UKataAnimInstance면
 * 그 Class Defaults의 TiltBoneChain을 읽어 본 이름으로 체인을 찾는다. Template ABP에는 스켈레톤이 없으므로 노드에 본을 지정하지 않는다.
 * 체인을 액터의 오른쪽 축으로 Pitch만큼 나눠 돌리며 위쪽이 양수다. 축이 액터 기준이라 메시의 회전 오프셋과 무관하다.
 *
 * Template ABP의 Slot 뒤에 두고 Pitch 핀은 Tilt Pitch, Alpha 핀은 Tilt Alpha에 바인딩한다.
 * 체인이 비었거나 현재 LOD에서 체인 본을 하나도 찾지 못하면 입력 포즈를 그대로 내보낸다.
 */
USTRUCT(BlueprintInternalUseOnly)
struct KATAFRAMEWORK_API FAnimNode_KataTilt : public FAnimNode_SkeletalControlBase
{
    GENERATED_BODY()

    /** 체인에 더할 Pitch(도, 위쪽 양수). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tilt", meta = (PinShownByDefault))
    float Pitch = 0.0f;

    //~ Begin FAnimNode_Base Interface
    virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    virtual void GatherDebugData(FNodeDebugData& DebugData) override;
    //~ End FAnimNode_Base Interface

protected:
    //~ Begin FAnimNode_SkeletalControlBase Interface
    virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
    virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
    virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
    //~ End FAnimNode_SkeletalControlBase Interface

private:
    // 초기화 때 Anim Instance의 Class Defaults에서 복사한 체인 설정. 실행 중에는 바뀌지 않는다.
    TArray<FName> ConfiguredBoneNames;
    TArray<float> ConfiguredBoneWeights;

    /** 현재 필요한 본 목록에서 찾은 체인. 컴팩트 포즈 인덱스 오름차순이다. */
    TArray<FCompactPoseBoneIndex> ChainBones;

    /** ChainBones와 같은 순서의 가중치. 찾은 본끼리 합이 1이 되도록 정규화해, 일부 본이 LOD에서 빠져도 체인 끝은 Pitch 전체만큼 돈다. */
    TArray<float> ChainWeights;
};
