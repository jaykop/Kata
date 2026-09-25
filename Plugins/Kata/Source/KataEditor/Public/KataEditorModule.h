#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/** 에셋 편집기와 프리뷰 도구를 위한 에디터 전용 계층. */
class KATAEDITOR_API FKataEditorModule final : public IModuleInterface
{
};

namespace KataEditor
{
    /**
     * Kata 액션 에디터 프리뷰 뷰포트 툴바의 ToolMenus 이름.
     *
     * 위성·통합 플러그인의 Editor 모듈이 UToolMenus::ExtendMenu로 프리뷰 툴바에 항목을 더할 때 사용한다.
     * 메뉴는 첫 Kata 에디터가 열릴 때 등록되지만 ToolMenus는 등록 전 확장도 받아 두므로 모듈 시작 시 확장해도 된다.
     * 메뉴 컨텍스트에는 UUnrealEdViewportToolbarContext가 들어 있으며 Viewport가 프리뷰 뷰포트를 가리킨다.
     */
    KATAEDITOR_API FName GetPreviewViewportToolbarMenuName();
}
