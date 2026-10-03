#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;
class SSearchableComboBox;
class UDataTable;

/**
 * FKataRowId 계열 영역 ID를 행 이름 드롭다운 한 줄로 보여 주는 디테일 패널 커스터마이즈.
 *
 * 영역마다 다른 것은 행을 찾을 테이블뿐이므로, 그 테이블을 가져오는 함수를 받아 만든다.
 * 목록은 콤보박스를 열 때마다 다시 읽어 테이블을 고친 직후에도 반영한다. 테이블 포인터는 보관하지 않는다.
 * 현재 값이 어느 테이블에도 없으면 (missing)을 붙여 빨간색으로 표시한다.
 */
class FKataRowIdCustomization : public IPropertyTypeCustomization
{
public:
    /** 영역 테이블을 OutTables에 채우는 함수. 데이터 컬렉션이 없으면 비워 둔다. */
    using FGetTables = TFunction<void(TArray<const UDataTable*>& OutTables)>;

    static TSharedRef<IPropertyTypeCustomization> MakeInstance(FGetTables InGetTables);

    explicit FKataRowIdCustomization(FGetTables InGetTables);

    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
    void RefreshOptions();
    void OnSelectionChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo);

    /** 값이 하나로 정해져 있으면 OutRowName에 담고 참을 반환한다. 여러 객체를 함께 편집해 값이 다르면 거짓이다. */
    bool GetCurrentRowName(FName& OutRowName) const;
    bool IsCurrentRowMissing() const;

    FText GetCurrentText() const;
    FSlateColor GetCurrentColor() const;
    FText GetToolTipText() const;

    FGetTables GetTables;
    TSharedPtr<IPropertyHandle> RowNameHandle;
    TArray<TSharedPtr<FString>> Options;
    TSharedPtr<SSearchableComboBox> ComboBox;
};
