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

	// 세션 기능을 처리하기 위한 메뉴 클래스가 이를 호출한다.
	// 참가할 플레이어수, 매칭 타입
	// 세션 생성을 호출하면 하위 시스템에서의 세션 설정에서 키 값을 설정할 수 있다.
	void CreateSession(int32 NumPublicConnections, FString MatchType);
	// 
	void FindSession(int32 MaxSearchResults);
	// 세션을 찾아서 어떤 세션을 조인할지 결정하면 SessionResult에 저장됨
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void DestroySession();
	void StartSession();


	// 메뉴 클래스에 콜백을 바인딩하기 위한 사용자 지정 델리게이트
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	FMultiplayerOnFindSessionsComplete MultiplayerOnFindSessionComplete;
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionComplete;
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;
	FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionComplete;

protected:
	// 델리게이트에 바인드할 콜백 함수
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
	IOnlineSessionPtr m_SessionInterface;
	// 세션 정보를 저장
	TSharedPtr<FOnlineSessionSettings>	m_LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch>	m_LastSessionSearch;

	// 온라인 세션 인터페이스 델리게이트 리스트에 추가
	// 플레이어 세션 하위 시스템 내부 콜백함수를 여기에 바인딩처리
	FOnCreateSessionCompleteDelegate	m_CreateSessionCompleteDelegate;
	// 델리게이트 리스트에 추가하는 순간 그걸 처리하는 기능이 핸들에 저장
	FDelegateHandle m_CreateSessionCompleteDelegateHandle;
	FOnFindSessionsCompleteDelegate		m_FindSessionCompleteDelegate;
	FDelegateHandle m_FindSessionCompleteDelegateHandle;
	FOnJoinSessionCompleteDelegate		m_JoinSessionCompleteDelegate;
	FDelegateHandle m_JoinSessionCompleteDelegateHandle;
	FOnDestroySessionCompleteDelegate	m_DestroySessionCompleteDelegate;
	FDelegateHandle m_DestroySessionCompleteDelegateHandle;
	FOnStartSessionCompleteDelegate		m_StartSessionCompleteDelegate;
	FDelegateHandle m_StartSessionCompleteDelegateHandle;

	// 이걸 확인하고 콜백하는데 세션이 파괴될때 이게 true면 새 세션을 생성
	bool m_bCreateSessionOnDestroy{ false };
	int32 m_LastNumPublicConnections;
	FString m_LastMatchType;
};
