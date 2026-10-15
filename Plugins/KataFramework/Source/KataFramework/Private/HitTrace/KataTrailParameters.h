#pragma once

#include "CoreMinimal.h"
#include "NiagaraParameterStore.h"

/**
 * UKataTask_Trail과 Niagara 시스템 사이의 User 파라미터 계약.
 *
 * 이름은 User 네임스페이스를 뺀 이름이다. 사용법 문서의 계약 표와 같게 유지한다.
 * 출력 배열은 이번 프레임에 새로 생성할 입자 하나당 한 항목이며, 모든 배열의 길이는 Count와 같다.
 */
namespace KataTrailParameters
{
    /** 에셋에서 읽는 입력. 샘플 사이의 최대 칼날 이동 거리(cm)와 최대 회전각(도). */
    inline const FName SampleDistance(TEXT("KataTrailSampleDistance"));
    inline const FName SampleAngle(TEXT("KataTrailSampleAngle"));

    /** 매 프레임 쓰는 출력. Count는 새 입자가 없는 프레임에 0이다. */
    inline const FName Count(TEXT("KataTrailCount"));
    /** Position 배열. 소켓 구간의 중점(월드). */
    inline const FName Positions(TEXT("KataTrailPositions"));
    /** float 배열. 소켓 구간의 길이(cm). 리본 폭으로 쓴다. */
    inline const FName Widths(TEXT("KataTrailWidths"));
    /** Vector 배열. 앞 소켓에서 뒤 소켓으로 향하는 단위 벡터. 리본 Facing Mode의 Custom Side Vector로 쓴다. */
    inline const FName Sides(TEXT("KataTrailSides"));
    /** int 배열. 소켓 구간 번호(0부터). 구간마다 리본 하나가 된다. */
    inline const FName RibbonIds(TEXT("KataTrailRibbonIds"));
    /** float 배열. 궤적 시작부터 샘플 시각까지의 Kata 시간(초). 리본 연결 순서로 쓴다. */
    inline const FName LinkOrders(TEXT("KataTrailLinkOrders"));
    /** float 배열. 샘플 시각부터 이번 프레임까지 지난 Kata 시간(초). 생성할 때 입자 나이에 더한다. */
    inline const FName Ages(TEXT("KataTrailAges"));
    /** Vector2D 배열. 칼날 전체 길이 대비 그 구간의 시작·끝 비율. 칼날 폭 방향 UV에 쓴다. */
    inline const FName BladeRanges(TEXT("KataTrailBladeRanges"));

    /** 출력 파라미터 순서. 이 순서의 비트로 에셋에 있는 출력을 표시한다. */
    inline TConstArrayView<FName> GetOutputNames()
    {
        static const FName Names[] = { Count, Positions, Widths, Sides, RibbonIds, LinkOrders, Ages, BladeRanges };
        return Names;
    }

    /** 에셋에 입력 파라미터가 없거나 0 이하일 때 쓰는 값. */
    constexpr float DefaultSampleDistance = 10.0f;
    constexpr float DefaultSampleAngle = 10.0f;

    /** 한 프레임에 만드는 샘플 수 상한. 구간 시작점은 따로 센다. */
    constexpr int32 MaxSamplesPerFrame = 64;

    /** User 네임스페이스를 붙인 파라미터 저장소 안의 이름. */
    inline FName MakeUserName(FName Name)
    {
        return FName(*(FString(TEXT("User.")) + Name.ToString()));
    }

    /** 저장소에 있는 출력 파라미터를 GetOutputNames 순서의 비트로 돌려준다. 타입은 보지 않고 이름만 비교한다. */
    inline uint32 FindOutputMask(const FNiagaraParameterStore& Parameters)
    {
        const TConstArrayView<FName> OutputNames = GetOutputNames();
        uint32 Mask = 0;
        for (int32 Index = 0; Index < OutputNames.Num(); ++Index)
        {
            const FName UserName = MakeUserName(OutputNames[Index]);
            for (const FNiagaraVariableWithOffset& Variable : Parameters.ReadParameterVariables())
            {
                if (Variable.GetName() == UserName)
                {
                    Mask |= 1u << Index;
                    break;
                }
            }
        }
        return Mask;
    }
}
