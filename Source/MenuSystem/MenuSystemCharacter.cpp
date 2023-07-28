// Copyright Epic Games, Inc. All Rights Reserved.

#include "MenuSystemCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

//////////////////////////////////////////////////////////////////////////
// AMenuSystemCharacter

AMenuSystemCharacter::AMenuSystemCharacter() :
	m_CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	m_FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	m_JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// Online Subsystem 액세스
	// 포인터 반환
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

	// 유효한지 체크
	if (OnlineSubsystem)
	{
		m_OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();

		/*if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Blue,
				FString::Printf(TEXT("Found subsystem %s"),
				*OnlineSubsystem->GetSubsystemName().ToString()));
		}*/
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMenuSystemCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AMenuSystemCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AMenuSystemCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &AMenuSystemCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &AMenuSystemCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AMenuSystemCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AMenuSystemCharacter::TouchStopped);
}

void AMenuSystemCharacter::CreateGameSession()
{
	// Called when pressing the 1 key
	if (!m_OnlineSessionInterface.IsValid())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString(TEXT("OnlineSessionInterface not Valid"))
			);
		}
		return;
	}

	//NAME_GameSession을 사용하면 이 이름을 가진 세션이 존재하는지 항상 확인할 수 있다.
	auto ExistingSession = m_OnlineSessionInterface->GetNamedSession(NAME_GameSession);

	// 널이 아니라면 이미 있다는것이기 때문에 세션을 제거한다.
	if (ExistingSession != nullptr)
	{
		m_OnlineSessionInterface->DestroySession(NAME_GameSession);
	}

	// 온라인세션 인터페이스의 델리게이트 리스트에 델리게이트 추가
	// 세션을 생성하고 나면 이 델리게이트에게 바인딩한 콜백 기능이 호출된다.(OnCreateSessionComplete() 함수)
	m_OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(m_CreateSessionCompleteDelegate);

	// 스마트 포인터를 초기화하는 방법
	//FOnlineSessionSettings 에는 세션 세팅을 위해 사용
	TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
	// 인터넷으로 할것이기 때문에 랜매치 x
	SessionSettings->bIsLANMatch = false;
	// 게임 플레이어 총원
	SessionSettings->NumPublicConnections = 4;
	// 게임 세션 실행중이면 다른유저도 가입 가능한지
	SessionSettings->bAllowJoinInProgress = true;
	// 스팀이 게임 세션을 설정할 때 존재한다는 것?
	// 전 세계 모든 플레이어와의 연결이 아닌 지역이 있고 지역에서 발생하는 세션 검색하도록
	SessionSettings->bAllowJoinViaPresence = true;
	// 스팀이 세션을 광고해서 다른 유저들이 참여할 수 있다.
	SessionSettings->bShouldAdvertise = true;
	// 현재 지역에서 진행중인 세션을 찾는다
	SessionSettings->bUsesPresence = true;
	SessionSettings->bUseLobbiesIfAvailable = true;

	// 키와 벨류로 세션 매치 타입을 지정
	// EOnlineDataAdvertisementType 는 광고 옵션을 지정
	// ViaOnlineServiceAndPing은 온라인 서비스와 핑으로 광고할 수 있다,
	SessionSettings->Set(FName("MatchType"), FString("FreeForAll"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// 로컬 플레이어를 얻어와 넷ID를 얻을 수 있다.
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	// 세션 생성
	m_OnlineSessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings);
}

void AMenuSystemCharacter::JoinGameSession()
{
	// 게임 세션을 찾는다
	// 인터페이스의 데이터가 유효하지 않다면 리턴
	if (!m_OnlineSessionInterface.IsValid())
	{
		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Blue,
			FString(TEXT("JoinGameSession() Success"))
		);
	}

	// 델리게이트리스트에 델리게이트 저장
	m_OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(m_FindSessionsCompleteDelegate);

	m_SessionSearch = MakeShareable(new FOnlineSessionSearch());
	// 세션을 찾을 개수?
	m_SessionSearch->MaxSearchResults = 10000;
	// LAN을 사용하지 않기떄문에 false
	m_SessionSearch->bIsLanQuery = false;
	// 매칭 서버를 찾기 위한 쿼리
	// Find 세션을 호출하고 세션 검색을 전달하면 쿼리 세팅을 설정해서 우리가 찾는 세션을 사용하도록 함?
	m_SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	// 로컬 플레이어를 얻어와 넷ID를 얻을 수 있다.
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	// 세션을 호출하고 세션이 완료되면 응답으로 콜백함수를 호출 요청
	m_OnlineSessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), m_SessionSearch.ToSharedRef());
}

void AMenuSystemCharacter::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Blue,
				FString::Printf(TEXT("Created session : %s"), *SessionName.ToString())
			);
		}

		UWorld* World = GetWorld();

		if (World)
		{
			World->ServerTravel(FString("/Game/ThirdPerson/Maps/Lobby?listen"));
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString(TEXT("Failed to create session!"))
			);
		}
	}
}

void AMenuSystemCharacter::OnFindSessionsComplete(bool bWasSuccessful)
{
	// 인터페이스가 유효한지
	if (!m_OnlineSessionInterface.IsValid())
		return;

	for (auto Result : m_SessionSearch->SearchResults)
	{
		// 세션 ID 문자열을 해당 함수로 얻을 수 있다.
		FString Id = Result.GetSessionIdStr();
		FString User = Result.Session.OwningUserName;
		FString MatchType;

		// 위에서 저장한 MatchType의 키값으로 저장된 Value값을 MatchType이란 FString변수에 저장
		Result.Session.SessionSettings.Get(FName("MatchType"), MatchType);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Cyan,
				FString::Printf(TEXT("Id : %s, User : %s"), *Id, *User)
			);
		}

		// IP주소를 얻어내려면 세션에 참여해야한다
		// 세션이 맞을경우 아래의 if문에 들어감
		if (MatchType == FString("FreeForAll"))
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					15.f,
					FColor::Cyan,
					FString::Printf(TEXT("Joining Match Type : %s"), *MatchType)
				);
			}

			m_OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(m_JoinSessionCompleteDelegate);

			// JoinSession 호출
			const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
			// 세션을 호출하고 세션이 완료되면 응답으로 콜백함수를 호출 요청
			m_OnlineSessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, Result);
		}
	}
}

void AMenuSystemCharacter::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (!m_OnlineSessionInterface.IsValid())
		return;

	// 세션의 IP주소를 얻어온다.
	// 세션에 등록된 IP주소를 얻어오기 때문에 IP주소를 직접 알아내서 적을 필요가없다.
	FString Address;
	if (m_OnlineSessionInterface->GetResolvedConnectString(NAME_GameSession, Address))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Yellow,
				FString::Printf(TEXT("Connect string : %s"), *Address)
			);
		}

		APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
		if (PlayerController)
		{
			PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
		}
	}
}

void AMenuSystemCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AMenuSystemCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AMenuSystemCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AMenuSystemCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AMenuSystemCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AMenuSystemCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
