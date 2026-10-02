# 지역 환경·헬기 편집 원본

ArtSource/Production 안의 .blend 파일을 Blender에서 열면 개별 모양과 재질을 편집할 수 있습니다. Content/Production 안에는 게임에 가져온 자료가 있습니다.

- make_landmarks.py: 현수교·윈치·채석장 크레인
- make_gate.py: 교량 통제문
- make_transport_and_architecture.py: 구조 헬기·회전 날개·문·방벽·벙커·격납고
- make_regional_props.py: 강변·마을·채석장·요새·비행장 소품 12종
- make_building_variants.py: 수상 가옥·상점·보급 창고
- import_world.py: 완성 모델·재질·피부·로터 소리를 Unreal에 가져오기

Blender 스크립트는 새 제작 장면을 만듭니다. 원본 파일을 수정하려면 먼저 사본으로 저장하십시오. 완성된 게임 실행에는 이 스크립트를 실행할 필요가 없습니다.

## 0.8 생활·작업 공간

`make_site_props.py`는 Props08에 새 소품 8종과 원통 표면을 보정한 연료 탱크 사본을 저장합니다. `import_site_props.py`는 해당 9개 모델만 가져오므로 이전 물·지면 재질을 다시 만들지 않습니다. `geometry_helpers.py`는 앞으로 생성하는 모델에 45도 기준 표면 스무딩을 적용합니다. 기존 모델 파일은 보존합니다.
