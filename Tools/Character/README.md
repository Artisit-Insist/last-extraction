# 캐릭터 제작 원본

일반 플레이에는 이 스크립트 실행이 필요하지 않습니다. 게임시작.command만 실행하세요.

인물 수정은 ArtSource/Production/CommandoRigged.blend에서 가능합니다. 52개 뼈대와 14개 동작을 보관했습니다.

제작자용 재생성 순서: Blender에서 build_rigged_character.py, fit_combat_boots.py 순서로 실행하고, Unreal Editor에서 import_all.py를 실행합니다. 먼저 프로젝트 C++를 빌드해야 합니다. 재생성은 해당 캐릭터 자산을 덮어쓰므로 수동 편집본을 먼저 별도 이름으로 보관하세요. cmu_motion.py와 create_commando.py는 같은 폴더에 있어야 합니다.


## 0.3 인물 표현과 탑승 동작

- CommandoVisual03.blend: 피부 이미지, 머리카락, 신발을 맞춘 새 인물 원본. 기존 CommandoRigged.blend와 동작 원본은 보존했습니다.
- fit_character_details.py / import_character_details.py: 새 외형을 기존 뼈에 연결하고 가져오기.
- CivilianMotion.blend / make_civilian_motion.py / import_civilian.py: 포로·탑승자의 대기·보행·달리기.
- CivilianBoarding.blend / make_boarding_pose.py / import_boarding_pose.py: 탑승 후 좌석에 앉는 동작과 헬기 창문 재질.
- 무장 지원 NPC는 주인공과 동일한 무기 파지·상체 사격 동작을 사용합니다.


### 0.6 역할별 복장

`build_role_variants.py`를 Blender에서 실행하면 기존 CommandoRigged.blend의 스켈레톤을 그대로 사용해 다섯 복장 메시와 CharacterRoles06.blend를 만듭니다. `import_role_variants.py`는 Unreal Python으로 새 Characters06/Materials06/Textures06 폴더에 가져옵니다. 원래 캐릭터와 애니메이션은 유지합니다. Cook 또는 실행 중인 에디터와 가져오기를 동시에 실행하지 마십시오.

구출 대상에게 붙은 WorkerWardrobe 표식은 무장 전후와 저장 복원 때 유지합니다. 마지막 탑승 장면에도 두 종류의 구출 대상 복장을 배치합니다. `-capture-preview -wardrobe-preview`는 실제 게임 자산 6종의 확인용 화면이며 일반 플레이에서는 동작하지 않습니다.

## 0.7 쓰러짐

`Animation07/FallMotions07.blend`에 방향별 쓰러짐과 의상 접촉 보정 동작이 있습니다. `build_fall_motion.py`, `import_fall_motion.py`, `validate_fall_motion.py` 순서로 제작·가져오기·형상 검사를 수행합니다. 같은 프로젝트의 Unreal 가져오기와 Cook을 동시에 실행하지 마십시오. 이전 인물 자료와 동작은 보존합니다.
