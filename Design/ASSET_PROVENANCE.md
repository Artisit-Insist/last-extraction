# 제작 자산 기록 — 개선 버전

## 이미지

ChatGPT 내장 image_gen 도구로 제작했다. 도구가 세부 모델명을 노출하지 않으므로 사용자 요청의 ‘Image 2.5’라고 단정하지 않는다.

- JungleKeyArt: 시작 화면.
- EquipmentAtlas: 탄약·구급·활·무전기 그림.
- CharacterDesign: 인물 디자인 참고 그림. 리깅 모델 자체는 별도로 제작했다.
- JungleGround: 게임 지면의 색상 이미지.
- Production/Camouflage: 인물 복장의 위장 패턴.
- Production/ExplosionAtlas: 4×4 폭발 효과 프레임.
- Production/VehiclePaint: 차량 표면의 도장 이미지. 정확한 프롬프트는 VehiclePaint.prompt.json에 보관했다.

## 3D 환경과 소품

Poly Haven의 CC0 식생·암석·상자·드럼통·나무 모델과 지면 재질, 숲 HDR 조명을 사용했다. 원본과 API 출처 정보는 ArtSource/Production의 각 자료 폴더에 보존했다. https://polyhaven.com/license

FieldRifle, TimberHut, HutRoof, Watchtower, Sandbags, IntelDesk, RadioStation, FieldGenerator, ArmoredPatrol은 이 프로젝트에서 Blender로 제작했다. 편집 가능한 .blend와 내보낸 FBX를 보관했다. 이전 버전의 기본 도형과 모델도 원본 보존을 위해 남겨 두었다.

## 인물과 애니메이션

MakeHuman Community CC0 인체 베이스와 체형 데이터를 사용하고 복장·장비·머리·리깅을 제작했다. 원본 및 라이선스는 ArtSource/Production/human_base에 있다.

CommandoRigged.blend는 어깨, 팔, 손가락, 척추, 골반, 다리, 발끝을 포함한 52개 뼈대를 가진 편집 원본이다. Unreal 가져오기 과정에서 최상위 뼈가 추가되어 런타임에는 53개로 표시된다.

걷기와 달리기는 CMU의 실제 모션캡처를 인물 뼈대에 맞게 변환했다. 전후좌우 이동과 반복 구간을 편집했다. 대기·사격·재장전·피격·회피·사망은 별도 제작한 동작이다. 원본 데이터, 출처, 조건은 ArtSource/Production/Mocap/SOURCE_AND_LICENSE.md, 동작 목록은 animation_manifest.json에 보관했다.

The data used in this project was obtained from mocap.cs.cmu.edu. The database was created with funding from NSF EIA-0196217.

## 음악과 효과음

영화 음악이나 영화의 효과음을 복제하지 않았다.

- Infiltration, Contact, Siege, Homeward: 프로젝트에서 작곡한 MIDI를 macOS 기본 악기로 렌더링한 네 곡. MIDI·WAV와 음량 검사 기록은 ArtSource/Production/Music에 있다.
- Forest: 합성한 바람·곤충·새 환경음.
- Rifle, Arrow, Explosion, Hit, Pickup: 게임 이벤트에 연결된 합성 효과음.
- JungleScore: 이전 버전의 192초 합성 음악을 원본 보존용으로 남겨 두었다.

## 글꼴

나눔고딕. 설치된 Unreal Engine의 NanumGothic.ttf를 프로젝트에 복사했으며 Fonts 폴더에 라이선스 원문을 동봉했다.

## 기획 참고

사용자 제공: https://namu.wiki/w/람보%20시리즈/게임

고전 람보 게임에서 영감을 받은 독립적인 개발 시제품이다. 공식 라이선스 게임이라고 표방하지 않는다. 그래픽·분량·완성도는 실제 플레이 검증 결과와 함께 판단해야 한다.


## 0.3 지역 환경과 구조 헬기 (2026-10-03)

- 현수교·교량 통제문·크레인, 구조 수송 헬기와 회전 날개·슬라이딩 문, 방벽·벙커·격납고: 이 프로젝트에서 작성한 Blender 기하 모델. 편집 원본과 생성 스크립트 포함.
- 강변 배·선착장·어망, 우물·시장 가판·빨랫줄, 케이블 릴·광차·공기 압축기, 레이더·방공 발사대·급유 시설: 이 프로젝트에서 작성한 Blender 기하 모델 12종.
- 수상 가옥·마을 상점·보급 창고: 추가 건물 3종. 기존 오두막과 다른 구조·지붕·출입구 사용.
- 구조 헬기 로터 소리: 이 프로젝트에서 합성한 8초 루프, PCM 48 kHz/16-bit/mono. 원본 RescueRotor.wav 포함.
- 피부·추가 머리카락·등산화 자료: MakeHuman Community 공식 CC0 system asset pack. 공식 안내: https://static.makehumancommunity.org/assets/assetpacks/makehuman_system_assets.html . 원본 mhmat/mhclo에 CC0 고지와 제작자 정보 보존. ArtSource/Production/MakeHumanSystemAssets/source.json 및 원본 파일 참조. 신체의 기존 모양에 맞춰 다시 맞춘 뒤 기존 52개 뼈에 연결함.
- 포로와 엔딩용 대기·걷기·달리기: 기존 CMU 보행 원본과 직접 작성한 대기 자세에서 팔의 무기 고정 자세를 제거한 별도 동작. CivilianMotion.blend 및 civilian_animation_manifest.json 포함.

위 지역 소품은 실제 3D 모델입니다. 별도 이미지 생성 모델로 만들었다고 표시하지 않습니다. Higgsfield GPT Image 2.5 사용이 확인된 자료는 Trailer 폴더의 시네마틱 이미지 시트이며 제작 기록은 그 폴더에 보관되어 있습니다.

## 0.4 표면 재질 개선

Surface04 폴더의 콘크리트, 훼손된 회벽, 낡은 금속 도장, 골함석, 아스팔트, 데님 조직, 암반 지형, 흙바닥, 자갈, 벗겨진 녹색 도장과 불규칙한 녹슨 금속은 Poly Haven CC0 자료입니다. 색상/DirectX 노멀/AO-거칠기-금속성 2K 자료 33개를 원본 그대로 보관했습니다. 각 파일의 공식 URL, 제작자, MD5 일치 검증값은 Surface04/source_manifest.json을 참조하십시오.

이 원본을 게임의 23개 표면 재질과 6개 지역 지면에 연결했습니다. 기존 0.3 재질 자산도 보존했습니다. 새 재질은 Unreal의 Materials04 폴더에서 편집하며, 생성 코드는 Tools/Surfaces/import_pbr_surfaces.py입니다.

0.4의 강 수면은 Unreal Single Layer Water와 이 프로젝트에서 작성한 수학식 잔물결입니다. M04_CombatBoots는 기존 MakeHuman 신발 원본 이미지를 그대로 참조하고 셰이더에서 가죽 색을 조정했습니다. 외부 생성 이미지라고 표시하지 않습니다.


## 0.6 인물 복장과 장비

- MakeHuman Community 공식 CC0 system asset pack의 male_casualsuit02, male_casualsuit06, male_worksuit01, short01, short03, eyebrow003 원본을 보존했다. 각 mhclo/mhmat의 CC0 고지와 제작자 정보, 기존 MakeHumanSystemAssets/source.json을 함께 제공한다.
- 경비병·정찰병·중무장병·민간인·노동자 5종은 기존 인체와 52개 뼈에 옷을 맞추고 웨이트를 옮겨 만든 파생 메시다. 새 얼굴 5종을 제작한 것은 아니다. 헬멧·정찰모·방탄복·장구류는 이 프로젝트의 Blender 스크립트로 모델링했다.
- 편집 원본은 ArtSource/Production/Characters06/CharacterRoles06.blend, 재제작 스크립트는 Tools/Character/build_role_variants.py, 언리얼 변환은 import_role_variants.py다. 이전 CommandoVisual03 원본을 덮어쓰지 않았다.
- 위장무늬 이미지는 기존 Camouflage.png를 재사용한다. 0.6 복장 작업을 새 GPT Image 2.5 이미지 생성이라고 표시하지 않는다. 데님 색상은 원본 이미지를 사용하며, 언리얼의 자동 노멀맵 오인식을 막기 위해 색상/노멀의 압축 형식을 명시한다.

## 0.7 방향별 쓰러짐과 낙하 무기

기존 52개 관절에 직접 제작한 4방향 동작과 의상 두께 보정 동작 3개를 사용합니다. 새 모션 캡처 자료를 구매·가져오지 않았습니다. 낙하용 FieldRifleDrop07은 기존 직접 제작한 FieldRifle의 별도 사본이며, Unreal에서 충돌 덩어리와 마찰·반발값을 구성했습니다. 이전 원본은 보존했습니다.

## 0.8 지역별 소품

Props08의 야영·진료 천막, 배 수리대, 부엌, 작업대, 지휘 테이블, 화물 수레, 불탄 쉼터는 직접 제작한 형상입니다. FuelStation08은 이전 직접 제작 모델의 표면 노멀을 보정한 사본입니다. 기존 CC0 재질과 직접 작성한 재질을 적용했습니다. 새 외부 유료 자료나 생성 이미지는 사용하지 않았습니다.
