#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "KataDataSettings.generated.h"

class UKataDataCollection;

/**
 * 게임이 쓸 데이터 컬렉션을 지정하는 프로젝트 설정. 프로젝트 설정의 Game > Kata Data에서 편집하고 DefaultGame.ini에 저장한다.
 *
 * 테이블 구성은 UKataDataCollection 에셋이 가지고, 이 설정은 그 에셋 하나만 가리킨다.
 * 기능별 튜닝 값은 이 클래스가 아니라 각 기능의 설정 클래스에 둔다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Kata Data"))
class KATAFRAMEWORK_API UKataDataSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UKataDataSettings();

    static const UKataDataSettings* Get();

    /** 게임이 쓸 데이터 컬렉션. */
    UPROPERTY(Config, EditAnywhere, Category = "Data")
    TSoftObjectPtr<UKataDataCollection> DataCollection;

    /**
     * 지정한 데이터 컬렉션을 돌려준다. 아직 로드되지 않았으면 동기로 로드하고 이후 계속 유지한다.
     * DataCollection이 비어 있거나 로드에 실패하면 nullptr을 반환한다. 로드 실패는 오류 로그로 알린다.
     */
    const UKataDataCollection* GetDataCollection() const;

    /** 이미 로드된 데이터 컬렉션만 돌려준다. 로드를 일으키면 안 되는 곳(테이블 로드 중 호출되는 콜백)에서 쓴다. */
    const UKataDataCollection* GetLoadedDataCollection() const;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    /** 로드한 컬렉션을 GC에서 보호한다. 설정 객체는 CDO라서 이 참조가 있는 동안 컬렉션과 그 테이블이 유지된다. DataCollection 설정을 바꾸면 이 참조를 해제한다. */
    UPROPERTY(Transient)
    mutable TObjectPtr<UKataDataCollection> LoadedCollection;
};
