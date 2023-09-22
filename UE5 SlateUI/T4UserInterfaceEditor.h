// Copyright VirtualFlow, Inc. All Rights Reserved.

#pragma once

#include "T4Framework/Public/T4FrameworkMinimal.h"
#include "ST4UserInterfaceDetailWidget.h"
#include "ST4UIOptionDetailWidget.h"

#include "T4EditorCommon/Public/Toolkit/T4EditorToolkitBase.h"

#include "T4GameData/Public/DB/T4GameDBTypes.h"
#include "T4GameData/Classes/Content/T4ProjectSettingsAsset.h"

#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/IToolkitHost.h"

#include "T4GameData/Public/T4GameDataTypes.h" // #453

/**
 * #453
 */
class IToolkitHost;
class IDetailsView;
class ST4EditorViewport;
class FT4PreviewEntityViewModel;
class UT4UserInterfaceAsset;
class UT4ProjectSettingsAsset;
class IT4GameplayWidgetManager;
class ST4UITypeTreeWidget; // #1156

class FT4UserInterfaceEditor
	: public FAssetEditorToolkit
	, public FEditorUndoClient
	, public FT4EditorToolkitBase
	, public FTickableEditorObject
{
public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FT4OnClose, FT4UserInterfaceEditor*);

public:
	FT4UserInterfaceEditor();
	virtual ~FT4UserInterfaceEditor();

	bool OnInitialize(
		const EToolkitMode::Type InMode,
		UObject* InUserInterfaceAsset,
		const TSharedPtr<IToolkitHost>& InInitToolkitHost,
		const IT4Framework* InContentEditorFramework
	);
	void OnSelectItemFromLaunch();
	void OnCloseWindow(); // #t4f-882

	//~ FEditorUndoClient interface
	void PostUndo(bool bSuccess) override;
	void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }

	// ~ FTickableEditorObject
	void Tick(float DeltaTime) override;
	bool IsTickable() const override { return true; }
	TStatId GetStatId() const override;

	/** IToolkit interface */
	void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	FName GetToolkitFName() const override;
	FText GetBaseToolkitName() const override;
	FText GetToolkitToolTipText() const override;
	FString GetWorldCentricTabPrefix() const override;
	FLinearColor GetWorldCentricTabColorScale() const override;
	FString GetDocumentationLink() const override
	{
		return TEXT("NotAvailable");
	}

	UT4UserInterfaceAsset* GetUserInterfaceAsset() const { return UserInterfaceAssetPtr.Get(); }
	UT4UserInterfaceAsset* GetEmptyUserInterfaceAsset();

	bool OnUserInterfaceSaveAsset(FString& OutErrorMessage);
	FT4OnClose& GetOnClose() { return OnClose; }

	// #453 Widget Preview
	void SelectedUserWidget(const FT4GameWidgetData* InWidgetData); // #1156 Test
	void SelectedDefaultUserWidget(const FT4GameWidgetData* InWidgetData); // #t4f-413 Default UI
	void SelectedUserWidget_UITypeName(FName InUITypeName, const ET4GameUITemplate& InSelectedGameUITemplate); // Override UI
	void SelectedUserWidget_DefaultUITypeName(FName InUITypeName); // Default UI

	// #1156 Etc
	FName GetGameUITypeName(const FT4GameWidgetData& InWidgetData); // #t4f-589
	FName GetSelectedUITypeName();
	ET4GameUITemplate GetSelectedGameUITemplate();
	void SetNewSelectedUITypeName(const FName& InSelectedUITypeName);
	void EditableGameWidgetData(OUT TArray<FT4GameWidgetData>& InWidgetData);
	const FT4GameWidgetData* FindEditableGameWidgetData(const FName& InGameUITypeName); // #t4f-589

	void HandleOnUINewSaveAs(FName InUIType);
	void HandleOnCustomUINew(const TArray<FName>& InName); // #t4f-589
	void HandleOnDefaultUINewSaveAs();
	void HandleOnExecuteSaveAsset();

	FName GetProjectID();
	IT4GameplayWidgetManager* GetWidgetManager() const;
	ET4LayerType GetContentEditorLayerType();

protected:
	const FName GetAppIdentifierName() const override { return FName(TEXT("T4UserInterfaceEditorApp")); } // #t4f-290
	const FName GetEditorStatusBarName() const override { return GetToolkitHost()->GetStatusBarName(); } // #t4f-290

	void Finailize() override;

	void NotifyOnExecuteSaveAsset() override;
	void NotifyOnExecuteCloseWindow() override;

	const FT4GameWidgetData* FindWidgetDataByUIOverrideAsset(
		const FName& InGameUITypeName,
		const ET4GameUITemplate& InGameUITemplate
	); // #t4f-589 오버라이드UI 정보에서 찾기
	const FT4GameWidgetData* FindCustomWidgetDataByUIOverrideAsset(const FName& InCustomUITypeName); // #t4f-589
	const FT4GameWidgetData* FindWidgetDataByUIDefaultAsset(const FName& InUITypeName);  // 기본UI 정보에서 FT4GameWidgetData찾기

	bool DeleteWidgetDataByUIType(
		const FName& InUITypeName,
		const ET4GameUITemplate& InSelectedGameUITemplate
	);
	void UIOverrideSaveAs(const FName& InGameUITypeName, bool InCustomUI = false); // #t4f-589 : 선택 UI 복사생성 부분을 분리

	FName GetNewCustomUIName(const FString InFilename, const FName& InGameUITypeName); //#t4f-589 : CustomUI Name을 중복되지 않게 생성해준다.

private:
	void CleanUp();
	void SetupCommands();

	void ExtendMenu();
	void ExtendToolbar();

	bool SetDataWidgets(const TArray<FT4GameWidgetData>& InWidgetDatas);	
	void SetProjectName(const FName& InProjectName);

	IT4GameplayWidgetManager* GetPreviewWidgetManager() const;
	IT4GameplayWidgetManager* GetPreviewWidgetManager_Default() const;
	IT4GameplayWidgetManager* GetContentWidgetManager() const;

	TSharedRef<SDockTab> SpawnTab_PreviewViewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_PreviewViewport_Default(const FSpawnTabArgs& Args); // #t4f-413
	//TSharedRef<SDockTab> SpawnTab_OverrideNew(const FSpawnTabArgs& Args); // #t4f-413 삭제예정
	TSharedRef<SDockTab> SpawnTab_OverrideList(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_OverrideNewList(const FSpawnTabArgs& Args); // #t4f-413
	TSharedRef<SDockTab> SpawnTab_UserInterfaceDetail(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_UIOptionDetail(const FSpawnTabArgs& Args); // #t4f-413

	// #t4f-413
	// Default UI Panel
	void HandleOnDefaultUITypeSelected(const FName InName);

	// Override UI Panel
	void HandleOnUITypeSelected(const FName InName);
	void HandleOnUITypeRemoveItem(const TArray<FName>& InNames);
	bool HandleOnUITypeVerifyRenameItem(const FName InRename, FString& OutErrorMessage); // #t4f-413 삭제예정
	void HandleOnUITypeRenameItem(const FName InOldName, const FName InNewName); // #t4f-413 삭제예정
	void HandleOnUITypeDuplicateItem(const FName InName); // #t4f-413 삭제예정

	// #1156
	void GameWidgetData_Add(UObject* InSavedUISources, const FT4GameWidgetData* InGameWidgetData);
	void GameWidgetData_Delete(
		const FName& InUITypeName,
		const ET4GameUITemplate& InSelectedGameUITemplate,
		bool InDeleteWidgetData = false
	);
	void Refresh();
	void UpdateWidgetManager_UIOverrideList();
	bool IsCustomUIName(const FName& InGameUITypeName); // #t4f-589 : CustomUIType이 같은게 있는지 확인
	ET4GameUITemplate GetUITemplateByName(const FString& InStringUITemplate); // #t4f-961

private:
	static const FName PreviewViewportTabId_Default; // #t4f-413
	static const FName PreviewViewportTabId;
	static const FName OverrideNewTabId; // #t4f-413 삭제 예정
	static const FName OverrideListTabId;
	static const FName UserInterfacePropertyId;
	static const FName UIOptionTabId; // #t4f-413
	static const FName AppIdentifier;

	bool bInitializing;
	FT4OnClose OnClose;

	UT4UserInterfaceAsset* EmptyUserInterfaceAssetOwner;
	TWeakObjectPtr<UT4UserInterfaceAsset> UserInterfaceAssetPtr;

	TSharedPtr<ST4EditorViewport> PreviewViewportPtr;
	TSharedPtr<ST4EditorViewport> PreviewViewportPtr_Default; // #t4f-413 : Default
	TSharedPtr<FT4PreviewEntityViewModel> PreviewViewModelPtr;
	TSharedPtr<FT4PreviewEntityViewModel> PreviewViewModelPtr_Default; // #t4f-413 : Default
	TSharedPtr<ST4UserInterfaceDetailWidget> UserInterfaceDetailPtr;
	TSharedPtr<ST4UIOptionDetailWidget> UIOptionDetailPtr; // #t4f-413 : UI Option Panel
	TSharedPtr<ST4UITypeTreeWidget> UITypeTreePtr; // #1156 : Override 선택 Widget
	TSharedPtr<ST4UITypeTreeWidget> UITypeDefaultTreePtr; // #t4f-413 : Default 선택 Widget
		
	// #453 Widget Preview
	TWeakObjectPtr<UT4UserWidgetBase> SelectedUserWidgetPtr; // #t4f-413 : Override
	TWeakObjectPtr<UT4UserWidgetBase> SelectedDefaultUserWidgetPtr; // #t4f-413 : Default
	TWeakObjectPtr<UT4UserWidgetBase> EditorPreviewBlockPtr;
	TWeakObjectPtr<UObject> UserWidgetOwner;

	FName SelectedUITypeName; // #1156
	FName SelectedDefaultUITypeName; // #t4f-413
	FName NewSelectedUITypeName; // #1156
	ET4GameUITemplate SelectedGameUITemplate; // #t4f-961

	// #t4f-270 : Framework를 GContentEditorPtr로 통일
	const IT4Framework* ContentEditorFramework;
};
