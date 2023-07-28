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

	// Online Subsystem �׼���
	// ������ ��ȯ
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

	// ��ȿ���� üũ
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

	//NAME_GameSession�� ����ϸ� �� �̸��� ���� ������ �����ϴ��� �׻� Ȯ���� �� �ִ�.
	auto ExistingSession = m_OnlineSessionInterface->GetNamedSession(NAME_GameSession);

	// ���� �ƴ϶�� �̹� �ִٴ°��̱� ������ ������ �����Ѵ�.
	if (ExistingSession != nullptr)
	{
		m_OnlineSessionInterface->DestroySession(NAME_GameSession);
	}

	// �¶��μ��� �������̽��� ��������Ʈ ����Ʈ�� ��������Ʈ �߰�
	// ������ �����ϰ� ���� �� ��������Ʈ���� ���ε��� �ݹ� ����� ȣ��ȴ�.(OnCreateSessionComplete() �Լ�)
	m_OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(m_CreateSessionCompleteDelegate);

	// ����Ʈ �����͸� �ʱ�ȭ�ϴ� ���
	//FOnlineSessionSettings ���� ���� ������ ���� ���
	TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
	// ���ͳ����� �Ұ��̱� ������ ����ġ x
	SessionSettings->bIsLANMatch = false;
	// ���� �÷��̾� �ѿ�
	SessionSettings->NumPublicConnections = 4;
	// ���� ���� �������̸� �ٸ������� ���� ��������
	SessionSettings->bAllowJoinInProgress = true;
	// ������ ���� ������ ������ �� �����Ѵٴ� ��?
	// �� ���� ��� �÷��̾���� ������ �ƴ� ������ �ְ� �������� �߻��ϴ� ���� �˻��ϵ���
	SessionSettings->bAllowJoinViaPresence = true;
	// ������ ������ �����ؼ� �ٸ� �������� ������ �� �ִ�.
	SessionSettings->bShouldAdvertise = true;
	// ���� �������� �������� ������ ã�´�
	SessionSettings->bUsesPresence = true;
	SessionSettings->bUseLobbiesIfAvailable = true;

	// Ű�� ������ ���� ��ġ Ÿ���� ����
	// EOnlineDataAdvertisementType �� ���� �ɼ��� ����
	// ViaOnlineServiceAndPing�� �¶��� ���񽺿� ������ ������ �� �ִ�,
	SessionSettings->Set(FName("MatchType"), FString("FreeForAll"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// ���� �÷��̾ ���� ��ID�� ���� �� �ִ�.
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	// ���� ����
	m_OnlineSessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings);
}

void AMenuSystemCharacter::JoinGameSession()
{
	// ���� ������ ã�´�
	// �������̽��� �����Ͱ� ��ȿ���� �ʴٸ� ����
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

	// ��������Ʈ����Ʈ�� ��������Ʈ ����
	m_OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(m_FindSessionsCompleteDelegate);

	m_SessionSearch = MakeShareable(new FOnlineSessionSearch());
	// ������ ã�� ����?
	m_SessionSearch->MaxSearchResults = 10000;
	// LAN�� ������� �ʱ⋚���� false
	m_SessionSearch->bIsLanQuery = false;
	// ��Ī ������ ã�� ���� ����
	// Find ������ ȣ���ϰ� ���� �˻��� �����ϸ� ���� ������ �����ؼ� �츮�� ã�� ������ ����ϵ��� ��?
	m_SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	// ���� �÷��̾ ���� ��ID�� ���� �� �ִ�.
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	// ������ ȣ���ϰ� ������ �Ϸ�Ǹ� �������� �ݹ��Լ��� ȣ�� ��û
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
	// �������̽��� ��ȿ����
	if (!m_OnlineSessionInterface.IsValid())
		return;

	for (auto Result : m_SessionSearch->SearchResults)
	{
		// ���� ID ���ڿ��� �ش� �Լ��� ���� �� �ִ�.
		FString Id = Result.GetSessionIdStr();
		FString User = Result.Session.OwningUserName;
		FString MatchType;

		// ������ ������ MatchType�� Ű������ ����� Value���� MatchType�̶� FString������ ����
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

		// IP�ּҸ� ������ ���ǿ� �����ؾ��Ѵ�
		// ������ ������� �Ʒ��� if���� ��
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

			// JoinSession ȣ��
			const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
			// ������ ȣ���ϰ� ������ �Ϸ�Ǹ� �������� �ݹ��Լ��� ȣ�� ��û
			m_OnlineSessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, Result);
		}
	}
}

void AMenuSystemCharacter::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (!m_OnlineSessionInterface.IsValid())
		return;

	// ������ IP�ּҸ� ���´�.
	// ���ǿ� ��ϵ� IP�ּҸ� ������ ������ IP�ּҸ� ���� �˾Ƴ��� ���� �ʿ䰡����.
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
