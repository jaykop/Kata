#include "Modules/ModuleManager.h"
#include "ProjectKataDebugLog.h"

DEFINE_LOG_CATEGORY(LogKataDebugMenu);

// 메뉴는 로컬 플레이어 서브시스템이 스스로 만들고 정리하므로 모듈 진입점에서 할 일이 없다.
IMPLEMENT_MODULE(FDefaultModuleImpl, ProjectKataDebug);
