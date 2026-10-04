#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

struct FAssetData;

/**
 * AKataCharacterSpawner의 Details 커스터마이즈.
 *
 * Source Table 선택기가 데이터 컬렉션의 NPC Character Tables 목록에 있는 테이블만 보여 주도록 선택 위젯을 직접 만든다.
 * 프로퍼티 메타(GetAssetFilter)는 이 선택기에 적용되지 않아, 필터를 위젯에 명시적으로 연결한다.
 * 나머지 프로퍼티는 기본 배치를 그대로 쓴다.
 */
class FKataCharacterSpawnerDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    /** 컬렉션의 NPC 테이블 목록에 없는 에셋이면 참을 반환해 선택기에서 뺀다. 컬렉션이 없으면 모두 뺀다. */
    static bool ShouldFilterSourceTable(const FAssetData& AssetData);
};
