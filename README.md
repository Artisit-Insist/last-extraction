# Last Extraction

고전 구출 액션 게임에서 영감을 받은 Unreal Engine 5.8용 프로젝트입니다. 정글, 강과 교량, 마을, 채석장, 요새, 비행장을 지나 동료들과 함께 탈출합니다.

**전달 버전 0.9.0 — Mac Apple Silicon용.** 2026년 10월 3일 사용자가 현재 버전을 수락했으며, 실제 1시간 분량 조건은 철회했습니다. 6개 지역·42개 임무·12명 구출과 13명 전원 탈출을 검증했습니다. 상용 대작과 동등한 그래픽 품질을 보증하는 표시는 아닙니다.

[실행 앱과 영상 받기](https://github.com/Artisit-Insist/last-extraction/releases/tag/v0.9.0)에서 Mac ZIP을 내려받고 압축을 푼 뒤 `게임시작.command`를 더블클릭하세요. 실행 파일과 편집 원본, 트레일러는 Release에 함께 제공합니다.

## 게임을 실행하려면

전달 폴더의 `게임시작.command`를 더블클릭하십시오. Mac 실행 앱은 `MacGame/LastExtraction.app`입니다. 코딩이나 Unreal 설치 없이 실행하는 용도입니다. 이 프로젝트 폴더는 편집용 원본이며, 실행 앱은 별도로 전달합니다.

이동은 WASD, 조준은 마우스, 사격은 왼쪽 클릭입니다. 가까이에서 E를 누르면 구출·문서 회수·장비 조작을 할 수 있습니다. 포로는 먼저 풀어준 뒤 초록 표식의 보급 지점까지 호송합니다. 도착해서 무장시키면 지원 전투에 합류합니다. 임무를 끝내거나 포로를 풀어주면 자동으로 저장합니다. Esc 후 Q로 현재 진행을 저장하고 메뉴로 돌아갈 수도 있습니다.

## 내용을 편집하려면

Unreal Engine 5.8에서 `LastExtraction.uproject`를 엽니다. C++ 소스도 포함되어 있어 개발 환경에서는 처음 열 때 빌드가 필요할 수 있습니다. Blender 모델·리깅·동작·음악 원본은 `ArtSource/Production`, 가져오기와 재제작 스크립트는 `Tools`에 있습니다.

`Content`는 실제 게임 자료입니다. `Binaries`, `Intermediate`, `Saved`는 로컬 실행·빌드 중 생성되며 소스 저장소에서 제외합니다. 개인 플레이 저장 파일도 업로드하지 않습니다.

## 출처와 검증

- `Design/ASSET_PROVENANCE.md`: 이미지, 모델, 음악, 폰트의 출처.
- `Design/WORLD_03_VALIDATION.md`: 지역별 이동, 동료 전투와 헬기 탑승 검사.
- `Design/SURFACE_04_VALIDATION.md`: 0.4 재질과 물 표현 검증.
- `Design/MISSION_05_VALIDATION.md`: 0.5 호송·무장·저장 복원과 동료 충돌 검사.
- `Design/TACTICS_09.md`: 적의 엄폐·측면 이동·수색과 전투 배치 검사.
- `Design/PROPS_08_VALIDATION.md`: 지역별 소품과 임무 장소, 호송 지점 배치 검사.
- `Design/FALL_07_VALIDATION.md`: 방향별 쓰러짐, 복장 접촉 보정과 무기 물리 낙하 검사.
- `Design/CHARACTERS_06_VALIDATION.md`: 역할별 복장, 리깅과 구출 전후 외형 검사.
- `Design/PRODUCTION_CHECKLIST.md`: 완료된 항목과 남은 작업.

영화의 공식 라이선스 게임이라고 표방하지 않습니다. 제작 경과 시간과 업로드 확인은 Release 설명과 별도 제작 시간 기록에 기재합니다.
