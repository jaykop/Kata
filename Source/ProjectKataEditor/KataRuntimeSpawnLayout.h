#pragma once

#include "CoreMinimal.h"
#include "KataRuntimeSpawnerUserSettings.h"

/** 런타임 스포너 도구의 배치 입력이다. */
struct FKataRuntimeSpawnLayout
{
    float Distance = 300.f;
    float YawOffset = 0.f;
    int32 Count = 1;
    EKataRuntimeSpawnPattern Pattern = EKataRuntimeSpawnPattern::Line;
    float Spacing = 150.f;
};

namespace KataRuntimeSpawnLayout
{
    /**
     * 플레이어 위치와 Yaw를 기준으로 개체별 위치와 회전을 계산한다. 높이는 플레이어 위치와 같으며 바닥 맞춤은 호출자가 한다.
     * 각 개체는 플레이어를 바라보는 Yaw에 YawOffset을 더한 방향을 본다.
     * Line은 정면 기준점(플레이어 정면 × Distance)을 중심으로 정면에 수직인 선 위에 Spacing 간격으로 놓는다.
     * Circle은 Distance를 반지름으로 정면을 가운데 두고 호 길이 Spacing 간격으로 좌우에 펼치며,
     * Count × Spacing이 둘레 이상이거나 Spacing이 0이면 360도에 균등하게 놓는다.
     */
    void Build(const FVector& PlayerLocation, float PlayerYaw, const FKataRuntimeSpawnLayout& Layout, TArray<FTransform>& OutTransforms);
}
