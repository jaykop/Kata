#include "HitTrace/KataHurtBoxComponent.h"

#include "CollisionShape.h"
#include "HitTrace/KataHitTraceSettings.h"
#include "PhysicsEngine/BodySetup.h"
#include "PrimitiveDrawingUtils.h"
#include "PrimitiveSceneProxy.h"
#include "PrimitiveViewRelevance.h"
#include "SceneManagement.h"
#include "SceneView.h"

namespace KataHurtBox
{
    // 스케일 계산은 엔진 FKSphereElem·FKSphylElem·FKBoxElem의 GetFinalScaled와 같은 규칙을 따른다.
    // 컴포넌트 함수와 렌더 스레드의 프록시가 함께 쓰도록 값만 받는 함수로 둔다.

    float ScaleSphereRadius(float Radius, const FVector& Scale)
    {
        return Radius * static_cast<float>(Scale.GetAbs().GetMin());
    }

    float ScaleCapsuleHalfHeight(float Radius, float HalfHeight, const FVector& Scale)
    {
        return FMath::Max(FMath::Max(HalfHeight, Radius) * static_cast<float>(FMath::Abs(Scale.Z)), 0.1f);
    }

    float ScaleCapsuleRadius(float Radius, float HalfHeight, const FVector& Scale)
    {
        const FVector ScaleAbs = Scale.GetAbs();
        return FMath::Clamp(Radius * static_cast<float>(FMath::Max(ScaleAbs.X, ScaleAbs.Y)), 0.1f, ScaleCapsuleHalfHeight(Radius, HalfHeight, Scale));
    }

    FVector ScaleBoxExtent(const FVector& Extent, const FVector& Scale)
    {
        return Extent * Scale.GetAbs();
    }

    /** 에디터 뷰포트에서 피격 영역을 선으로 그린다. 게임에서는 bHiddenInGame이면 보이지 않고, show Collision에서는 보인다. */
    class FSceneProxy final : public FPrimitiveSceneProxy
    {
    public:
        explicit FSceneProxy(const UKataHurtBoxComponent* Component)
            : FPrimitiveSceneProxy(Component)
            , Shape(Component->Shape)
            , SphereRadius(Component->SphereRadius)
            , CapsuleRadius(Component->CapsuleRadius)
            , CapsuleHalfHeight(Component->CapsuleHalfHeight)
            , BoxExtent(Component->BoxExtent)
            , Color(Component->ShapeColor)
        {
            bWillEverBeLit = false;
        }

        virtual SIZE_T GetTypeHash() const override
        {
            static size_t UniquePointer;
            return reinterpret_cast<size_t>(&UniquePointer);
        }

        virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap,
            FMeshElementCollector& Collector) const override
        {
            const FMatrix& ShapeToWorld = GetLocalToWorld();
            const FVector Scale(ShapeToWorld.GetScaledAxis(EAxis::X).Size(), ShapeToWorld.GetScaledAxis(EAxis::Y).Size(), ShapeToWorld.GetScaledAxis(EAxis::Z).Size());
            const FVector Origin = ShapeToWorld.GetOrigin();
            const FVector AxisX = ShapeToWorld.GetUnitAxis(EAxis::X);
            const FVector AxisY = ShapeToWorld.GetUnitAxis(EAxis::Y);
            const FVector AxisZ = ShapeToWorld.GetUnitAxis(EAxis::Z);

            for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
            {
                if ((VisibilityMap & (1 << ViewIndex)) == 0)
                {
                    continue;
                }
                const FSceneView* View = Views[ViewIndex];
                FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
                const FLinearColor DrawColor = GetViewSelectionColor(Color, *View, IsSelected(), IsHovered(), false, IsIndividuallySelected());

                switch (Shape)
                {
                case EKataHurtBoxShape::Sphere:
                {
                    const float Radius = ScaleSphereRadius(SphereRadius, Scale);
                    const int32 Sides = FMath::Clamp(static_cast<int32>(Radius / 4.0f), 16, 64);
                    DrawCircle(PDI, Origin, AxisX, AxisY, DrawColor, Radius, Sides, SDPG_World);
                    DrawCircle(PDI, Origin, AxisX, AxisZ, DrawColor, Radius, Sides, SDPG_World);
                    DrawCircle(PDI, Origin, AxisY, AxisZ, DrawColor, Radius, Sides, SDPG_World);
                    break;
                }
                case EKataHurtBoxShape::Capsule:
                    DrawWireCapsule(PDI, Origin, AxisX, AxisY, AxisZ, DrawColor,
                        ScaleCapsuleRadius(CapsuleRadius, CapsuleHalfHeight, Scale), ScaleCapsuleHalfHeight(CapsuleRadius, CapsuleHalfHeight, Scale), 16, SDPG_World);
                    break;
                case EKataHurtBoxShape::Box:
                default:
                    DrawOrientedWireBox(PDI, Origin, AxisX, AxisY, AxisZ, ScaleBoxExtent(BoxExtent, Scale), DrawColor, SDPG_World);
                    break;
                }
            }
        }

        virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
        {
            const bool bShowForCollision = View->Family->EngineShowFlags.Collision && IsCollisionEnabled();

            FPrimitiveViewRelevance Result;
            Result.bDrawRelevance = IsShown(View) || bShowForCollision;
            Result.bDynamicRelevance = true;
            Result.bShadowRelevance = false;
            Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
            return Result;
        }

        virtual uint32 GetMemoryFootprint() const override
        {
            return sizeof(*this) + GetAllocatedSize();
        }

    private:
        const EKataHurtBoxShape Shape;
        const float SphereRadius;
        const float CapsuleRadius;
        const float CapsuleHalfHeight;
        const FVector BoxExtent;
        const FColor Color;
    };
}

UKataHurtBoxComponent::UKataHurtBoxComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 프로젝트가 Kata Hit Trace 설정에서 고른 프로필로 시작한다. 판정도 같은 프로필의 Object Type으로 찾으므로 둘이 저절로 맞는다.
    // 프로필이 없으면 질의로 찾히기만 하고 어떤 채널에도 반응하지 않는 기본값을 쓴다. 판정은 이때 WorldDynamic으로 찾는다.
    const FName ProfileName = UKataHitTraceSettings::Get()->GetValidHurtBoxProfileName();
    if (!ProfileName.IsNone())
    {
        SetCollisionProfileName(ProfileName);
    }
    else
    {
        SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        SetCollisionObjectType(ECC_WorldDynamic);
        SetCollisionResponseToAllChannels(ECR_Ignore);
    }
    SetGenerateOverlapEvents(false);
    CanCharacterStepUpOn = ECB_No;
    SetCanEverAffectNavigation(false);

    bHiddenInGame = true;
    CastShadow = false;
    bCastDynamicShadow = false;
    bExcludeFromLightAttachmentGroup = true;
    bUseEditorCompositing = true;
}

void UKataHurtBoxComponent::AddHurtBoxTag(FGameplayTag Tag)
{
    HurtBoxTags.AddTag(Tag);
}

void UKataHurtBoxComponent::RemoveHurtBoxTag(FGameplayTag Tag)
{
    HurtBoxTags.RemoveTag(Tag);
}

float UKataHurtBoxComponent::GetScaledSphereRadius() const
{
    return KataHurtBox::ScaleSphereRadius(SphereRadius, GetComponentTransform().GetScale3D());
}

float UKataHurtBoxComponent::GetScaledCapsuleRadius() const
{
    return KataHurtBox::ScaleCapsuleRadius(CapsuleRadius, CapsuleHalfHeight, GetComponentTransform().GetScale3D());
}

float UKataHurtBoxComponent::GetScaledCapsuleHalfHeight() const
{
    return KataHurtBox::ScaleCapsuleHalfHeight(CapsuleRadius, CapsuleHalfHeight, GetComponentTransform().GetScale3D());
}

FVector UKataHurtBoxComponent::GetScaledBoxExtent() const
{
    return KataHurtBox::ScaleBoxExtent(BoxExtent, GetComponentTransform().GetScale3D());
}

FPrimitiveSceneProxy* UKataHurtBoxComponent::CreateSceneProxy()
{
    return new KataHurtBox::FSceneProxy(this);
}

UBodySetup* UKataHurtBoxComponent::GetBodySetup()
{
    // 엔진은 OnCreatePhysicsState 도중(물리 상태 생성 표시가 이미 켜진 뒤)에 이 함수를 부른다.
    // 여기서 물리 상태를 다시 만들면 바디가 이중으로 초기화되고 버려진 바디가 씬에 남으므로 BodySetup만 채운다.
    // 템플릿에서 복사된 참조처럼 다른 컴포넌트가 소유한 BodySetup은 공유하지 않고 새로 만든다.
    if (!IsValid(HurtBoxBodySetup) || HurtBoxBodySetup->GetOuter() != this)
    {
        HurtBoxBodySetup = nullptr;
        RebuildBodySetup();
    }
    return HurtBoxBodySetup;
}

FBoxSphereBounds UKataHurtBoxComponent::CalcBounds(const FTransform& LocalToWorld) const
{
    const FVector Scale = LocalToWorld.GetScale3D();
    FVector WorldExtent;
    switch (Shape)
    {
    case EKataHurtBoxShape::Sphere:
        WorldExtent = FVector(KataHurtBox::ScaleSphereRadius(SphereRadius, Scale));
        return FBoxSphereBounds(LocalToWorld.GetLocation(), WorldExtent, WorldExtent.X);
    case EKataHurtBoxShape::Capsule:
    {
        const float Radius = KataHurtBox::ScaleCapsuleRadius(CapsuleRadius, CapsuleHalfHeight, Scale);
        const float HalfHeight = KataHurtBox::ScaleCapsuleHalfHeight(CapsuleRadius, CapsuleHalfHeight, Scale);
        WorldExtent = FVector(Radius, Radius, HalfHeight);
        break;
    }
    case EKataHurtBoxShape::Box:
    default:
        WorldExtent = KataHurtBox::ScaleBoxExtent(BoxExtent, Scale);
        break;
    }
    // 스케일은 이미 크기에 반영했으므로 회전과 위치만으로 경계를 옮긴다.
    const FTransform RigidTransform(LocalToWorld.GetRotation(), LocalToWorld.GetLocation());
    return FBoxSphereBounds(FBox(-WorldExtent, WorldExtent)).TransformBy(RigidTransform);
}

bool UKataHurtBoxComponent::IsZeroExtent() const
{
    switch (Shape)
    {
    case EKataHurtBoxShape::Sphere:
        return SphereRadius <= 0.0f;
    case EKataHurtBoxShape::Capsule:
        return CapsuleRadius <= 0.0f && CapsuleHalfHeight <= 0.0f;
    case EKataHurtBoxShape::Box:
    default:
        return BoxExtent.IsZero();
    }
}

FCollisionShape UKataHurtBoxComponent::GetCollisionShape(float Inflation) const
{
    switch (Shape)
    {
    case EKataHurtBoxShape::Sphere:
        return FCollisionShape::MakeSphere(GetScaledSphereRadius()).Inflate(Inflation);
    case EKataHurtBoxShape::Capsule:
        return FCollisionShape::MakeCapsule(GetScaledCapsuleRadius(), GetScaledCapsuleHalfHeight()).Inflate(Inflation);
    case EKataHurtBoxShape::Box:
    default:
        return FCollisionShape::MakeBox(GetScaledBoxExtent()).Inflate(Inflation);
    }
}

void UKataHurtBoxComponent::RebuildBodySetup()
{
    // 엔진 UShapeComponent의 BodySetup 생성 도우미는 모듈 밖으로 공개되지 않아, 같은 절차를 여기에서 직접 한다.
    // 도형 종류를 바꿀 수 있어야 하므로 요소 배열을 매번 비우고 현재 도형 하나만 넣는다.
    if (!IsValid(HurtBoxBodySetup))
    {
        HurtBoxBodySetup = NewObject<UBodySetup>(this, NAME_None, RF_Transient);
        HurtBoxBodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
        HurtBoxBodySetup->bNeverNeedsCookedCollisionData = true;
    }

    FKAggregateGeom& Geometry = HurtBoxBodySetup->AggGeom;
    Geometry.EmptyElements();
    switch (Shape)
    {
    case EKataHurtBoxShape::Sphere:
        Geometry.SphereElems.Add(FKSphereElem(SphereRadius));
        break;
    case EKataHurtBoxShape::Capsule:
    {
        const float HalfHeight = FMath::Max(CapsuleHalfHeight, CapsuleRadius);
        Geometry.SphylElems.Add(FKSphylElem(CapsuleRadius, (HalfHeight - CapsuleRadius) * 2.0f));
        break;
    }
    case EKataHurtBoxShape::Box:
    default:
        Geometry.BoxElems.Add(FKBoxElem(BoxExtent.X * 2.0f, BoxExtent.Y * 2.0f, BoxExtent.Z * 2.0f));
        break;
    }

    BodyInstance.BodySetup = HurtBoxBodySetup;
}

#if WITH_EDITOR
void UKataHurtBoxComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    const FName PropertyName = PropertyChangedEvent.GetMemberPropertyName();
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UKataHurtBoxComponent, Shape)
        || PropertyName == GET_MEMBER_NAME_CHECKED(UKataHurtBoxComponent, SphereRadius)
        || PropertyName == GET_MEMBER_NAME_CHECKED(UKataHurtBoxComponent, CapsuleRadius)
        || PropertyName == GET_MEMBER_NAME_CHECKED(UKataHurtBoxComponent, CapsuleHalfHeight)
        || PropertyName == GET_MEMBER_NAME_CHECKED(UKataHurtBoxComponent, BoxExtent))
    {
        // 편집으로 도형이 바뀌면 BodySetup을 고친 뒤 이미 만든 바디를 새 도형으로 다시 만든다.
        RebuildBodySetup();
        if (IsPhysicsStateCreated())
        {
            RecreatePhysicsState();
        }
        UpdateBounds();
        MarkRenderStateDirty();
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UKataHurtBoxComponent, ShapeColor))
    {
        MarkRenderStateDirty();
    }
    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
