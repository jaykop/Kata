#include "Equipment/KataPreviewSetup_Equipment.h"

#include "Equipment/KataEquipmentComponent.h"
#include "Equipment/KataEquipmentSetup.h"
#include "GameFramework/Actor.h"

void UKataPreviewSetup_Equipment::ApplyToPreview(AActor* PreviewActor) const
{
    if (PreviewActor == nullptr)
    {
        return;
    }

    UKataEquipmentComponent* EquipmentComponent = PreviewActor->FindComponentByClass<UKataEquipmentComponent>();
    if (EquipmentComponent == nullptr)
    {
        // AKataCharacter가 아닌 프리뷰 액터에도 장비를 보여 줄 수 있게 임시 컴포넌트를 붙인다. 프리뷰 월드와 함께 사라진다.
        EquipmentComponent = NewObject<UKataEquipmentComponent>(PreviewActor, NAME_None, RF_Transient);
        PreviewActor->AddInstanceComponent(EquipmentComponent);
        EquipmentComponent->RegisterComponent();
    }

    if (EquipmentSetup != nullptr)
    {
        EquipmentComponent->SetEquipmentSetup(EquipmentSetup);
    }
    for (const FKataStartingEquipment& Entry : Equipment)
    {
        if (Entry.EquipmentId.IsValid())
        {
            EquipmentComponent->EquipImmediately(Entry.EquipmentId, Entry.Slot);
        }
    }
}
