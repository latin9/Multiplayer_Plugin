// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")), FString LobbyPath = FString(TEXT("/Game/ThirdPerson/Maps/Lobby")));

protected:
	virtual bool Initialize() override;
	// 레벨이 세계에서 제거될 때 호출
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld) override;

	// 멀티플레이 세션 서브 시스템에서의 사용자 지정 델리게이트를 위한 콜백
	// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam는 무조건 UFUNCTION으로 생성해야됨
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);
	void OnFindSession(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);

	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

private:
	// 블루프린트의 버튼이랑 c++버튼이랑 연동된다는 의미이다
	// 블루프린트의 이름과 c++ 변수명이 동일해야한다.
	UPROPERTY(meta = (BindWidget))
	class UButton* HostButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* JoinButton;

	UFUNCTION()
	void HostButtonCliked();

	UFUNCTION()
	void JoinButtonCliked();

	void MenuTearDown();

	// 모든 온라인 세션 기능을 처리하도록 설계된 서브시스템
	class UMultiplayerSessionsSubsystem* m_MultiplayerSessionsSubsystem;

	int32 m_NumPublicConnections{4};
	FString m_Matchtype{TEXT("FreeForAll")};
	FString m_PathToLobby{ TEXT("") };

};
