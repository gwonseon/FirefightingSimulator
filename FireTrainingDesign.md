# FireFightSumul 구현 설계

## 범위

3D 1인칭 화재 훈련에서 열쇠로 문을 열고, 소화기의 핀을 뽑아 화재를 진압한다. 최초 발화 위치는 레벨의 `ATargetPoint`로 지정한다. 이후 번짐은 주변 바닥을 검사해 결정하고, 불 액터는 풀에서 재사용한다. 문서는 구현 기준이며 아래 초기 수치는 에디터에서 조정할 값이다.

## 폴더와 클래스

`Source/FireFightSumul/Public`과 `Private`에 같은 분류를 사용한다. 아래 트리는 정리 후 목표 위치를 표시한다. 모듈과 GameMode 파일은 모듈 루트에 둔다.

```text
Source/FireFightSumul/
├─ Public/
│  ├─ Player/
│  │  ├─ FireFighter.h
│  │  ├─ PlayerInteractionComponent.h
│  │  └─ EquippedExtinguisher.h
│  ├─ MapObject/
│  │  ├─ MapObject.h
│  │  ├─ KeyPickup.h
│  │  ├─ LockableDoor.h
│  │  ├─ WorldExtinguisher.h
│  │  ├─ BreakableGlass.h
│  │  └─ Fire/
│  │     ├─ FireActor.h
│  │     └─ FireManager.h
│  ├─ Interface/
│  │  └─ Interactable.h
│  └─ UI/
├─ Private/
│  ├─ Player/
│  │  ├─ FireFighter.cpp
│  │  ├─ PlayerInteractionComponent.cpp
│  │  └─ EquippedExtinguisher.cpp
│  ├─ MapObject/
│  │  ├─ MapObject.cpp
│  │  ├─ KeyPickup.cpp
│  │  ├─ LockableDoor.cpp
│  │  ├─ WorldExtinguisher.cpp
│  │  ├─ BreakableGlass.cpp
│  │  └─ Fire/
│  │     ├─ FireActor.cpp
│  │     └─ FireManager.cpp
│  ├─ Interface/
│  └─ UI/
├─ FireFightSumul.Build.cs
├─ FireFightSumul.h/.cpp
└─ FireFightSumulGameMode.h/.cpp
```

| 타입 | 상속 및 책임 |
|---|---|
| `AFireFighter` | `ACharacter`를 상속한다. 기존 이동·달리기·점프·시점과 상호작용 입력을 바인딩하고 `TSet<FName>`으로 보유 열쇠를 관리한다. |
| `UPlayerInteractionComponent` | 플레이어가 소유하는 `UActorComponent`다. 시선 Trace, 조작 대상과 모드, 검사 카메라·커서 전환, 장착과 조준선 표시를 조율한다. |
| `AEquippedExtinguisher` | `AActor`를 상속한다. 카메라에 붙는 호스·노즐, 조준과 분사 판정, 원본 소화기 참조를 관리한다. |
| `AMapObject` | 공통 `SceneRoot`를 가진 추상 `AActor`다. `AKeyPickup`, `ALockableDoor`, `AWorldExtinguisher`, `ABreakableGlass`, `AFireActor`가 상속한다. |
| `UInteractable` / `IInteractable` | 문과 배치형 소화기의 `E` 동작을 `Interact(AFireFighter*)`로 노출한다. 호출부는 `IInteractable::Execute_Interact()`를 사용한다. |
| `AFireManager` | 훈련 맵에 하나를 배치하는 `AActor`다. 최초 발화 후보, 타이머와 이벤트, 주변 셀 검사, 활성 화재와 풀을 관리한다. |
| `ATargetPoint` | 엔진 기본 액터다. 매니저의 `InitialIgnitionPoints`에 등록해 최초 발화 위치를 지정한다. |
| `WBP_Crosshair` | `UUserWidget` 기반 블루프린트다. 첫 구현에서는 `Content/FireTraining/UI`에 두고 UI C++이 필요해지면 `UI` 폴더를 사용한다. |

현재 `FireFighter`는 이미 `Player`에 있다. 기존 `MapObjects`의 액터 파일은 구현 단계에서 `MapObject`로, `Interactable.h`는 `Interface`로, 루트의 `EquippedExtinguisher`는 `Player`로 옮기고 include 경로를 함께 갱신한다. 템플릿 파일은 현재 위치를 유지한다.

## 플레이어와 맵 오브젝트

| 모드 | `E` | 좌클릭 |
|---|---|---|
| `Free` | 시선 대상의 `Interact()` 호출 | 시선 대상이 문이면 `TryToggleOpen()` 호출 |
| `InspectingExtinguisher` | 검사 취소 | 핀을 잡고 드래그하며 놓거나 취소하면 드래그 종료 |
| `AimingExtinguisher` | 첫 버전에서는 별도 동작을 배정하지 않음 | 누르는 동안 분사하고 놓거나 취소하면 중지 |

- **열쇠·문.** 열쇠는 오버랩 시 `KeyId`를 등록한다. 문은 닫힌 상태에서 일치하는 `RequiredKeyId`를 가진 플레이어에게 잠금 전환을 허용한다. 잠금이 해제된 문은 좌클릭으로 경첩을 회전한다. 애니메이션을 추가할 때는 회전 중 추가 개폐와 재잠금을 막는다.
- **검사·핀.** 소화기에 `E`를 누르면 해당 액터를 조작 대상으로 저장하고 검사 카메라로 전환한다. 커서 Trace로 핀을 잡은 뒤 핀 축 방향의 이동량을 계산한다. 임계 거리 전에 놓으면 원위치한다. 성공하면 핀을 제거하고 장착형을 카메라에 붙인 뒤 배치형 본체의 메시와 충돌을 끈다. 성공·취소·대상 제거 시 한 정리 경로에서 시점과 이동·커서 상태를 복구한다.
- **잔량·분사.** 배치형 원본이 현재 잔량과 핀 상태를 소유한다. 최대 용량, 초당 소모량, 약제 단위당 진화량은 블루프린트에서 조정한다. 장착형은 일정 간격으로 원본에서 실제 소모된 양을 받아, 조준 방향에 맞은 불에 `ApplySuppression(Amount)`를 호출한다. 빗나가도 잔량을 소모하며, 마지막 분사는 남은 양에 비례한다. HUD는 장착 모드에서 조준선을 표시한다.
- **유리.** `ABreakableGlass::BreakGlass()`에서 메시와 충돌을 끈다. 어떤 도구나 입력으로 파손할지는 시나리오 규칙을 정한 뒤 연결한다.

## 발화·진화·번짐

### 상태와 설정

| 소유자 | 데이터 |
|---|---|
| `AFireActor` | `CurrentIntensity`, `MaxIntensity`, 강화 임계값, 성장률, 마지막 분사 이후 성장 재개 지연. 화세가 양수이고 최대치의 60% 미만이면 약화, 60% 이상이면 강화, 0이면 완전 소화다. |
| `AFireManager` | 최초 발화 후보 배열, 랜덤 시드와 발화 확률, 동시 화재 한도, 풀 용량, 셀 크기, 번짐 간격. 셀 상태는 `Available → Burning → Extinguished`로 관리한다. |
| Niagara | `User.Intensity`로 불꽃 크기와 발생량을 조절한다. 불꽃·연기 에미터에 플립북 텍스처를 연결한다. |

### 최초 발화와 번짐

1. **최초 발화.** 매니저 타이머는 가용한 `TargetPoint` 후보가 있을 때 확률을 한 번 판정하고 성공 시 한 곳을 고른다. 시나리오 이벤트는 지정한 위치를 직접 요청한다. 최초 불은 지정된 좌표에 놓고 해당 좌표의 셀을 점유한다.
2. **번짐 후보.** 매니저는 강화 상태를 유지한 불만 처리한다. 초기값은 1m 셀과 5초 간격이다. 상하좌우 네 칸을 검사하고, 바닥 Trace로 후보 위치를 보정한다. 출발점과 높이 차가 허용 범위 이내이며, 벽이나 닫힌 문이 `FireSpread` Trace를 막지 않고, 불을 놓을 공간이 있는 곳만 남긴다. 셀 키에 층 구분을 포함해 위아래층의 같은 XY를 별도로 기록한다.
3. **공통 점화.** 랜덤·이벤트·번짐은 `TryIgniteAt(Location, Cause)`를 사용한다. 셀 상태, 동시 화재 한도, 풀 여유를 재확인한다. 조건을 통과하면 셀을 점유하고 액터를 활성화한다. 활성화에 실패하면 셀 점유와 풀 획득을 되돌린다. 강화 불 한 곳은 검사 주기마다 최대 한 칸만 점화한다.
4. **번짐 중단과 재개.** 새 불은 약화 상태에서 시작하고, 일정 시간 분사를 받지 않으면 성장한다. 강화 기준을 넘으면 그때부터 자기 번짐 대기를 시작한다. 약화 또는 완전 소화에 들어가면 진행 중인 번짐 대기를 초기화한다. 이미 번진 불은 각자의 화세를 유지한다.

### 풀 수명 주기

```text
대기 → 획득 및 초기화 → 활성 → 완전 소화 및 잔연기 → 반납 → 대기
```

매니저가 시작 시 풀을 준비하고, 대기 액터의 충돌과 Niagara를 끈다. 획득할 때 위치, 화세, 성장·번짐 타이머, 충돌, 이펙트를 새 발화에 맞게 초기화한다. 완전 소화 시 판정과 번짐은 즉시 종료하고, 잔연기 정리가 끝나면 풀에 반납한다. 정리 중인 액터는 재획득 대상에서 제외한다. **풀에 반납해도 진화된 셀의 기록은 시나리오 초기화까지 유지한다.**

## 구현 순서와 검증 기준

| 순서 | 구현 | 완료 기준 |
|---|---|---|
| 1 | 기존 소스 재배치 및 include 갱신. 훈련 맵의 GameMode·Pawn 확인. | Editor 타깃 빌드가 통과하고 훈련 맵에서 `AFireFighter`를 조종한다. 현재 전역 기본 맵과 GameMode는 First Person 템플릿 설정을 사용하므로 `Main` 맵의 실제 오버라이드를 확인한다. |
| 2 | `UPlayerInteractionComponent`, `E`·좌클릭 라우팅, 문 개폐 연결. | 열쇠 없이 잠금 해제가 거절된다. 맞는 열쇠를 얻으면 잠금 해제, 개폐, 닫힌 문 재잠금이 된다. |
| 3 | 소화기 검사, 핀, 장착, 약제 소모와 조준선 연결. | 중간에 놓거나 검사를 취소하면 입력이 복구된다. 핀 제거 후 호스가 보이며 분사 중에만 잔량이 감소한다. |
| 4 | 단일 `AFireActor`와 분사 판정 연결. | 강화 불이 분사를 받아 약화되고 화세 0에서 소화된다. 남은 약제만큼만 마지막 분사 효과가 적용된다. |
| 5 | `AFireManager`, 최초 발화와 풀 연결. | 타이머와 이벤트 발화가 같은 셀에 겹치지 않는다. 진화 후 같은 액터를 다른 위치에서 재사용해도 이전 상태가 남지 않는다. |
| 6 | 주변 셀 번짐 및 충돌 검사 연결. | 열린 공간에서 한 칸씩 번진다. 닫힌 문, 층 차이, 화재 한도, 점유 셀이 후보를 막는다. 원본 불을 약화시키면 번짐이 멈춘다. |
| 7 | Niagara 불꽃·연기 플립북과 훈련 맵 검증. | 화세에 따라 크기와 밀도가 달라지고 소화 후 잔연기가 정리된다. 여러 불이 활성화된 상황에서 성능과 진화 판정을 확인한다. |
