# FireFightSumul 외부 작업용 프로젝트

이 압축 파일은 Unreal Engine 5.5.4 편집용 소스 프로젝트입니다. 다른 PC에서 생성된 `Binaries`, `Intermediate`, `.sln` 등의 파일은 포함하지 않았습니다.

## 필수 환경

- Windows 10/11 64비트
- Unreal Engine **5.5.4**
- Visual Studio 2022
- 동봉된 `.vsconfig`에 지정된 C++ 구성 요소
  - MSVC 14.38
  - Windows SDK 10.0.22621
  - Desktop/Game development with C++

## 처음 실행하기

1. ZIP을 `C:\Work\FireFightSumul`처럼 짧은 영문 경로에 풉니다.
2. `.vsconfig`를 Visual Studio Installer로 열어 누락된 구성 요소를 설치합니다.
3. `SetupProject.cmd`를 더블클릭합니다.
4. 스크립트가 이 PC의 UE 경로로 프로젝트 파일을 생성하고 `FireFightSumulEditor`를 빌드한 뒤 에디터를 실행합니다.

기존 PC에서 받은 `.sln`, `Binaries`, `Intermediate`를 이 폴더에 복사하지 마십시오.

## 문제가 발생하면

`SetupLogs` 폴더의 다음 파일을 전달해 주십시오.

- `GenerateProjectFiles.log`
- `Build.log`

프로젝트의 전체 절대경로와 화면에 표시된 첫 번째 `ERROR` 문장도 함께 전달해 주십시오.

## 참고

- ZIP에는 실제 Unreal 에셋이 들어 있으므로 Git LFS를 별도로 실행할 필요가 없습니다.
- 소스 프로젝트는 외부 PC에서 최초 한 번 C++ 빌드가 필요합니다.
- 컴파일 없이 실행만 필요하다면 이 소스 프로젝트가 아니라 별도의 패키징된 Windows 빌드를 사용해야 합니다.
