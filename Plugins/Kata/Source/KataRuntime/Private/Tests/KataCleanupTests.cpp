#include "Action/KataAction.h"
#include "Runtime/KataActionComponent.h"
#include "Runtime/KataActionInstance.h"
#include "Tasks/KataTask_ApplyLooseTag.h"
#include "Tasks/KataTask_CancelWindow.h"
#include "Tasks/KataTask_SendGameplayEvent.h"
#include "Tasks/KataTask_TransitionWindow.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

namespace KataCleanupTests
{
    constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

    /**
     * 테스트가 쓰는 태그. 플러그인은 태그를 정의하지 않으므로 ProjectKata의 Config/Tags에 등록된 태그를 빌려 쓴다.
     * 그래서 이 테스트는 ProjectKata에서만 통과한다. ASC 태그 수는 부모 태그에 자식 수가 합쳐지므로 자식이 없는 태그만 고른다.
     */
    struct FTags
    {
        FGameplayTag Loose = FGameplayTag::RequestGameplayTag(TEXT("Status.SuperArmor"));
        FGameplayTag Granted = FGameplayTag::RequestGameplayTag(TEXT("Status.Invincible"));
        FGameplayTag BlockedAbility = FGameplayTag::RequestGameplayTag(TEXT("Status.IgnoreDamage"));
        FGameplayTag Transition = FGameplayTag::RequestGameplayTag(TEXT("Window.Transition.Combo"));
        FGameplayTag CancelHeld = FGameplayTag::RequestGameplayTag(TEXT("Window.Cancel.Move"));
        FGameplayTag CancelPress = FGameplayTag::RequestGameplayTag(TEXT("Window.Cancel.Jump"));
        FGameplayTag Event = FGameplayTag::RequestGameplayTag(TEXT("Event.Hit"));
        // KataTags는 ASC에 붙지 않는 분류 태그라 다른 용도의 태그와 수가 섞이지 않는다.
        FGameplayTag KataAttack = FGameplayTag::RequestGameplayTag(TEXT("Trigger.Attack.Light"));
        FGameplayTag KataDodge = FGameplayTag::RequestGameplayTag(TEXT("Trigger.Dodge"));
    };

    /** 임시 게임 월드에 ASC와 액션 컴포넌트를 가진 액터 하나를 만든다. 받은 Gameplay Event 수를 센다. */
    struct FFixture
    {
        FTags Tags;
        UWorld* World = nullptr;
        AActor* Owner = nullptr;
        UAbilitySystemComponent* ASC = nullptr;
        UKataActionComponent* Actions = nullptr;
        int32 EventCount = 0;
        FDelegateHandle EventHandle;

        FFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
                .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
                .CreateFXSystem(false).SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
            check(World);
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

            Owner = World->SpawnActor<AActor>();
            check(Owner);
            USceneComponent* Root = NewObject<USceneComponent>(Owner);
            Owner->AddInstanceComponent(Root);
            Owner->SetRootComponent(Root);
            Root->RegisterComponent();

            ASC = NewObject<UAbilitySystemComponent>(Owner);
            Owner->AddInstanceComponent(ASC);
            ASC->RegisterComponent();
            ASC->InitAbilityActorInfo(Owner, Owner);

            Actions = NewObject<UKataActionComponent>(Owner);
            Owner->AddInstanceComponent(Actions);
            Actions->RegisterComponent();

            EventHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(Tags.Event).AddLambda([this](const FGameplayEventData*)
            {
                ++EventCount;
            });
        }

        ~FFixture()
        {
            if (FGameplayEventMulticastDelegate* Delegate = ASC->GenericGameplayEventCallbacks.Find(Tags.Event))
            {
                Delegate->Remove(EventHandle);
            }
            GEngine->DestroyWorldContext(World);
            World->DestroyWorld(false);
        }

        UKataActionInstance* Play(UKataAction* Action, EKataStartResult& OutResult)
        {
            UKataActionInstance* Instance = nullptr;
            OutResult = Actions->PlayKataAction(Action, FKataContext(), Instance);
            return Instance;
        }
    };

    template <typename TaskType>
    TaskType* AddTask(UKataAction* Action, float Duration)
    {
        TaskType* Task = NewObject<TaskType>(Action);
        Task->TaskId = FKataTaskId::NewId();
        Task->Duration = Duration;
        FKataTimelineEntry Entry;
        Entry.Task = Task;
        Action->TimelineTasks.Add(Entry);
        return Task;
    }

    /**
     * 네 태스크와 GAS 활성 상태(Active Granted Tags, Blocked Ability Tags)를 모두 쓰는 액션을 만든다.
     * 모든 태스크가 시각 0에 시작하므로 PlayKataAction 안에서 자원을 잡는다. 1초짜리 태스크가 있어 Tick 없이는 Running으로 남는다.
     */
    UKataAction* MakeFullAction(const FTags& Tags, const FGameplayTag& KataTag, bool bCanInterrupt,
        const FGameplayTag& BlockedKataTag = FGameplayTag())
    {
        UKataAction* Action = NewObject<UKataAction>(GetTransientPackage());
        Action->KataTags.AddTag(KataTag);
        Action->ActiveGrantedTags.AddTag(Tags.Granted);
        Action->BlockingPolicy.BlockedAbilityTags.AddTag(Tags.BlockedAbility);
        Action->BlockingPolicy.bCanInterruptActiveKata = bCanInterrupt;
        if (BlockedKataTag.IsValid())
        {
            Action->BlockingPolicy.BlockedKataTags.AddTag(BlockedKataTag);
        }

        AddTask<UKataTask_ApplyLooseTag>(Action, 1.0f)->Tags.AddTag(Tags.Loose);
        AddTask<UKataTask_TransitionWindow>(Action, 1.0f)->Windows.AddDefaulted_GetRef().WindowTag = Tags.Transition;
        UKataTask_CancelWindow* Cancel = AddTask<UKataTask_CancelWindow>(Action, 1.0f);
        FKataCancelWindowEntry& Held = Cancel->Windows.AddDefaulted_GetRef();
        Held.CancelTag = Tags.CancelHeld;
        Held.bCancelWhileHeld = true;
        Cancel->Windows.AddDefaulted_GetRef().CancelTag = Tags.CancelPress;
        // 이벤트 태스크는 순간 태스크라 시작에서 한 번 보내고 끝난다. 회수할 자원이 없으므로 보낸 횟수만 센다.
        AddTask<UKataTask_SendGameplayEvent>(Action, 0.0f)->EventTag = Tags.Event;
        return Action;
    }

    /** 캔슬 창 하나만 여는 액션을 만든다. 판정 테스트에서 열리지 않은 태그를 남겨 두려고 쓴다. */
    UKataAction* MakeCancelOnlyAction(const FTags& Tags, const FGameplayTag& CancelTag, bool bCancelWhileHeld)
    {
        UKataAction* Action = NewObject<UKataAction>(GetTransientPackage());
        Action->KataTags.AddTag(Tags.KataAttack);
        FKataCancelWindowEntry& Entry = AddTask<UKataTask_CancelWindow>(Action, 1.0f)->Windows.AddDefaulted_GetRef();
        Entry.CancelTag = CancelTag;
        Entry.bCancelWhileHeld = bCancelWhileHeld;
        return Action;
    }

    /**
     * Running이 실행 중인 유일한 액션일 때(없으면 null) ASC와 컴포넌트에 남은 자원이 정확히 그 몫인지 확인한다.
     * 모든 액션이 같은 태그를 쓰므로, 교체·반복 뒤에도 수가 1을 넘거나 종료 뒤 0이 아니면 정리가 새는 것이다.
     * 실패를 한 번만 보고하도록 결과를 묶어 반환한다.
     */
    bool ExpectHeldBy(FAutomationTestBase& Test, const FString& Label, const FFixture& Fixture, const UKataActionInstance* Running)
    {
        const int32 Expected = Running != nullptr ? 1 : 0;
        bool bOk = true;
        bOk &= Test.TestEqual(*(Label + TEXT(": loose tag count")), Fixture.ASC->GetTagCount(Fixture.Tags.Loose), Expected);
        bOk &= Test.TestEqual(*(Label + TEXT(": granted tag count")), Fixture.ASC->GetTagCount(Fixture.Tags.Granted), Expected);
        bOk &= Test.TestTrue(*(Label + TEXT(": ability block matches running action")),
            Fixture.ASC->AreAbilityTagsBlocked(FGameplayTagContainer(Fixture.Tags.BlockedAbility)) == (Running != nullptr));
        bOk &= Test.TestTrue(*(Label + TEXT(": component tracks the running action")),
            Fixture.Actions->GetActiveInstance() == Running && Fixture.Actions->IsPlayingKata() == (Running != nullptr));
        if (Running != nullptr)
        {
            bOk &= Test.TestTrue(*(Label + TEXT(": windows are open while running")),
                Running->IsTransitionWindowOpen(Fixture.Tags.Transition) && Running->IsCancelWindowOpen(Fixture.Tags.CancelHeld)
                && Running->IsCancelWindowOpen(Fixture.Tags.CancelPress));
        }
        return bOk;
    }

    /** 끝난 인스턴스가 창과 실행 중 태스크를 남기지 않았는지 확인한다. */
    bool ExpectReleased(FAutomationTestBase& Test, const FString& Label, const FFixture& Fixture, const UKataActionInstance* Ended)
    {
        if (!Test.TestNotNull(*(Label + TEXT(": ended instance")), Ended))
        {
            return false;
        }
        bool bOk = Test.TestTrue(*(Label + TEXT(": instance is ended")), Ended->GetInstanceState() == EKataInstanceState::Ended);
        bOk &= Test.TestTrue(*(Label + TEXT(": windows are closed")),
            !Ended->IsTransitionWindowOpen(Fixture.Tags.Transition) && !Ended->IsCancelWindowOpen(Fixture.Tags.CancelHeld)
            && !Ended->IsCancelWindowOpen(Fixture.Tags.CancelPress));
        bOk &= Test.TestEqual(*(Label + TEXT(": no running task instances")), Ended->GetActiveTaskInstances().Num(), 0);
        return bOk;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataCleanupEndPathsTest, "Kata.Runtime.Cleanup.EndPaths", KataCleanupTests::Flags)
bool FKataCleanupEndPathsTest::RunTest(const FString& Parameters)
{
    using namespace KataCleanupTests;
    struct FEndPath
    {
        const TCHAR* Name;
        EKataEndReason Reason;
        // 액션을 끝내고, 끝낸 뒤에도 실행 중인 액션(교체 경로의 새 액션)을 반환한다.
        TFunction<UKataActionInstance*(FFixture&)> End;
    };
    const FEndPath Paths[] = {
        { TEXT("StopKata"), EKataEndReason::Interrupted, [](FFixture& Fixture) -> UKataActionInstance*
        {
            Fixture.Actions->StopKata(EKataEndReason::Interrupted);
            return nullptr;
        } },
        { TEXT("TryCancelKata"), EKataEndReason::Cancelled, [](FFixture& Fixture) -> UKataActionInstance*
        {
            Fixture.Actions->TryCancelKata(Fixture.Tags.CancelPress);
            return nullptr;
        } },
        { TEXT("CancelAbilitiesAndKata"), EKataEndReason::Interrupted, [](FFixture& Fixture) -> UKataActionInstance*
        {
            Fixture.Actions->CancelAbilitiesAndKata(FGameplayTagContainer(), FGameplayTagContainer());
            return nullptr;
        } },
        { TEXT("ReplacingPlay"), EKataEndReason::Interrupted, [](FFixture& Fixture) -> UKataActionInstance*
        {
            EKataStartResult Result;
            return Fixture.Play(MakeFullAction(Fixture.Tags, Fixture.Tags.KataDodge, true), Result);
        } },
    };

    for (const FEndPath& Path : Paths)
    {
        FFixture Fixture;
        EKataStartResult Result;
        UKataActionInstance* Instance = Fixture.Play(MakeFullAction(Fixture.Tags, Fixture.Tags.KataAttack, true), Result);
        const FString Label = Path.Name;
        if (!TestTrue(*(Label + TEXT(": action starts")), Result == EKataStartResult::Started && Instance != nullptr))
        {
            continue;
        }
        ExpectHeldBy(*this, Label + TEXT(" before end"), Fixture, Instance);
        TestEqual(*(Label + TEXT(": start event is sent once")), Fixture.EventCount, 1);

        UKataActionInstance* StillRunning = Path.End(Fixture);
        ExpectReleased(*this, Label, Fixture, Instance);
        TestTrue(*(Label + TEXT(": end reason is recorded")), Instance->GetEndReason() == Path.Reason);
        // 교체 경로는 새 액션이 같은 태그를 다시 잡으므로 수가 2가 아니라 1이어야 한다.
        ExpectHeldBy(*this, Label + TEXT(" after end"), Fixture, StillRunning);
        TestEqual(*(Label + TEXT(": ending sends no event")), Fixture.EventCount, StillRunning != nullptr ? 2 : 1);

        if (StillRunning != nullptr)
        {
            Fixture.Actions->StopKata(EKataEndReason::Interrupted);
            ExpectReleased(*this, Label + TEXT(" replacement"), Fixture, StillRunning);
            ExpectHeldBy(*this, Label + TEXT(" after replacement stops"), Fixture, nullptr);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataCleanupReplaceTest, "Kata.Runtime.Cleanup.ReplaceAndReject", KataCleanupTests::Flags)
bool FKataCleanupReplaceTest::RunTest(const FString& Parameters)
{
    using namespace KataCleanupTests;
    FFixture Fixture;
    // 공격 액션은 실행 중 회피 액션을 막는다.
    UKataAction* Attack = MakeFullAction(Fixture.Tags, Fixture.Tags.KataAttack, true, Fixture.Tags.KataDodge);
    EKataStartResult Result;
    UKataActionInstance* First = Fixture.Play(Attack, Result);
    if (!TestTrue(TEXT("First action starts"), Result == EKataStartResult::Started))
    {
        return false;
    }

    // 거절된 시작은 기존 액션을 그대로 두고 새 자원을 잡지 않는다.
    Fixture.Play(MakeFullAction(Fixture.Tags, Fixture.Tags.KataDodge, true), Result);
    TestTrue(TEXT("Blocked kata tag rejects the new action"), Result == EKataStartResult::BlockedByActiveKata);
    Fixture.Play(MakeFullAction(Fixture.Tags, Fixture.Tags.KataAttack, false), Result);
    TestTrue(TEXT("Action without interrupt permission is rejected"), Result == EKataStartResult::BlockedByActiveKata);
    ExpectHeldBy(*this, TEXT("After rejected starts"), Fixture, First);
    TestEqual(TEXT("Rejected starts send no event"), Fixture.EventCount, 1);

    // 같은 에셋으로 자신을 교체해도 자원은 한 몫만 남는다.
    UKataActionInstance* Second = Fixture.Play(Attack, Result);
    TestTrue(TEXT("Same asset replaces itself"), Result == EKataStartResult::Started && Second != First);
    ExpectReleased(*this, TEXT("Replaced instance"), Fixture, First);
    ExpectHeldBy(*this, TEXT("After self replacement"), Fixture, Second);

    Fixture.Actions->StopKata(EKataEndReason::Interrupted);
    ExpectHeldBy(*this, TEXT("After final stop"), Fixture, nullptr);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataCleanupRepeatedEndTest, "Kata.Runtime.Cleanup.RepeatedEndRequests", KataCleanupTests::Flags)
bool FKataCleanupRepeatedEndTest::RunTest(const FString& Parameters)
{
    using namespace KataCleanupTests;
    FFixture Fixture;
    EKataStartResult Result;
    UKataActionInstance* Instance = Fixture.Play(MakeFullAction(Fixture.Tags, Fixture.Tags.KataAttack, true), Result);
    if (!TestTrue(TEXT("Action starts"), Result == EKataStartResult::Started && Instance != nullptr))
    {
        return false;
    }

    Fixture.Actions->StopKata(EKataEndReason::Interrupted);
    // 이미 끝난 뒤의 종료 요청은 자원을 두 번 회수하지 않고 처음 종료 사유를 바꾸지 않는다.
    Fixture.Actions->StopKata(EKataEndReason::Cancelled);
    Instance->RequestEnd(EKataEndReason::Completed);
    TestFalse(TEXT("Cancel after end is rejected"), Fixture.Actions->TryCancelKata(Fixture.Tags.CancelPress));
    Fixture.Actions->CancelAbilitiesAndKata(FGameplayTagContainer(), FGameplayTagContainer());

    TestTrue(TEXT("First end reason is kept"), Instance->GetEndReason() == EKataEndReason::Interrupted);
    ExpectReleased(*this, TEXT("After repeated end requests"), Fixture, Instance);
    ExpectHeldBy(*this, TEXT("After repeated end requests"), Fixture, nullptr);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataCleanupRandomSequenceTest, "Kata.Runtime.Cleanup.RandomSequence", KataCleanupTests::Flags)
bool FKataCleanupRandomSequenceTest::RunTest(const FString& Parameters)
{
    using namespace KataCleanupTests;
    FFixture Fixture;
    UKataAction* Attack = MakeFullAction(Fixture.Tags, Fixture.Tags.KataAttack, true);
    UKataAction* Dodge = MakeFullAction(Fixture.Tags, Fixture.Tags.KataDodge, true);

    // 시드를 고정해 실패한 순서를 그대로 다시 재현할 수 있게 한다.
    FRandomStream Random(1017);
    UKataActionInstance* Running = nullptr;
    FGameplayTag RunningKataTag;
    int32 ExpectedEvents = 0;
    for (int32 Step = 0; Step < 200; ++Step)
    {
        const int32 Operation = Random.RandRange(0, 5);
        const FString Label = FString::Printf(TEXT("Step %d op %d"), Step, Operation);
        switch (Operation)
        {
        case 0:
        case 1:
        {
            EKataStartResult Result;
            UKataActionInstance* Previous = Running;
            Running = Fixture.Play(Operation == 0 ? Attack : Dodge, Result);
            RunningKataTag = Operation == 0 ? Fixture.Tags.KataAttack : Fixture.Tags.KataDodge;
            ++ExpectedEvents;
            if (!TestTrue(*(Label + TEXT(": play starts")), Result == EKataStartResult::Started))
            {
                return false;
            }
            if (Previous != nullptr && !ExpectReleased(*this, Label + TEXT(" replaced"), Fixture, Previous))
            {
                return false;
            }
            break;
        }
        case 2:
            Fixture.Actions->StopKata(EKataEndReason::Interrupted);
            Running = nullptr;
            break;
        case 3:
        {
            // 두 캔슬 창이 모두 열려 있으므로 새 입력은 물론 누르고 있는 입력도 받아들여야 한다.
            const bool bHeldInput = Random.RandRange(0, 1) == 1;
            const FGameplayTag CancelTag = bHeldInput ? Fixture.Tags.CancelHeld : Fixture.Tags.CancelPress;
            const bool bCancelled = Fixture.Actions->TryCancelKata(CancelTag, !bHeldInput);
            if (!TestTrue(*(Label + TEXT(": cancel result matches running state")), bCancelled == (Running != nullptr)))
            {
                return false;
            }
            Running = nullptr;
            break;
        }
        case 4:
        {
            // 필터 없음, 공격 태그, 회피 태그 중 하나로 중단을 요청한다. 실행 중인 액션의 분류 태그가 맞을 때만 끝난다.
            const int32 Filter = Random.RandRange(0, 2);
            FGameplayTagContainer WithTags;
            if (Filter > 0)
            {
                WithTags.AddTag(Filter == 1 ? Fixture.Tags.KataAttack : Fixture.Tags.KataDodge);
            }
            Fixture.Actions->CancelAbilitiesAndKata(WithTags, FGameplayTagContainer());
            if (WithTags.IsEmpty() || WithTags.HasTagExact(RunningKataTag))
            {
                Running = nullptr;
            }
            break;
        }
        default:
            if (Running != nullptr)
            {
                Running->RequestEnd(EKataEndReason::Completed);
                Running = nullptr;
            }
            break;
        }
        if (Running == nullptr)
        {
            RunningKataTag = FGameplayTag();
        }
        if (!ExpectHeldBy(*this, Label, Fixture, Running))
        {
            return false;
        }
    }

    Fixture.Actions->StopKata(EKataEndReason::Interrupted);
    ExpectHeldBy(*this, TEXT("After the sequence"), Fixture, nullptr);
    TestEqual(TEXT("Each successful start sends exactly one event"), Fixture.EventCount, ExpectedEvents);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataCleanupCancelJudgementTest, "Kata.Runtime.Cleanup.CancelWindowJudgement", KataCleanupTests::Flags)
bool FKataCleanupCancelJudgementTest::RunTest(const FString& Parameters)
{
    using namespace KataCleanupTests;
    FFixture Fixture;
    EKataStartResult Result;

    // 새 입력만 받는 창: 열리지 않은 태그와 누르고 있는 입력은 거절한다.
    UKataActionInstance* PressOnly = Fixture.Play(MakeCancelOnlyAction(Fixture.Tags, Fixture.Tags.CancelPress, false), Result);
    if (!TestTrue(TEXT("Press-only action starts"), Result == EKataStartResult::Started && PressOnly != nullptr))
    {
        return false;
    }
    TestFalse(TEXT("Tag without an open window is rejected"), Fixture.Actions->TryCancelKata(Fixture.Tags.CancelHeld));
    TestFalse(TEXT("Held input is rejected by a press-only window"), Fixture.Actions->TryCancelKata(Fixture.Tags.CancelPress, false));
    TestTrue(TEXT("Rejected cancels keep the action running"), PressOnly->IsRunning());
    TestTrue(TEXT("New press is accepted"), Fixture.Actions->TryCancelKata(Fixture.Tags.CancelPress, true));
    TestTrue(TEXT("Accepted cancel ends as Cancelled"), PressOnly->GetEndReason() == EKataEndReason::Cancelled);

    // 누르고 있는 입력을 허용한 창은 새 입력이 아니어도 받는다.
    UKataActionInstance* Held = Fixture.Play(MakeCancelOnlyAction(Fixture.Tags, Fixture.Tags.CancelHeld, true), Result);
    TestTrue(TEXT("Held input is accepted by a hold window"), Held != nullptr && Fixture.Actions->TryCancelKata(Fixture.Tags.CancelHeld, false));

    // 중단 필터는 실행 중인 액션의 KataTags로 고르고, 필터를 쓰면 전체 중단 알림을 보내지 않는다.
    int32 CancelAllCount = 0;
    const FDelegateHandle CancelAllHandle = Fixture.Actions->OnCancelAllRequested.AddLambda([&CancelAllCount](EKataEndReason)
    {
        ++CancelAllCount;
    });
    UKataActionInstance* Filtered = Fixture.Play(MakeFullAction(Fixture.Tags, Fixture.Tags.KataAttack, true), Result);
    FGameplayTagContainer AttackTags(Fixture.Tags.KataAttack);
    Fixture.Actions->CancelAbilitiesAndKata(FGameplayTagContainer(Fixture.Tags.KataDodge), FGameplayTagContainer());
    TestTrue(TEXT("Unmatched WithTags keeps the action"), Filtered != nullptr && Filtered->IsRunning());
    Fixture.Actions->CancelAbilitiesAndKata(AttackTags, AttackTags);
    TestTrue(TEXT("WithoutTags excludes a matching action"), Filtered != nullptr && Filtered->IsRunning());
    Fixture.Actions->CancelAbilitiesAndKata(AttackTags, FGameplayTagContainer(), nullptr, EKataEndReason::Cancelled);
    TestTrue(TEXT("Matching WithTags ends with the given reason"),
        Filtered != nullptr && !Filtered->IsRunning() && Filtered->GetEndReason() == EKataEndReason::Cancelled);
    TestEqual(TEXT("Filtered cancels do not broadcast cancel-all"), CancelAllCount, 0);
    Fixture.Actions->CancelAbilitiesAndKata(FGameplayTagContainer(), FGameplayTagContainer());
    TestEqual(TEXT("Unfiltered cancel broadcasts once even with nothing running"), CancelAllCount, 1);
    Fixture.Actions->OnCancelAllRequested.Remove(CancelAllHandle);

    ExpectHeldBy(*this, TEXT("After judgement checks"), Fixture, nullptr);
    return true;
}

#endif // 개발용 자동화 테스트
