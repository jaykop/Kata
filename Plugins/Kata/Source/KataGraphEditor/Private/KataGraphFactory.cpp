#include "KataGraphFactory.h"

#include "KataGraph.h"

UKataGraphFactory::UKataGraphFactory()
{
    SupportedClass = UKataGraph::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UKataGraphFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
    EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    return NewObject<UKataGraph>(InParent, Class, Name, Flags | RF_Transactional);
}
