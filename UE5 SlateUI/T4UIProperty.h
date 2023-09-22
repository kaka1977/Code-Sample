// Copyright VirtualFlow, Inc. All Rights Reserved.

#pragma once

#include "T4Framework/Public/T4FrameworkMinimal.h"

#include "ViewModel/T4ContentViewModelTypes.h"

#include "IDetailCustomization.h"
#include "DetailCategoryBuilder.h"
//#include "T4GameData/Public/T4GameDataTypes.h"
#include "T4GameData/Classes/Content/T4UserInterfaceAsset.h" //#453

/**
 * #1156
 */
class FT4UserInterfaceEditor;
struct FT4UIPropertyOptions
{
	FT4UserInterfaceEditor* UserInterfaceEditor;
};

class IDetailsView;
class IPropertyHandle;
class IStructureDetailsView;
class FT4UIProperty : public FEditorUndoClient, public IDetailCustomization
{
public:
	FT4UIProperty(const FT4UIPropertyOptions& InOption);
	~FT4UIProperty();

	//~ FEditorUndoClient interface
	void PostUndo(bool bSuccess) override; // #125
	void PostRedo(bool bSuccess) override { PostUndo(bSuccess); } // #125
	//~ FEditorUndoClient interface

	void CustomizeDetails(IDetailLayoutBuilder& InBuilder);

	void OnRefresh();
	FReply HandleOnSave();

protected:

	bool CustomizePropertyOverride(
		IDetailLayoutBuilder& InBuilder,
		IDetailCategoryBuilder& InDetailCategoryBuilder,
		TSharedRef<IPropertyHandle> InPropertyHandle
	);

	void OnSelectedWidget(FName InWidgetSoftpath); // #453

private:

	FT4UserInterfaceEditor* UserInterfaceEditor;
	IDetailLayoutBuilder* DetailLayoutRef;
};
