#include "Action/KataPropertyOverride.h"

#include "UObject/Object.h"
#include "UObject/UnrealType.h"

namespace
{
    /** Instanced 서브오브젝트 하나를 DestOwner 소유로 복제한다. 원본이 없으면 null을 넣는다. */
    UObject* DuplicateInstancedSubobject(UObject* DestOwner, UObject* Source)
    {
        if (Source == nullptr)
        {
            return nullptr;
        }
        return DuplicateObject<UObject>(Source, DestOwner, MakeUniqueObjectName(DestOwner, Source->GetClass()));
    }

    /** 인스턴스 소유권이 필요한 오브젝트 프로퍼티를 복제해 넣는다. */
    bool CopyInstancedObjectProperty(const FObjectProperty* ObjectProperty, void* DestValuePtr, const void* SourceValuePtr, UObject* DestOwner)
    {
        UObject* SourceObject = ObjectProperty->GetObjectPropertyValue(SourceValuePtr);
        UObject* DuplicatedObject = DuplicateInstancedSubobject(DestOwner, SourceObject);
        ObjectProperty->SetObjectPropertyValue(DestValuePtr, DuplicatedObject);
        return true;
    }
}

namespace KataPropertyOverride
{
    FName GetPropertyPath(const FProperty* Member, const FProperty* Changed)
    {
        if (!Member)
        {
            return NAME_None;
        }
        if (Member != Changed)
        {
            if (const FStructProperty* Struct = CastField<FStructProperty>(Member))
            {
                for (TFieldIterator<FProperty> It(Struct->Struct); It; ++It)
                {
                    if (*It == Changed)
                    {
                        return FName(*(Member->GetName() + TEXT(".") + Changed->GetName()));
                    }
                }
            }
        }
        return Member->GetFName();
    }


    bool CopyOverriddenProperty(UObject* DestOwner, const UObject* SourceOwner, FName PropertyName, FString& OutError)
    {
        if (DestOwner == nullptr || SourceOwner == nullptr)
        {
            OutError = TEXT("Null object in property override");
            return false;
        }
        if (PropertyName.IsNone())
        {
            OutError = TEXT("Empty overridden property name");
            return false;
        }

        TArray<FString> Segments;
        PropertyName.ToString().ParseIntoArray(Segments, TEXT("."));
        UStruct* DestStruct = DestOwner->GetClass();
        UStruct* SourceStruct = SourceOwner->GetClass();
        void* DestContainer = DestOwner;
        const void* SourceContainer = SourceOwner;
        FProperty* DestProperty = nullptr;
        FProperty* SourceProperty = nullptr;
        void* DestValuePtr = nullptr;
        const void* SourceValuePtr = nullptr;
        for (int32 Index = 0; Index < Segments.Num(); ++Index)
        {
            DestProperty = FindFProperty<FProperty>(DestStruct, FName(*Segments[Index]));
            SourceProperty = FindFProperty<FProperty>(SourceStruct, FName(*Segments[Index]));
            if (!DestProperty || !SourceProperty || !DestProperty->SameType(SourceProperty))
            {
                OutError = FString::Printf(TEXT("Invalid or incompatible override path '%s'"), *PropertyName.ToString());
                return false;
            }
            DestValuePtr = DestProperty->ContainerPtrToValuePtr<void>(DestContainer);
            SourceValuePtr = SourceProperty->ContainerPtrToValuePtr<void>(SourceContainer);
            if (Index + 1 < Segments.Num())
            {
                const FStructProperty* DestNested = CastField<FStructProperty>(DestProperty);
                const FStructProperty* SourceNested = CastField<FStructProperty>(SourceProperty);
                if (!DestNested || !SourceNested)
                {
                    OutError = FString::Printf(TEXT("Override path '%s' crosses a non-struct property"), *PropertyName.ToString());
                    return false;
                }
                DestStruct = DestNested->Struct;
                SourceStruct = SourceNested->Struct;
                DestContainer = DestValuePtr;
                SourceContainer = SourceValuePtr;
            }
        }
        if (!DestProperty)
        {
            OutError = TEXT("Empty override path");
            return false;
        }

        if (!DestProperty->ContainsInstancedObjectProperty())
        {
            DestProperty->CopyCompleteValue(DestValuePtr, SourceValuePtr);
            return true;
        }

        // Instanced 참조는 값 복사만 하면 부모·자식이 같은 객체를 공유하게 되므로 복제한다.
        if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(DestProperty))
        {
            return CopyInstancedObjectProperty(ObjectProperty, DestValuePtr, SourceValuePtr, DestOwner);
        }

        if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(DestProperty))
        {
            if (const FObjectProperty* InnerObjectProperty = CastField<FObjectProperty>(ArrayProperty->Inner))
            {
                FScriptArrayHelper DestHelper(ArrayProperty, DestValuePtr);
                FScriptArrayHelper SourceHelper(ArrayProperty, SourceValuePtr);
                DestHelper.Resize(SourceHelper.Num());
                for (int32 Index = 0; Index < SourceHelper.Num(); ++Index)
                {
                    CopyInstancedObjectProperty(InnerObjectProperty, DestHelper.GetRawPtr(Index), SourceHelper.GetRawPtr(Index), DestOwner);
                }
                return true;
            }
        }

        // Map/Set 안의 Instanced 객체처럼 소유권 규칙을 확정하지 않은 형태는 조용히 처리하지 않는다.
        OutError = FString::Printf(TEXT("Property '%s' contains instanced objects in an unsupported container"), *PropertyName.ToString());
        return false;
    }
}
