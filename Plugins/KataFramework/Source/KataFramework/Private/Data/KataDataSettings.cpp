#include "Data/KataDataSettings.h"

#include "Data/KataDataCollection.h"
#include "KataFrameworkLog.h"

UKataDataSettings::UKataDataSettings()
{
    CategoryName = TEXT("Game");
    SectionName = TEXT("Kata Data");
}

const UKataDataSettings* UKataDataSettings::Get()
{
    return GetDefault<UKataDataSettings>();
}

const UKataDataCollection* UKataDataSettings::GetDataCollection() const
{
    if (LoadedCollection == nullptr && !DataCollection.IsNull())
    {
        LoadedCollection = DataCollection.LoadSynchronous();
        if (LoadedCollection == nullptr)
        {
            UE_LOG(LogKataFramework, Error, TEXT("Kata Data: failed to load data collection %s."), *DataCollection.ToString());
        }
    }
    return LoadedCollection;
}

const UKataDataCollection* UKataDataSettings::GetLoadedDataCollection() const
{
    return LoadedCollection != nullptr ? LoadedCollection.Get() : DataCollection.Get();
}

#if WITH_EDITOR
void UKataDataSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // 칸이 바뀌었으면 이전 컬렉션을 더 붙잡지 않는다. 다음 조회 때 새 칸으로 다시 로드한다.
    LoadedCollection = nullptr;
}
#endif
