#include "Spawning/KataWireCylinderComponent.h"

#include "PrimitiveDrawingUtils.h"
#include "PrimitiveSceneProxy.h"
#include "PrimitiveViewRelevance.h"
#include "SceneManagement.h"
#include "SceneView.h"

namespace KataWireCylinder
{
    // 컴포넌트의 경계 계산과 렌더 스레드의 프록시가 같은 규칙을 쓰도록 값만 받는 함수로 둔다.

    float ScaleRadius(float Radius, const FVector& Scale)
    {
        return FMath::Max(Radius, 0.f) * static_cast<float>(FMath::Max(FMath::Abs(Scale.X), FMath::Abs(Scale.Y)));
    }

    float ScaleHalfHeight(float HalfHeight, const FVector& Scale)
    {
        return FMath::Max(HalfHeight, 0.f) * static_cast<float>(FMath::Abs(Scale.Z));
    }

    float Sanitize(float Value)
    {
        return FMath::IsFinite(Value) ? FMath::Max(Value, 0.f) : 0.f;
    }

    /** 에디터 뷰포트에 원기둥 선을 그린다. 게임에서 숨길지는 컴포넌트의 bHiddenInGame을 따른다. */
    class FSceneProxy final : public FPrimitiveSceneProxy
    {
    public:
        explicit FSceneProxy(const UKataWireCylinderComponent* Component)
            : FPrimitiveSceneProxy(Component)
            , Radius(Sanitize(Component->Radius))
            , HalfHeight(Sanitize(Component->HalfHeight))
            , Color(Component->ShapeColor)
            , LineThickness(Sanitize(Component->LineThickness))
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
            const FVector AxisX = ShapeToWorld.GetUnitAxis(EAxis::X);
            const FVector AxisY = ShapeToWorld.GetUnitAxis(EAxis::Y);
            const FVector AxisZ = ShapeToWorld.GetUnitAxis(EAxis::Z);
            const float ScaledRadius = ScaleRadius(Radius, Scale);
            const float ScaledHalfHeight = ScaleHalfHeight(HalfHeight, Scale);
            // 엔진 DrawWireCylinder는 중심을 받으므로 바닥면 원점에서 절반 높이만큼 올린 곳을 넘긴다.
            const FVector Center = ShapeToWorld.GetOrigin() + AxisZ * ScaledHalfHeight;
            const int32 Sides = FMath::Clamp(static_cast<int32>(ScaledRadius / 8.0f), 16, 64);

            for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
            {
                if ((VisibilityMap & (1 << ViewIndex)) == 0)
                {
                    continue;
                }
                const FSceneView* View = Views[ViewIndex];
                FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
                const FLinearColor DrawColor = GetViewSelectionColor(Color, *View, IsSelected(), IsHovered(), false, IsIndividuallySelected());
                if (ScaledHalfHeight > UE_KINDA_SMALL_NUMBER)
                {
                    DrawWireCylinder(PDI, Center, AxisX, AxisY, AxisZ, DrawColor, ScaledRadius, ScaledHalfHeight, Sides, SDPG_World, LineThickness);
                }
                else
                {
                    DrawCircle(PDI, Center, AxisX, AxisY, DrawColor, ScaledRadius, Sides, SDPG_World, LineThickness);
                }
            }
        }

        virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
        {
            FPrimitiveViewRelevance Result;
            Result.bDrawRelevance = IsShown(View);
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
        const float Radius;
        const float HalfHeight;
        const FColor Color;
        const float LineThickness;
    };
}

UKataWireCylinderComponent::UKataWireCylinderComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = false;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetGenerateOverlapEvents(false);
    SetCanEverAffectNavigation(false);
    bUseEditorCompositing = true;
}

void UKataWireCylinderComponent::SetCylinderSize(float InRadius, float InHalfHeight)
{
    const float NewRadius = KataWireCylinder::Sanitize(InRadius);
    const float NewHalfHeight = KataWireCylinder::Sanitize(InHalfHeight);
    if (Radius == NewRadius && HalfHeight == NewHalfHeight)
    {
        return;
    }
    Radius = NewRadius;
    HalfHeight = NewHalfHeight;
    UpdateBounds();
    MarkRenderStateDirty();
}

FPrimitiveSceneProxy* UKataWireCylinderComponent::CreateSceneProxy()
{
    return new KataWireCylinder::FSceneProxy(this);
}

FBoxSphereBounds UKataWireCylinderComponent::CalcBounds(const FTransform& LocalToWorld) const
{
    const FVector Scale = LocalToWorld.GetScale3D();
    const float ScaledRadius = KataWireCylinder::ScaleRadius(KataWireCylinder::Sanitize(Radius), Scale);
    const float ScaledHeight = 2.f * KataWireCylinder::ScaleHalfHeight(KataWireCylinder::Sanitize(HalfHeight), Scale);
    // 스케일은 이미 크기에 반영했으므로 회전과 위치만으로 경계를 옮긴다. 바닥면이 원점이라 경계는 Z 0에서 한쪽으로만 잡는다.
    // Z 스케일이 음수면 프록시의 Z 축이 뒤집혀 아래로 그려지므로 경계도 같은 쪽으로 잡는다.
    const double TopZ = Scale.Z < 0.0 ? -static_cast<double>(ScaledHeight) : static_cast<double>(ScaledHeight);
    const FTransform RigidTransform(LocalToWorld.GetRotation(), LocalToWorld.GetLocation());
    const FBox LocalBox(FVector(-ScaledRadius, -ScaledRadius, FMath::Min(0.0, TopZ)), FVector(ScaledRadius, ScaledRadius, FMath::Max(0.0, TopZ)));
    return FBoxSphereBounds(LocalBox).TransformBy(RigidTransform);
}
