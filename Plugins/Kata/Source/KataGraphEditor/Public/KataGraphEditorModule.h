#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FKataGraphNodeFactory;

/** 그래프 에디터 모듈. 노드 위젯 팩토리와 스타일셋의 수명을 관리한다. */
class FKataGraphEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    TSharedPtr<FKataGraphNodeFactory> NodeFactory;
};
