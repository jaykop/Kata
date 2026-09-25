#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "KataHitTraceTypes.generated.h"

class AActor;
class UAbilitySystemComponent;
class UKataTask_HitTrace;

/** HitBox가 판정 영역을 만드는 방식. */
UENUM(BlueprintType)
enum class EKataHitBoxMode : uint8
{
    /** 칼날처럼 두 소켓 사이의 선분을 여러 점으로 나눠 각 점의 이동 경로를 추적한다. */
    SocketTrace,
    /** 소켓 하나에 붙인 도형을 이전 위치에서 현재 위치로 쓸어 판정한다. */
    ShapeSweep
};

/** ShapeSweep에서 쓰는 도형. */
UENUM(BlueprintType)
enum class EKataHitBoxShape : uint8
{
    Sphere,
    Capsule,
    Box
};

/** 판정 기준 메시를 캐릭터 본체와 무기 중 어디에서 가져올지 정한다. */
UENUM(BlueprintType)
enum class EKataHitBoxMeshSource : uint8
{
    /** 캐릭터 본체 메시. 주먹·발차기처럼 몸의 소켓을 쓸 때 고른다. */
    Character,
    /** UKataHitBoxComponent에 등록한 무기 메시. */
    Weapon
};

/** 판정 영역 디버그 시각화 단계. Shipping처럼 ENABLE_DRAW_DEBUG가 꺼진 빌드에서는 무엇도 그리지 않는다. */
UENUM(BlueprintType)
enum class EKataHitTraceDebugMode : uint8
{
    Off,
    /** 현재 판정 영역만 그린다. */
    Area,
    /** 판정 영역에 더해 서브스텝 궤적, 시작·종료 보정 구간, 히트 지점을 그린다. */
    Detailed
};

/**
 * 판정이 찾아낸 히트 한 건.
 *
 * UKataHitSubsystem이 프레임 안에서 제출 순서대로 처리하고 곧바로 버린다.
 * 같은 프레임 안에서만 쓰므로 참조를 오래 쥐지 않지만, 처리 전 GC에 대비해 강참조 UPROPERTY로 둔다.
 */
USTRUCT()
struct KATAFRAMEWORK_API FKataHitRecord
{
    GENERATED_BODY()

    /** 공격한 액터. Kata Context의 Avatar다. */
    UPROPERTY()
    TObjectPtr<AActor> InstigatorActor = nullptr;

    /** 맞은 액터. */
    UPROPERTY()
    TObjectPtr<AActor> TargetActor = nullptr;

    /** 공격한 쪽의 ASC. 없으면 nullptr이다. */
    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> SourceAbilitySystem = nullptr;

    /** 히트를 만든 태스크 정의. 처리기 목록을 여기에서 읽는다. */
    UPROPERTY()
    TObjectPtr<UKataTask_HitTrace> SourceTask = nullptr;

    UPROPERTY()
    FHitResult HitResult;

    /** Subsystem 안에서 증가하는 제출 순번. 같은 프레임의 처리 순서를 나타낸다. */
    UPROPERTY()
    int32 Sequence = 0;
};
