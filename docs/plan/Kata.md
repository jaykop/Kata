1> 클로드가 작성한, 한국어로 읽기에 이상한 주석 검수
- 주석 작성 시 지침이 필요?
2> 에디터 코드 / 디버그 코드 / 런타임 코드 구분 필요?
- 코드 작성 시 지침이 필요?
3> 코드 재사용을 위한 C++ KataFL이 필요
- 파일 이름은 KataFL_Condition.h/KataFL_Condition.cpp와 같은 식
- 커스텀 구조체를 parameter로 받지 않음
- CheckAngle, CheckDistance, CheckTag, CompareValue 등 반복 사용될 수 있는 조건 식을 공용 함수로 뺀다
- KataCondition이 위의 함수를 사용하도록 한다
- BP로 제공해도 좋겠다
4> 기본 태그 구조를 만들어야 하나?
- 에디터 실행할 때마다 TagGenerator.bat 실행?
- Native 폴더 하위에는 코드 참조 가능한 태그 ini + 그 외에는 에디터에서 직접 수정가능한 태그 ini로 분류?
- 혹은 더 나은 방법이 있나?
- 플러그인에서 Tag를 제공해도 괜찮은가?