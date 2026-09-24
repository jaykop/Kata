#pragma once

#include "CoreMinimal.h"

class FProperty;
class UObject;

/**
 * 프로퍼티 단위 오버라이드를 적용하는 헬퍼.
 * 지정한 프로퍼티만 복사한다. 구조체 필드는 개별 경로로, 배열은 하나의 값으로 취급한다.
 */
namespace KataPropertyOverride
{
    /** Details 변경 이벤트에서 구조체 필드 경로를 구한다. 배열과 객체 내부 변경은 소유 프로퍼티 전체를 가리킨다. */
    KATARUNTIME_API FName GetPropertyPath(const FProperty* Member, const FProperty* Changed);

    /**
     * SourceOwner의 프로퍼티 하나를 DestOwner로 복사한다.
     * Instanced 서브오브젝트는 참조를 공유하지 않도록 DestOwner를 Outer로 복제한다.
     * 객체 배열과, 필드로 Instanced 객체를 직접 가진 구조체 배열을 지원한다.
     *
     * @param PropertyName  복사할 프로퍼티 경로(예: LoopPolicy.MaxLoopCount). 두 객체에 같은 타입으로 있어야 한다.
     * @param OutError      실패 사유를 담을 영어 진단 문자열.
     * @return 복사에 성공하면 true.
     */
    KATARUNTIME_API bool CopyOverriddenProperty(UObject* DestOwner, const UObject* SourceOwner, FName PropertyName, FString& OutError);
}
