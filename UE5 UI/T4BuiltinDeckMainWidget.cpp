// Copyright VirtualFlow, Inc. All Rights Reserved.

#include "UserWidget/T4BuiltinDeckMainWidget.h"
//#include "UserWidget/Platform/T4BuiltinDeckSlotMain.h"
#include "UserWidget/Platform/T4BuiltinDeckInfo.h"
#include "UserWidget/Platform/T4BuiltinDeckDetail.h"
#include "UserWidget/Common/T4BuiltinButtonExt.h"
#include "UserWidget/Common/T4BuiltinButtonImage.h"

#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"

#include "Player/T4BuiltinPlayerController.h"
#include "Game/Client/ClientObject/Player/T4PlayerClientObject.h"
#include "Game/Client/Lobby/Platform/T4PlatformLobbyAPI.h" // #t4f-732

#include "UserWidget/T4UIUtil.h"
#include "T4GameData/Classes/T4ProjectTableRow.h"

// Json
#include "Engine/DataTable.h"
#include "Misc/PackageName.h" // #646
#include "Misc/FileHelper.h" // #533

#include "Content/T4ProjectAsset.h"
#include "Content/T4ProjectMasterTableAsset.h" // #195
#include "HAL/FileManager.h" // #122
#include "Misc/Paths.h" // #122
#include "Dom/JsonObject.h" // #122
#include "Serialization/JsonReader.h" // #122
#include "Serialization/JsonSerializer.h" // #122
//#include "DataTableEditorUtils.h"

#include "T4GameplayInternal.h"

#define LOCTEXT_NAMESPACE "T4BuiltinDeckMainWidget"

/**
  * #t4f-678
 */
UT4BuiltinDeckMainWidget::UT4BuiltinDeckMainWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Initialize(false)
{
}

void UT4BuiltinDeckMainWidget::InitWidget()
{
	Super::InitWidget();

	/*BindWidgetVariable(DeckSlotMain, "C_DeckSlotMain");
	if (nullptr != DeckSlotMain)
	{
		DeckSlotMain->ClickDetailDelegate.BindUObject(this, &UT4BuiltinDeckMainWidget::OnClickDeckDetail);
	}*/

	BindWidgetVariable(Btn_Back, "C_BtnExt_Back");
	if (nullptr != Btn_Back)
	{
		Btn_Back->ClickDelegate.BindUObject(this, &UT4BuiltinDeckMainWidget::OnClickedBack);
		Btn_Back->SetValue(TEXT("Logout"));
	}
	BindWidgetVariable(BtnImg_Profile, "C_BtnImg_Profile");
	if (nullptr != BtnImg_Profile)
	{
		BtnImg_Profile->ClickDelegate.BindUObject(this, &UT4BuiltinDeckMainWidget::OnClickedProfile);
	}

	BindWidgetVariable(ScrollBox_DeckInfo, "C_ScrollBox");
	BindWidgetVariable(DeckDetail, "C_DeckDetail");
	if (nullptr != DeckDetail)
	{
		DeckDetail->ClickCloseDelegate.BindUObject(this, &UT4BuiltinDeckMainWidget::OnClickCloseDeckDetail);
	}

	T4UIUtil::ScrollBoxClearChild(ScrollBox_DeckInfo);
}

void UT4BuiltinDeckMainWidget::ReleaseWidget()
{
	Super::ReleaseWidget();

	if (UsetProjectServiceTablePtr.IsValid())
	{
		UsetProjectServiceTablePtr->RemoveFromRoot();
		UsetProjectServiceTablePtr.Reset();
	}

	DeckDetail->ReleaseWidget();
}

void UT4BuiltinDeckMainWidget::OnStart()
{
	if (bIsShowOnStart)
	{
		WidgetInititalze();
		ShowWidget();
	}
}

void UT4BuiltinDeckMainWidget::WidgetInititalze()
{
	if (LoadDeckInfo())
	{
		CreateDeck();
	}

	SetVisibleDeckDetail(false);
}

void UT4BuiltinDeckMainWidget::SetValue()
{

}

void UT4BuiltinDeckMainWidget::OnClickCloseDeckDetail()
{
	SetVisibleDeckDetail(false);
	//DeckSlotMain->MediaPlay();
}

void UT4BuiltinDeckMainWidget::OnClickDeckDetail(FName InProjectID)
{
	if (nullptr != DeckDetail)
	{
		const FT4ProjectServiceTableRow* ProjectServiceTableRow = GetUserProjectServiceTableRow(InProjectID);
		if (nullptr != ProjectServiceTableRow)
		{
			//DeckSlotMain->MediaStop();
			DeckDetail->SetValue(ProjectServiceTableRow);
			DeckDetail->MediaPlay();
			SetVisibleDeckDetail(true);
		}
	}
}

void UT4BuiltinDeckMainWidget::OnClickedBack()
{
	IT4Framework* ClientFramework = T4Framework::GetFramework(LayerType);
	check(nullptr != ClientFramework);
	T4PlatformLobbyAPI::Request_AuthLogout(ClientFramework); // #t4f-772
}

void UT4BuiltinDeckMainWidget::OnClickedProfile()
{
	IT4Framework* ClientFramework = T4Framework::GetFramework(LayerType);
	check(nullptr != ClientFramework);
	T4PlatformLobbyAPI::EnterProfileRequest(ClientFramework);
}

void UT4BuiltinDeckMainWidget::SetVisibleDeckDetail(bool InVisible)
{
	// ToDo : 연출 추가 필요~~
	T4UIUtil::Toggle(DeckDetail, InVisible);
}

bool UT4BuiltinDeckMainWidget::LoadDeckInfo()
{
	if(!Initialize)
	// Json Loading
	{
		static FString UserProjectsSettingPath = FPaths::Combine(
			FPaths::ProjectContentDir(),
			TEXT("VirtualFlow/UserData/UserProject/DeckMain.t4proj")
		);
		/*const FString UserProjectSettingJsonFilePath = FPaths::Combine(
			FPaths::ProjectContentDir(),
			*UserProjectsSettingPath
		);*/

		FString JsonString;
		FFileHelper::LoadFileToString(JsonString, *UserProjectsSettingPath);
		if (JsonString.IsEmpty())
		{
			T4_LOG(Error, TEXT("Failed to load DeckMain Info. (%s)"), *UserProjectsSettingPath);
			Initialize = false;
		}
		else
		{
			TryCreateUserProjectServiceDataTable();
			check(UsetProjectServiceTablePtr.IsValid());
			UsetProjectServiceTablePtr->CreateTableFromJSONString(JsonString);

			Initialize = true;
			T4_LOG(Verbose, TEXT("LoadDeckInfo Item Loaded..."))
		}
	}

	return Initialize;
}

void UT4BuiltinDeckMainWidget::TryCreateUserProjectServiceDataTable()
{
	if (UsetProjectServiceTablePtr.IsValid())
	{
		return;
	}

	UDataTable* UsetProjectServiceTable = NewObject<UDataTable>(
		GetTransientPackage(),
		FName(TEXT("UserProjectServiceDataTable"))
		);
	check(UsetProjectServiceTable);

	UsetProjectServiceTable->RowStruct = FT4ProjectServiceTableRow::StaticStruct();

	UsetProjectServiceTable->AddToRoot();
	UsetProjectServiceTablePtr = UsetProjectServiceTable;
}

const FT4ProjectServiceTableRow* UT4BuiltinDeckMainWidget::GetUserProjectServiceTableRow(
	UDataTable* InDataTable,
	FName InProjectID
) const
{
	if (nullptr == InDataTable || InProjectID == NAME_None)
	{
		return nullptr;
	}
	const TMap<FName, uint8*>& AllRowMap = InDataTable->GetRowMap();
	if (0 >= AllRowMap.Num())
	{
		return nullptr;
	}
	if (!AllRowMap.Contains(InProjectID))
	{
		return nullptr;
	}
	return reinterpret_cast<FT4ProjectServiceTableRow*>(AllRowMap[InProjectID]);
}

const FT4ProjectServiceTableRow* UT4BuiltinDeckMainWidget::GetUserProjectServiceTableRow(FName InProjectID) const
{
	const FT4ProjectServiceTableRow* ProjectServiceTableRow = nullptr;
	if (UsetProjectServiceTablePtr.IsValid())
	{
		ProjectServiceTableRow = GetUserProjectServiceTableRow(UsetProjectServiceTablePtr.Get(), InProjectID);
	}
	if (nullptr == ProjectServiceTableRow)
	{
		T4_LOG(Warning, TEXT("ProjectID Not found. (%s)"), *(InProjectID.ToString()));
		return nullptr;
	}
	return ProjectServiceTableRow;
}

void UT4BuiltinDeckMainWidget::CreateDeck()
{
	if (!Initialize)
	{
		return;
	}

	const FT4ProjectServiceTableRow* DeckMainInfoTableRow = GetUserProjectServiceTableRow("DeckMainInfo");
	if (nullptr != DeckMainInfoTableRow)
	{
		// 1. MainProject
		FName MainProjectID = T4Framework::GetPlatformProjectID(); // DeckMainInfoTableRow->MainContentProject.ProjectID;
		const FT4ProjectServiceTableRow* ProjectServiceTableRow = GetUserProjectServiceTableRow(MainProjectID);
		if (nullptr != ProjectServiceTableRow)
		{
			/*if (nullptr != DeckSlotMain)
			{
				DeckSlotMain->SetValue(ProjectServiceTableRow);
				DeckSlotMain->MediaPlay();
			}*/

			// Pack Contents
			// 1. SortOrder
			TArray<FT4DeckPackData> DeckSlotData = DeckMainInfoTableRow->PackContentProject;
			struct FDeckSlotsorter
			{
				bool operator()(const FT4DeckPackData& A, const FT4DeckPackData& B) const
				{
					return A.SortOrder < B.SortOrder;
				}
			};
			DeckSlotData.Sort(FDeckSlotsorter());

			for (FT4DeckPackData DeckPackData : DeckSlotData)
			{
				CreateDeckInfo(DeckPackData);
			}
		}
	}
}

void UT4BuiltinDeckMainWidget::CreateDeckInfo(const FT4DeckPackData& InDeckPackData)
{
	IT4GameplayWidgetManager* WidgetManager = GetWidgetManager();
	check(WidgetManager);

	UClass* SlotUClass = WidgetManager->GetUClass(LayerType, ET4GameUIType::T4DeckInfo);
	if (nullptr != SlotUClass)
	{
		UT4BuiltinDeckInfo* DeckInfo = CreateWidget<UT4BuiltinDeckInfo>(GetBuiltinPlayerController(), SlotUClass);

		if (nullptr != DeckInfo)
		{
			UserBindVariable(DeckInfo, true);
			DeckInfo->SetValue(InDeckPackData.TitleDeckPack, InDeckPackData.ContentsProject, this);

			UScrollBoxSlot* ScrollBoxSlot = T4UIUtil::ScrollBoxAddChild(ScrollBox_DeckInfo, DeckInfo);
			if (nullptr != ScrollBoxSlot)
			{
				ScrollBoxSlot->SetPadding(FMargin(2.0f));
			}

			DeckInfo->ClickDeckInfoSlotDelegate.BindUObject(this, &UT4BuiltinDeckMainWidget::OnClickDeckDetail);
			
		}
	}
}


#undef LOCTEXT_NAMESPACE //"T4BuiltinDeckMainWidget"