1> 추가로 필요한 기본 태스크가 뭐가 있을까
- Apply Gameplay Effect
- Apply Gameplay Tag
=> 위의 2개는 꼭 나누어야 할까?
- Camera Shake
- Spawn VFX
- Play Sound
=> 위의 3개는 Cue로 실행하거나 묶음으로 별도 관리해야 할까?

2> KataSet 개념 필요
- 복수의 KataAction을 할당할 수 있는 구조체
- KataGraph는 1개 KataAction을 실행하는 노드 + KataSet 중 1개를 선택하는 노드를 가진다
- KataSet은 할당된 KataAction들은 어떻게 실행할 지 정책을 결정한다 (Weight, Priority)
- 실행하기로 선택된 KataAction이 실행하지 못한 경우, 다음 순위의 KataAction을 실행할지 말지 여부도 결정한다
- 각 KataAction은 정책에 따라 Weight나 Priority를 할당할 수 있다
=> 고민 거리
- KataSet 전용 에디터뷰가 필요해보인다
- KataSet 자체 조건, Cooldown 등도 필요해보인다