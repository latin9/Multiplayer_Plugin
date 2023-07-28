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
	// ������ ���迡�� ���ŵ� �� ȣ��
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld) override;

	// ��Ƽ�÷��� ���� ���� �ý��ۿ����� ����� ���� ��������Ʈ�� ���� �ݹ�
	// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam�� ������ UFUNCTION���� �����ؾߵ�
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);
	void OnFindSession(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);

	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

private:
	// �������Ʈ�� ��ư�̶� c++��ư�̶� �����ȴٴ� �ǹ��̴�
	// �������Ʈ�� �̸��� c++ �������� �����ؾ��Ѵ�.
	UPROPERTY(meta = (BindWidget))
	class UButton* HostButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* JoinButton;

	UFUNCTION()
	void HostButtonCliked();

	UFUNCTION()
	void JoinButtonCliked();

	void MenuTearDown();

	// ��� �¶��� ���� ����� ó���ϵ��� ����� ����ý���
	class UMultiplayerSessionsSubsystem* m_MultiplayerSessionsSubsystem;

	int32 m_NumPublicConnections{4};
	FString m_Matchtype{TEXT("FreeForAll")};
	FString m_PathToLobby{ TEXT("") };

};
