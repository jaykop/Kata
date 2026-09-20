#pragma once

#include "CoreMinimal.h"
#include "KataRuntimeLog.h"

// 벤더 코드가 쓰던 로그 매크로를 Kata의 LogKata로 연결한다.
#define LOG_INFO(FMT, ...) UE_LOG(LogKata, Display, (FMT), ##__VA_ARGS__)
#define LOG_WARNING(FMT, ...) UE_LOG(LogKata, Warning, (FMT), ##__VA_ARGS__)
#define LOG_ERROR(FMT, ...) UE_LOG(LogKata, Error, (FMT), ##__VA_ARGS__)
