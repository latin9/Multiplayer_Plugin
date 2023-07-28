// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem()	:
	m_CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	m_FindSessionCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionComplete)),
	m_JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	m_DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	m_StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (Subsystem)
	{
		m_SessionInterface = Subsystem->GetSessionInterface();
	}
}

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
	// ��ȿ���� üũ
	if (!m_SessionInterface.IsValid())
	{
		return;
	}

	auto ExistingSession = m_SessionInterface->GetNamedSession(NAME_GameSession);

	// ���� �ƴ϶�� �̹� ������ �����Ǿ� �ִٴ°�
	if (ExistingSession != nullptr)
	{
		m_bCreateSessionOnDestroy = true;
		m_LastNumPublicConnections = NumPublicConnections;
		m_LastMatchType = MatchType;

		DestroySession();
	}

	// m_CreateSessionCompleteDelegateHandle�� ���߿� ��������Ʈ ��Ͽ��� ���� �� �ִ�.
	m_CreateSessionCompleteDelegateHandle = m_SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(m_CreateSessionCompleteDelegate);

	m_LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	// LAN���� �ƴ����� �ٶ� �ڵ����� �����ȴ�.
	m_LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	m_LastSessionSettings->NumPublicConnections = NumPublicConnections;
	m_LastSessionSettings->bAllowJoinInProgress = true;
	m_LastSessionSettings->bAllowJoinViaPresence = true;
	m_LastSessionSettings->bShouldAdvertise = true;
	m_LastSessionSettings->bUsesPresence = true;
	m_LastSessionSettings->bUseLobbiesIfAvailable = true;
	m_LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	// �̰��� 1�� �����ϸ� ���� ����ڰ� ��ü ���� �� ȣ������ ������ �� �ִٰ� �Ѵ�.
	m_LastSessionSettings->BuildUniqueId = 1;

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	// ���� ���н� �Ʒ��� ��
	if (!m_SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *m_LastSessionSettings))
	{
		// ���� ���н� ��������Ʈ ����Ʈ���� �ش� ��������Ʈ ����
		m_SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(m_CreateSessionCompleteDelegateHandle);

		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::FindSession(int32 MaxSearchResults)
{
	if (!m_SessionInterface.IsValid())
		return;

	m_FindSessionCompleteDelegateHandle = m_SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(m_FindSessionCompleteDelegate);

	m_LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	m_LastSessionSearch->MaxSearchResults = MaxSearchResults;
	m_LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	m_LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	// ���� �÷��̾ ���� ��ID�� ���� �� �ִ�.
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	// ������ ȣ���ϰ� ������ �Ϸ�Ǹ� �������� �ݹ��Լ��� ȣ�� ��û
	if (!m_SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), m_LastSessionSearch.ToSharedRef()))
	{
		// ������ ����
		// ��������Ʈ ����
		m_SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(m_FindSessionCompleteDelegateHandle);

		// �����Ѱ��̱� ������ �� �迭�� false�� ����
		MultiplayerOnFindSessionComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!m_SessionInterface.IsValid())
	{
		// UnknownError : EOnJoinSessionCompleteResult�� ����� �ٷ��� ���� ������ �߻�������
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	m_JoinSessionCompleteDelegateHandle = m_SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(m_JoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!m_SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		m_SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(m_JoinSessionCompleteDelegateHandle);

		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

void UMultiplayerSessionsSubsystem::DestroySession()
{
	if (!m_SessionInterface.IsValid())
	{
		MultiplayerOnDestroySessionComplete.Broadcast(false);

		return;
	}

	m_DestroySessionCompleteDelegateHandle = m_SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(m_DestroySessionCompleteDelegate);

	if (!m_SessionInterface->DestroySession(NAME_GameSession))
	{
		m_SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(m_DestroySessionCompleteDelegateHandle);
		MultiplayerOnDestroySessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::StartSession()
{
	if (!m_SessionInterface.IsValid())
		return;

	m_StartSessionCompleteDelegateHandle = m_SessionInterface->AddOnStartSessionCompleteDelegate_Handle(m_StartSessionCompleteDelegate);

	if (!m_SessionInterface->StartSession(NAME_GameSession))
	{
		m_SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(m_StartSessionCompleteDelegateHandle);

		MultiplayerOnStartSessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (m_SessionInterface)
	{
		m_SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(m_CreateSessionCompleteDelegateHandle);
	}

	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnFindSessionComplete(bool bWasSuccessful)
{
	if (m_SessionInterface)
	{
		m_SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(m_FindSessionCompleteDelegateHandle);
	}

	if (m_LastSessionSearch->SearchResults.Num() <= 0)
	{
		// �迭�� ����ִٸ� �� �迭�� false�� ����
		MultiplayerOnFindSessionComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	MultiplayerOnFindSessionComplete.Broadcast(m_LastSessionSearch->SearchResults, bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (m_SessionInterface)
	{
		m_SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(m_JoinSessionCompleteDelegateHandle);
	}

	MultiplayerOnJoinSessionComplete.Broadcast(Result);
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (m_SessionInterface)
	{
		m_SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(m_DestroySessionCompleteDelegateHandle);
	}

	if (bWasSuccessful && m_bCreateSessionOnDestroy)
	{
		// ������ �����ϴ��� �Ǽ��� ������ �������� �ʴ´�.
		m_bCreateSessionOnDestroy = false;
		CreateSession(m_LastNumPublicConnections, m_LastMatchType);
	}

	MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (m_SessionInterface)
	{
		m_SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(m_StartSessionCompleteDelegateHandle);
	}

	if (bWasSuccessful)
	{
		MultiplayerOnStartSessionComplete.Broadcast(bWasSuccessful);
	}
}
