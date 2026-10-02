# 제작 원본 스크립트

일반 플레이에는 이 스크립트가 필요하지 않습니다. 상위 폴더의 `게임시작.command`를 사용하세요.

개발자가 수정하는 경우:

- compose_audio.py: NumPy로 오리지널 음악과 효과음을 재생성합니다. 이 Mac의 Blender 5.2 내장 arm64 Python/NumPy로 실행했습니다.
- model_assets.py: Blender 백그라운드 모드에서 개별 FBX를 생성합니다.
- create_assets.py / ground_material.py / soft_material.py: Unreal Editor Python으로 맵과 재질을 만듭니다. 최초 생성용이며 이미 존재하는 에셋에는 중복 실행하지 않습니다.
- import_production_assets.py: Unreal Editor Python으로 오디오와 FBX를 가져옵니다. FBX 경고창을 사용하는 임포터 때문에 GUI 초기화가 없는 `-run=pythonscript -nullrhi` 대신 에디터의 `-ExecutePythonScript=` 방식으로 실행해야 합니다.

엔진: /Users/Shared/Epic Games/UE_5.8
빌드: Mac / arm64 / Development.
이 환경에서는 UBA의 응답 파일 처리 문제를 피하기 위해 UBT에 `-NoUBA -NoUbaLocal`을 전달했습니다.
Xcode 앱 조립을 분리할 경우에는 실제 Xcode 조립을 완료한 뒤 `UE_BUILD_FROM_XCODE=1`로 UBT의 중복 후처리를 생략했습니다. 게임 서명은 이 Mac에서 실행하는 로컬 서명입니다.
최종 앱은 Saved/StagedBuilds/Mac/LastExtraction.app에서 복사합니다. Binaries/Mac의 실행 바이너리만 복사하면 게임 자료가 빠지므로 사용하지 않습니다.
