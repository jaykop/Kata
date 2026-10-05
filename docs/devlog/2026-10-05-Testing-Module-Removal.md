# 전용 테스트 모듈 제거

작성: 2026-10-05  
갱신: 2026-10-05  
유형: 결정·구현 기록  
대상: 샘플 프로젝트의 ProjectKataTesting  
기준: 모듈 제거 작업의 미커밋 변경

## 배경과 결론

사용자가 입력으로 액션을 실행할 수 있다고 확인했고, 전용 테스트 모듈을 제거하기로 결정했다. 별도 테스트 콘솔 명령을 확장하는 대신 샘플 입력 경로를 사용한다.

## 변경 내용

- ProjectKataTesting의 소스, 내장 테스트 액션, 테스트 액터·디버그 태스크와 콘솔 명령을 제거했다.
- 프로젝트 및 Game·Editor Target의 모듈 등록과 테스트 타입 Redirect를 제거했다.
- 현재 구조 지침과 런타임 사용법, 에셋 이전 안내를 갱신했다.
- Content/KataTest 에셋과 NeverCook 설정은 유지했다. 모듈 제거는 콘텐츠 폴더 삭제를 포함하지 않는다.

## 확인 범위와 결과

제거 전 Content와 Plugins의 uasset·umap 1,154개에서 모듈명 및 테스트 타입명을 ASCII·UTF-16 문자열로 검색했고 일치 항목은 없었다. Asset Registry 조회나 에디터 로드 확인은 수행하지 않았다.

빌드·실행 검사는 수행하지 않았다. 입력 실행 경로가 있다는 사용자 설명은 PM-2의 Pre·Post Command와 그래프 대상 전달을 모두 확인했다는 의미는 아니다.

## 남은 제한과 후속 작업

외부에 보관한 옛 테스트 타입 참조 에셋은 지원하지 않는다. [#18](https://github.com/jaykop/Kata/issues/18)의 별도 실행 도구 필요 여부와 [#1](https://github.com/jaykop/Kata/issues/1)의 실행 확인 조건은 사용자 확인 후 이슈에 반영한다.

## 연관 문서 반영

- [런타임 사용법](../manual/Runtime-Usage.md): 테스트 하네스 안내를 샘플 입력 실행 안내로 변경.
- [에셋 이전 안내](../manual/Asset-Migration.md): 테스트 타입 및 Redirect 제거 반영.
- [플러그인 분리 계획](../plan/Plugin-Modularization-Plan.md): 전용 테스트 모듈 제외.
- 과거 devlog의 테스트 모듈 설명은 당시 기록으로 유지한다.
