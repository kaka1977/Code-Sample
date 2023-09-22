// Copyright VirtualFlow, Inc. All Rights Reserved.

#pragma once

#include "T4GameplayMinimal.h"
#include "T4BuiltinWidget.h"
#include "UI/T4GameplayUIStructs.h"
#include "Containers/Queue.h"

#include "T4BuiltinDialogueChatWidget.generated.h"

class UThrobber;
class UOverlay;
class UTextBlock;
class UEditableTextBox;
class UScrollBox;
class USizeBox;
class UT4BuiltinButtonExt;
class UT4BuiltinDialogueTextBlock;
class UT4Builtin_AIChatListItem;
class FT4GenAIAgent;

USTRUCT()
struct FT4TagItemData
{
	GENERATED_USTRUCT_BODY()

public:
	FT4TagItemData()
	{
		Index = 0;
		QueueTag = TEXT("");
	}

	FT4TagItemData(int32 InIndex, const FString& InQueueTag)
	{
		Index = InIndex;
		QueueTag = InQueueTag;
	}

	int32 Index;
	FString QueueTag;
};

UCLASS()
class T4GAMEPLAY_API UT4BuiltinDialogueChatWidget : public UT4BuiltinWidget
{
	GENERATED_UCLASS_BODY()

public:
	void InitWidget() override;
	void ReleaseWidget() override;	
	void OnStart() override;
	
	void ShowWidget() override;
	void SetVisibility(ESlateVisibility InVisibility) override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void AddChatMessage(
		const ET4GameDBType& InType,
		const FString& InNickname,
		const FString& InMessage,
		ET4GameDialogueChatAction InChatAction // GenAI 생성 메시지는 마지막 대사를 덮어쓰거나, 완료 알림에 사용
	);

	void DelegateClearAndBind();
	bool IsDialogStatus() { return bDialogStatus; }

protected:
	void ResetChatHistory();
	void OnResponseContent(const FText& Text);
	void AddChatAndChangeUITextJustification(FT4DialogueDataInfo& InUIDialogueDataInfo);
	void SetQuestFlowTitle(const FString& InMessage);
	UClass* GetSlotUClassNPC(const ET4DialogueChatNPCAnswerType& InDialogueChatNPCAnswerType);
	void SetCustomText(const FT4DialogueDataInfo& InUIDialogueDataInfo);

	void AddQueueTagList(const TArray<FString>& InTagList);
	void PopQueueTagList();
	void ClearQueueTagList();
	void ParseSourceStringByTag(FString& InSourceString);
	void AddTagChat(int32 InTagIndex, FT4DialogueDataInfo& InUIDialogueDataInfo);

public:
	UFUNCTION()
	void HandleOnDialogueTextEnd(bool InDialogueTextEnd);

	UFUNCTION()
	void HandleOnClickedTag(int32 InTagIndex);

private:
	UFUNCTION()
	void DialogueChatStart(const FString& InTitle);
	UFUNCTION()
	void DialogueChatUpdate(const FT4DialogueDataInfo& InUIDialogueDataInfo);
	UFUNCTION()
	void DialogueChatFinish();

	UFUNCTION()
	void ShowChatSay();
	UFUNCTION()
	void OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void OnClickedOK();
	UFUNCTION()
	void OnClickedClose();

private:
	UPROPERTY(Transient)
	UEditableTextBox* EditableTextBox;

	UPROPERTY(Transient)
	UScrollBox* ScrollBox_ChatHistory;

	UPROPERTY(Transient)
	UT4BuiltinButtonExt* BtnExt_OK;

	UPROPERTY(Transient)
	USizeBox* PanelClose;
	UPROPERTY(Transient)
	UT4BuiltinButtonExt* BtnExt_Close;

	UPROPERTY(Transient)
	UThrobber* Throbber_Status;

	UPROPERTY(Transient)
	UOverlay* Overlay_Title;

	UPROPERTY(Transient)
	UTextBlock* Text_TopTitle;

	UPROPERTY(Transient)
	bool bDialogStatus;

	TQueue<FT4TagItemData> QueueTagList;

	FT4DialogueDataInfo OwnerDialogueDataInfoBackup;
	bool StartChat;

	TWeakObjectPtr<UT4Builtin_AIChatListItem> LastChatListItemPtr;
};
