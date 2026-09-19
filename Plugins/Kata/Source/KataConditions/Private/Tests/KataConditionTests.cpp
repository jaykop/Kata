#include "Conditions/KataCondition_Angle.h"
#include "Conditions/KataCondition_Attribute.h"
#include "Conditions/KataCondition_Distance.h"
#include "Conditions/KataCondition_Tag.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystemComponent.h"
#include "AbilitySystemTestAttributeSet.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "NativeGameplayTags.h"
#include <limits>

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_KataTest_State, "Kata.Tests.Condition.State");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_KataTest_State_Child, "Kata.Tests.Condition.State.Child");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_KataTest_Other, "Kata.Tests.Condition.Other");

namespace KataConditionTests
{
    constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

    struct FFixture
    {
        UWorld* World;
        AActor* Self;
        AActor* Target;
        UAbilitySystemComponent* SelfASC;
        UAbilitySystemComponent* TargetASC;
        FKataConditionContext Context;

        FFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
                .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
                .CreateFXSystem(false).SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
            check(World);
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            Self = SpawnActor();
            Target = SpawnActor();
            SelfASC = AddASC(Self);
            TargetASC = AddASC(Target);
            Context.SelfActor = Self;
            Context.TargetActor = Target;
        }

        ~FFixture()
        {
            GEngine->DestroyWorldContext(World);
            World->DestroyWorld(false);
        }

        AActor* SpawnActor()
        {
            AActor* Actor = World->SpawnActor<AActor>();
            check(Actor);
            USceneComponent* Root = NewObject<USceneComponent>(Actor);
            Actor->AddInstanceComponent(Root);
            Actor->SetRootComponent(Root);
            Root->RegisterComponent();
            return Actor;
        }

        static UAbilitySystemComponent* AddASC(AActor* Actor)
        {
            UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Actor);
            Actor->AddInstanceComponent(ASC);
            ASC->RegisterComponent();
            ASC->InitAbilityActorInfo(Actor, Actor);
            return ASC;
        }

        static UAbilitySystemTestAttributeSet* AddAttributes(UAbilitySystemComponent* ASC)
        {
            UAbilitySystemTestAttributeSet* Attributes = NewObject<UAbilitySystemTestAttributeSet>(ASC->GetOwner());
            ASC->AddAttributeSetSubobject(Attributes);
            Attributes->Health = 30.0f;
            Attributes->MaxHealth = 100.0f;
            return Attributes;
        }

        static UStaticMeshComponent* AddSocket(AActor* Actor, FName Tag, FName Name, FVector Offset)
        {
            UStaticMesh* Mesh = NewObject<UStaticMesh>();
            UStaticMeshSocket* Socket = NewObject<UStaticMeshSocket>(Mesh);
            Socket->SocketName = Name;
            Socket->RelativeLocation = Offset;
            Socket->RelativeScale = FVector::OneVector;
            Mesh->Sockets.Add(Socket);

            UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(Actor);
            Actor->AddInstanceComponent(Component);
            Component->ComponentTags.Add(Tag);
            Component->SetStaticMesh(Mesh);
            Component->SetWorldTransform(Actor->GetActorTransform());
            // Socket transforms do not require render/physics registration or cooked mesh data.
            return Component;
        }
    };

    FGameplayAttribute Attribute(FName Name)
    {
        FProperty* Property = FindFProperty<FProperty>(UAbilitySystemTestAttributeSet::StaticClass(), Name);
        check(Property);
        return FGameplayAttribute(Property);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataTagConditionTest, "Kata.Conditions.Tag.MatchingAndSubject", KataConditionTests::Flags)
bool FKataTagConditionTest::RunTest(const FString& Parameters)
{
    KataConditionTests::FFixture Fixture;
    UKataCondition_Tag* Condition = NewObject<UKataCondition_Tag>();
    Fixture.SelfASC->AddLooseGameplayTag(TAG_KataTest_State_Child);
    Fixture.TargetASC->AddLooseGameplayTag(TAG_KataTest_Other);
    Condition->Tags.AddTag(TAG_KataTest_State);

    TestTrue(TEXT("Owned child satisfies requested parent"), Condition->IsSatisfied(Fixture.Context));
    Condition->bExactMatch = true;
    TestFalse(TEXT("Exact match does not accept parent"), Condition->IsSatisfied(Fixture.Context));
    Condition->Tags.Reset();
    Condition->Tags.AddTag(TAG_KataTest_State_Child);
    TestTrue(TEXT("Exact child matches"), Condition->IsSatisfied(Fixture.Context));
    Condition->Tags.AddTag(TAG_KataTest_Other);
    TestTrue(TEXT("Any accepts one of two tags"), Condition->IsSatisfied(Fixture.Context));
    Condition->MatchMode = EKataTagMatchMode::All;
    TestFalse(TEXT("All rejects missing tag"), Condition->IsSatisfied(Fixture.Context));
    Fixture.SelfASC->AddLooseGameplayTag(TAG_KataTest_Other);
    TestTrue(TEXT("All accepts both tags"), Condition->IsSatisfied(Fixture.Context));

    Condition->Tags.Reset();
    Condition->Tags.AddTag(TAG_KataTest_Other);
    Condition->Subject = EKataConditionSubject::Target;
    TestTrue(TEXT("Target subject reads target ASC"), Condition->IsSatisfied(Fixture.Context));
    Fixture.Context.TargetAbilitySystem = Fixture.SelfASC;
    Fixture.TargetASC->RemoveLooseGameplayTag(TAG_KataTest_Other);
    TestTrue(TEXT("Explicit ASC overrides actor lookup"), Condition->IsSatisfied(Fixture.Context));
    Fixture.Context.TargetActor.Reset();
    TestTrue(TEXT("Explicit ASC works without actor"), Condition->IsSatisfied(Fixture.Context));
    Fixture.Context.TargetAbilitySystem.Reset();
    TestEqual(TEXT("Missing ASC is diagnostic"), Condition->Evaluate(Fixture.Context).Reason, FName(TEXT("MissingAbilitySystem")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataInvertConditionTest, "Kata.Conditions.Base.InvertAndInvalid", KataConditionTests::Flags)
bool FKataInvertConditionTest::RunTest(const FString& Parameters)
{
    KataConditionTests::FFixture Fixture;
    UKataCondition_Tag* Condition = NewObject<UKataCondition_Tag>();
    Condition->Tags.AddTag(TAG_KataTest_Other);
    Condition->bInvert = true;
    TestTrue(TEXT("Absent tag is satisfied by inversion"), Condition->IsSatisfied(Fixture.Context));
    Fixture.SelfASC->AddLooseGameplayTag(TAG_KataTest_Other);
    TestEqual(TEXT("Present tag becomes Fail"), Condition->Evaluate(Fixture.Context).Status, EKataConditionStatus::Fail);
    TestEqual(TEXT("Inverted result has diagnostic"), Condition->Evaluate(Fixture.Context).Reason, FName(TEXT("InvertedCondition")));
    Condition->Tags.Reset();
    TestEqual(TEXT("Empty Any stays Invalid under inversion"), Condition->Evaluate(Fixture.Context).Status, EKataConditionStatus::Invalid);
    Condition->MatchMode = EKataTagMatchMode::All;
    TestEqual(TEXT("Empty All is invalid configuration"), Condition->Evaluate(Fixture.Context).Reason, FName(TEXT("EmptyTags")));
    Condition->Tags.AddTag(TAG_KataTest_Other);
    TestEqual(TEXT("Missing data stays Invalid"), Condition->Evaluate(FKataConditionContext()).Status, EKataConditionStatus::Invalid);

    UKataCondition_Distance* Distance = NewObject<UKataCondition_Distance>();
    Distance->bInvert = true;
    Fixture.Context.TargetActor.Reset();
    TestEqual(TEXT("Missing target cannot pass inverted distance"), Distance->Evaluate(Fixture.Context).Status, EKataConditionStatus::Invalid);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataAttributeAbsoluteTest, "Kata.Conditions.Attribute.AbsoluteAndComparisons", KataConditionTests::Flags)
bool FKataAttributeAbsoluteTest::RunTest(const FString& Parameters)
{
    KataConditionTests::FFixture Fixture;
    UAbilitySystemTestAttributeSet* Attributes = Fixture.AddAttributes(Fixture.SelfASC);
    UKataCondition_Attribute* Condition = NewObject<UKataCondition_Attribute>();
    Condition->Attribute = KataConditionTests::Attribute(TEXT("Health"));
    Condition->CompareValue = 30.0f;

    const EKataNumericComparison Operators[] = {
        EKataNumericComparison::LessThan, EKataNumericComparison::LessOrEqual,
        EKataNumericComparison::GreaterThan, EKataNumericComparison::GreaterOrEqual,
        EKataNumericComparison::Equal, EKataNumericComparison::NotEqual
    };
    const bool ExpectedAtBoundary[] = { false, true, false, true, true, false };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Operators); ++Index)
    {
        Condition->Comparison = Operators[Index];
        TestEqual(FString::Printf(TEXT("Comparison %d handles equality boundary"), Index), Condition->IsSatisfied(Fixture.Context), ExpectedAtBoundary[Index]);
    }

    Condition->Comparison = EKataNumericComparison::Equal;
    Condition->CompareValue = 30.005f;
    Condition->EqualityTolerance = 0.01f;
    TestTrue(TEXT("Equal uses explicit tolerance"), Condition->IsSatisfied(Fixture.Context));
    Condition->Comparison = EKataNumericComparison::NotEqual;
    TestFalse(TEXT("NotEqual complements tolerant Equal"), Condition->IsSatisfied(Fixture.Context));
    Condition->Comparison = EKataNumericComparison::GreaterOrEqual;
    Condition->CompareValue = 1.0f;
    Condition->Subject = EKataConditionSubject::Target;
    TestEqual(TEXT("Uninstalled AttributeSet is Invalid"), Condition->Evaluate(Fixture.Context).Reason, FName(TEXT("MissingAttribute")));

    Condition->Subject = EKataConditionSubject::Self;
    Condition->Attribute = KataConditionTests::Attribute(TEXT("Mana"));
    Attributes->Mana.SetBaseValue(100.0f);
    Attributes->Mana.SetCurrentValue(25.0f);
    Condition->CompareValue = 50.0f;
    TestFalse(TEXT("Reads current GAS value rather than base"), Condition->IsSatisfied(Fixture.Context));
    Attributes->Mana.SetCurrentValue(std::numeric_limits<float>::quiet_NaN());
    Condition->bInvert = true;
    TestEqual(TEXT("Nonfinite attribute cannot become a valid inverse"), Condition->Evaluate(Fixture.Context).Reason, FName(TEXT("NonFiniteAttribute")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataAttributeRatioTest, "Kata.Conditions.Attribute.RatioAndInvalidMaximum", KataConditionTests::Flags)
bool FKataAttributeRatioTest::RunTest(const FString& Parameters)
{
    KataConditionTests::FFixture Fixture;
    UAbilitySystemTestAttributeSet* Attributes = Fixture.AddAttributes(Fixture.SelfASC);
    UKataCondition_Attribute* Condition = NewObject<UKataCondition_Attribute>();
    Condition->Attribute = KataConditionTests::Attribute(TEXT("Health"));
    Condition->MaxAttribute = KataConditionTests::Attribute(TEXT("MaxHealth"));
    Condition->Mode = EKataAttributeMode::Ratio;
    Condition->CompareValue = 0.3f;
    Condition->Comparison = EKataNumericComparison::GreaterOrEqual;
    TestTrue(TEXT("30/100 includes 0.3 lower boundary"), Condition->IsSatisfied(Fixture.Context));
    Condition->Comparison = EKataNumericComparison::LessOrEqual;
    TestTrue(TEXT("30/100 includes 0.3 upper boundary"), Condition->IsSatisfied(Fixture.Context));
    Attributes->Health = 150.0f;
    Condition->CompareValue = 1.2f;
    Condition->Comparison = EKataNumericComparison::GreaterThan;
    TestTrue(TEXT("Ratio is not clamped at 100 percent"), Condition->IsSatisfied(Fixture.Context));
    Condition->bInvert = true;
    for (const float Maximum : { 0.0f, -1.0f, std::numeric_limits<float>::infinity() })
    {
        Attributes->MaxHealth = Maximum;
        TestEqual(TEXT("Invalid denominator is not inverted"), Condition->Evaluate(Fixture.Context).Reason, FName(TEXT("InvalidRatioMaximum")));
    }
    Condition->MaxAttribute = FGameplayAttribute();
    TestEqual(TEXT("Missing Max selector is Invalid"), Condition->Evaluate(Fixture.Context).Reason, FName(TEXT("InvalidMaxAttribute")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataDistanceConditionTest, "Kata.Conditions.Distance.PlanarSpatialAndRange", KataConditionTests::Flags)
bool FKataDistanceConditionTest::RunTest(const FString& Parameters)
{
    KataConditionTests::FFixture Fixture;
    UKataCondition_Distance* Condition = NewObject<UKataCondition_Distance>();
    Fixture.Target->SetActorLocation(FVector(300.0, 400.0, 1200.0));
    Condition->MinDistance = 500.0f;
    Condition->MaxDistance = 500.0f;
    TestTrue(TEXT("2D ignores height and includes both range endpoints"), Condition->IsSatisfied(Fixture.Context));
    Condition->Space = EKataConditionSpace::Spatial3D;
    TestFalse(TEXT("3D includes height"), Condition->IsSatisfied(Fixture.Context));
    Condition->MinDistance = 1300.0f;
    Condition->MaxDistance = 1300.0f;
    TestTrue(TEXT("3D distance is 1300 cm"), Condition->IsSatisfied(Fixture.Context));
    Fixture.Target->SetActorLocation(FVector::ZeroVector);
    Condition->MinDistance = 0.0f;
    Condition->MaxDistance = 0.0f;
    TestTrue(TEXT("Coincident actors have valid zero distance"), Condition->IsSatisfied(Fixture.Context));
    Condition->MinDistance = 100.0f;
    Condition->MaxDistance = 50.0f;
    Condition->bInvert = true;
    TestEqual(TEXT("Reversed range remains invalid"), Condition->Evaluate(Fixture.Context).Reason, FName(TEXT("InvalidDistanceRange")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataDistanceSocketTest, "Kata.Conditions.Distance.SocketsAndAmbiguity", KataConditionTests::Flags)
bool FKataDistanceSocketTest::RunTest(const FString& Parameters)
{
    KataConditionTests::FFixture Fixture;
    Fixture.Target->SetActorLocation(FVector(300.0, 0.0, 0.0));
    Fixture.AddSocket(Fixture.Self, TEXT("Weapon"), TEXT("Tip"), FVector(100.0, 0.0, 0.0));
    Fixture.AddSocket(Fixture.Target, TEXT("Body"), TEXT("Chest"), FVector(-50.0, 0.0, 300.0));
    UKataCondition_Distance* Condition = NewObject<UKataCondition_Distance>();
    Condition->SelfLocation.Mode = EKataLocationMode::Socket;
    Condition->SelfLocation.ComponentTag = TEXT("Weapon");
    Condition->SelfLocation.SocketName = TEXT("Tip");
    Condition->MinDistance = 200.0f;
    Condition->MaxDistance = 200.0f;
    TestTrue(TEXT("Self socket to target actor distance"), Condition->IsSatisfied(Fixture.Context));
    Condition->TargetLocation.Mode = EKataLocationMode::Socket;
    Condition->TargetLocation.ComponentTag = TEXT("Body");
    Condition->TargetLocation.SocketName = TEXT("Chest");
    Condition->MinDistance = 150.0f;
    Condition->MaxDistance = 150.0f;
    TestTrue(TEXT("Both endpoints use sockets before 2D projection"), Condition->IsSatisfied(Fixture.Context));
    Condition->SelfLocation.SocketName = TEXT("Missing");
    Condition->bInvert = true;
    TestEqual(TEXT("Unknown socket does not fall back to component origin"), Condition->Evaluate(Fixture.Context).Reason, FName(TEXT("MissingSocket")));
    Condition->SelfLocation.SocketName = TEXT("Tip");
    Condition->SelfLocation.ComponentTag = NAME_None;
    TestEqual(TEXT("Non-character needs explicit component tag"), Condition->Evaluate(Fixture.Context).Reason, FName(TEXT("MissingSocketComponent")));
    Condition->SelfLocation.ComponentTag = TEXT("Weapon");
    Fixture.AddSocket(Fixture.Self, TEXT("Weapon"), TEXT("Tip"), FVector::ZeroVector);
    TestEqual(TEXT("Duplicate component tags are rejected"), Condition->Evaluate(Fixture.Context).Reason, FName(TEXT("AmbiguousComponentTag")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataAnglePlanarTest, "Kata.Conditions.Angle.PlanarYawAndBoundaries", KataConditionTests::Flags)
bool FKataAnglePlanarTest::RunTest(const FString& Parameters)
{
    KataConditionTests::FFixture Fixture;
    UKataCondition_Angle* Condition = NewObject<UKataCondition_Angle>();
    Fixture.Target->SetActorLocation(FVector(100.0, 100.0, 1000.0));
    TestTrue(TEXT("45-degree edge included and height ignored in 2D"), Condition->IsSatisfied(Fixture.Context));
    Fixture.Target->SetActorLocation(FVector(99.0, 100.0, 0.0));
    TestFalse(TEXT("Outside cone fails"), Condition->IsSatisfied(Fixture.Context));
    Fixture.Target->SetActorLocation(FVector(0.0, 100.0, 0.0));
    Condition->HalfAngleDegrees = 0.0f;
    Condition->YawOffsetDegrees = 90.0f;
    TestTrue(TEXT("Positive yaw turns toward right (+Y)"), Condition->IsSatisfied(Fixture.Context));
    Condition->YawOffsetDegrees = -90.0f;
    TestFalse(TEXT("Negative yaw faces the other side"), Condition->IsSatisfied(Fixture.Context));
    Fixture.Target->SetActorLocation(FVector(0.0, -100.0, 0.0));
    TestTrue(TEXT("Negative yaw turns toward left"), Condition->IsSatisfied(Fixture.Context));
    Fixture.Self->SetActorRotation(FRotator(0.0, 90.0, 0.0));
    Fixture.Target->SetActorLocation(FVector(-100.0, 0.0, 0.0));
    Condition->YawOffsetDegrees = 90.0f;
    TestTrue(TEXT("Offset is relative to self forward"), Condition->IsSatisfied(Fixture.Context));
    Condition->YawOffsetDegrees = 450.0f;
    TestTrue(TEXT("Yaw wraps consistently"), Condition->IsSatisfied(Fixture.Context));
    Fixture.Target->SetActorLocation(FVector(0.0, 0.0, 50.0));
    Condition->bInvert = true;
    TestEqual(TEXT("Vertical-only separation has no 2D target direction"), Condition->Evaluate(Fixture.Context).Reason, FName(TEXT("UndefinedTargetDirection")));
    Fixture.Self->SetActorRotation(FRotator(90.0, 0.0, 0.0));
    Fixture.Target->SetActorLocation(FVector(100.0, 0.0, 0.0));
    TestEqual(TEXT("Vertical forward has no planar direction"), Condition->Evaluate(Fixture.Context).Reason, FName(TEXT("UndefinedForwardDirection")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataAngleSpatialTest, "Kata.Conditions.Angle.SpatialAndDegenerate", KataConditionTests::Flags)
bool FKataAngleSpatialTest::RunTest(const FString& Parameters)
{
    KataConditionTests::FFixture Fixture;
    UKataCondition_Angle* Condition = NewObject<UKataCondition_Angle>();
    Condition->Space = EKataConditionSpace::Spatial3D;
    Fixture.Target->SetActorLocation(FVector(100.0, 0.0, 100.0));
    TestTrue(TEXT("3D includes 45-degree vertical cone edge"), Condition->IsSatisfied(Fixture.Context));
    Fixture.Target->SetActorLocation(FVector(100.0, 0.0, 101.0));
    TestFalse(TEXT("3D rejects excessive vertical angle"), Condition->IsSatisfied(Fixture.Context));
    Fixture.Self->SetActorRotation(FRotator(30.0, 20.0, 45.0));
    Condition->YawOffsetDegrees = 90.0f;
    Condition->HalfAngleDegrees = 0.0f;
    Fixture.Target->SetActorLocation(Fixture.Self->GetActorRightVector() * 100.0);
    TestTrue(TEXT("3D yaw rotates about self local up axis"), Condition->IsSatisfied(Fixture.Context));
    Condition->HalfAngleDegrees = 180.0f;
    Fixture.Target->SetActorLocation(-Fixture.Self->GetActorForwardVector() * 100.0);
    TestTrue(TEXT("180-degree half angle covers all directions"), Condition->IsSatisfied(Fixture.Context));
    Fixture.Target->SetActorLocation(Fixture.Self->GetActorLocation());
    Condition->bInvert = true;
    TestEqual(TEXT("Coincident actors remain invalid for angle"), Condition->Evaluate(Fixture.Context).Status, EKataConditionStatus::Invalid);
    Condition->YawOffsetDegrees = std::numeric_limits<float>::infinity();
    TestEqual(TEXT("Nonfinite offset is rejected"), Condition->Evaluate(Fixture.Context).Reason, FName(TEXT("NonFiniteYawOffset")));
    return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataConditionValidationTest, "Kata.Conditions.Base.EditorValidation", KataConditionTests::Flags)
bool FKataConditionValidationTest::RunTest(const FString& Parameters)
{
    UKataCondition_Tag* Tag = NewObject<UKataCondition_Tag>();
    FDataValidationContext TagValidation;
    TestEqual(TEXT("Empty tags fail asset validation"), Tag->IsDataValid(TagValidation), EDataValidationResult::Invalid);
    UKataCondition_Attribute* Attribute = NewObject<UKataCondition_Attribute>();
    FDataValidationContext AttributeValidation;
    TestEqual(TEXT("Unselected attribute fails asset validation"), Attribute->IsDataValid(AttributeValidation), EDataValidationResult::Invalid);
    UKataCondition_Distance* Distance = NewObject<UKataCondition_Distance>();
    Distance->MaxDistance = -1.0f;
    FDataValidationContext DistanceValidation;
    TestEqual(TEXT("Negative distance fails asset validation"), Distance->IsDataValid(DistanceValidation), EDataValidationResult::Invalid);
    UKataCondition_Angle* Angle = NewObject<UKataCondition_Angle>();
    Angle->HalfAngleDegrees = 181.0f;
    FDataValidationContext AngleValidation;
    TestEqual(TEXT("Out-of-range angle fails asset validation"), Angle->IsDataValid(AngleValidation), EDataValidationResult::Invalid);
    return true;
}
#endif

#endif // WITH_DEV_AUTOMATION_TESTS
