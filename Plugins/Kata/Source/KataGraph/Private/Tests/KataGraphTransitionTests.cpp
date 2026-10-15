#include "KataActionNode.h"
#include "KataAliasNode.h"
#include "KataEdge.h"
#include "KataEntryNode.h"
#include "KataGraph.h"
#include "KataGraphInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystemComponent.h"
#include "Action/KataAction.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Runtime/KataActionComponent.h"
#include "Runtime/KataActionInstance.h"
#include "Tasks/KataTask_TransitionWindow.h"
#include "UObject/Package.h"

namespace KataGraphTransitionTests
{
    constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

    /**
     * 테스트가 쓰는 태그. 플러그인은 태그를 정의하지 않으므로 ProjectKata의 Config/Tags에 등록된 태그를 빌려 쓴다.
     * 그래서 이 테스트는 ProjectKata에서만 통과한다. Trigger.Attack은 Trigger.Attack.Light가 등록되며 생기는 부모 태그다.
     */
    struct FTags
    {
        FGameplayTag Attack = FGameplayTag::RequestGameplayTag(TEXT("Trigger.Attack.Light"));
        FGameplayTag AttackParent = FGameplayTag::RequestGameplayTag(TEXT("Trigger.Attack"));
        FGameplayTag Dodge = FGameplayTag::RequestGameplayTag(TEXT("Trigger.Dodge"));
        FGameplayTag OpenWindow = FGameplayTag::RequestGameplayTag(TEXT("Window.Transition.Combo"));
        FGameplayTag ClosedWindow = FGameplayTag::RequestGameplayTag(TEXT("Window.Transition.Dodge"));
        // 테스트 ASC에는 붙이지 않으므로 이 태그를 요구하는 액션은 게임플레이 사유로 시작을 거절당한다.
        FGameplayTag NeverOwned = FGameplayTag::RequestGameplayTag(TEXT("Status.Dead"));
    };

    /** 임시 게임 월드에 ASC와 액션 컴포넌트를 가진 액터 하나를 만들고, 그래프에 넘길 Context를 채운다. */
    struct FFixture
    {
        FTags Tags;
        UWorld* World = nullptr;
        AActor* Owner = nullptr;
        UAbilitySystemComponent* ASC = nullptr;
        UKataActionComponent* Actions = nullptr;
        FKataContext Context;

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

            // 그래프 인스턴스는 받은 Context를 그대로 쓰므로 실행 주체를 직접 채운다.
            Context.OwnerActor = Owner;
            Context.AvatarActor = Owner;
        }

        ~FFixture()
        {
            GEngine->DestroyWorldContext(World);
            World->DestroyWorld(false);
        }

        UKataGraphInstance* Start(UKataGraph* Graph) const
        {
            UKataGraphInstance* Instance = NewObject<UKataGraphInstance>(Actions);
            Instance->InitializeInstance(Graph, Actions, Context);
            return Instance;
        }
    };

    /**
     * 열린 전이 창 하나를 가진 실행 중 액션. 시각 0의 창 태스크가 PlayKataAction 안에서 창을 열고,
     * 1초짜리라 Tick 없이는 Running으로 남는다.
     */
    UKataAction* MakeRunningAction(const FTags& Tags)
    {
        UKataAction* Action = NewObject<UKataAction>(GetTransientPackage());
        UKataTask_TransitionWindow* Window = NewObject<UKataTask_TransitionWindow>(Action);
        Window->TaskId = FKataTaskId::NewId();
        Window->Duration = 1.0f;
        Window->Windows.AddDefaulted_GetRef().WindowTag = Tags.OpenWindow;
        FKataTimelineEntry Entry;
        Entry.Task = Window;
        Action->TimelineTasks.Add(Entry);
        return Action;
    }

    /** 태스크가 없어 PlayKataAction 안에서 Completed로 끝나는 순간 액션. */
    UKataAction* MakeInstantAction()
    {
        return NewObject<UKataAction>(GetTransientPackage());
    }

    /** 시작 요구 태그를 갖추지 못해 MissingRequiredTags로 거절되는 액션. */
    UKataAction* MakeRejectedAction(const FTags& Tags)
    {
        UKataAction* Action = MakeRunningAction(Tags);
        Action->ActivationRequiredTags.AddTag(Tags.NeverOwned);
        return Action;
    }

    /** 창이 없는 전이 창 태스크 때문에 해석 오류(ResolveFailed)가 나는 액션. 계약 오류 경로를 만든다. */
    UKataAction* MakeBrokenAction()
    {
        UKataAction* Action = NewObject<UKataAction>(GetTransientPackage());
        UKataTask_TransitionWindow* Window = NewObject<UKataTask_TransitionWindow>(Action);
        Window->TaskId = FKataTaskId::NewId();
        Window->Duration = 1.0f;
        FKataTimelineEntry Entry;
        Entry.Task = Window;
        Action->TimelineTasks.Add(Entry);
        return Action;
    }

    /**
     * 에디터가 저장할 때 만드는 런타임 그래프 모양을 코드로 만든다.
     * 런타임은 AllNodes와 노드별 ChildrenNodes·Edges만 읽으므로 그 셋만 채운다. 선언 순서가 같은 조건의 평가 순서다.
     */
    struct FGraphBuilder
    {
        UKataGraph* Graph = NewObject<UKataGraph>(GetTransientPackage());

        template <typename NodeType>
        NodeType* AddNode()
        {
            NodeType* Node = NewObject<NodeType>(Graph);
            Node->Graph = Graph;
            Graph->AllNodes.Add(Node);
            return Node;
        }

        UKataEntryNode* Entry(bool bSubGraphEntry = false)
        {
            UKataEntryNode* Node = AddNode<UKataEntryNode>();
            Node->bIsSubGraphEntry = bSubGraphEntry;
            return Node;
        }

        UKataActionNode* Action(UKataAction* InAction)
        {
            UKataActionNode* Node = AddNode<UKataActionNode>();
            Node->Action = InAction;
            return Node;
        }

        UKataAliasNode* AnyState()
        {
            UKataAliasNode* Node = AddNode<UKataAliasNode>();
            Node->bAnyState = true;
            return Node;
        }

        /** 트리거가 비어 있으면 자동 엣지다. */
        UKataEdge* Link(UKataGraphNodeBase* From, UKataGraphNodeBase* To, const FGameplayTag& Trigger = FGameplayTag())
        {
            UKataEdge* Edge = NewObject<UKataEdge>(Graph);
            Edge->TriggerTag = Trigger;
            From->ChildrenNodes.AddUnique(To);
            To->ParentNodes.AddUnique(From);
            From->AddEdge(To, Edge);
            return Edge;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataGraphTriggersTest, "Kata.Graph.Transition.Triggers", KataGraphTransitionTests::Flags)
bool FKataGraphTriggersTest::RunTest(const FString& Parameters)
{
    using namespace KataGraphTransitionTests;
    FFixture Fixture;
    const FTags& Tags = Fixture.Tags;
    FGraphBuilder Builder;
    UKataActionNode* A = Builder.Action(MakeRunningAction(Tags));
    UKataActionNode* B = Builder.Action(MakeRunningAction(Tags));
    UKataActionNode* C = Builder.Action(MakeRunningAction(Tags));
    UKataActionNode* D = Builder.Action(MakeRunningAction(Tags));
    Builder.Link(Builder.Entry(), A);
    Builder.Link(A, B, Tags.Attack)->RequiredWindowTag = Tags.OpenWindow;
    Builder.Link(A, C, Tags.Dodge)->RequiredWindowTag = Tags.ClosedWindow;
    // 부모 태그 엣지는 자식 트리거를 받는다. 현재 액션이 끝난 뒤 전이하도록 예약한다.
    Builder.Link(B, D, Tags.AttackParent)->Timing = EKataTransitionTiming::OnActionEnd;

    UKataGraphInstance* Graph = Fixture.Start(Builder.Graph);
    if (!TestTrue(TEXT("Automatic entry edge starts the first action"), Graph->GetCurrentNode() == A))
    {
        return false;
    }
    UKataActionInstance* FirstAction = Graph->GetCurrentActionInstance();

    TestTrue(TEXT("Closed window buffers the trigger"), Graph->SendTriggerWithResult(Tags.Dodge) == EKataGraphTriggerResult::Buffered);
    TestTrue(TEXT("Buffered trigger keeps the current node"), Graph->GetCurrentNode() == A);
    TestTrue(TEXT("Open window transitions immediately"), Graph->SendTriggerWithResult(Tags.Attack) == EKataGraphTriggerResult::Transitioned);
    TestTrue(TEXT("Graph moves to the target node"), Graph->GetCurrentNode() == B);
    TestTrue(TEXT("Previous action ends as Branched"), FirstAction != nullptr && FirstAction->GetEndReason() == EKataEndReason::Branched);

    TestTrue(TEXT("Trigger without an edge is NoMatch"), Graph->SendTriggerWithResult(Tags.Dodge) == EKataGraphTriggerResult::NoMatch);
    TestTrue(TEXT("Parent-tag edge reserves the child trigger"), Graph->SendTriggerWithResult(Tags.Attack) == EKataGraphTriggerResult::Reserved);
    TestTrue(TEXT("Reservation keeps the current node"), Graph->GetCurrentNode() == B && Graph->GetPendingTargetNode() == D);
    Graph->GetCurrentActionInstance()->RequestEnd(EKataEndReason::Completed);
    TestTrue(TEXT("Completion follows the reservation"), Graph->GetCurrentNode() == D && Graph->IsRunning());

    // 같은 트리거의 엣지가 둘이면 Priority가 높은 쪽이 이긴다.
    FGraphBuilder PriorityBuilder;
    UKataActionNode* Start = PriorityBuilder.Action(MakeRunningAction(Tags));
    UKataActionNode* Low = PriorityBuilder.Action(MakeRunningAction(Tags));
    UKataActionNode* High = PriorityBuilder.Action(MakeRunningAction(Tags));
    PriorityBuilder.Link(PriorityBuilder.Entry(), Start);
    PriorityBuilder.Link(Start, Low, Tags.Attack);
    PriorityBuilder.Link(Start, High, Tags.Attack)->Priority = 5;
    Graph->RequestEnd(EKataEndReason::Interrupted);
    UKataGraphInstance* PriorityGraph = Fixture.Start(PriorityBuilder.Graph);
    PriorityGraph->SendTriggerWithResult(Tags.Attack);
    TestTrue(TEXT("Higher priority edge wins"), PriorityGraph->GetCurrentNode() == High);
    PriorityGraph->RequestEnd(EKataEndReason::Interrupted);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataGraphRejectionTest, "Kata.Graph.Transition.RejectionVsContractError", KataGraphTransitionTests::Flags)
bool FKataGraphRejectionTest::RunTest(const FString& Parameters)
{
    using namespace KataGraphTransitionTests;
    FFixture Fixture;
    const FTags& Tags = Fixture.Tags;

    // 실행 중인 액션이 있을 때의 게임플레이 거절은 그 액션과 그래프를 그대로 둔다.
    {
        FGraphBuilder Builder;
        UKataActionNode* A = Builder.Action(MakeRunningAction(Tags));
        Builder.Link(Builder.Entry(), A);
        Builder.Link(A, Builder.Action(MakeRejectedAction(Tags)), Tags.Attack);
        UKataGraphInstance* Graph = Fixture.Start(Builder.Graph);
        TestTrue(TEXT("Running: gameplay refusal is Rejected"), Graph->SendTriggerWithResult(Tags.Attack) == EKataGraphTriggerResult::Rejected);
        TestTrue(TEXT("Running: refusal reason is kept"), Graph->GetLastActionStartResult() == EKataStartResult::MissingRequiredTags);
        TestTrue(TEXT("Running: current action and graph continue"),
            Graph->IsRunning() && Graph->GetCurrentNode() == A && Graph->GetCurrentActionInstance() != nullptr
            && Graph->GetCurrentActionInstance()->IsRunning());
        Graph->RequestEnd(EKataEndReason::Interrupted);
    }

    // 첫 액션을 기다리는 중의 거절은 그래프를 끝내지 않고 진입 대기에 머문다.
    {
        FGraphBuilder Builder;
        UKataEntryNode* Entry = Builder.Entry();
        UKataActionNode* Refused = Builder.Action(MakeRejectedAction(Tags));
        Builder.Link(Entry, Refused);
        Builder.Link(Entry, Refused, Tags.Dodge);
        UKataGraphInstance* Graph = Fixture.Start(Builder.Graph);
        TestTrue(TEXT("Entry: refused automatic entry waits for entry"),
            Graph->GetState() == EKataGraphInstanceState::WaitingForEntry && Graph->IsRunning() && !Graph->HasStartedAction());
        TestTrue(TEXT("Entry: refused trigger is Rejected"), Graph->SendTriggerWithResult(Tags.Dodge) == EKataGraphTriggerResult::Rejected);
        TestTrue(TEXT("Entry: graph still waits for entry"), Graph->GetState() == EKataGraphInstanceState::WaitingForEntry);
        Graph->RequestEnd(EKataEndReason::Interrupted);
    }

    // 실행 중인 액션도 진입 대기도 아닐 때(완료 뒤 자동 전이)의 거절은 그래프를 Completed로 끝낸다.
    {
        FGraphBuilder Builder;
        UKataActionNode* Instant = Builder.Action(MakeInstantAction());
        Builder.Link(Builder.Entry(), Instant);
        Builder.Link(Instant, Builder.Action(MakeRejectedAction(Tags)));
        UKataGraphInstance* Graph = Fixture.Start(Builder.Graph);
        TestTrue(TEXT("After completion: refusal ends the graph as Completed"),
            Graph->GetState() == EKataGraphInstanceState::Ended && Graph->GetEndReason() == EKataEndReason::Completed);
    }

    // 해석 오류처럼 게임플레이 사유가 아닌 시작 실패는 계약 오류로 그래프를 끝낸다.
    {
        AddExpectedErrorPlain(TEXT("Kata graph failed to start action"), EAutomationExpectedErrorFlags::Contains, 1);
        // 해석 진단이 오류 로그로 남을 수 있으므로 같은 원인의 진단 로그는 실패로 보지 않는다.
        AddExpectedErrorPlain(TEXT("NoWindows"), EAutomationExpectedErrorFlags::Contains, -1);
        FGraphBuilder Builder;
        UKataActionNode* A = Builder.Action(MakeRunningAction(Tags));
        Builder.Link(Builder.Entry(), A);
        Builder.Link(A, Builder.Action(MakeBrokenAction()), Tags.Attack);
        UKataGraphInstance* Graph = Fixture.Start(Builder.Graph);
        TestTrue(TEXT("Contract: start failure is Failed"), Graph->SendTriggerWithResult(Tags.Attack) == EKataGraphTriggerResult::Failed);
        TestTrue(TEXT("Contract: failure reason is kept"), Graph->GetLastActionStartResult() == EKataStartResult::ResolveFailed);
        TestTrue(TEXT("Contract: graph ends with ContractError"),
            Graph->GetState() == EKataGraphInstanceState::Ended && Graph->GetEndReason() == EKataEndReason::ContractError);
    }
    Fixture.Actions->StopKata(EKataEndReason::Interrupted);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataGraphAnyStateTest, "Kata.Graph.Transition.AnyStateAlias", KataGraphTransitionTests::Flags)
bool FKataGraphAnyStateTest::RunTest(const FString& Parameters)
{
    using namespace KataGraphTransitionTests;
    FFixture Fixture;
    const FTags& Tags = Fixture.Tags;

    // Any State 엣지는 모든 실행 노드에서 쓸 수 있다. 같은 우선순위면 현재 노드의 직접 엣지가 먼저다.
    FGraphBuilder Builder;
    UKataActionNode* A = Builder.Action(MakeRunningAction(Tags));
    UKataActionNode* Direct = Builder.Action(MakeRunningAction(Tags));
    UKataActionNode* ViaAlias = Builder.Action(MakeRunningAction(Tags));
    Builder.Link(Builder.Entry(), A);
    Builder.Link(A, Direct, Tags.Dodge);
    UKataEdge* AliasEdge = Builder.Link(Builder.AnyState(), ViaAlias, Tags.Dodge);

    UKataGraphInstance* Graph = Fixture.Start(Builder.Graph);
    Graph->SendTriggerWithResult(Tags.Dodge);
    TestTrue(TEXT("Direct edge wins a priority tie"), Graph->GetCurrentNode() == Direct);
    Graph->SendTriggerWithResult(Tags.Dodge);
    TestTrue(TEXT("Any State covers a node without its own edge"), Graph->GetCurrentNode() == ViaAlias);
    Graph->RequestEnd(EKataEndReason::Interrupted);

    AliasEdge->Priority = 1;
    Graph = Fixture.Start(Builder.Graph);
    Graph->SendTriggerWithResult(Tags.Dodge);
    TestTrue(TEXT("Higher priority alias edge beats a direct edge"), Graph->GetCurrentNode() == ViaAlias);
    // Any State 엣지가 현재 노드 자신을 가리키면 건너뛴다.
    TestTrue(TEXT("Alias edge to the current node is skipped"),
        Graph->SendTriggerWithResult(Tags.Dodge) == EKataGraphTriggerResult::NoMatch && Graph->GetCurrentNode() == ViaAlias);
    Graph->RequestEnd(EKataEndReason::Interrupted);

    // 첫 액션을 기다리는 동안에는 진입 노드만 평가하고 Alias는 평가하지 않는다.
    FGraphBuilder WaitingBuilder;
    WaitingBuilder.Link(WaitingBuilder.Entry(), WaitingBuilder.Action(MakeRejectedAction(Tags)));
    WaitingBuilder.Link(WaitingBuilder.AnyState(), WaitingBuilder.Action(MakeRunningAction(Tags)), Tags.Dodge);
    UKataGraphInstance* Waiting = Fixture.Start(WaitingBuilder.Graph);
    TestTrue(TEXT("Waiting graph ignores Any State"),
        Waiting->GetState() == EKataGraphInstanceState::WaitingForEntry
        && Waiting->SendTriggerWithResult(Tags.Dodge) == EKataGraphTriggerResult::NoMatch);
    Waiting->RequestEnd(EKataEndReason::Interrupted);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataGraphSubGraphEntryTest, "Kata.Graph.Transition.SubGraphEntry", KataGraphTransitionTests::Flags)
bool FKataGraphSubGraphEntryTest::RunTest(const FString& Parameters)
{
    using namespace KataGraphTransitionTests;
    FFixture Fixture;
    const FTags& Tags = Fixture.Tags;

    // 에디터가 SubGraph를 펼친 뒤의 모양: 복사한 진입 노드는 bIsSubGraphEntry이고 바깥 엣지가 그 진입 노드로 들어온다.
    FGraphBuilder Builder;
    UKataActionNode* A = Builder.Action(MakeRunningAction(Tags));
    UKataEntryNode* SubEntry = Builder.Entry(true);
    UKataActionNode* SubFirst = Builder.Action(MakeRunningAction(Tags));
    Builder.Link(Builder.Entry(), A, Tags.Dodge);
    Builder.Link(SubEntry, SubFirst);
    Builder.Link(A, SubEntry, Tags.Attack);

    UKataGraphInstance* Graph = Fixture.Start(Builder.Graph);
    TestTrue(TEXT("SubGraph entry does not start the graph"),
        Graph->GetState() == EKataGraphInstanceState::WaitingForEntry && !Graph->HasStartedAction());
    TestTrue(TEXT("Main entry starts the graph"), Graph->SendTriggerWithResult(Tags.Dodge) == EKataGraphTriggerResult::Transitioned
        && Graph->GetCurrentNode() == A);
    TestTrue(TEXT("Incoming edge passes through the SubGraph entry"),
        Graph->SendTriggerWithResult(Tags.Attack) == EKataGraphTriggerResult::Transitioned && Graph->GetCurrentNode() == SubFirst);
    Graph->RequestEnd(EKataEndReason::Interrupted);

    // 진입 노드에서 나가는 엣지는 비어 있거나 들어온 트리거와 맞아야 지나갈 수 있다.
    FGraphBuilder MismatchBuilder;
    UKataActionNode* Outer = MismatchBuilder.Action(MakeRunningAction(Tags));
    UKataEntryNode* MismatchEntry = MismatchBuilder.Entry(true);
    MismatchBuilder.Link(MismatchBuilder.Entry(), Outer);
    MismatchBuilder.Link(MismatchEntry, MismatchBuilder.Action(MakeRunningAction(Tags)), Tags.Dodge);
    MismatchBuilder.Link(Outer, MismatchEntry, Tags.Attack);
    UKataGraphInstance* Mismatch = Fixture.Start(MismatchBuilder.Graph);
    TestTrue(TEXT("SubGraph entry edge with another trigger blocks the pass"),
        Mismatch->SendTriggerWithResult(Tags.Attack) == EKataGraphTriggerResult::NoMatch && Mismatch->GetCurrentNode() == Outer);
    Mismatch->RequestEnd(EKataEndReason::Interrupted);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataGraphDepthTest, "Kata.Graph.Transition.SynchronousDepth", KataGraphTransitionTests::Flags)
bool FKataGraphDepthTest::RunTest(const FString& Parameters)
{
    using namespace KataGraphTransitionTests;
    FFixture Fixture;
    const FTags& Tags = Fixture.Tags;
    AddExpectedErrorPlain(TEXT("Kata graph exceeded the synchronous transition limit"), EAutomationExpectedErrorFlags::Contains, 2);

    // 순간 액션이 이어지면 액션 시작이 한 호출 안에 중첩된다. 한도는 중첩된 시작 32번이다.
    auto BuildChain = [&Tags](int32 InstantCount, UKataActionNode*& OutLast)
    {
        FGraphBuilder Builder;
        UKataGraphNodeBase* Previous = Builder.Entry();
        for (int32 Index = 0; Index < InstantCount; ++Index)
        {
            UKataActionNode* Instant = Builder.Action(MakeInstantAction());
            Builder.Link(Previous, Instant);
            Previous = Instant;
        }
        OutLast = Builder.Action(MakeRunningAction(Tags));
        Builder.Link(Previous, OutLast);
        return Builder.Graph;
    };

    UKataActionNode* Last = nullptr;
    UKataGraphInstance* WithinLimit = Fixture.Start(BuildChain(31, Last));
    TestTrue(TEXT("32 nested starts reach the running action"), WithinLimit->IsRunning() && WithinLimit->GetCurrentNode() == Last);
    WithinLimit->RequestEnd(EKataEndReason::Interrupted);

    UKataGraphInstance* OverLimit = Fixture.Start(BuildChain(32, Last));
    TestTrue(TEXT("33rd nested start ends the graph with ContractError"),
        OverLimit->GetState() == EKataGraphInstanceState::Ended && OverLimit->GetEndReason() == EKataEndReason::ContractError);

    // 자동 엣지로 자기 자신에게 돌아오는 순간 액션은 한도에서 멈춘다.
    FGraphBuilder LoopBuilder;
    UKataActionNode* Loop = LoopBuilder.Action(MakeInstantAction());
    LoopBuilder.Link(LoopBuilder.Entry(), Loop);
    LoopBuilder.Link(Loop, Loop);
    UKataGraphInstance* Looping = Fixture.Start(LoopBuilder.Graph);
    TestTrue(TEXT("Instant self loop ends with ContractError"),
        Looping->GetState() == EKataGraphInstanceState::Ended && Looping->GetEndReason() == EKataEndReason::ContractError);
    return true;
}

#endif // 개발용 자동화 테스트
