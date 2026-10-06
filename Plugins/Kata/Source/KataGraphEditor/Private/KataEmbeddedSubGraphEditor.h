#pragma once

#include "CoreMinimal.h"

class UKataGraph;

/** 노드 배치와 이름 편집에서 공용으로 사용하는 내장 원본 작업. 트랜잭션은 호출자가 시작한다. */
namespace KataEmbeddedSubGraphEditor
{
    UKataGraph* Create(UKataGraph* Owner);
    /** 하위 내장 트리의 저작 노드·엣지를 함께 복제한다. 실행 사본을 제외하며 원본을 수정하지 않는다. */
    UKataGraph* CopyForClipboard(UKataGraph* Source, UObject* Outer);
    /** 클립보드 사본을 새 내장으로 편입한다. 호출자의 붙여넣기 트랜잭션으로 복구할 수 있다. */
    UKataGraph* PasteCopy(UKataGraph* Owner, UKataGraph* ClipboardGraph);
    bool ValidateName(const UKataGraph* Owner, const UKataGraph* Graph, const FText& Name, FText& OutError);
    bool Rename(UKataGraph* Owner, UKataGraph* Graph, const FText& Name, FText& OutError);
    /** 저작 포트의 활성 참조가 없는 원본을 제거한다. Undo용 객체는 파괴하지 않는다. */
    bool RemoveUnused(UKataGraph* Owner);
}
