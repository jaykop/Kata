#include "Animation/KataRootMotionCurveBake.h"

#include "Animation/AnimCompositeBase.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimationAsset.h"
#include "Animation/KataFL_RootMotionCurveEditor.h"
#include "ContentBrowserMenuContexts.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Misc/MessageDialog.h"
#include "ScopedTransaction.h"
#include "ToolMenus.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "KataRootMotionCurveBake"

namespace
{
    /** 몽타주 구성 시퀀스의 프레임 레이트를 모를 때 쓰는 최소 초당 키 수. */
    constexpr double MinKeysPerSecond = 30.0;

    /** 세그먼트가 컴포지트를 가리키면 샘플링 레이트가 매우 높게 나오므로 키 수에 상한을 둔다. */
    constexpr double MaxKeysPerSecond = 240.0;

    /** 같은 시각으로 볼 키 간격(초). */
    constexpr double KeyTimeTolerance = 1.e-4;

    /** 커브와 원본의 키 사이 위치 차이가 이 값을 넘으면 경고한다(cm). 수정자의 기본 허용치와 같다. */
    constexpr double PositionWarningTolerance = 0.5;

    /**
     * 굽기 키 시각을 만든다. 세그먼트 경계에서는 이동의 기울기가 바뀌므로 경계를 키로 넣고,
     * 나머지는 구성 시퀀스 중 가장 높은 프레임 레이트(세그먼트 재생 속도 반영)로 고르게 나눈다.
     */
    TArray<double> MakeBakeTimes(const UAnimMontage& Montage)
    {
        const double Length = Montage.GetPlayLength();
        double KeysPerSecond = MinKeysPerSecond;

        TArray<double> Times;
        for (const FAnimSegment& Segment : Montage.SlotAnimTracks[0].AnimTrack.AnimSegments)
        {
            if (const UAnimSequenceBase* Reference = Segment.GetAnimReference())
            {
                KeysPerSecond = FMath::Max(KeysPerSecond, Reference->GetSamplingFrameRate().AsDecimal() * FMath::Abs(Segment.AnimPlayRate));
            }
            Times.Add(Segment.StartPos);
            Times.Add(Segment.GetEndPos());
        }
        KeysPerSecond = FMath::Min(KeysPerSecond, MaxKeysPerSecond);

        const int32 NumIntervals = FMath::Max(1, FMath::CeilToInt32(Length * KeysPerSecond - UE_KINDA_SMALL_NUMBER));
        for (int32 Index = 0; Index <= NumIntervals; ++Index)
        {
            Times.Add(Length * Index / NumIntervals);
        }

        for (double& Time : Times)
        {
            Time = FMath::Clamp(Time, 0.0, Length);
        }
        Times.Sort();

        TArray<double> UniqueTimes;
        UniqueTimes.Reserve(Times.Num());
        for (const double Time : Times)
        {
            if (UniqueTimes.IsEmpty() || Time - UniqueTimes.Last() > KeyTimeTolerance)
            {
                UniqueTimes.Add(Time);
            }
        }
        return UniqueTimes;
    }

    /** 몽타주 구간의 현재 루트 모션. 몽타주 커브는 굽기 대상이므로 읽지 않는다. */
    FTransform ExtractCurrentMotion(const UAnimMontage& Montage, double Start, double End, bool& bOutUsedSequenceCurve)
    {
        return KataFL::ExtractMontageRootMotion(Montage, static_cast<float>(Start), static_cast<float>(End), false, bOutUsedSequenceCurve);
    }

    void ShowBakeNotification(int32 NumBaked, int32 NumSkipped)
    {
        FNotificationInfo Info(FText::Format(
            LOCTEXT("BakeResult", "Baked Kata root motion curves for {0} montage(s), skipped {1}. See LogKataRootMotionCurve for details."),
            NumBaked, NumSkipped));
        Info.ExpireDuration = 5.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
    }

    void ExecuteBake(const TArray<FAssetData> MontageAssets)
    {
        TArray<UAnimMontage*> Montages;
        bool bAnyExistingCurve = false;
        for (const FAssetData& AssetData : MontageAssets)
        {
            if (UAnimMontage* Montage = Cast<UAnimMontage>(AssetData.GetAsset()))
            {
                Montages.Add(Montage);
                bAnyExistingCurve |= KataFL::HasAnyRootMotionCurveInModel(*Montage);
            }
        }
        if (Montages.IsEmpty())
        {
            return;
        }

        if (bAnyExistingCurve)
        {
            // 굽기는 사용자가 명시적으로 실행하는 재추출이므로 덮어쓰지만, 편집한 커브가 사라지는 것은 미리 알린다.
            const EAppReturnType::Type Answer = FMessageDialog::Open(EAppMsgType::YesNo,
                LOCTEXT("ConfirmOverwrite", "Some selected montages already have Kata root motion curves. Baking overwrites them, including any edits. Continue?"),
                LOCTEXT("ConfirmOverwriteTitle", "Bake Kata Root Motion Curves"));
            if (Answer != EAppReturnType::Yes)
            {
                return;
            }
        }

        const FScopedTransaction Transaction(LOCTEXT("BakeTransaction", "Bake Kata Root Motion Curves"));
        int32 NumBaked = 0;
        for (UAnimMontage* Montage : Montages)
        {
            NumBaked += KataFL::BakeRootMotionCurvesToMontage(*Montage) ? 1 : 0;
        }
        ShowBakeNotification(NumBaked, Montages.Num() - NumBaked);
    }
}

bool KataFL::BakeRootMotionCurvesToMontage(UAnimMontage& Montage)
{
    if (Montage.SlotAnimTracks.IsEmpty() || !Montage.HasRootMotion())
    {
        UE_LOG(LogKataRootMotionCurve, Warning, TEXT("Kata root motion curve: montage '%s' has no root motion; nothing was baked"), *Montage.GetName());
        return false;
    }

    const TArray<double> Times = MakeBakeTimes(Montage);
    if (Times.Num() < 2)
    {
        UE_LOG(LogKataRootMotionCurve, Warning, TEXT("Kata root motion curve: montage '%s' has no play length; nothing was baked"), *Montage.GetName());
        return false;
    }

    // 키마다 0초부터의 누적 루트 Transform을 만든다. 실행 시 구간 변화량을 누적하는 순서와 같다.
    TArray<FTransform> RootFromStart;
    TArray<FKataRootMotionCurveValue> Values;
    RootFromStart.Reserve(Times.Num());
    Values.Reserve(Times.Num());
    RootFromStart.Add(FTransform::Identity);
    Values.Add(FKataRootMotionCurveValue());

    bool bUsedSequenceCurve = false;
    for (int32 Index = 1; Index < Times.Num(); ++Index)
    {
        FRootMotionMovementParams Root;
        Root.Set(RootFromStart.Last());
        Root.Accumulate(ExtractCurrentMotion(Montage, Times[Index - 1], Times[Index], bUsedSequenceCurve));
        RootFromStart.Add(Root.GetRootMotionTransform());
        Values.Add(KataFL::MakeRootMotionCurveValue(RootFromStart.Last(), &Values.Last()));
    }

    // 키 사이 중간 시각에서 선형 보간 결과와 실제 이동을 비교한다. 키 시각에서는 정의상 같다.
    double MaxPositionError = 0.0;
    for (int32 Index = 1; Index < Times.Num(); ++Index)
    {
        const double MidTime = (Times[Index - 1] + Times[Index]) * 0.5;
        FRootMotionMovementParams Root;
        Root.Set(RootFromStart[Index - 1]);
        bool bUnused = false;
        Root.Accumulate(ExtractCurrentMotion(Montage, Times[Index - 1], MidTime, bUnused));

        const FVector CurveTranslation = FMath::Lerp(Values[Index - 1].Translation, Values[Index].Translation, 0.5);
        MaxPositionError = FMath::Max(MaxPositionError, FVector::Dist(Root.GetRootMotionTransform().GetTranslation(), CurveTranslation));
    }

    KataFL::WriteRootMotionCurves(Montage, Times, Values, LOCTEXT("BakeRootMotionCurves", "Bake Kata Root Motion Curves"));

    const FKataRootMotionCurveValue& End = Values.Last();
    UE_LOG(LogKataRootMotionCurve, Display,
        TEXT("Kata root motion curve: baked montage '%s' with %d keys from %s (end translation %s, end yaw %.2f, max interval error %.3f cm)"),
        *Montage.GetName(), Values.Num(), bUsedSequenceCurve ? TEXT("sequence curves and root motion") : TEXT("original root motion"),
        *End.Translation.ToCompactString(), End.Yaw, MaxPositionError);

    if (MaxPositionError > PositionWarningTolerance)
    {
        UE_LOG(LogKataRootMotionCurve, Warning,
            TEXT("Kata root motion curve: baked curves of '%s' differ from the current root motion between keys by up to %.3f cm"),
            *Montage.GetName(), MaxPositionError);
    }
    return true;
}

void KataFL::RegisterRootMotionCurveBakeMenu()
{
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.AnimMontage");
    if (Menu == nullptr)
    {
        return;
    }

    FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
    Section.AddDynamicEntry("KataBakeRootMotionCurves", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
    {
        const UContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UContentBrowserAssetContextMenuContext>();
        if (Context == nullptr)
        {
            return;
        }

        TArray<FAssetData> MontageAssets;
        for (const FAssetData& AssetData : Context->SelectedAssets)
        {
            if (AssetData.IsInstanceOf(UAnimMontage::StaticClass()))
            {
                MontageAssets.Add(AssetData);
            }
        }
        if (MontageAssets.IsEmpty())
        {
            return;
        }

        InSection.AddMenuEntry(
            "KataBakeRootMotionCurves",
            LOCTEXT("BakeMenuLabel", "Bake Kata Root Motion Curves"),
            LOCTEXT("BakeMenuTooltip", "Bakes the montage's current root motion (sequence curves or original root motion) into Kata.RootMotion curves on the montage. Existing montage curves are overwritten."),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([MontageAssets]()
            {
                ExecuteBake(MontageAssets);
            })));
    }));
}

#undef LOCTEXT_NAMESPACE
