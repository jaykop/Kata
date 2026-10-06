#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FKataGraphNodeFactory;

/** 그래프 에디터 모듈. 위젯·스타일과 패키지 저장용 그래프 빌드 훅의 수명을 관리한다. */
class FKataGraphEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    TSharedPtr<FKataGraphNodeFactory> NodeFactory;
    FDelegateHandle PackagePreSaveHandle;
};
