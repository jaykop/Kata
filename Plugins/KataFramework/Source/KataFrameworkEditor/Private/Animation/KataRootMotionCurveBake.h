#pragma once

#include "CoreMinimal.h"

class UAnimMontage;

namespace KataFL
{
    /**
     * 몽타주의 현재 루트 모션을 몽타주 트랙 시간축의 Kata 루트 모션 커브로 굽는다.
     *
     * 값은 시퀀스 커브가 있으면 그 커브, 없으면 원래 루트 모션으로 계산하며 몽타주에 이미 있는 커브는 읽지 않는다.
     * 첫 슬롯 트랙의 0초부터 끝까지 섹션 순서와 무관하게 트랙 순서대로 누적하고, 기존 몽타주 커브는 덮어쓴다.
     * 루트 모션이 없는 몽타주는 아무것도 쓰지 않고 false를 반환한다. 결과 요약과 경고는 LogKataRootMotionCurve에 남긴다.
     */
    bool BakeRootMotionCurvesToMontage(UAnimMontage& Montage);

    /**
     * 콘텐츠 브라우저의 몽타주 우클릭 메뉴에 굽기 명령을 추가한다.
     * 호출 측이 FToolMenuOwnerScoped로 소유자를 지정해 모듈 종료 시 함께 지워지게 한다.
     */
    void RegisterRootMotionCurveBakeMenu();
}
