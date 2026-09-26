#pragma once

#include "CoreMinimal.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DeveloperSettings.h"
#include "KataHitTraceSettings.generated.h"

/**
 * Hit Trace의 프로젝트 설정. 프로젝트 설정의 Kata Hit Trace에서 편집하고 DefaultGame.ini에 저장한다.
 *
 * 플러그인은 프로젝트의 콜리전 채널 칸을 정하지 않는다. 프로젝트가 HurtBox용 Object Channel과 프로필을 정의하고 여기에서 고른다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Kata Hit Trace"))
class KATAFRAMEWORK_API UKataHitTraceSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UKataHitTraceSettings();

    static const UKataHitTraceSettings* Get();

    /**
     * HurtBox가 쓰는 콜리전 프로필. 새 UKataHurtBoxComponent가 이 프로필로 시작하고,
     * Hit Trace 판정은 이 프로필의 Object Type으로 HurtBox를 찾는다.
     * 프로필은 모든 채널을 무시하는 질의 전용(QueryOnly)으로 만드는 것을 권장한다.
     *
     * 비어 있거나 없는 프로필이면 HurtBox는 WorldDynamic·모든 채널 무시로 시작하고 판정도 WorldDynamic으로 찾는다.
     * 컴포넌트 기본값은 클래스를 만들 때 읽으므로, 바꾼 값은 에디터를 다시 켠 뒤 새 기본값에 반영된다.
     */
    UPROPERTY(Config, EditAnywhere, Category = "Hurt Box")
    FCollisionProfileName HurtBoxCollisionProfile;

    /** HurtBoxCollisionProfile이 실제로 정의된 프로필이면 그 이름, 아니면 NAME_None. */
    FName GetValidHurtBoxProfileName() const;

    /** Hit Trace 판정이 HurtBox를 찾을 Object Type. 유효한 프로필이 없으면 ECC_WorldDynamic이다. */
    ECollisionChannel GetHurtBoxObjectType() const;
};
