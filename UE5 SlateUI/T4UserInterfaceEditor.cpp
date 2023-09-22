// Copyright VirtualFlow, Inc. All Rights Reserved.

#include "T4UserInterfaceEditor.h"
#include "ST4UITypeTreeWidget.h"

#include "T4ContentEditorModule.h"
#include "T4ContentEditorStyle.h"
#include "T4ContentEditorCommands.h"

#include "Utility/T4AssetContentUtils.h"

#include "T4EditorCommon/Public/T4EditorCommonConstants.h"
#include "T4EditorCommon/Public/T4EditorCommonStyle.h"
#include "T4EditorCommon/Public/ViewModel/T4PreviewEntityViewModel.h"
#include "T4EditorCommon/Public/Viewport/T4EditorViewportClient.h"
#include "T4EditorCommon/Public/Viewport/T4EditorViewport.h"

#include "T4Asset/Public/T4AssetUtils.h"
#include "T4Framework/Public/T4Framework.h"
#include "T4Framework/Classes/UserWidget/T4UserWidgetBase.h" // #1156
#include "T4GameData/Classes/Content/T4UserInterfaceAsset.h"
#include "T4Gameplay/Public/T4GameplayWidgetManager.h" // #453
#include "T4Gameplay/Public/Settings/T4GameplaySettings.h"

#include "FileHelpers.h"
#include "Engine/Blueprint.h"
#include "Modules/ModuleManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ObjectTools.h"

#include "T4ContentEditorInternal.h"

const FName FT4UserInterfaceEditor::PreviewViewportTabId_Default = FName(TEXT("Default PreviewViewport "));
const FName FT4UserInterfaceEditor::PreviewViewportTabId = FName(TEXT("Overrided PreviewViewport "));
const FName FT4UserInterfaceEditor::OverrideNewTabId = FName(TEXT("UI Default List"));
const FName FT4UserInterfaceEditor::OverrideListTabId = FName(TEXT("UI Override List"));
const FName FT4UserInterfaceEditor::UserInterfacePropertyId = FName(TEXT("Override Detail Property"));
const FName FT4UserInterfaceEditor::UIOptionTabId = FName(TEXT("Project UIOption Property"));

#define LOCTEXT_NAMESPACE "T4UserInterfaceEditor"

#define T4UIEDITOR_NAME FString(TEXT("T4UIEditor")) // #559

static const FText T4UIOverrideErrorTitleText = LOCTEXT("T4UIOverride", "T4UIOverride Error");
static const FText T4UICustomErrorTitleText = LOCTEXT("T4UICustom", "T4UICustom Error");

/**
  * #453
 */
FT4UserInterfaceEditor::FT4UserInterfaceEditor()
	: bInitializing(false)
	, SelectedUITypeName(NAME_None)
	, SelectedDefaultUITypeName(NAME_None)
	, NewSelectedUITypeName(NAME_None)
	, SelectedGameUITemplate(ET4GameUITemplate::Default)
{
}

FT4UserInterfaceEditor::~FT4UserInterfaceEditor()
{
	CleanUp();
	/*OnClose.Broadcast(this);
	OnFinialize();*/
}

void FT4UserInterfaceEditor::OnSelectItemFromLaunch()
{	
	FocusWindow();
}

void FT4UserInterfaceEditor::OnCloseWindow() // #t4f-882
{	
	//CleanUp();
	//NotifyOnExecuteCloseWindow();

	// 탭을 클릭하여 닫는 경우
	FTabManager* TabManagerPtr = GetTabManager().Get();
	if (nullptr != TabManagerPtr)
	{
		GetTabManager()->GetOwnerTab()->RequestCloseTab();
	}
	CleanUp();
}

FName FT4UserInterfaceEditor::GetProjectID()
{
	if (nullptr != ContentEditorFramework)
	{
		return ContentEditorFramework->GetProjectID();
	}

	return NAME_None;
}

bool FT4UserInterfaceEditor::OnInitialize(
	const EToolkitMode::Type InMode,
	UObject* InAsset,
	const TSharedPtr<IToolkitHost>& InInitToolkitHost,
	const IT4Framework* InContentEditorFramework
)
{
	DEFINE_LICENSE_EXPIRED_MACRO(this); // #1126

	ToolkitAssetName = *(FSoftObjectPath(InAsset).GetAssetName());
	TrySendLogging(0.0f); // #148

	bInitializing = true;
	ContentEditorFramework = InContentEditorFramework; // #t4f-270

	if (!UserInterfaceAssetPtr.IsValid())
	{
		UserInterfaceAssetPtr = Cast<UT4UserInterfaceAsset>(InAsset);
		UserInterfaceAssetPtr->SetFlags(RF_Transactional); // Undo, Redo
		SetProjectName(GetProjectID()); // #t4f-413
	}

	// 0. Viewport - Default
	{
		FT4PreviewEntityViewModelOptions ViewModelOptions_Default;

		PreviewViewModelPtr_Default = MakeShareable(new FT4PreviewEntityViewModel(ViewModelOptions_Default));
		PreviewViewportPtr_Default = SNew(ST4EditorViewport)
			.ViewModel(PreviewViewModelPtr_Default.Get());
			//.ViewportDelegates(&PreviewViewportDelegates);
		PreviewViewModelPtr_Default->OnStartPlay(PreviewViewportPtr_Default->GetViewportClient());

		IT4Framework* Framework = PreviewViewModelPtr_Default->GetFramework();
		check(nullptr != Framework);
		Framework->SetPreviewUIMode(true);
	}

	// 1. Viewport - Override
	{
		FT4PreviewEntityViewModelOptions ViewModelOptions;

		PreviewViewModelPtr = MakeShareable(new FT4PreviewEntityViewModel(ViewModelOptions));
		PreviewViewportPtr = SNew(ST4EditorViewport)
			.ViewModel(PreviewViewModelPtr.Get());
			//.ViewportDelegates(&PreviewViewportDelegates);
		PreviewViewModelPtr->OnStartPlay(PreviewViewportPtr->GetViewportClient());

		IT4Framework* Framework = PreviewViewModelPtr->GetFramework();
		check(nullptr != Framework);
		Framework->SetPreviewUIMode(true);
	}

	// 2. New Widget
	{
		// #t4f-879 WidgetManager의 초기화 필요
		ET4LayerType PreviewLayerType = ContentEditorFramework->GetLayerType();
		IT4GameplayWidgetManager* WidgetManager = GetContentWidgetManager();
		if (nullptr != WidgetManager)
		{
			WidgetManager->InitializeTableData(PreviewLayerType);
		}

		// #t4f-413
		TArray<FT4GameWidgetData> EditableWidgetData;
		EditableGameWidgetData(EditableWidgetData);
		UITypeDefaultTreePtr = SNew(ST4UITypeTreeWidget, &EditableWidgetData, this, TEXT("UIOverride New"), TEXT("CustomUI New"))
			.bAutoHeight(true)
			.OnAddItem(this, &FT4UserInterfaceEditor::HandleOnDefaultUINewSaveAs)
#if !UE_BUILD_SHIPPING // #t4f-589 : 기능 추가 이후에 제거
			.OnRemoveItem(this, &FT4UserInterfaceEditor::HandleOnCustomUINew)
#endif
			.OnSelected(this, &FT4UserInterfaceEditor::HandleOnDefaultUITypeSelected);

		UITypeDefaultTreePtr->OnRefresh(true);
	}

	// 3. Property
	{
		FT4UserInterfaceWidgetOptions UserInterfaceWidgetOptions;
		UserInterfaceWidgetOptions.UserInterfaceEditor = this;
		UserInterfaceDetailPtr = SNew(ST4UserInterfaceDetailWidget, UserInterfaceWidgetOptions);
	}

	// 3-1. UIOption Panel
	{
		FT4ProjectUIOptionWidgetOptions UIOptionWidgetOptions;
		UIOptionWidgetOptions.UserInterfaceEditor = this;
		UIOptionDetailPtr = SNew(ST4UIOptionDetailWidget, UIOptionWidgetOptions);
	}

	// 4. UIType List  #1156 
	{
		if (UITypeTreePtr.IsValid())
		{
			UITypeTreePtr->SetInitializeValue(NAME_None);
			UITypeTreePtr->OnRefresh(false);
		}

		UITypeTreePtr = SNew(ST4UITypeTreeWidget, &UserInterfaceAssetPtr.Get()->OverrideUserWidgetSettings.WidgetDatas, this, NAME_None, NAME_None)
			.bAutoHeight(true)
			.OnSelected(this, &FT4UserInterfaceEditor::HandleOnUITypeSelected)
			.OnRemoveItem(this, &FT4UserInterfaceEditor::HandleOnUITypeRemoveItem);
			//.OnVerifyRenameItem(this, &FT4UserInterfaceEditor::HandleOnUITypeVerifyRenameItem)
			//.OnRenameItem(this, &FT4UserInterfaceEditor::HandleOnUITypeRenameItem)
			//.OnDuplicateItem(this, &FT4UserInterfaceEditor::HandleOnUITypeDuplicateItem);

		UITypeTreePtr->OnRefresh(true);
	}

	TSharedPtr<FTabManager::FLayout> EditorDefaultLayout;

	{
		EditorDefaultLayout = FTabManager::NewLayout("T4UserInterfaceEditor_Layout_v1.35")
			->AddArea
			(
				FTabManager::NewPrimaryArea()
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient(0.9f)
					->SetOrientation(Orient_Horizontal)
					->Split
					(
						FTabManager::NewSplitter()
						->SetSizeCoefficient(0.4f)
						->SetOrientation(Orient_Vertical)
						->Split
						(
							FTabManager::NewStack()
							->SetSizeCoefficient(0.5f)
							->AddTab(PreviewViewportTabId_Default, ETabState::OpenedTab)->SetHideTabWell(false)
						)
						->Split
						(
							FTabManager::NewStack()
							->SetSizeCoefficient(0.5f)
							->AddTab(PreviewViewportTabId, ETabState::OpenedTab)->SetHideTabWell(false)
						)
					)
					->Split
					(
						FTabManager::NewSplitter()
						->SetSizeCoefficient(0.4f)
						->SetOrientation(Orient_Vertical)
						->Split
						(
							FTabManager::NewStack()
							->SetSizeCoefficient(0.5f)
							->AddTab(OverrideNewTabId, ETabState::OpenedTab)->SetHideTabWell(false)
						)
						->Split
						(
							FTabManager::NewSplitter()
							->SetSizeCoefficient(0.5f)
							->SetOrientation(Orient_Vertical)
							->Split
							(
								FTabManager::NewStack()
								->SetSizeCoefficient(0.3f)
								->AddTab(OverrideListTabId, ETabState::OpenedTab)->SetHideTabWell(false)							
							)
							->Split
							(
								FTabManager::NewStack()
								->SetSizeCoefficient(0.2f)
								->AddTab(UserInterfacePropertyId, ETabState::OpenedTab)->SetHideTabWell(false)
							)
						)
					)
					->Split
					(
						FTabManager::NewSplitter()
						->SetSizeCoefficient(0.2f)
						->SetOrientation(Orient_Vertical)
						->Split
						(
							FTabManager::NewStack()
							->SetSizeCoefficient(0.2f)
							->AddTab(UIOptionTabId, ETabState::OpenedTab)->SetHideTabWell(false)
						)
					)
				)
			);

	}

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(
		InMode,
		nullptr,
		GetAppIdentifierName(),
		EditorDefaultLayout.ToSharedRef(),
		bCreateDefaultStandaloneMenu,
		bCreateDefaultToolbar,
		UserInterfaceAssetPtr.Get()
	);

	SetupCommands();

	ExtendMenu();
	ExtendToolbar();
	RegenerateMenusAndToolbars();
	
	GEditor->RegisterForUndo(this);
	bInitializing = false;

	DEFINE_LICENSE_VALIDATION_WARNING_MACRO(); // #1126
	return true;
}

void FT4UserInterfaceEditor::PostUndo(bool bSuccess)
{
}

void FT4UserInterfaceEditor::CleanUp()
{
	if (PreviewViewportPtr.IsValid())
	{
		PreviewViewportPtr->OnCleanup();
		PreviewViewportPtr.Reset();
	}
	if (PreviewViewportPtr_Default.IsValid()) // #t4f-413
	{
		PreviewViewportPtr_Default->OnCleanup();
		PreviewViewportPtr_Default.Reset();
	}

	if (PreviewViewModelPtr.IsValid())
	{
		PreviewViewModelPtr->OnCleanup();
		PreviewViewModelPtr.Reset();
	}
	if (PreviewViewModelPtr_Default.IsValid()) // #t4f-413
	{
		PreviewViewModelPtr_Default->OnCleanup();
		PreviewViewModelPtr_Default.Reset();
	}
	if (UserInterfaceAssetPtr.IsValid())
	{
		UserInterfaceAssetPtr.Reset();
	}
	if (UIOptionDetailPtr.IsValid()) // #t4f-413
	{
		UIOptionDetailPtr.Reset();
	}
	if (UITypeTreePtr.IsValid())
	{
		UITypeTreePtr.Reset();
	}
	if (UITypeDefaultTreePtr.IsValid()) // #t4f-413
	{
		UITypeDefaultTreePtr.Reset();
	}

	GEditor->UnregisterForUndo(this);
}

void FT4UserInterfaceEditor::Tick(float DeltaTime)
{
	TickEditorBaseInternal(DeltaTime); // #148
}

void FT4UserInterfaceEditor::Finailize()
{
	CleanUp();
}

void FT4UserInterfaceEditor::NotifyOnExecuteSaveAsset()
{
	HandleOnExecuteSaveAsset();
}

void FT4UserInterfaceEditor::NotifyOnExecuteCloseWindow()
{
	GEngine->TrimMemory();
	CloseWindow();
}

TStatId FT4UserInterfaceEditor::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FT4UserInterfaceEditor, STATGROUP_Tickables);
}

void FT4UserInterfaceEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(
		LOCTEXT("T4UserInterfaceEditorTabSpawnerMenu", "T4UserInterfaceEditor")
	);

	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(PreviewViewportTabId_Default, FOnSpawnTab::CreateSP(this, &FT4UserInterfaceEditor::SpawnTab_PreviewViewport_Default)) // #t4f-413
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FT4EditorCommonStyle::GetStyleSetName(), "T4EditorCommonStyle.TabMainViewport_16X"));

	InTabManager->RegisterTabSpawner(PreviewViewportTabId, FOnSpawnTab::CreateSP(this, &FT4UserInterfaceEditor::SpawnTab_PreviewViewport))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FT4EditorCommonStyle::GetStyleSetName(), "T4EditorCommonStyle.TabMainViewport_16X"));
	
	// #t4f-413 삭제예정
	/*InTabManager->RegisterTabSpawner(OverrideNewTabId, FOnSpawnTab::CreateSP(this, &FT4UserInterfaceEditor::SpawnTab_OverrideNew))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FT4EditorCommonStyle::GetStyleSetName(), "T4EditorCommonStyle.TabMainViewport_16X"));*/

	InTabManager->RegisterTabSpawner(OverrideNewTabId, FOnSpawnTab::CreateSP(this, &FT4UserInterfaceEditor::SpawnTab_OverrideNewList)) // #t4f-413
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FT4EditorCommonStyle::GetStyleSetName(), "T4EditorCommonStyle.TabMainViewport_16X"));

	InTabManager->RegisterTabSpawner(OverrideListTabId, FOnSpawnTab::CreateSP(this, &FT4UserInterfaceEditor::SpawnTab_OverrideList))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FT4EditorCommonStyle::GetStyleSetName(), "T4EditorCommonStyle.TabMainViewport_16X"));

	InTabManager->RegisterTabSpawner(UserInterfacePropertyId, FOnSpawnTab::CreateSP(this, &FT4UserInterfaceEditor::SpawnTab_UserInterfaceDetail))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FT4ContentEditorStyle::GetStyleSetName(), "T4ContentEditorStyle.TabUserInterfaceDetail_16x"));
	
	InTabManager->RegisterTabSpawner(UIOptionTabId, FOnSpawnTab::CreateSP(this, &FT4UserInterfaceEditor::SpawnTab_UIOptionDetail)) // #t4f-413
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FT4ContentEditorStyle::GetStyleSetName(), "T4ContentEditorStyle.TabUserInterfaceDetail_16x"));

}

void FT4UserInterfaceEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(PreviewViewportTabId_Default);
	InTabManager->UnregisterTabSpawner(PreviewViewportTabId);
	InTabManager->UnregisterTabSpawner(OverrideNewTabId);
	InTabManager->UnregisterTabSpawner(OverrideListTabId);
	InTabManager->UnregisterTabSpawner(UserInterfacePropertyId);
	InTabManager->UnregisterTabSpawner(UIOptionTabId); // #t4f-413
}

FName FT4UserInterfaceEditor::GetToolkitFName() const
{
	return FName("T4UserInterfaceEditor");
}

FText FT4UserInterfaceEditor::GetBaseToolkitName() const
{
	return LOCTEXT("T4UserInterfaceEditorToolKitName", "T4UserInterfaceEditor");
}

FText FT4UserInterfaceEditor::GetToolkitToolTipText() const // #148
{
	const UObject* EditingObject = GetEditingObject();
	check(nullptr != EditingObject);
	FString NameString = FString::Printf(
		TEXT("%s: "),
		*(GetToolkitFName().ToString())
	);
	if (const AActor* ObjectAsActor = Cast<AActor>(EditingObject))
	{
		NameString += ObjectAsActor->GetActorLabel();
	}
	else
	{
		NameString += EditingObject->GetName();
	}
	return FText::FromString(NameString);
}

FString FT4UserInterfaceEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("T4UserInterfaceEditorCentricTabPrefix", "T4UserInterfaceEditor ").ToString();
}

FLinearColor FT4UserInterfaceEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.0f, 0.0f, 0.2f, 0.5f);
}

void FT4UserInterfaceEditor::SetupCommands()
{
	TSharedRef<FUICommandList> CommandList = GetToolkitCommands();
	SetupLicenseCommands(CommandList); // #1126
}

void FT4UserInterfaceEditor::ExtendMenu()
{

}

void FT4UserInterfaceEditor::ExtendToolbar()
{
	struct Local
	{
		static void FillToolbar(
			FToolBarBuilder& InToolbarBuilder,
			FT4UserInterfaceEditor* InEditor
		)
		{
			InToolbarBuilder.BeginSection("UserInterfaceEditorToolBar");
			{
				InEditor->ExtendLicenseToolbar(InToolbarBuilder); // #1126
			}
			InToolbarBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic(
			&Local::FillToolbar,
			this
		)
	);

	AddToolbarExtender(ToolbarExtender);

	FT4ContentEditorModule& ContentEditorModule = FModuleManager::LoadModuleChecked<FT4ContentEditorModule>("T4ContentEditor");
	AddToolbarExtender(
		ContentEditorModule.GetToolBarExtensibilityManager()->GetAllExtenders(
			GetToolkitCommands(),
			GetEditingObjects()
		)
	);
}

TSharedRef<SDockTab> FT4UserInterfaceEditor::SpawnTab_PreviewViewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == PreviewViewportTabId);
	return SNew(SDockTab)
		[
			PreviewViewportPtr.ToSharedRef()
		];
}

TSharedRef<SDockTab> FT4UserInterfaceEditor::SpawnTab_PreviewViewport_Default(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == PreviewViewportTabId_Default);
	return SNew(SDockTab)
		[
			PreviewViewportPtr_Default.ToSharedRef()
		];
}
// #t4f-413 삭제예정
//TSharedRef<SDockTab> FT4UserInterfaceEditor::SpawnTab_OverrideNew(const FSpawnTabArgs& Args)
//{
//	check(Args.GetTabId() == OverrideNewTabId);
//	return SNew(SDockTab)
//		[
//			UINewOverridePtr.ToSharedRef() // #1156
//		];
//}

TSharedRef<SDockTab> FT4UserInterfaceEditor::SpawnTab_OverrideList(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == OverrideListTabId);
	return SNew(SDockTab)
		[
			UITypeTreePtr.ToSharedRef() // #1156
		];
}

TSharedRef<SDockTab> FT4UserInterfaceEditor::SpawnTab_OverrideNewList(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == OverrideNewTabId);
	return SNew(SDockTab)
		[
			UITypeDefaultTreePtr.ToSharedRef() // #1156
		];
}

TSharedRef<SDockTab> FT4UserInterfaceEditor::SpawnTab_UserInterfaceDetail(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == UserInterfacePropertyId);
	return SNew(SDockTab)
		[
			UserInterfaceDetailPtr.ToSharedRef()
		];
}

TSharedRef<SDockTab> FT4UserInterfaceEditor::SpawnTab_UIOptionDetail(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == UIOptionTabId);
	return SNew(SDockTab)
		[
			UIOptionDetailPtr.ToSharedRef()
		];
}

UT4UserInterfaceAsset* FT4UserInterfaceEditor::GetEmptyUserInterfaceAsset()
{
	if (nullptr == EmptyUserInterfaceAssetOwner)
	{
		EmptyUserInterfaceAssetOwner = NewObject<UT4UserInterfaceAsset>();
	}
	return EmptyUserInterfaceAssetOwner;
}

bool FT4UserInterfaceEditor::SetDataWidgets(const TArray<FT4GameWidgetData>& InWidgetDatas)
{
	if (!UserInterfaceAssetPtr.IsValid())
		return false;

	for (const FT4GameWidgetData& WidgetData : InWidgetDatas)
	{
		UserInterfaceAssetPtr.Get()->OverrideUserWidgetSettings.WidgetDatas.Add(WidgetData);
	}

	return true;
}

void FT4UserInterfaceEditor::SetProjectName(const FName& InProjectName)
{
	if (!UserInterfaceAssetPtr.IsValid())
	{
		return;
	}

	UserInterfaceAssetPtr.Get()->ProjectName = InProjectName;
}

bool FT4UserInterfaceEditor::OnUserInterfaceSaveAsset(FString& OutErrorMessage) // #453
{
	UT4UserInterfaceAsset* UserInterfaceAsset = UserInterfaceAssetPtr.Get();
	if (nullptr == UserInterfaceAsset)
	{
		OutErrorMessage = TEXT("No Set UserInterfaceAsset");
		return false;
	}
	T4_EDITOR_TRANSACTION_SCOPED(LOCTEXT("OnUISaveAsset", "Save UIAsset"));
	bool bResult = T4AssetUtil::UserInterfaceAssetSaveAll(UserInterfaceAsset, OutErrorMessage);
	return bResult;
}

void FT4UserInterfaceEditor::SelectedUserWidget(const FT4GameWidgetData* InWidgetData)
{
	UT4UserInterfaceAsset* UserInterfaceAsset = UserInterfaceAssetPtr.Get();
	if (nullptr != UserInterfaceAsset)
	{
		// ToDo : Load Widget
		IT4Framework* Framework = PreviewViewModelPtr->GetFramework(); // #87
		check(nullptr != Framework);
		ET4LayerType PreviewLayerType = Framework->GetLayerType();
		IT4GameplayWidgetManager* WidgetManager = GetGameplayWidgetManagerByLayer(PreviewLayerType);
		if (nullptr != WidgetManager)
		{
			// 1. Remove Widget
			if (SelectedUserWidgetPtr != nullptr)
			{
				SelectedUserWidgetPtr.Get()->Show(false);
			}
			if (EditorPreviewBlockPtr != nullptr)
			{
				EditorPreviewBlockPtr.Get()->Show(false);
			}

			// 2. AddToScreen
			WidgetManager->SyncLoadWidget(PreviewLayerType, InWidgetData, true, [&](UT4UserWidgetBase* Widget)
				{
					if (nullptr != Widget)
					{
						SelectedUserWidgetPtr = Widget;

						if (nullptr != SelectedUserWidgetPtr)
						{
							SelectedUserWidgetPtr.Get()->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
							SelectedUserWidgetPtr.Get()->Show(true);
						}
					}
				}
			);

			// #453 Attach EditorPreview Block~~
			FName ConvertWidgetName = WidgetManager->ConvertUITypeToName(ET4GameUIType::EditorPreview);// #t4f-589
			WidgetManager->SyncLoadWidget(PreviewLayerType, ConvertWidgetName, true, [&](UT4UserWidgetBase* pWidget)
				{
					if (nullptr != pWidget)
					{
						EditorPreviewBlockPtr = pWidget;

						if (nullptr != EditorPreviewBlockPtr)
						{
							EditorPreviewBlockPtr.Get()->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
							EditorPreviewBlockPtr.Get()->Show(true);
						}
					}
				}
			);
		}
	}
}

void FT4UserInterfaceEditor::SelectedDefaultUserWidget(const FT4GameWidgetData* InWidgetData)
{
	UT4UserInterfaceAsset* UserInterfaceAsset = UserInterfaceAssetPtr.Get();
	if (nullptr != UserInterfaceAsset)
	{
		// ToDo : Load Widget
		IT4Framework* Framework = PreviewViewModelPtr_Default->GetFramework(); // #87
		check(nullptr != Framework);
		ET4LayerType PreviewLayerType = Framework->GetLayerType();
		IT4GameplayWidgetManager* WidgetManager = GetPreviewWidgetManager_Default(); //#t4f-479
		if (nullptr != WidgetManager)
		{
			// 1. Remove Widget
			if (SelectedDefaultUserWidgetPtr != nullptr)
			{
				SelectedDefaultUserWidgetPtr.Get()->Show(false);
			}

			// 2. AddToScreen
			WidgetManager->SyncLoadWidget(PreviewLayerType, InWidgetData, true, [&](UT4UserWidgetBase* Widget)
				{
					if (nullptr != Widget)
					{
						SelectedDefaultUserWidgetPtr = Widget;

						if (nullptr != SelectedDefaultUserWidgetPtr)
						{
							SelectedDefaultUserWidgetPtr.Get()->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
							SelectedDefaultUserWidgetPtr.Get()->Show(true);
						}
					}
				}
			);

			// #t4f-479 Attach EditorPreview Block~~
			FName ConvertWidgetName = WidgetManager->ConvertUITypeToName(ET4GameUIType::EditorPreview);// #t4f-589
			WidgetManager->SyncLoadWidget(PreviewLayerType, ConvertWidgetName, true, [&](UT4UserWidgetBase* pWidget)
				{
					if (nullptr != pWidget)
					{
						EditorPreviewBlockPtr = pWidget;

						if (nullptr != EditorPreviewBlockPtr)
						{
							EditorPreviewBlockPtr.Get()->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
							EditorPreviewBlockPtr.Get()->Show(true);
						}
					}
				}
			);
		}
	}
}

void FT4UserInterfaceEditor::SelectedUserWidget_UITypeName(
	FName InUITypeName,
	const ET4GameUITemplate& InSelectedGameUITemplate
)// #1156
{
	// #t4f-270 : Framework를 GContentEditorPtr로 통일
	IT4GameplayWidgetManager* WidgetManager = GetContentWidgetManager();
	if (nullptr != WidgetManager)
	{
		const FT4GameWidgetData* WidgetData = FindWidgetDataByUIOverrideAsset(InUITypeName, InSelectedGameUITemplate);
		if (nullptr != WidgetData)
		{
			SelectedUserWidget(WidgetData);
		}
	}
}

void FT4UserInterfaceEditor::SelectedUserWidget_DefaultUITypeName(FName InUITypeName)// #t4f-413
{
	// #t4f-270 : Framework를 GContentEditorPtr로 통일
	IT4GameplayWidgetManager* WidgetManager = GetContentWidgetManager();
	if (nullptr != WidgetManager)
	{
		// #t4f-961 선택된 UITemplate을 찾아야함
		 const FT4GameWidgetData* GameWidgetData = FindEditableGameWidgetData(InUITypeName);
		if (nullptr != GameWidgetData)
		{
			SelectedDefaultUserWidget(GameWidgetData);
		}
	}
}

// #t4f-589
const FT4GameWidgetData* FT4UserInterfaceEditor::FindWidgetDataByUIOverrideAsset(
	const FName& InGameUITypeName,
	const ET4GameUITemplate& InGameUITemplate
)
{
	IT4GameplayWidgetManager* WidgetManager = GetContentWidgetManager();
	if (nullptr != WidgetManager)
	{
		if (UserInterfaceAssetPtr.IsValid())
		{
			if (UserInterfaceAssetPtr.Get()->OverrideUserWidgetSettings.WidgetDatas.Num() > 0)
			{
				const FT4GameWidgetData* WidgetData = UserInterfaceAssetPtr.Get()->OverrideUserWidgetSettings.WidgetDatas.FindByPredicate(
					[&, InGameUITypeName, InGameUITemplate, WidgetManager](const FT4GameWidgetData& InWidgetData)
					{
						return (InGameUITypeName == WidgetManager->GetGameUITypeName(InWidgetData) &&
							InGameUITemplate == InWidgetData.UITemplate);
					}
				);

				if (nullptr != WidgetData)
				{
					return WidgetData;
				}
			}
		}
	}

	return nullptr;
}

const FT4GameWidgetData* FT4UserInterfaceEditor::FindCustomWidgetDataByUIOverrideAsset(const FName& InCustomUITypeName)
{
	if (UserInterfaceAssetPtr.IsValid())
	{
		if (UserInterfaceAssetPtr.Get()->OverrideUserWidgetSettings.WidgetDatas.Num() > 0)
		{
			const FT4GameWidgetData* WidgetData = UserInterfaceAssetPtr.Get()->OverrideUserWidgetSettings.WidgetDatas.FindByPredicate(
				[&, InCustomUITypeName](const FT4GameWidgetData& InWidgetData)
				{
					return InCustomUITypeName == InWidgetData.CustomUITypeData.CustomTypeName;
				}
			);

			if (nullptr != WidgetData)
			{
				return WidgetData;
			}
		}
	}

	return nullptr;
}

const FT4GameWidgetData* FT4UserInterfaceEditor::FindWidgetDataByUIDefaultAsset(const FName& InUITypeName)
{
	// #t4f-270 : Framework를 GContentEditorPtr로 통일
	ET4LayerType PreviewLayerType = ContentEditorFramework->GetLayerType();
	IT4GameplayWidgetManager* WidgetManager = GetContentWidgetManager();
	if (nullptr != WidgetManager)
	{
		WidgetManager->InitializeTableData(PreviewLayerType);
		const FT4GameWidgetData* WidgetData = WidgetManager->FindWidgetInfo(InUITypeName);// #t4f-589
		if (nullptr != WidgetData)
		{
			return WidgetData;
		}
	}

	return nullptr;
}

void FT4UserInterfaceEditor::HandleOnDefaultUITypeSelected(const FName InName)
{
	check(UITypeDefaultTreePtr.IsValid());
	FName UITypeSelection = UITypeDefaultTreePtr->GetItemValueSelected();
	if (UITypeSelection == NAME_None)
	{
		return;
	}

	// #t4f-961 : 선택Colume의 "Template"을 찾는다.
	FString UITemplateSelection = UITypeDefaultTreePtr->GetItemStringSelected(TEXT("Template"));
	SelectedGameUITemplate = GetUITemplateByName(UITemplateSelection);

	SelectedUserWidget_DefaultUITypeName(UITypeSelection);
	SelectedDefaultUITypeName = UITypeSelection; // #1156
	SetNewSelectedUITypeName(SelectedDefaultUITypeName); // #t4f-413 : Default UI 선택
}

void FT4UserInterfaceEditor::HandleOnUITypeSelected(const FName InName)
{
	check(UITypeTreePtr.IsValid());
	FName UITypeSelection = UITypeTreePtr->GetItemValueSelected();
	if (UITypeSelection == NAME_None)
	{
		return;
	}
	// #t4f-961 : 선택Colume의 "Template"을 찾는다.
	FString UITemplateSelection = UITypeTreePtr->GetItemStringSelected(TEXT("Template"));
	SelectedGameUITemplate = GetUITemplateByName(UITemplateSelection);

	SelectedUserWidget_UITypeName(UITypeSelection, SelectedGameUITemplate);
	SelectedUITypeName = UITypeSelection; // #1156
	UserInterfaceDetailPtr->OnRefresh();
}

void FT4UserInterfaceEditor::HandleOnUITypeRemoveItem(const TArray<FName>& InNames)
{
	check(UITypeTreePtr.IsValid());
	FName UITypeSelection = UITypeTreePtr->GetItemValueSelected();
	if (UITypeSelection == NAME_None)
	{
		return;
	}
	// #t4f-961 : 선택Colume의 "Template"을 찾는다.
	FString UITemplateSelection = UITypeTreePtr->GetItemStringSelected(TEXT("Template"));
	ET4GameUITemplate _SelectedGameUITemplate = GetUITemplateByName(UITemplateSelection);

	FString WriteMessage;
	if (!T4_EDITOR_GET_MESSAGE(T4UIEDITOR_NAME, TEXT("UITypeRemove"), WriteMessage))
	{
		WriteMessage = TEXT("Are you sure you want to delete both UIType and UIAsset? \nYes - Both UIType and UIAsset \nNo - UIType Only");
	}

	EAppReturnType::Type YerNoCancelRemoveItem = FMessageDialog::Open(EAppMsgType::YesNoCancel,
		FText::Format(LOCTEXT("UIType Remove", "{0}"), FText::FromString(WriteMessage)));

	if (YerNoCancelRemoveItem == EAppReturnType::Yes)
	{
		//1. Delete UIOverrideAsset
		GameWidgetData_Delete(UITypeSelection, _SelectedGameUITemplate, true);
		// 2. Delete WidgetData
		DeleteWidgetDataByUIType(UITypeSelection, _SelectedGameUITemplate);
	}
	else if (YerNoCancelRemoveItem == EAppReturnType::No)
	{
		// 2. Delete WidgetData
		DeleteWidgetDataByUIType(UITypeSelection, _SelectedGameUITemplate);
	}
	else if (YerNoCancelRemoveItem == EAppReturnType::Cancel)
	{
		return;
	}

	Refresh();
}

bool FT4UserInterfaceEditor::HandleOnUITypeVerifyRenameItem(const FName InRename, FString& OutErrorMessage)
{
	// #1156 사용 필요 없음~~

	return false;
}

void FT4UserInterfaceEditor::HandleOnUITypeRenameItem(const FName InOldName, const FName InNewName)
{
	// #1156 사용 필요 없음~~
}

void FT4UserInterfaceEditor::HandleOnUITypeDuplicateItem(const FName InName)
{
	// #1156 사용 필요 없음~~
}

FName FT4UserInterfaceEditor::GetGameUITypeName(const FT4GameWidgetData& InWidgetData)// #t4f-589
{
	IT4GameplayWidgetManager* WidgetManager = GetContentWidgetManager();
	if (nullptr != WidgetManager)
	{
		return WidgetManager->GetGameUITypeName(InWidgetData);
	}

	return NAME_None;
}

void FT4UserInterfaceEditor::EditableGameWidgetData(OUT TArray<FT4GameWidgetData>& InWidgetData)
{
	ET4LayerType PreviewLayerType = ContentEditorFramework->GetLayerType();
	IT4GameplayWidgetManager* WidgetManager = GetContentWidgetManager();
	if (nullptr != WidgetManager)
	{
		WidgetManager->FindDefaultWidgetDataByEditable(PreviewLayerType, InWidgetData); // #t4f-961
	}
}

// #t4f-448
const FT4GameWidgetData* FT4UserInterfaceEditor::FindEditableGameWidgetData(const FName& InGameUITypeName)// #t4f-589
{
	ET4LayerType PreviewLayerType = ContentEditorFramework->GetLayerType();
	IT4GameplayWidgetManager* WidgetManager = GetContentWidgetManager();
	if (nullptr != WidgetManager)
	{
		return WidgetManager->GetDefaultWidgetDataByEditable(PreviewLayerType, InGameUITypeName, SelectedGameUITemplate);
	}

	return nullptr;
}

FName FT4UserInterfaceEditor::GetSelectedUITypeName()
{
	return SelectedUITypeName;
}

ET4GameUITemplate FT4UserInterfaceEditor::GetSelectedGameUITemplate()
{
	return SelectedGameUITemplate;
}

void FT4UserInterfaceEditor::SetNewSelectedUITypeName(const FName& InSelectedUITypeName)
{
	NewSelectedUITypeName = InSelectedUITypeName;
}

void FT4UserInterfaceEditor::HandleOnUINewSaveAs(FName InUIType) // Param 사용하지 않음
{
	return; // #t4f-413  사용하지 않음
}

void FT4UserInterfaceEditor::HandleOnDefaultUINewSaveAs() // #t4f-413
{
	if (NewSelectedUITypeName == NAME_None)
	{
		FString WriteMessage;
		if (T4_EDITOR_GET_MESSAGE(T4UIEDITOR_NAME, TEXT("NoSelect_UIType"), WriteMessage))
		{
			T4_LOG(Warning, TEXT("%s"), *WriteMessage);
		}

		FMessageDialog::Open(
			EAppMsgType::Ok,
			EAppReturnType::Ok,
			FText::FromString(WriteMessage),
			&T4UIOverrideErrorTitleText
		);

		return;
	}

	// 중복인 경우 생성 못하게 한다!!!
	const FT4GameWidgetData* WidgetData = FindWidgetDataByUIOverrideAsset(NewSelectedUITypeName, SelectedGameUITemplate);
	if (nullptr != WidgetData)
	{
		FString WriteMessage;
		if (T4_EDITOR_GET_MESSAGE(T4UIEDITOR_NAME, TEXT("AlreadyExist_UIType"), WriteMessage))
		{
			T4_LOG(Warning, TEXT("%s"), *WriteMessage);
		}

		FMessageDialog::Open(
			EAppMsgType::Ok,
			EAppReturnType::Ok,
			FText::FromString(WriteMessage),
			&T4UIOverrideErrorTitleText
		);

		return;
	}

	UIOverrideSaveAs(NewSelectedUITypeName); // #t4f-589 : 선택 UI 복사생성 부분을 분리
}

// #t4f-589 InName 사용하지 않음
void FT4UserInterfaceEditor::HandleOnCustomUINew(const TArray<FName>& InName)
{
	if (NewSelectedUITypeName == NAME_None)
	{
		FString WriteMessage;
		if (T4_EDITOR_GET_MESSAGE(T4UIEDITOR_NAME, TEXT("NoSelect_UIType"), WriteMessage))
		{
			T4_LOG(Warning, TEXT("%s"), *WriteMessage);
		}

		FMessageDialog::Open(
			EAppMsgType::Ok,
			EAppReturnType::Ok,
			FText::FromString(WriteMessage),
			&T4UIOverrideErrorTitleText
		);

		return;
	}

	// #t4f-589 CustomUI 지원하는 타입확인
	IT4GameplayWidgetManager* WidgetManager = GetContentWidgetManager();
	if (nullptr != WidgetManager)
	{
		TArray<FName> CustomUIEditableTypeName;
		ET4LayerType PreviewLayerType = ContentEditorFramework->GetLayerType();
		WidgetManager->FindCustomWidgetDataByEditable(PreviewLayerType, CustomUIEditableTypeName);

		if (!CustomUIEditableTypeName.Contains(NewSelectedUITypeName))
		{
			FString WriteMessage;
			if (T4_EDITOR_GET_MESSAGE(T4UIEDITOR_NAME, TEXT("DontSupport_CustomUI"), WriteMessage))
			{
				T4_LOG(Warning, TEXT("%s"), *WriteMessage);
			}

			FMessageDialog::Open(
				EAppMsgType::Ok,
				EAppReturnType::Ok,
				FText::FromString(WriteMessage),
				&T4UICustomErrorTitleText
			);

			return;
		}
	}

	// 중복인 경우에도 생성 가능하다. 단 이름(FName)은 바뀌어야 한다
	UIOverrideSaveAs(NewSelectedUITypeName, true); // #t4f-589
}

void FT4UserInterfaceEditor::UIOverrideSaveAs(const FName& InGameUITypeName, bool InCustomUI/* = false*/) // #t4f-589 : 선택 UI 복사생성 부분을 분리
{
	const FT4GameWidgetData* GameWidgetData = FindEditableGameWidgetData(InGameUITypeName);// #t4f-448
	if (nullptr != GameWidgetData)
	{
		// #1156 TSoftObjectPtr<UBlueprint> WidgetPath 적용
		UBlueprint* LoadedUnit = GameWidgetData->WidgetPath.LoadSynchronous();
		if (nullptr == LoadedUnit)
		{
			T4_LOG(Warning, TEXT("[Error] LoadWidgetPath = %s"), *GameWidgetData->WidgetPath.ToString());
			return;
		}
		TSubclassOf<UT4UserWidgetBase> WidgetClassRef = nullptr;
		WidgetClassRef = static_cast<TSubclassOf<UT4UserWidgetBase>>(LoadedUnit->GeneratedClass);
		check(nullptr != WidgetClassRef);

		FSoftObjectPath AssetRef = FSoftObjectPath(WidgetClassRef);
		FString Filename;
		TArray<UObject*> UISourcesToSave;
		TArray<UObject*> SavedUISources;

		TArray<FAssetData> AssetList;
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
		if (AssetRegistryModule.Get().GetAssetsByPackageName(*AssetRef.GetLongPackageName(), AssetList, true))
		{
			for (int32 AssetIdx = 0; AssetIdx < AssetList.Num(); ++AssetIdx)
			{
				UObject* LoadObject = AssetList[AssetIdx].GetAsset();
				if (nullptr == LoadObject)
				{
					continue;
				}
#if USES_SUPPORT_UE5_1 // #t4f-511
				Filename = AssetList[AssetIdx].GetObjectPathString();
#else
				Filename = AssetList[AssetIdx].ObjectPath.ToString();
#endif
				UISourcesToSave.Add(LoadObject);
			}
		}
		FEditorFileUtils::SaveAssetsAs(UISourcesToSave, SavedUISources);

		{
			IT4GameplayWidgetManager* WidgetManager = GetContentWidgetManager();
			if (nullptr != WidgetManager)
			{
				GameWidgetData = FindEditableGameWidgetData(InGameUITypeName);
			}
		}
		// #t4f-589
		FT4GameWidgetData CustomGameWidgetData;
		if (InCustomUI)
		{
			CustomGameWidgetData.UITemplate = GameWidgetData->UITemplate;
			CustomGameWidgetData.Type = ET4GameUIType::CustomUI; // ** 주의 **
			CustomGameWidgetData.UITemplate = GameWidgetData->UITemplate; // #t4f-961
			CustomGameWidgetData.WidgetPath = GameWidgetData->WidgetPath;
			CustomGameWidgetData.ZOrder = GameWidgetData->ZOrder;
			CustomGameWidgetData.bRegisterOnPreview = GameWidgetData->bRegisterOnPreview;
			CustomGameWidgetData.bIsShowOnStart = GameWidgetData->bIsShowOnStart;
			CustomGameWidgetData.bIsEditorEditable = GameWidgetData->bIsEditorEditable; // #t4f-879
			CustomGameWidgetData.bIsCustomEditable = GameWidgetData->bIsCustomEditable; // #t4f-882
			//CustomGameWidgetData.bIsHideSkip = GameWidgetData->bIsHideSkip; // #t4f-1062
			// #t4f-589
			// 1. CustomTypeName
			CustomGameWidgetData.CustomUITypeData.CustomTypeName = GetNewCustomUIName(Filename, InGameUITypeName);
			// 2. ParentUIType
			CustomGameWidgetData.CustomUITypeData.ParentUIType = GameWidgetData->Type;
		}

		if (SavedUISources.Num() == 0)
		{
			FString WriteMessage;
			if (T4_EDITOR_GET_MESSAGE(T4UIEDITOR_NAME, TEXT("YesNoSave_UIType"), WriteMessage))
			{
				T4_LOG(Warning, TEXT("%s"), *WriteMessage);
			}

			// FText::Format( LOCTEXT("BlendSpace", "Blend Space {0}"), FText::FromString(BlendSpace->GetName()) ).ToString();
			EAppReturnType::Type YerNoAddItem = FMessageDialog::Open(EAppMsgType::YesNo,
				FText::Format(LOCTEXT("UIType Remove", "{0}"), FText::FromString(WriteMessage)));

			// #t4f-589 : CustomUI일 경우 FT4CustomUITypeData 세팅 저장
			if (InCustomUI)
			{
				IT4GameplayWidgetManager* WidgetManager = GetContentWidgetManager();
				if (nullptr != WidgetManager)
				{
					if (YerNoAddItem == EAppReturnType::Yes)
					{
						GameWidgetData_Add(nullptr, &CustomGameWidgetData);
					}
				}
			}
			else
			{
				if (YerNoAddItem == EAppReturnType::Yes)
				{
					GameWidgetData_Add(nullptr, GameWidgetData);
				}
			}
		}
		else
		{
			if (InCustomUI)
			{
				GameWidgetData_Add(SavedUISources[0], &CustomGameWidgetData);
			}
			else
			{
				GameWidgetData_Add(SavedUISources[0], GameWidgetData);
			}
		}
	}
}

FName FT4UserInterfaceEditor::GetNewCustomUIName(const FString InFilename, const FName& InGameUITypeName)
{
	// 중복이 되지 않도록 생성해서 넣기 : 갯수를 구해서 넘버링을 한다
	FString MakeTextString;
	int32 Index = 0;
	while (1)
	{	
		MakeTextString = FString::Printf(
			TEXT("%s_%s"),
			*InGameUITypeName.ToString(),
			*FString::FromInt(Index)
		);
		++Index;

		if (IsCustomUIName(*MakeTextString) == false || Index > 100) // 100개이상 못만들게함
			break;
	}

	return *MakeTextString;
}

void FT4UserInterfaceEditor::HandleOnExecuteSaveAsset()
{
	if (UserInterfaceAssetPtr.IsValid())
	{
		T4AssetUtil::SaveAsset(UserInterfaceAssetPtr.Get(), false);
	}

	Refresh();
}

void FT4UserInterfaceEditor::GameWidgetData_Add(UObject* InSavedUISources, const FT4GameWidgetData* InGameWidgetData)
{
	if (InGameWidgetData == nullptr)
		return;

	FT4GameWidgetData NewWidgetData;
	NewWidgetData.Type = InGameWidgetData->Type;
	NewWidgetData.UITemplate = InGameWidgetData->UITemplate; // #t4f-961
	NewWidgetData.ZOrder = InGameWidgetData->ZOrder;
	NewWidgetData.bRegisterOnPreview = InGameWidgetData->bRegisterOnPreview;
	NewWidgetData.bIsShowOnStart = InGameWidgetData->bIsShowOnStart;
	NewWidgetData.bIsEditorEditable = InGameWidgetData->bIsEditorEditable; // #t4f-879
	NewWidgetData.bIsCustomEditable = InGameWidgetData->bIsCustomEditable; // #t4f-882
	//NewWidgetData.bIsHideSkip = InGameWidgetData->bIsHideSkip; // #t4f-1062
	NewWidgetData.CustomUITypeData = InGameWidgetData->CustomUITypeData; // #t4f-589

	if (InSavedUISources != nullptr)
	{
		FSoftObjectPath AssetRef = FSoftObjectPath(InSavedUISources);
		NewWidgetData.WidgetPath = static_cast<TSoftObjectPtr<UBlueprint>>(AssetRef);
	}

	TArray<FT4GameWidgetData> WidgetDatas;
	WidgetDatas.Add(NewWidgetData);
	SetDataWidgets(WidgetDatas);

	HandleOnExecuteSaveAsset(); // #t4f-589 : 자동 Save를 해주자!!!
	//Refresh();
}

void FT4UserInterfaceEditor::GameWidgetData_Delete(
	const FName& InUITypeName,
	const ET4GameUITemplate& InSelectedGameUITemplate, 
	bool InDeleteWidgetData/* = false*/
)
{
	FString UIOverrideAssetPathStr;
	const FT4GameWidgetData* WidgetData = FindWidgetDataByUIOverrideAsset(InUITypeName, InSelectedGameUITemplate);// #t4f-589
	if (nullptr != WidgetData)
	{
		UBlueprint* LoadedUnit = WidgetData->WidgetPath.LoadSynchronous();
		if (nullptr == LoadedUnit)
		{
			T4_LOG(Warning, TEXT("[Error] LoadWidgetPath = %s"), *WidgetData->WidgetPath.ToString());
			return;
		}

		TSubclassOf<UT4UserWidgetBase> WidgetClassRef = nullptr;
		WidgetClassRef = static_cast<TSubclassOf<UT4UserWidgetBase>>(LoadedUnit->GeneratedClass);
		if (nullptr != WidgetClassRef)
		{
			FSoftObjectPath AssetRef = FSoftObjectPath(WidgetClassRef);
			UIOverrideAssetPathStr = AssetRef.GetLongPackageName();

			if (InDeleteWidgetData)
			{
				DeleteWidgetDataByUIType(InUITypeName, InSelectedGameUITemplate);
			}
		}

		TArray<FAssetData> UIOverrideAssetDatas;
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		if (AssetRegistryModule.Get().GetAssetsByPackageName(*UIOverrideAssetPathStr, UIOverrideAssetDatas, true))
		{
			if (UIOverrideAssetDatas.Num() > 0)
			{
				ObjectTools::DeleteAssets(UIOverrideAssetDatas, true);
				//GEditor->SyncBrowserToObjects(UIOverrideAssetDatas);
			}
/*
			for (int32 AssetIdx = 0; AssetIdx < UIOverrideAssetDatas.Num(); ++AssetIdx)
			{
				UObject* LoadObject = UIOverrideAssetDatas[AssetIdx].GetAsset();
				if (nullptr == LoadObject)
				{
					continue;
				}

				
				FAssetRegistryModule::AssetDeleted(LoadObject);
				LoadObject->ClearFlags(RF_Standalone | RF_Public);
				LoadObject->RemoveFromRoot();
				LoadObject->MarkPendingKill();
			}
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
*/
			HandleOnExecuteSaveAsset(); // 자동 Save를 해주자!!!
		}
	}
}

bool FT4UserInterfaceEditor::DeleteWidgetDataByUIType(
	const FName& InUITypeName,
	const ET4GameUITemplate& InSelectedGameUITemplate
)
{
	IT4GameplayWidgetManager* WidgetManager = GetContentWidgetManager();
	if (nullptr == WidgetManager)
		return false;

	if (UserInterfaceAssetPtr.IsValid())
	{
		if (UserInterfaceAssetPtr.Get()->OverrideUserWidgetSettings.WidgetDatas.Num() > 0)
		{
			int32 FindIndex = UserInterfaceAssetPtr.Get()->OverrideUserWidgetSettings.WidgetDatas.IndexOfByPredicate(
				[&, InUITypeName, InSelectedGameUITemplate](const FT4GameWidgetData& InWidgetData)
				{
					return (InUITypeName == WidgetManager->GetGameUITypeName(InWidgetData) &&
						InSelectedGameUITemplate == InWidgetData.UITemplate);
				}
			);

			if (FindIndex != INDEX_NONE)
			{
				UserInterfaceAssetPtr.Get()->OverrideUserWidgetSettings.WidgetDatas.RemoveAt(FindIndex);
				return true;
			}
		}
	}

	return false;
}

void FT4UserInterfaceEditor::Refresh()
{
	if (UITypeTreePtr.IsValid())
	{
		UITypeTreePtr->OnRefresh(true);
	}

	// #t4f-413 : Refresh 하면 리스트가 사라짐 ???
	//if (UITypeDefaultTreePtr.IsValid())
	//{
	//	UITypeDefaultTreePtr->OnRefresh(false);
	//}

	if (UserInterfaceDetailPtr.IsValid())
	{
		UserInterfaceDetailPtr->OnRefresh();
	}
	if (UIOptionDetailPtr.IsValid()) // #t4f-413
	{
		UIOptionDetailPtr->OnRefresh();
	}

	if (PreviewViewModelPtr.IsValid())
	{
		PreviewViewModelPtr->RefreshAllAndCleanup();
	}
	if (PreviewViewModelPtr_Default.IsValid()) // #t4f-413
	{
		PreviewViewModelPtr_Default->RefreshAllAndCleanup();
	}

	UpdateWidgetManager_UIOverrideList(); // #1156
}

// #1156 Editor에서 UIOverride 리스트가 갱신 될경우 WidgetManger에도 UIOverride 리스트가 함께 갱신되도록
void FT4UserInterfaceEditor::UpdateWidgetManager_UIOverrideList()
{
	// #t4f-270 : Framework를 GContentEditorPtr로 통일
	UT4UserInterfaceAsset* UserInterfaceAsset = UserInterfaceAssetPtr.Get();
	if (nullptr != UserInterfaceAsset && nullptr != ContentEditorFramework)
	{
		ET4LayerType PreviewLayerType = ContentEditorFramework->GetLayerType();
		IT4GameplayWidgetManager* WidgetManager = GetGameplayWidgetManagerByLayer(PreviewLayerType);
		if (nullptr != WidgetManager)
		{
			WidgetManager->Update_TableData(PreviewLayerType, UserInterfaceAsset->OverrideUserWidgetSettings);
		}
	}
}

IT4GameplayWidgetManager* FT4UserInterfaceEditor::GetPreviewWidgetManager() const
{
	if (!PreviewViewModelPtr.IsValid())
	{
		return nullptr;
	}
	IT4Framework* Framework = PreviewViewModelPtr->GetFramework(); // #87
	check(nullptr != Framework);
	ET4LayerType PreviewLayerType = Framework->GetLayerType();
	IT4GameplayWidgetManager* WidgetManager = GetGameplayWidgetManagerByLayer(PreviewLayerType);
	return WidgetManager;
}

IT4GameplayWidgetManager* FT4UserInterfaceEditor::GetPreviewWidgetManager_Default() const // #t4f-479
{
	if (!PreviewViewModelPtr_Default.IsValid())
	{
		return nullptr;
	}
	IT4Framework* Framework = PreviewViewModelPtr_Default->GetFramework(); // #87
	check(nullptr != Framework);
	ET4LayerType PreviewLayerType = Framework->GetLayerType();
	IT4GameplayWidgetManager* WidgetManager = GetGameplayWidgetManagerByLayer(PreviewLayerType);
	return WidgetManager;
}

IT4GameplayWidgetManager* FT4UserInterfaceEditor::GetContentWidgetManager() const
{
	if (nullptr == ContentEditorFramework)
	{
		return nullptr;
	}
	ET4LayerType PreviewLayerType = ContentEditorFramework->GetLayerType();
	IT4GameplayWidgetManager* WidgetManager = GetGameplayWidgetManagerByLayer(PreviewLayerType);
	return WidgetManager;
}

ET4LayerType FT4UserInterfaceEditor::GetContentEditorLayerType()
{
	if (nullptr != ContentEditorFramework)
	{
		return ContentEditorFramework->GetLayerType();
	}

	return ET4LayerType::Null;
}

IT4GameplayWidgetManager* FT4UserInterfaceEditor::GetWidgetManager() const
{
	return GetContentWidgetManager();
}

// #t4f-589 : CustomUITypeName이 같은게 있는지 확인
bool FT4UserInterfaceEditor::IsCustomUIName(const FName& InGameUITypeName)
{
	if (UserInterfaceAssetPtr.IsValid())
	{
		for (const FT4GameWidgetData& WidgetData : UserInterfaceAssetPtr.Get()->OverrideUserWidgetSettings.WidgetDatas)
		{
			FT4GameWidgetData ResultData = WidgetData;
			if (ResultData.Type != ET4GameUIType::CustomUI)
				continue;

			const FT4GameWidgetData* FindWidgetData = UserInterfaceAssetPtr.Get()->OverrideUserWidgetSettings.WidgetDatas.FindByPredicate(
				[&, InGameUITypeName](const FT4GameWidgetData& InWidgetData)
				{
					return InGameUITypeName == InWidgetData.CustomUITypeData.CustomTypeName;
				}
			);

			if (nullptr != FindWidgetData)
			{
				return true;
			}
		}
	}

	return false;
}

ET4GameUITemplate FT4UserInterfaceEditor::GetUITemplateByName(const FString& InStringUITemplate) // #t4f-961
{
	ET4GameUITemplate ResultValue = ET4GameUITemplate::Default;
	if (InStringUITemplate == TEXT("TileBase"))
	{
		ResultValue = ET4GameUITemplate::TileBase;
	}
	else if (InStringUITemplate == TEXT("ModularParts"))
	{
		ResultValue = ET4GameUITemplate::ModularParts;
	}
	else if (InStringUITemplate == TEXT("StoryBook"))
	{
		ResultValue = ET4GameUITemplate::StoryBook;
	}
	else
	{
		ResultValue = ET4GameUITemplate::Default;
	}

	return ResultValue;
}
#undef LOCTEXT_NAMESPACE