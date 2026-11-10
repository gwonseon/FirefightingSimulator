# FireFightSumul 불 구현 인수인계

## 다음 채팅에서 할 일

Unreal Engine 5.5 C++ 프로젝트 `FireFightSumul`에서 **불 MVP를 구현**한다. 작업 순서는 열쇠·문, 불, 소화기다. 이번 단계에서는 불의 발화·화세·번짐과 오브젝트 풀까지 연결하고, 소화기를 붙이기 전에 임시 디버그 입력으로 불을 끌 수 있게 한다. 프로젝트 전체 맥락은 [FireTrainingDesign.md](FireTrainingDesign.md)에 있다.

## 현재 프로젝트 상태

- `AKeyPickup`과 `ALockableDoor`가 구현되어 있다. `E`로 잠금을 전환하고, 잠금이 풀린 문은 좌클릭으로 로컬 Z축을 기준으로 서서히 개폐된다. `Open Speed`는 블루프린트에서 수정할 수 있다.
- `AFireFighter`와 `UPlayerInteractionComponent`가 입력을 처리한다. 기존 `IInteractable::Interact()`는 `E` 상호작용의 진입점이다. 불의 진화는 `AFireActor::ApplySuppression()`으로 연결한다.
- `AWorldExtinguisher`에는 검사 카메라·핀 컴포넌트와 `TryRemovePin()`이 있고, `AEquippedExtinguisher`에는 호스·노즐 컴포넌트가 있다. 플레이어의 검사·장착·분사 연결은 다음 단계다.
- 인수인계 작성 시점에 `AFireActor`, `AFireManager`, 불 전용 Niagara·플립북 에셋은 소스 및 에셋 목록에서 확인되지 않았다. 문 변경 후 `FireFightSumulEditor Win64 Development` 빌드는 성공했고, 실제 맵에서 문을 조작하는 PIE 검증은 확인하지 못했다.
- 작업 맵은 `Content/Map/Main.umap`이다. 맵 에셋에서 `BP_GameMode` 참조를, 해당 블루프린트에서 `BP_FireFighter` 참조를 확인했다. 전역 기본 맵은 템플릿 `FirstPersonMap`이므로 PIE는 `Main`을 열어서 확인한다.

## 클래스와 파일 배치

```text
AMapObject
└─ AFireActor                     화세·진화·성장, 분사 판정, Niagara
AActor
├─ AFireManager                   최초 발화, 번짐, 셀 점유, 고정 크기 풀
└─ ATargetPoint                   엔진 기본 클래스, 최초 발화 위치 표시
```

신규 파일은 `Source/FireFightSumul/Public/MapObject/Fire/`와 `Source/FireFightSumul/Private/MapObject/Fire/`에 `FireActor.h/.cpp`, `FireManager.h/.cpp`를 둔다. 기존 `AMapObject` 헤더는 현재 `Public/MapObjects/MapObject.h`에 있다. 불 구현 시작 시 실제 경로를 확인해 include하고, 기존 파일 재배치는 별도 작업으로 다룬다.

| 클래스 | 소유하는 값과 진입점 |
|---|---|
| `AFireActor` | `CurrentIntensity`, `MaxIntensity`, 강화 임계값, 성장률, 마지막 분사 이후 성장 재개 지연. `ActivateFire()`, `ApplySuppression(Amount)`, `DeactivateFire()`를 제공한다. 소화되면 매니저에 알린다. |
| `AFireManager` | `InitialIgnitionPoints`, 발화 타이머·이벤트, 활성 불과 셀 상태, 고정 크기 액터 풀, 번짐 검사 주기. 발화는 `TryIgniteAt(Location, Cause)`로 모은다. |
| `ATargetPoint` | 에디터에서 직접 배치하고 매니저의 최초 발화 배열에 등록한다. 번짐 위치는 런타임에 계산한다. |

## 화세와 풀 수명 주기

- **화세 상태.** 0이면 완전 소화, 0 초과부터 최대치의 60% 미만이면 약화, 60% 이상이면 강화다. 60%는 에디터에서 조절할 시작값이다. 진화량은 양수만 반영하고 화세를 0과 최대치 사이로 제한한다.
- **성장.** 번져서 생긴 불은 약화 상태로 시작한다. 마지막 분사 이후 일정 시간이 지나면 다시 성장할 수 있다. 완전 소화된 불은 성장을 종료한다.
- **풀.** 매니저가 시작 시 고정 수량을 준비한다. 대기 액터는 숨기고 분사 충돌과 Niagara를 끈다. 획득 시 위치, 화세, 성장·번짐 시간, 충돌, Niagara 파라미터를 초기화한다. 완전 소화 시 판정과 타이머를 정리하고 즉시 반납한다. 잔연기 이후 반납은 후속 작업이다.
- **셀 상태.** `Available → Burning → Extinguished`는 매니저가 위치별로 기록한다. 액터를 반납해도 진화 완료 셀은 이번 시나리오에서 계속 제외한다. 풀의 여유와 연소 중인 셀 수는 별도로 확인한다.

## 최초 발화와 런타임 번짐

1. **최초 발화.** 시연에서는 `TargetPoint` 한 곳을 고정 타이머나 이벤트로 한 번 발화시킨다. 최초 불은 `TargetPoint` 좌표에 놓고 해당 셀을 점유한다. 확률 발화를 추가할 때는 타이머마다 확률을 한 번 판정한 뒤 가용 후보 한 곳을 선택한다.
2. **후보 탐색.** 강화 상태를 약 5초 유지한 불의 주변을 검사한다. 맵에 배치하지 않는 약 1m 간격의 논리 격자를 쓰고, 상하좌우 네 후보 중 최대 한 곳을 점화한다. 불이 약화되거나 꺼지면 그 불의 번짐 대기를 초기화한다.
3. **공간 검사.** 후보에서 아래로 Trace해 바닥에 위치를 맞춘다. 첫 시연 범위는 한 층이다. 허용 높이 차이를 넘는 후보, 벽이나 닫힌 문으로 막힌 후보, 새 불이 들어갈 공간이 없는 후보는 제외한다. 문은 실제 문짝 충돌을 이용해 차폐를 판정한다.
4. **점유와 풀.** `TryIgniteAt()`에서 셀 점유, 동시 화재 한도, 풀 여유를 확인한다. 동일 셀에 발화 요청이 겹쳐도 불은 한 개만 생긴다. 활성화가 실패하면 풀과 점유를 되돌린다. 이미 번진 불은 출발점의 불을 꺼도 독립적으로 유지된다.

## Niagara와 소화기 연결 계약

- **표현.** Niagara Sprite Renderer에 플립북 텍스처를 연결하고 `User.Intensity`에 정규화된 화세를 전달한다. 첫 시연은 불꽃 시스템 하나로 진행한다. 화세 변경과 분사 판정은 C++이 관리하고 Niagara는 결과를 표현한다. C++에서 `UNiagaraComponent`를 사용할 경우 `FireFightSumul.Build.cs`의 Niagara 모듈 의존성을 확인한다.
- **소화기 연결.** 다음 단계에서 장착형 소화기가 분사 Trace로 불을 맞힌 뒤 `ApplySuppression(Amount)`를 호출한다. 원본 배치형 소화기가 약제 잔량을 소유하고, 실제 분사에 사용한 약제량에 비례해 진화량을 계산한다. 이번 단계의 임시 입력은 같은 함수를 호출해 이 연결을 미리 검증한다.

## 구현 순서와 완료 기준

| 순서 | 작업 | 검증 |
|---|---|---|
| 1 | `AFireActor`의 화세·분사 판정과 `AFireManager`의 최소 풀 연결 | 불 하나에 임시 진화량을 적용하면 `강화 → 약화 → 완전 소화`가 된다. |
| 2 | `TargetPoint`와 최초 발화 타이머 연결 | 맵에서 한 번 발화하고, 소화한 액터를 다른 셀에서 다시 획득해도 이전 화세·타이머·이펙트가 남지 않는다. |
| 3 | 한 층에서 인접 셀 번짐 연결 | 열린 공간에서 한 칸 번진다. 벽·닫힌 문 뒤, 이미 점유 또는 진화된 셀에는 번지지 않는다. 원래 불을 일찍 약화시키면 번짐이 멈춘다. |
| 4 | Niagara 플립북 연결과 `Main` PIE 검증 | 불꽃이 화세에 따라 달라지고, 한 번은 번지기 전에 진화하고 한 번은 번진 두 불을 모두 끈다. |

하루 MVP의 우선순위는 **한 층에서 최초 발화 → 화세 감소 → 완전 소화 → 풀 재사용 → 한 칸 번짐**이다. 초기 동시 화재 한도는 2~3개로 잡고, 확률 발화·다층 번짐·잔연기는 후속 작업으로 둔다.

## 새 채팅에 붙여 넣을 요청

> 프로젝트 루트의 `FireImplementationHandoff.md`를 읽고 현재 소스 상태를 확인한 뒤, 불 MVP를 구현해줘. 기존 열쇠·문을 유지하고 `AFireActor`와 `AFireManager`의 화세·최초 발화·고정 풀을 먼저 연결해줘. 임시 진화 입력으로 검증하고, 한 층 인접 셀 번짐과 Niagara 플립북까지 진행해줘. 소화기 장착·분사는 다음 단계라서 `ApplySuppression()` 진입점만 준비해줘.
