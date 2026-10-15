#include "HitTrace/KataHitSubsystem.h"

#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HitTrace/KataHitBoxComponent.h"
#include "HitTrace/KataHitPoseSampler.h"
#include "HitTrace/KataTrailParameters.h"
#include "KataFrameworkLog.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFloat.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraDataInterfaceArrayInt.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraSystemInstanceController.h"
#include "NiagaraTypes.h"
#include "Runtime/KataActionInstance.h"
#include "Runtime/KataTaskInstance.h"
#include "Tasks/KataTask_Trail.h"

namespace KataTrail
{
    using KataHitPoseSampler::FSocketPose;

    /**
     * 에셋 User 파라미터의 float 값을 읽는다. 없거나 0 이하면 Default를 쓰고, 1 미만은 1로 올린다.
     * 파라미터를 새로 추가하면 기본값이 0이므로, 0을 1로 올리면 매 프레임 샘플 상한까지 만들게 된다.
     */
    float ReadSampleSpacing(const UNiagaraSystem& System, FName Name, float Default)
    {
        const FNiagaraVariableBase Variable(FNiagaraTypeDefinition::GetFloatDef(), KataTrailParameters::MakeUserName(Name));
        const float Value = System.GetExposedParameters().GetParameterValueOrDefault<float>(Variable, Default);
        return Value > 0.0f ? FMath::Max(Value, 1.0f) : Default;
    }

    /**
     * 구간 [StartAlpha, EndAlpha] 동안 소켓이 움직인 최대 거리와 칼날 방향이 돈 각도를 잰다.
     * 재샘플링한 경로는 호이므로 여러 지점을 이어 누적한다. 선형 경로는 양 끝만 봐도 같다.
     */
    void MeasurePath(const KataHitPoseSampler::FFramePath& Path, float StartAlpha, float EndAlpha,
        double& OutMaxDistance, double& OutAngleDegrees)
    {
        const int32 ProbeCount = Path.IsSampled() ? 8 : 1;
        const int32 SocketCount = Path.GetCurrentPose().Num();
        TArray<double, TInlineAllocator<8>> SocketDistances;
        SocketDistances.SetNumZeroed(SocketCount);
        OutAngleDegrees = 0.0;

        FSocketPose ProbeFrom;
        Path.PoseAt(StartAlpha, ProbeFrom);
        for (int32 Probe = 1; Probe <= ProbeCount; ++Probe)
        {
            FSocketPose ProbeTo;
            Path.PoseAt(FMath::Lerp(StartAlpha, EndAlpha, static_cast<float>(Probe) / static_cast<float>(ProbeCount)), ProbeTo);
            for (int32 Index = 0; Index < SocketCount; ++Index)
            {
                SocketDistances[Index] += FVector::Dist(ProbeFrom[Index].GetLocation(), ProbeTo[Index].GetLocation());
            }
            // 칼날의 회전은 소켓 자체의 회전이 아니라 첫 소켓에서 끝 소켓으로 향하는 방향의 변화다.
            const FVector FromDirection = (ProbeFrom.Last().GetLocation() - ProbeFrom[0].GetLocation()).GetSafeNormal();
            const FVector ToDirection = (ProbeTo.Last().GetLocation() - ProbeTo[0].GetLocation()).GetSafeNormal();
            OutAngleDegrees += FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(FromDirection, ToDirection), -1.0, 1.0)));
            ProbeFrom = MoveTemp(ProbeTo);
        }

        OutMaxDistance = 0.0;
        for (const double SocketDistance : SocketDistances)
        {
            OutMaxDistance = FMath::Max(OutMaxDistance, SocketDistance);
        }
    }

    /** 이번 프레임에 새로 만들 입자들. 모든 배열의 길이가 같다. */
    struct FParticleBatch
    {
        TArray<FVector> Positions;
        TArray<float> Widths;
        TArray<FVector> Sides;
        TArray<int32> RibbonIds;
        TArray<float> LinkOrders;
        TArray<float> Ages;
        TArray<FVector2D> BladeRanges;

        /** 한 샘플 포즈를 이웃한 소켓 구간마다 입자 하나로 바꿔 붙인다. */
        void AppendSample(const FSocketPose& Pose, float LinkOrder, float Age)
        {
            double BladeLength = 0.0;
            for (int32 Index = 0; Index + 1 < Pose.Num(); ++Index)
            {
                BladeLength += FVector::Dist(Pose[Index].GetLocation(), Pose[Index + 1].GetLocation());
            }

            double Covered = 0.0;
            for (int32 Index = 0; Index + 1 < Pose.Num(); ++Index)
            {
                const FVector A = Pose[Index].GetLocation();
                const FVector B = Pose[Index + 1].GetLocation();
                const FVector Segment = B - A;
                const double Length = Segment.Size();

                Positions.Add((A + B) * 0.5);
                Widths.Add(static_cast<float>(Length));
                Sides.Add(Length > UE_KINDA_SMALL_NUMBER ? Segment / Length : FVector::ZeroVector);
                RibbonIds.Add(Index);
                LinkOrders.Add(LinkOrder);
                Ages.Add(Age);
                BladeRanges.Add(BladeLength > UE_KINDA_SMALL_NUMBER
                    ? FVector2D(Covered / BladeLength, (Covered + Length) / BladeLength)
                    : FVector2D(0.0, 1.0));
                Covered += Length;
            }
        }

        int32 Num() const { return Positions.Num(); }
    };

    /**
     * 배열 User 파라미터에 이번 프레임 값을 쓴다.
     * 게임 월드에서는 UNiagaraDataInterfaceArrayFunctionLibrary의 함수(LibrarySetter)를 쓴다. 에디터 빌드의 이 함수들은 월드가 게임 월드가
     * 아니면(액션 편집기 프리뷰) 호출마다 DI를 복제해 편집용 오버라이드로 저장하고 시스템을 다시 초기화한다(UNiagaraDataInterfaceArray::CreateAndSetVariant).
     * 매 프레임 쓰는 궤적은 그때마다 인스턴스가 새로 만들어져 입자가 생기지 않으므로, 게임 월드가 아니면 컴포넌트가 가진 DI의 배열에 바로 쓴다.
     * 이 경로는 라이브러리의 인스턴스 잠금과 GPU 동기화 표시를 거치지 않아 CPU 이미터만 갱신되며, 스크립트가 배열에 쓰지 않는다고 가정한다.
     * 컴포넌트는 활성화할 때 에셋의 DI를 복제해 가지므로 에셋의 값은 바뀌지 않는다.
     */
    template<typename TDataInterface, typename TElement, typename TConvert>
    void WriteArray(UNiagaraComponent& Component, bool bGameWorld, FName Name, const TArray<TElement>& Data,
        void (*LibrarySetter)(UNiagaraComponent*, FName, const TArray<TElement>&), TConvert Convert)
    {
        if (bGameWorld)
        {
            LibrarySetter(&Component, Name, Data);
            return;
        }
        if (TDataInterface* DataInterface = UNiagaraFunctionLibrary::GetDataInterface<TDataInterface>(&Component, Name))
        {
            auto& Target = DataInterface->GetArrayReference();
            Target.SetNum(Data.Num());
            for (int32 Index = 0; Index < Data.Num(); ++Index)
            {
                Target[Index] = Convert(Data[Index]);
            }
        }
    }

    /** 에셋에 있는 출력 파라미터에만 이번 프레임 입자를 넘긴다. Count는 새 입자가 없어도 매 프레임 써서 이전 값이 다시 생성되지 않게 한다. */
    void WriteBatch(UNiagaraComponent& Component, uint32 OutputMask, const FParticleBatch& Batch)
    {
        using namespace KataTrailParameters;
        const auto Has = [OutputMask](FName Name)
        {
            const int32 Index = GetOutputNames().IndexOfByKey(Name);
            return Index != INDEX_NONE && (OutputMask & (1u << Index)) != 0;
        };

        const UWorld* World = Component.GetWorld();
        const bool bGameWorld = World != nullptr && World->IsGameWorld();
        const auto Same = [](const auto& Value) { return Value; };
        using FLibrary = UNiagaraDataInterfaceArrayFunctionLibrary;

        if (Has(Positions))
        {
            // 라이브러리처럼 시스템 인스턴스의 LWC 타일 기준 시뮬레이션 좌표로 바꾼다. 게임 월드에서는 라이브러리가 바꾼다.
            FNiagaraLWCConverter LwcConverter;
            if (const FNiagaraSystemInstanceControllerConstPtr Controller = Component.GetSystemInstanceController())
            {
                if (const FNiagaraSystemInstance* SystemInstance = Controller->GetSystemInstance_Unsafe())
                {
                    LwcConverter = SystemInstance->GetLWCConverter();
                }
            }
            WriteArray<UNiagaraDataInterfaceArrayPosition>(Component, bGameWorld, Positions, Batch.Positions, &FLibrary::SetNiagaraArrayPosition,
                [&LwcConverter](const FVector& Position) { return LwcConverter.ConvertWorldToSimulationPosition(Position); });
        }
        if (Has(Widths))
        {
            WriteArray<UNiagaraDataInterfaceArrayFloat>(Component, bGameWorld, Widths, Batch.Widths, &FLibrary::SetNiagaraArrayFloat, Same);
        }
        if (Has(Sides))
        {
            WriteArray<UNiagaraDataInterfaceArrayFloat3>(Component, bGameWorld, Sides, Batch.Sides, &FLibrary::SetNiagaraArrayVector,
                [](const FVector& Side) { return FVector3f(Side); });
        }
        if (Has(RibbonIds))
        {
            WriteArray<UNiagaraDataInterfaceArrayInt32>(Component, bGameWorld, RibbonIds, Batch.RibbonIds, &FLibrary::SetNiagaraArrayInt32, Same);
        }
        if (Has(LinkOrders))
        {
            WriteArray<UNiagaraDataInterfaceArrayFloat>(Component, bGameWorld, LinkOrders, Batch.LinkOrders, &FLibrary::SetNiagaraArrayFloat, Same);
        }
        if (Has(Ages))
        {
            WriteArray<UNiagaraDataInterfaceArrayFloat>(Component, bGameWorld, Ages, Batch.Ages, &FLibrary::SetNiagaraArrayFloat, Same);
        }
        if (Has(BladeRanges))
        {
            WriteArray<UNiagaraDataInterfaceArrayFloat2>(Component, bGameWorld, BladeRanges, Batch.BladeRanges, &FLibrary::SetNiagaraArrayVector2D,
                [](const FVector2D& Range) { return FVector2f(Range); });
        }
        if (Has(Count))
        {
            Component.SetVariableInt(Count, Batch.Num());
        }
    }
}

int32 UKataHitSubsystem::RegisterTrail(const FKataTrailRegistration& Registration)
{
    const UKataTask_Trail* Task = Registration.Task;
    if (Task == nullptr || Task->System == nullptr || Registration.Mesh == nullptr
        || Registration.NiagaraComponent == nullptr || Registration.InstigatorActor == nullptr || Task->Sockets.Num() < 2)
    {
        return INDEX_NONE;
    }

    for (const FName SocketName : Task->Sockets)
    {
        // 없는 소켓을 읽으면 엔진이 컴포넌트 트랜스폼을 돌려주어 엉뚱한 자리에 궤적이 생긴다. 조용히 넘기지 않는다.
        if (!Registration.Mesh->DoesSocketExist(SocketName))
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Kata trail task '%s' needs socket '%s' but mesh '%s' has none"),
                *GetNameSafe(Task), *SocketName.ToString(), *GetPathNameSafe(Registration.Mesh));
            return INDEX_NONE;
        }
    }

    // 등록 시점을 놓친 컴포넌트도 다음 Tick부터 포즈를 기록하도록 여기서 한 번 더 붙인다.
    RegisterHitBoxComponent(Registration.HitBoxComponent);

    FKataActiveTrail& Entry = ActiveTrails.AddDefaulted_GetRef();
    Entry.Handle = NextHandle++;
    Entry.Task = Registration.Task;
    Entry.TaskInstance = Registration.TaskInstance;
    Entry.ActionInstance = Registration.ActionInstance;
    Entry.Mesh = Registration.Mesh;
    Entry.HitBoxComponent = Registration.HitBoxComponent;
    Entry.InstigatorActor = Registration.InstigatorActor;
    Entry.NiagaraComponent = Registration.NiagaraComponent;
    Entry.StartTime = Registration.StartTime;
    Entry.EndTime = FMath::Max(Registration.StartTime, Registration.EndTime);

    // 에셋 정의를 한 번만 읽는다. 없는 출력 배열에 매 프레임 쓰면 Niagara가 경고를 반복한다.
    const UNiagaraSystem& System = *Task->System;
    Entry.SampleDistance = KataTrail::ReadSampleSpacing(System, KataTrailParameters::SampleDistance, KataTrailParameters::DefaultSampleDistance);
    Entry.SampleAngle = KataTrail::ReadSampleSpacing(System, KataTrailParameters::SampleAngle, KataTrailParameters::DefaultSampleAngle);
    Entry.OutputMask = KataTrailParameters::FindOutputMask(System.GetExposedParameters());
    return Entry.Handle;
}

void UKataHitSubsystem::CloseTrail(int32 Handle, bool bFinalSegment)
{
    FKataActiveTrail* Entry = ActiveTrails.FindByPredicate([Handle](const FKataActiveTrail& Item) { return Item.Handle == Handle; });
    if (Entry == nullptr || Entry->bEmissionStopped)
    {
        return;
    }
    if (!bFinalSegment)
    {
        // 취소·중단된 공격은 궤적을 더 잇지 않는다. 이미 그린 부분은 수명대로 사라진다.
        if (UNiagaraComponent* Component = Entry->NiagaraComponent.Get())
        {
            Component->Deactivate();
        }
        Entry->bEmissionStopped = true;
        return;
    }

    Entry->bClosing = true;
    if (const UKataActionInstance* Action = Entry->ActionInstance.Get())
    {
        // 의존성 대기로 늦게 시작한 태스크는 정의상의 종료 시각보다 늦게 끝날 수 있으므로 실제 종료 시각으로 맞춘다.
        Entry->EndTime = FMath::Max(Entry->StartTime, Action->GetCurrentTime());
    }
}

bool UKataHitSubsystem::ProcessTrail(FKataActiveTrail& Entry, float DeltaTime)
{
    using namespace KataTrail;

    UNiagaraComponent* Component = Entry.NiagaraComponent.Get();
    if (!IsValid(Component))
    {
        // 남은 입자까지 사라져 컴포넌트가 스스로 소멸했다.
        return false;
    }

    // 히트스톱처럼 Avatar가 멈추면 궤적도 멈춘다. 방출을 멈춘 뒤 남은 입자도 같은 배율로 사라진다.
    AActor* Instigator = Entry.InstigatorActor.Get();
    Component->SetCustomTimeDilation(Instigator != nullptr ? Instigator->CustomTimeDilation : 1.0f);
    if (Entry.bEmissionStopped)
    {
        return true;
    }

    const auto StopEmission = [&Entry, Component]()
    {
        Component->Deactivate();
        Entry.bEmissionStopped = true;
    };

    const UKataTask_Trail* Task = Entry.Task;
    const UMeshComponent* Mesh = Entry.Mesh.Get();
    // 태스크 인스턴스가 사라졌는데 닫히지 않았다면 소유 액션이 정리 없이 사라진 것이다. 더 그리지 않는다.
    if (Task == nullptr || Mesh == nullptr || Instigator == nullptr || (!Entry.bClosing && !Entry.TaskInstance.IsValid()))
    {
        StopEmission();
        return true;
    }

    // 이번 Tick에 해당하는 Kata 시각. 액션이 먼저 사라졌으면 종료 시각을 이번 포즈의 시각으로 본다.
    const UKataActionInstance* Action = Entry.ActionInstance.Get();
    const float CurrentTime = Action != nullptr ? Action->GetCurrentTime() : Entry.EndTime;

    // 판정과 같은 방법으로 두 프레임 사이 소켓 경로를 만든다. 입력 배율의 출처도 Hit Trace와 같다.
    KataHitPoseSampler::FFramePathInput PathInput;
    PathInput.Mesh = Mesh;
    PathInput.SocketNames = Task->Sockets;
    PathInput.HitBoxComponent = Entry.HitBoxComponent.Get();
    PathInput.TickIndex = TickIndex;
    PathInput.CurrentTime = CurrentTime;
    PathInput.DeltaTime = DeltaTime;
    PathInput.KataTimeDilation = Action != nullptr ? Action->GetCustomTimeDilation() : Instigator->CustomTimeDilation;
    PathInput.AnimationTimeDilation = Instigator->CustomTimeDilation;
    PathInput.bSampleAnimation = true;

    KataHitPoseSampler::FFramePath Path;
    Path.Build(PathInput, Entry.PoseHistory);

    // 이번 프레임 중 궤적 구간 [StartTime, EndTime]에 속하는 부분만 쓴다. 종료 Tick에서는 EndTime 포즈까지만 넘긴다.
    const float SegmentStart = FMath::Max(Path.GetPreviousTime(), Entry.StartTime);
    const float SegmentEnd = FMath::Min(CurrentTime, Entry.EndTime);
    const bool bHasSegment = Path.GetFrameSpan() > UE_KINDA_SMALL_NUMBER && SegmentEnd > SegmentStart + UE_KINDA_SMALL_NUMBER;

    TArray<float, TInlineAllocator<KataTrailParameters::MaxSamplesPerFrame + 1>> SampleTimes;
    if (!Entry.bStartPointWritten)
    {
        // 첫 처리는 구간 시작점부터 넘긴다. 직전 포즈가 없어 이번 프레임 구간이 없으면 지금 포즈가 시작점이다.
        SampleTimes.Add(bHasSegment ? SegmentStart : FMath::Clamp(CurrentTime, Entry.StartTime, Entry.EndTime));
        Entry.bStartPointWritten = true;
    }

    int32 StepCount = 0;
    if (bHasSegment)
    {
        double MaxDistance = 0.0;
        double AngleDegrees = 0.0;
        MeasurePath(Path, Path.AlphaAt(SegmentStart), Path.AlphaAt(SegmentEnd), MaxDistance, AngleDegrees);
        const double RequiredSteps = FMath::Max(MaxDistance / Entry.SampleDistance, AngleDegrees / Entry.SampleAngle);
        StepCount = FMath::Clamp(FMath::CeilToInt(RequiredSteps), 1, KataTrailParameters::MaxSamplesPerFrame);
        // 이전 프레임의 마지막 샘플이 이번 구간의 시작이므로 k = 1부터 넘긴다.
        for (int32 Step = 1; Step <= StepCount; ++Step)
        {
            SampleTimes.Add(FMath::Lerp(SegmentStart, SegmentEnd, static_cast<float>(Step) / static_cast<float>(StepCount)));
        }
    }

    FParticleBatch Batch;
    FSocketPose Pose;
    for (const float SampleTime : SampleTimes)
    {
        Path.PoseAt(Path.AlphaAt(SampleTime), Pose);
        // 입자 나이는 샘플 시각부터 이번 프레임까지 지난 시간에서 시작해야 저프레임에서도 꼬리가 고르게 사라진다.
        Batch.AppendSample(Pose, SampleTime - Entry.StartTime, FMath::Max(0.0f, CurrentTime - SampleTime));
    }
    WriteBatch(*Component, Entry.OutputMask, Batch);

    UE_LOG(LogKataFramework, VeryVerbose,
        TEXT("Kata trail %d tick %llu: time [%.4f, %.4f] segment [%.4f, %.4f] window [%.4f, %.4f] montage %.4f steps %d particles %d sampled %d realPrevious %d closing %d"),
        Entry.Handle, TickIndex, Path.GetPreviousTime(), CurrentTime, SegmentStart, SegmentEnd, Entry.StartTime, Entry.EndTime,
        Path.GetMontagePosition(), StepCount, Batch.Num(), Path.IsSampled() ? 1 : 0, Path.HasRealPreviousPose() ? 1 : 0, Entry.bClosing ? 1 : 0);

    Path.Commit(Entry.PoseHistory);

    if (Entry.bClosing)
    {
        StopEmission();
    }
    return true;
}
