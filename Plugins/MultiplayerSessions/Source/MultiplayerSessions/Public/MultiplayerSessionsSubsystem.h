// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerSessionsSubsystem.generated.h"

// 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsComplete, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete, bool, bWasSuccessful);


/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UMultiplayerSessionsSubsystem();

	// ���� ����� ó���ϱ� ���� �޴� Ŭ������ �̸� ȣ���Ѵ�.
	// ������ �÷��̾��, ��Ī Ÿ��
	// ���� ������ ȣ���ϸ� ���� �ý��ۿ����� ���� �������� Ű ���� ������ �� �ִ�.
	void CreateSession(int32 NumPublicConnections, FString MatchType);
	// 
	void FindSession(int32 MaxSearchResults);
	// ������ ã�Ƽ� � ������ �������� �����ϸ� SessionResult�� �����
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void DestroySession();
	void StartSession();


	// �޴� Ŭ������ �ݹ��� ���ε��ϱ� ���� ����� ���� ��������Ʈ
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	FMultiplayerOnFindSessionsComplete MultiplayerOnFindSessionComplete;
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionComplete;
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;
	FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionComplete;

protected:
	// ��������Ʈ�� ���ε��� �ݹ� �Լ�
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
	IOnlineSessionPtr m_SessionInterface;
	// ���� ������ ����
	TSharedPtr<FOnlineSessionSettings>	m_LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch>	m_LastSessionSearch;

	// �¶��� ���� �������̽� ��������Ʈ ����Ʈ�� �߰�
	// �÷��̾� ���� ���� �ý��� ���� �ݹ��Լ��� ���⿡ ���ε�ó��
	FOnCreateSessionCompleteDelegate	m_CreateSessionCompleteDelegate;
	// ��������Ʈ ����Ʈ�� �߰��ϴ� ���� �װ� ó���ϴ� ����� �ڵ鿡 ����
	FDelegateHandle m_CreateSessionCompleteDelegateHandle;
	FOnFindSessionsCompleteDelegate		m_FindSessionCompleteDelegate;
	FDelegateHandle m_FindSessionCompleteDelegateHandle;
	FOnJoinSessionCompleteDelegate		m_JoinSessionCompleteDelegate;
	FDelegateHandle m_JoinSessionCompleteDelegateHandle;
	FOnDestroySessionCompleteDelegate	m_DestroySessionCompleteDelegate;
	FDelegateHandle m_DestroySessionCompleteDelegateHandle;
	FOnStartSessionCompleteDelegate		m_StartSessionCompleteDelegate;
	FDelegateHandle m_StartSessionCompleteDelegateHandle;

	// �̰� Ȯ���ϰ� �ݹ��ϴµ� ������ �ı��ɶ� �̰� true�� �� ������ ����
	bool m_bCreateSessionOnDestroy{ false };
	int32 m_LastNumPublicConnections;
	FString m_LastMatchType;
};
