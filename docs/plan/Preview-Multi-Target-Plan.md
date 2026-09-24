# 프리뷰 멀티 타겟 배치 계획

작성: 2026-09-24  
갱신: 2026-09-24  
연결 이슈: [#10 프리뷰 멀티 타겟 액터 배치](https://github.com/jaykop/Kata/issues/10)  
현재 상태 근거: [현재 구현 상태](../devlog/Implementation-Status.md)  
대체 관계: 삭제한 Next-Work-Plan의 "보류한 항목 — 멀티 타겟 액터 배치" 절을 옮겼다.

## 목적과 현재 상태

프리뷰에는 Target을 하나만 배치할 수 있다. 히트 판정과 거리 조건을 여러 대상으로 확인하려면 여러 개를 배치할 수 있어야 한다.
설계는 정했지만 아직 구현하지 않았다.

## 범위

- 포함: 프리뷰 에디터에서 Target 액터 여러 개를 배치하고 조작하는 기능.
- 제외: 런타임 다중 타겟. 별도 설계가 필요하므로 프리뷰 요구로 끌어들이지 않는다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| 설정 구조 | 확정 | `PreviewTargetClass`·`PreviewTargetTransform` 단일 쌍을 `TArray<FKataPreviewActorSpec>`으로 교체한다. |
| 기존 에셋 이전 | 확정 | 기존 단일 프로퍼티는 Redirect 또는 `PostLoad` 승격으로 옮긴다. 방식은 착수 시 정한다. |
| Context 대상 | 확정 | `FKataContext::TargetActor`는 단일 필드로 유지한다. 배열의 Primary만 Context에 넣고, 나머지는 히트 판정·거리 조건 검증용으로만 둔다. |
| 뷰포트 조작 | 확정 | 조작은 `EKataPreviewActorSlot`으로 이미 일반화했다. 자리를 `Target` 하나에서 `Target[i]`로 넓히면 되고, 뷰포트 클라이언트의 위젯·입력·커밋 경로는 바뀌지 않는다. `GetActorForSlot`과 `ApplyPreviewActorTransform`의 분기만 배열 인덱스로 확장한다. |

## 작업 순서와 완료 조건

| ID | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|
| M1 | 설정 구조 교체와 기존 에셋 이전 | 없음 | 기존 에셋을 열면 단일 Target이 배열의 첫 항목으로 보임 |
| M2 | 뷰포트 슬롯 확장 | M1 | 각 Target을 선택·이동할 수 있고 Primary가 Context에 들어감 |

## 영향과 제한

- 직렬화된 프리뷰 설정이 바뀐다. 기존 에셋 이전 경로를 반드시 함께 구현한다.

## 완료 시 갱신할 문서

- [현재 구현 상태](../devlog/Implementation-Status.md): 프리뷰 Target 구성.
- [에디터 사용법](../manual/Editor-Usage.md): 여러 Target의 배치와 선택.
- 결정 이유는 devlog로 옮기고 이 문서를 삭제한다.
