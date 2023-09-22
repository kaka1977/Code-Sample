#include "UserWidget/T4BuiltinDialogueChatWidget.h"
#include "UserWidget/Common/T4BuiltinButtonExt.h"
#include "UserWidget/Common/T4BuiltinDialogueTextBlock.h"
#include "UserWidget/Item/T4Builtin_AIChatListItem.h"

#include "Game/T4GamePacket.h"
#include "Game/Common/Protocol/CtoS/T4GamePacketCS_Game.h"

#include "UserWidget/T4UIUtil.h"
#include "Player/T4BuiltinPlayerController.h"
#include "Game/Client/ClientObject/Player/T4PlayerClientObject.h"
#include "QuestFlow/T4QFNodeConstants.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/Throbber.h"
#include "Components/SizeBox.h"

#include "T4GameplayInternal.h"


UT4BuiltinDialogueChatWidget::UT4BuiltinDialogueChatWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bDialogStatus(false)
{
	QueueTagList.Empty();
}

void UT4BuiltinDialogueChatWidget::InitWidget()
{
	Super::InitWidget();

	BindWidgetVariable(Overlay_Title, "Overlay_Title");
	BindWidgetVariable(Text_TopTitle, "C_Txt_Title");
	BindWidgetVariable(EditableTextBox, "C_EditableTextBox_Input");
	BindWidgetVariable(ScrollBox_ChatHistory, "C_ScrollBox_ChatHistory");
	if (nullptr != ScrollBox_ChatHistory)
	{
		T4UIUtil::ScrollBoxClearChild(ScrollBox_ChatHistory);
	}

	BindWidgetVariable(BtnExt_OK, "C_BtnExt_OK");
	BindWidgetVariable(PanelClose, "SizeBox");
	BindWidgetVariable(BtnExt_Close, "C_BtnExt_Exit");
	if (nullptr != BtnExt_OK)
	{
		BtnExt_OK->ClickDelegate.BindUObject(this, &UT4BuiltinDialogueChatWidget::OnClickedOK);
	}
	if (nullptr != BtnExt_Close)
	{
		BtnExt_Close->ClickDelegate.BindUObject(this, &UT4BuiltinDialogueChatWidget::OnClickedClose);
	}
	BindWidgetVariable(Throbber_Status, "C_Throbber_Status");

	T4UIUtil::SetTextFromString(Text_TopTitle, TEXT(""));
}

void UT4BuiltinDialogueChatWidget::ReleaseWidget()
{
	Super::ReleaseWidget();

	if (nullptr != EditableTextBox)
	{
		EditableTextBox->OnTextCommitted.Clear();
	}

	ResetChatHistory();
	DelegateClearAndBind();
}

void UT4BuiltinDialogueChatWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (StartChat && nullptr != ScrollBox_ChatHistory)
	{
		ScrollBox_ChatHistory->ScrollToEnd();
	}
}

void UT4BuiltinDialogueChatWidget::ResetChatHistory()
{
	if (nullptr != EditableTextBox)
	{
		T4UIUtil::SetTextFromString(EditableTextBox, TEXT(""));
		EditableTextBox->SetKeyboardFocus();
	}

	if (nullptr != ScrollBox_ChatHistory)
	{
		T4UIUtil::ScrollBoxClearChild(ScrollBox_ChatHistory);
	}
	bDialogStatus = false;
	StartChat = false;
	ClearQueueTagList();
}

void UT4BuiltinDialogueChatWidget::DelegateClearAndBind()
{
	UT4PlayerClientObject* ClientObject = GetPlayerClientObject();
	if (nullptr != ClientObject)
	{
		ClientObject->OnUIDialogueChatStart.Unbind();
		ClientObject->OnUIDialogueChatStart.BindUFunction(this, TEXT("DialogueChatStart"));

		ClientObject->OnUIDialogueChatUpdate.Unbind();
		ClientObject->OnUIDialogueChatUpdate.BindUFunction(this, TEXT("DialogueChatUpdate"));

		ClientObject->OnUIDialogueChatFinish.Unbind();
		ClientObject->OnUIDialogueChatFinish.BindUFunction(this, TEXT("DialogueChatFinish"));
	}
}

void UT4BuiltinDialogueChatWidget::OnStart()
{
	Super::OnStart();

	{
		AT4BuiltinPlayerController* PlayerController = GetBuiltinPlayerController();
		check(PlayerController);
		PlayerController->GetOnUIChatSay().Clear();
		PlayerController->GetOnUIChatSay().BindUFunction(this, TEXT("ShowChatSay"));
	}

	if (nullptr != EditableTextBox)
	{
		EditableTextBox->OnTextCommitted.Clear();
		EditableTextBox->OnTextCommitted.AddDynamic(this, &UT4BuiltinDialogueChatWidget::OnTextCommitted);
	}

	DelegateClearAndBind();
	bDialogStatus = false;
	StartChat = false;
	ClearQueueTagList();
	T4UIUtil::Toggle(PanelClose, true);
}

void UT4BuiltinDialogueChatWidget::SetVisibility(ESlateVisibility InVisibility)
{
	if (ESlateVisibility::Hidden != InVisibility && ESlateVisibility::Collapsed != InVisibility)
	{
		HideWidgetsOnScreen();
		HideJoystick();
		UT4BuiltinWidget::SetVisibility(InVisibility);
	}
	else
	{
		UT4BuiltinWidget::SetVisibility(ESlateVisibility::Hidden);
		UnhideJoystick();
		UnhideWidgetsOnScreen();
	}
}

void UT4BuiltinDialogueChatWidget::ShowWidget()
{
	if (!IsShowWidget())
	{
		Super::ShowWidget();
		ResetChatHistory();
	}
}

void UT4BuiltinDialogueChatWidget::ShowChatSay()
{
	T4UIUtil::Toggle(EditableTextBox, true);
	EditableTextBox->SetKeyboardFocus();
}

void UT4BuiltinDialogueChatWidget::OnClickedOK()
{
	OnResponseContent(EditableTextBox->GetText());
}

void UT4BuiltinDialogueChatWidget::OnClickedClose()
{
	UT4PlayerClientObject* ClientObject = GetPlayerClientObject();
	check(ClientObject);

	ClientObject->OnSendDialogueResponse(ET4GameDialogueResult::OK);
	T4_LOG(Log, TEXT("OnClickedClose"));
}

void UT4BuiltinDialogueChatWidget::OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	AT4BuiltinPlayerController* PlayerController = GetBuiltinPlayerController();
	check(PlayerController);
	
	// Enter
	if (ETextCommit::OnEnter == CommitMethod)
	{
		OnResponseContent(Text);
	}
}

void UT4BuiltinDialogueChatWidget::OnResponseContent(const FText& Text) // #t4f-846
{
	AT4BuiltinPlayerController* PlayerController = GetBuiltinPlayerController();
	check(PlayerController);

	if (Text.IsEmpty())
	{
		// nothing
		return;
	}
	else
	{
		FString SendMessage = Text.ToString();

		FT4DialogueDataInfo DialogueDataInfo;
		DialogueDataInfo.SetNickname(PlayerController->GetNickname());
		DialogueDataInfo.SetSourceString(SendMessage);
		DialogueDataInfo.SetSpeakerDBType(ET4GameDBType::Player);
		DialogueDataInfo.SetTypewriterActive(false); // #t4f-1005 : PC 타이핑은 즉시 출력

		AddChatAndChangeUITextJustification(DialogueDataInfo);// #t4f-943

		//T4_LOG(Log, TEXT("Request : %s"), *SendMessage);
		//AddChat(ET4GameDBType::Player, DialogueDataInfo);

		UT4PlayerClientObject* ClientObject = GetPlayerClientObject();
		check(ClientObject);
		// #t4f-1003 : ShortAnswer는 필요없어 보임(남겨둠). Interactive로 계속 대화 가능하게만 하도록
		ClientObject->OnSendDialogueResponseAnswer(ET4GameDialogueResult::Interactive, Text.ToString());
	}

	// Clear
	T4UIUtil::SetTextFromString(EditableTextBox, TEXT(""));
}

void UT4BuiltinDialogueChatWidget::AddChatMessage(
	const ET4GameDBType& InType,
	const FString& InNickname, 
	const FString& InMessage,
	ET4GameDialogueChatAction InChatAction // #t4f-1005 : GenAI 생성 메시지는 마지막 대사를 덮어쓰거나, 완료 알림에 사용
) // #t4f-922
{
	if (!LastChatListItemPtr.IsValid())
	{
		return;
	}

	T4UIUtil::Toggle(PanelClose, true);
	if (ET4GameDialogueChatAction::Add_Completed == InChatAction ||
		ET4GameDialogueChatAction::Override_Completed == InChatAction)
	{
		// #t4f-1005 : GenAI 생성 메시지는 마지막 대사를 덮어쓰거나, 완료 알림에 사용
		if (nullptr != Throbber_Status)
		{
			Throbber_Status->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// #t4f-1005 : empty라면 (...)과 같은 타이핑 중 표시, 명시적 옵션이 필요할 수도 있다.
	bool bWaitingChat = InMessage.IsEmpty();

	FT4DialogueDataInfo DialogueDataInfo;
	DialogueDataInfo.SetNickname(InNickname);
	DialogueDataInfo.SetSpeakerDBType(InType); // #t4f-919
	DialogueDataInfo.SetTypewriterActive(OwnerDialogueDataInfoBackup.TypewriterActive); // 초기 옵션 유지

	if (bWaitingChat)
	{
		DialogueDataInfo.SetWaiting(true); // #t4f-1031
	}
	else
	{
		DialogueDataInfo.SetSourceString(InMessage);
	}

	if (LastChatListItemPtr.IsValid())
	{
		if (ET4GameDialogueChatAction::Add_Completed == InChatAction)
		{
			// 완료이나, 메시지는 추가 (즉시 종료 대비)
		}
		else if (ET4GameDialogueChatAction::Override_Completed == InChatAction || // 완료도 Override 로
				 ET4GameDialogueChatAction::Override == InChatAction) // #t4f-1005 : GenAI 생성 메시지는 마지막 대사를 덮어쓴다
		{
			DialogueDataInfo.SetTextJustification(LastChatListItemPtr->GetJustification());
			LastChatListItemPtr->SetWaiting(bWaitingChat);
			LastChatListItemPtr->SetValue(DialogueDataInfo);

			return;
		}
		else if (LastChatListItemPtr->IsWaiting()) // #t4f-1005 : ... 텍스트 업데이트
		{
			DialogueDataInfo.SetTextJustification(LastChatListItemPtr->GetJustification());
			LastChatListItemPtr->SetWaiting(bWaitingChat);
			LastChatListItemPtr->SetValue(DialogueDataInfo);
			return;
		}
	}

	AddChatAndChangeUITextJustification(DialogueDataInfo); // #t4f-943
	// #t4f-1077 TypeWriter OFF일때는 출력을때 함께 해주면 된다.
	if (!DialogueDataInfo.TypewriterActive)
	{
		while (!QueueTagList.IsEmpty())
		{
			PopQueueTagList();
		}
	}

	if (bWaitingChat && LastChatListItemPtr.IsValid())
	{
		LastChatListItemPtr->SetWaiting(bWaitingChat);
	}
}

// ***** TypeWriter TRUE일때만 동작한다. *****
void UT4BuiltinDialogueChatWidget::HandleOnDialogueTextEnd(bool InDialogueTextEnd) // #t4f-846
{
	if (InDialogueTextEnd)
	{
		if (QueueTagList.IsEmpty()) // #t4f-1077
		{
			StartChat = false;
		}
		else
		{
			// #t4f-1077 Queue에서 하나씩 꺼내서 End까지 순차적으로 재생시킨다.
			PopQueueTagList();
		}
	}
	else
	{
		StartChat = true;
	}
}

void UT4BuiltinDialogueChatWidget::DialogueChatStart(const FString& InTitle)
{
	OnOpenInit();
	SetQuestFlowTitle(InTitle);
}

void UT4BuiltinDialogueChatWidget::DialogueChatUpdate(const FT4DialogueDataInfo& InUIDialogueDataInfo)
{
	if (!bDialogStatus)
	{
		AT4BuiltinPlayerController* BuiltinPlayerController = GetBuiltinPlayerController();
		if (nullptr != BuiltinPlayerController)
		{
			BuiltinPlayerController->DialogueClose(); // *** 주의 *** Dialogue에서 통합 관리
		}
		ShowWidget();
		OwnerDialogueDataInfoBackup = InUIDialogueDataInfo;  // #t4f-834
		bDialogStatus = true;

		// #t4f-1054
		if (nullptr != BuiltinPlayerController)
		{
			BuiltinPlayerController->ShowQuestInfoInventory();
		}

		SetCustomText(InUIDialogueDataInfo); // #t4f-1069
	}

	// Widget Setting~~~~
	// #t4f-919 : TextJustification을 변경시키기 위해 새로운 변수에 담아서 처리함
	FT4DialogueDataInfo UIDialogueDataInfo(InUIDialogueDataInfo);
	AddChatAndChangeUITextJustification(UIDialogueDataInfo); // #t4f-943

	if (InUIDialogueDataInfo.CustomTag != NAME_None)
	{
		if (nullptr != EditableTextBox)
		{
			// #t4f-1053 : 사용자 닉네임이 아닐 경우, (닉네임)
			if (GQFDialogueCustomTagNicknameRequest == InUIDialogueDataInfo.CustomTag)
			{
				FString NicknameRequestString = T4_GAMEPLAY_GET_STRINGSTR(T4GAMEPLAY_NAME, TEXT("DialogueChat_NicknameRequest"));
				EditableTextBox->SetHintText(FText::FromString(NicknameRequestString)); // #t4f-1053
			}
		}
		T4UIUtil::Toggle(PanelClose, false);
	}
}

void UT4BuiltinDialogueChatWidget::DialogueChatFinish()
{
	if (bDialogStatus)
	{
		ResetChatHistory();
		HideWidget();

		HideItemDescription(); // #t4f-1028
	}
}

void UT4BuiltinDialogueChatWidget::AddChatAndChangeUITextJustification(FT4DialogueDataInfo& InUIDialogueDataInfo)
{
	//#t4f-943
	AT4BuiltinPlayerController* PlayerController = GetBuiltinPlayerController();
	check(PlayerController);

	IT4GameplayWidgetManager* WidgetManager = GetWidgetManager();
	check(WidgetManager);

	UClass* SlotUClass = nullptr;
	switch (InUIDialogueDataInfo.SpeakerDBKey.Type)
	{
	case ET4GameDBType::Player:
	{
		SlotUClass = WidgetManager->GetUClass(LayerType, ET4GameUIType::T4AIChatSlot_PC);
	}
	break;
	case ET4GameDBType::NPC:
	{
		SlotUClass = GetSlotUClassNPC(InUIDialogueDataInfo.ChatNPCAnswerType);
	}
	break;
	}

	if (nullptr == SlotUClass)
	{
		return;
	}

	ParseSourceStringByTag(InUIDialogueDataInfo.SourceString); // #t4f-1077
	UT4Builtin_AIChatListItem* ItemWidget = CreateWidget<UT4Builtin_AIChatListItem>(GetBuiltinPlayerController(), SlotUClass);
	if (nullptr != ItemWidget)
	{
		UserBindVariable(ItemWidget);

		//#t4f-943 DialogueChat 의 PC/NPC 정렬을 Widget설정으로 적용
		InUIDialogueDataInfo.SetTextJustification(ItemWidget->GetJustification()); // Change Justification

		// #t4f-846
		ItemWidget->DialogueTextEndDelegate.Unbind();
		ItemWidget->DialogueTextEndDelegate.BindUObject(this, &UT4BuiltinDialogueChatWidget::HandleOnDialogueTextEnd);
		ItemWidget->SetValue(InUIDialogueDataInfo);

		UScrollBoxSlot* ScrollBoxSlot = T4UIUtil::ScrollBoxAddChild(ScrollBox_ChatHistory, ItemWidget);
		if (nullptr != ScrollBoxSlot)
		{

			ScrollBoxSlot->SetPadding(FMargin(0.0f, 2.0f));
			ScrollBox_ChatHistory->ScrollToEnd();
			StartChat = true;
		}

		if (ET4GameDBType::NPC == InUIDialogueDataInfo.SpeakerDBKey.Type) // NPC만 저장해야 한다.
		{
			LastChatListItemPtr = ItemWidget; // #t4f-1005 : 마지막 채팅을 저장해둔다.
		}
	}
}

void UT4BuiltinDialogueChatWidget::SetQuestFlowTitle(const FString& InMessage) // #t4f-1017
{
	if (InMessage.Len() <= 0)
	{
		T4UIUtil::SetTextFromString(Text_TopTitle, TEXT(""));
		T4UIUtil::Toggle(Overlay_Title, false);
		return;
	}

	T4UIUtil::SetTextFromString(Text_TopTitle, InMessage);
	T4UIUtil::Toggle(Overlay_Title, true);
}

// #t4f-1035 : ET4DialogueChatNPCAnswerType에 따라 달라진다.
UClass* UT4BuiltinDialogueChatWidget::GetSlotUClassNPC(const ET4DialogueChatNPCAnswerType& InDialogueChatNPCAnswerType) // #t4f-1035
{
	IT4GameplayWidgetManager* WidgetManager = GetWidgetManager();
	check(WidgetManager);

	UClass* ResultValue = nullptr;
	switch (InDialogueChatNPCAnswerType)
	{
		case ET4DialogueChatNPCAnswerType::Image:
		{
			ResultValue = WidgetManager->GetUClass(LayerType, ET4GameUIType::T4AIChatSlot_NPC_Image);
		}
		break;
		case ET4DialogueChatNPCAnswerType::Button:
		{
			ResultValue = WidgetManager->GetUClass(LayerType, ET4GameUIType::T4AIChatSlot_NPC_Button);
		}
		break;
		case ET4DialogueChatNPCAnswerType::Default:
		{
			ResultValue = WidgetManager->GetUClass(LayerType, ET4GameUIType::T4AIChatSlot_NPC);
		}
		break;
	}

	return ResultValue;
}

// #t4f-1069 CustomType의 Text가 모두 다르기 때문에 예되적으로 매칭을 해둠.
void UT4BuiltinDialogueChatWidget::SetCustomText(const FT4DialogueDataInfo& InUIDialogueDataInfo)
{
	if (nullptr != EditableTextBox)
	{
		if (InUIDialogueDataInfo.ChatEditHintText.Len())
		{
			EditableTextBox->SetHintText(FText::FromString(InUIDialogueDataInfo.ChatEditHintText));
		}
		else
		{
			FString EditHintString;
			if (InUIDialogueDataInfo.DialogueGameUIType == TEXT("DialogueChat"))
			{
				EditHintString = T4_GAMEPLAY_GET_STRINGSTR(T4GAMEPLAY_NAME, TEXT("DialogueChat_EditHint"));
			}
			else if (InUIDialogueDataInfo.DialogueGameUIType == TEXT("DialogueChat_1"))
			{
				EditHintString = T4_GAMEPLAY_GET_STRINGSTR(T4GAMEPLAY_NAME, TEXT("DialogueChat1_EditHint"));
			}
			else if (InUIDialogueDataInfo.DialogueGameUIType == TEXT("DialogueChat_2"))
			{
				EditHintString = T4_GAMEPLAY_GET_STRINGSTR(T4GAMEPLAY_NAME, TEXT("DialogueChat2_EditHint"));
			}

			EditableTextBox->SetHintText(FText::FromString(EditHintString));
		}
	}

	if (nullptr != BtnExt_Close)
	{
		if (InUIDialogueDataInfo.OKBtnText.Len())
		{
			BtnExt_Close->SetValue(InUIDialogueDataInfo.OKBtnText);
		}
		else
		{
			if (InUIDialogueDataInfo.DialogueGameUIType == TEXT("DialogueChat_1"))
			{
				BtnExt_Close->SetValue(T4_GAMEPLAY_GET_STRINGSTR(T4GAMEPLAY_NAME, TEXT("DialogueChat1_OK")));
			}
			else if (InUIDialogueDataInfo.DialogueGameUIType == TEXT("DialogueChat_2"))
			{
				BtnExt_Close->SetValue(T4_GAMEPLAY_GET_STRINGSTR(T4GAMEPLAY_NAME, TEXT("DialogueChat2_OK")));
			}
		}
	}
}

void UT4BuiltinDialogueChatWidget::HandleOnClickedTag(int32 InTagIndex) // #t4f-1077
{
	UT4PlayerClientObject* ClientObject = GetPlayerClientObject();
	if (nullptr != ClientObject)
	{
		FString TagIndexString = FString::Printf(TEXT("%i"), InTagIndex);
		ClientObject->OnSendDialogueResponseAnswer(
			ET4GameDialogueResult::TagSelected,
			TagIndexString
		);
		ClientObject->OnSendDialogueResponse(ET4GameDialogueResult::OK);
		T4_LOG(Verbose, TEXT("HandleOnClickedTag : %i"), InTagIndex);
	}
}

// #t4f-1077 : Tag로 구분된 SourceString을 Queue에 Add
void UT4BuiltinDialogueChatWidget::AddQueueTagList(const TArray<FString>& InTagList)
{
	for (int32 i = 0; i < InTagList.Num(); i++)
	{
		FT4TagItemData TagItemData(i, InTagList[i]);
		QueueTagList.Enqueue(TagItemData);
	}
}

void UT4BuiltinDialogueChatWidget::PopQueueTagList()
{
	FT4TagItemData* TagItemData = QueueTagList.Peek();
	if (nullptr != TagItemData)
	{
		OwnerDialogueDataInfoBackup.SourceString = TagItemData->QueueTag;
		AddTagChat(TagItemData->Index, OwnerDialogueDataInfoBackup);
		QueueTagList.Dequeue(*TagItemData);
	}
}

void UT4BuiltinDialogueChatWidget::ClearQueueTagList()
{
	QueueTagList.Empty();
}

void UT4BuiltinDialogueChatWidget::ParseSourceStringByTag(FString& InSourceString)
{
	TArray<FString> arrMessage;
	T4UIUtil::FStringParserTag(InSourceString, arrMessage);

	// SourceString에서 Tag를 제외한 것이 있다면 Tag 이하는 제거한다!!!
	int32 indexTagS = InSourceString.Find(TEXT("<"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
	if (indexTagS != INDEX_NONE)
	{
		InSourceString = InSourceString.Mid(0, indexTagS);
	}

	ClearQueueTagList();
	AddQueueTagList(arrMessage);
}

void UT4BuiltinDialogueChatWidget::AddTagChat(int32 InTagIndex, FT4DialogueDataInfo& InUIDialogueDataInfo)
{
	AT4BuiltinPlayerController* PlayerController = GetBuiltinPlayerController();
	check(PlayerController);

	IT4GameplayWidgetManager* WidgetManager = GetWidgetManager();
	check(WidgetManager);

	UClass* SlotUClass = nullptr;
	switch (InUIDialogueDataInfo.SpeakerDBKey.Type)
	{
	case ET4GameDBType::Player:
	{
		SlotUClass = WidgetManager->GetUClass(LayerType, ET4GameUIType::T4AIChatSlot_PC);
	}
	break;
	case ET4GameDBType::NPC:
	{
		SlotUClass = GetSlotUClassNPC(ET4DialogueChatNPCAnswerType::Button); // Test
	}
	break;
	}

	if (nullptr == SlotUClass)
	{
		return;
	}

	UT4Builtin_AIChatListItem* ItemWidget = CreateWidget<UT4Builtin_AIChatListItem>(GetBuiltinPlayerController(), SlotUClass);
	if (nullptr != ItemWidget)
	{
		UserBindVariable(ItemWidget);

		//#t4f-943 DialogueChat 의 PC/NPC 정렬을 Widget설정으로 적용
		InUIDialogueDataInfo.SetTextJustification(ItemWidget->GetJustification()); // Change Justification

		// #t4f-846
		ItemWidget->DialogueTextEndDelegate.Unbind();
		ItemWidget->DialogueTextEndDelegate.BindUObject(this, &UT4BuiltinDialogueChatWidget::HandleOnDialogueTextEnd);
		ItemWidget->DialogueTextButtonDelegate.Unbind();
		ItemWidget->DialogueTextButtonDelegate.BindUObject(this, &UT4BuiltinDialogueChatWidget::HandleOnClickedTag);
		ItemWidget->SetValue(InTagIndex, InUIDialogueDataInfo);

		UScrollBoxSlot* ScrollBoxSlot = T4UIUtil::ScrollBoxAddChild(ScrollBox_ChatHistory, ItemWidget);
		if (nullptr != ScrollBoxSlot)
		{
			ScrollBoxSlot->SetPadding(FMargin(0.0f, 2.0f));
			ScrollBox_ChatHistory->ScrollToEnd();
			StartChat = true;
		}
	}
}