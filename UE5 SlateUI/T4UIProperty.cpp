// Copyright VirtualFlow, Inc. All Rights Reserved.


#include "SubEditor/UIEditor/T4UIProperty.h"
#include "T4UserInterfaceEditor.h"

#include "Modules/ModuleManager.h"

#include "T4GameData/Classes/Content/T4UserInterfaceAsset.h"
#include "T4EditorCommon/Public/DetailView/T4DetailCustomizationMacros.h"
#include "T4EditorCommon/Public/T4EditorCommonMacros.h"
#include "T4EditorCommon/Public/T4EditorCommonConstants.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailPropertyRow.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "IStructureDetailsView.h"
#include "T4Gameplay/Public/UI/T4GameplayUI.h" // #453
//#include "Widgets/ST4WidgetDataDropListWidget.h" // #453

#include "T4ContentEditorInternal.h" // #453

#define LOCTEXT_NAMESPACE "T4UIProperty"

static const FText T4UIPropertyErrorTitleText = LOCTEXT("T4UIPropertyError", "UIProperty Error");

FT4UIProperty::FT4UIProperty(
	const FT4UIPropertyOptions& InOption
) : UserInterfaceEditor(InOption.UserInterfaceEditor)
  , DetailLayoutRef(nullptr)
{
	GEditor->RegisterForUndo(this);
}

FT4UIProperty::~FT4UIProperty()
{
	GEditor->UnregisterForUndo(this);
}

void FT4UIProperty::PostUndo(bool bSuccess)
{
}

void FT4UIProperty::CustomizeDetails(IDetailLayoutBuilder& InBuilder)
{
	if (nullptr == UserInterfaceEditor)
		return;

	DetailLayoutRef = &InBuilder;

	UT4UserInterfaceAsset* UserInterfaceAsset = UserInterfaceAsset = UserInterfaceEditor->GetUserInterfaceAsset();
	if (nullptr == UserInterfaceAsset)
	{
		UserInterfaceAsset = UserInterfaceEditor->GetEmptyUserInterfaceAsset();
	}

	DETAIL_HIDE_CLASS_PROPERTY(UT4UserInterfaceAsset, WidgetType);

	{
		// #t4f-589 Save 버튼 추가
		FString SaveUIOverrideString;
		if (!T4_EDITOR_GET_MESSAGE_THIS(TEXT("SaveUIOverrideAsset"), SaveUIOverrideString))
		{
			SaveUIOverrideString = TEXT("Save UIOverride Asset");
		}

		TSharedRef<SWidget> NewUserInterfaceHeaderWidget = // #t4f-589 Save 버튼 추가
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(1.0f, 0.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(SaveUIOverrideString))
					DEFINE_BUTTON_SAVE_STYLE_MACRO()
					.OnClicked(this, &FT4UIProperty::HandleOnSave)
				]
			];

		// #1156
		static const FName UIPropertyCategorySortNames[] = {
		TEXT("UserWidgetSettings"),
		TEXT("ProjectUIOption"), // #t4f-413
		};

		for (const FName CategoryName : UIPropertyCategorySortNames)
		{
			IDetailCategoryBuilder& DetailCategoryBuilder = InBuilder.EditCategory(CategoryName);
			DetailCategoryBuilder.HeaderContent(NewUserInterfaceHeaderWidget);

			TArray<TSharedRef<IPropertyHandle>> CommonProperties;
			DetailCategoryBuilder.GetDefaultProperties(CommonProperties);
			for (TSharedRef<IPropertyHandle> PropertyHandle : CommonProperties)
			{
				InBuilder.HideProperty(PropertyHandle);
				if (CategoryName != TEXT("Hide") &&
					!CustomizePropertyOverride(InBuilder, DetailCategoryBuilder, PropertyHandle))
				{
					//DetailCategoryBuilder.AddProperty(PropertyHandle); // #1156 선택이 없을 경우 빈공간으로 놔두기
				}
			}
		}
	}
}

bool FT4UIProperty::CustomizePropertyOverride(
	IDetailLayoutBuilder& InBuilder,
	IDetailCategoryBuilder& InDetailCategoryBuilder,
	TSharedRef<IPropertyHandle> InPropertyHandle
)
{
	UT4UserInterfaceAsset* UserInterfaceAsset = UserInterfaceAsset = UserInterfaceEditor->GetUserInterfaceAsset();
	if (nullptr == UserInterfaceAsset)
	{
		UserInterfaceAsset = UserInterfaceEditor->GetEmptyUserInterfaceAsset();
	}

	bool bResult = false;

	FName WidgetTypeName_Selected = UserInterfaceEditor->GetSelectedUITypeName();
	if (WidgetTypeName_Selected == NAME_None)
		return bResult;
	// #t4f-969
	ET4GameUITemplate GameUITemplate_Selected = UserInterfaceEditor->GetSelectedGameUITemplate();

	// #453 리뉴얼~~
	const FString PropertyName = InPropertyHandle->GetMetaDataProperty()->GetName();
	if (DETAIL_COMPARE_PROPERTY(PropertyName, OverrideUserWidgetSettings))
	{
		uint32 NumChildren = 0;
		FPropertyAccess::Result Result = InPropertyHandle->GetNumChildren(NumChildren);
		if (FPropertyAccess::Result::Success == Result)
		{
			for (uint32 idx = 0; idx < NumChildren; ++idx)
			{
				// #453 ET4GameUIType 하나에 N개가 존재하므로(오버라이드), 하나에 넣기 위함
				TArray<FName> arrGameWidgetData;
				TArray<FName> arrCustomTypeName;
				TSharedPtr<IPropertyHandle> ChildHandlePtr = InPropertyHandle->GetChildHandle(idx);

				uint32 NumSubChildren = 0;
				if (FPropertyAccess::Result::Success == ChildHandlePtr->GetNumChildren(NumSubChildren))
				{
					for (uint32 idxsub = 0; idxsub < NumSubChildren; ++idxsub)
					{
						TSharedPtr<IPropertyHandle> SubChildHandlePtr = ChildHandlePtr->GetChildHandle(idxsub);
						const FName SubChildPropertyName = ChildHandlePtr->GetMetaDataProperty()->GetFName();

						void* Data;
						FPropertyAccess::Result PropertyResult = SubChildHandlePtr->GetValueData(Data);
						if (FPropertyAccess::Result::Success != PropertyResult)
						{
							continue;
						}

						FT4GameWidgetData WidgetData = *(FT4GameWidgetData*)Data;
						{
							FName WidgetTypeName = UserInterfaceEditor->GetGameUITypeName(WidgetData); // Categoty처럼 사용할 예정
							// #t4f-969 : Template 비교도 함께 한다.
							if (WidgetTypeName_Selected == WidgetTypeName &&
								GameUITemplate_Selected == WidgetData.UITemplate)
							{
								arrGameWidgetData.Add(WidgetTypeName);
							}
							else
							{
								continue;
							}
						}

						uint32 NumPropertyChildren = 0;
						FPropertyAccess::Result _Result = SubChildHandlePtr->GetNumChildren(NumPropertyChildren);
						if (FPropertyAccess::Result::Success == _Result)
						{
							// WidgetData에서 Key-Type/Value-각 항목으로 분리를 해내자.
							for (uint32 idxprop = 0; idxprop < NumPropertyChildren; ++idxprop)
							{
								TSharedPtr<IPropertyHandle> PropertyChildHandlePtr = SubChildHandlePtr->GetChildHandle(idxprop);
								const FString ChildPropertyName = PropertyChildHandlePtr->GetMetaDataProperty()->GetName();


								if (DETAIL_COMPARE_PROPERTY(ChildPropertyName, Type)) // Type 표시는 필요해 보인다.
								{
									InDetailCategoryBuilder
										.AddCustomRow(LOCTEXT("T4UIType", "UIType"))
										.NameContent()
										[
											SNew(STextBlock)
											//.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
											.Font(IDetailLayoutBuilder::GetDetailFont())
											.Text(FText::Format(LOCTEXT("Type", "{0}"), FText::FromString(ChildPropertyName)))
										]
										.ValueContent()
										[
											PropertyChildHandlePtr->CreatePropertyValueWidget()
										];

									bResult = true;
								}
								else if (DETAIL_COMPARE_PROPERTY(ChildPropertyName, CustomUITypeData)) // #t4f-589
								{
									uint32 NumCustomPropertyChildren = 0;
									FPropertyAccess::Result _Result = PropertyChildHandlePtr->GetNumChildren(NumCustomPropertyChildren);
									if (FPropertyAccess::Result::Success == _Result)
									{
										// WidgetData에서 Key-Type/Value-각 항목으로 분리를 해내자.
										for (uint32 idxprop2 = 0; idxprop2 < NumCustomPropertyChildren; ++idxprop2)
										{
											TSharedPtr<IPropertyHandle> CustomPropertyChildHandlePtr = PropertyChildHandlePtr->GetChildHandle(idxprop2);
											const FString CustomChildPropertyName = CustomPropertyChildHandlePtr->GetMetaDataProperty()->GetName();

											if (DETAIL_COMPARE_PROPERTY(CustomChildPropertyName, CustomTypeName))
											{
												InDetailCategoryBuilder
													.AddCustomRow(LOCTEXT("T4CustomTypeName", "CustomUITypeName"))
													.NameContent()
													[
														SNew(STextBlock)
														.Font(IDetailLayoutBuilder::GetDetailFont())
														.Text(FText::Format(LOCTEXT("CustomTypeName", "{0}"), FText::FromString(CustomChildPropertyName)))
													]
													.ValueContent()
													[
														CustomPropertyChildHandlePtr->CreatePropertyValueWidget()
													];
											}
											else if (DETAIL_COMPARE_PROPERTY(CustomChildPropertyName, ParentUIType))
											{
												InDetailCategoryBuilder
													.AddCustomRow(LOCTEXT("T4ParentUIType", "UIParentType"))
													.NameContent()
													[
														SNew(STextBlock)
														.Font(IDetailLayoutBuilder::GetDetailFont())
														.Text(FText::Format(LOCTEXT("ParentUIType", "{0}"), FText::FromString(CustomChildPropertyName)))
													]
													.ValueContent()
													[
														CustomPropertyChildHandlePtr->CreatePropertyValueWidget()
													];
											}
										}
									}
									bResult = true;
								}
								else if (DETAIL_COMPARE_PROPERTY(ChildPropertyName, WidgetClass))
								{
									bResult = false;
								}
								/*else if (DETAIL_COMPARE_PROPERTY(ChildPropertyName, WidgetClass))
								{
									TSharedPtr<ST4WidgetDataDropListWidget> NewWidgetDataDropListWidgetPtr = SNew(ST4WidgetDataDropListWidget, WidgetData.Type)
										.PropertyHandle(PropertyChildHandlePtr)
										.OnSelected(this, &FT4UIProperty::OnSelectedWidget)
										.OnComboOpening(this, &FT4UIProperty::OnSelectedWidget); // #453
									NewWidgetDataDropListWidgetPtr->SetNoNameDescription(TEXT("Not Selected"));
									NewWidgetDataDropListWidgetPtr->OnRefresh();

									InDetailCategoryBuilder
										.AddCustomRow(LOCTEXT("T4UIWidgetClass", "WidgetClass"))
										.NameContent()
										[
											SNew(STextBlock)
											.Font(IDetailLayoutBuilder::GetDetailFont())
											.Text(FText::Format(LOCTEXT("WidgetClass", "{0}"), FText::FromString(ChildPropertyName)))
										]
										.ValueContent()
										.HAlign(HAlign_Fill)
										[
											NewWidgetDataDropListWidgetPtr.ToSharedRef()
										];
								}*/
								else if (DETAIL_COMPARE_PROPERTY(ChildPropertyName, WidgetPath))
								{
									InDetailCategoryBuilder
										.AddCustomRow(LOCTEXT("T4WidgetPath", "WidgetPath"))
										.NameContent()
										[
											SNew(STextBlock)
											.Font(IDetailLayoutBuilder::GetDetailFont())
										.Text(FText::Format(LOCTEXT("WidgetPath", "{0}"), FText::FromString(ChildPropertyName)))
										]
									.ValueContent()
										[
											PropertyChildHandlePtr->CreatePropertyValueWidget()
										];

									bResult = true;
								}
								else if (DETAIL_COMPARE_PROPERTY(ChildPropertyName, ZOrder))
								{
									InDetailCategoryBuilder
										.AddCustomRow(LOCTEXT("T4ZOrder", "ZOrder"))
										.NameContent()
										[
											SNew(STextBlock)
											.Font(IDetailLayoutBuilder::GetDetailFont())
											.Text(FText::Format(LOCTEXT("ZOrder", "{0}"), FText::FromString(ChildPropertyName)))
										]
									.ValueContent()
										[
											PropertyChildHandlePtr->CreatePropertyValueWidget()
										];

									bResult = true;
								}
								else if (DETAIL_COMPARE_PROPERTY(ChildPropertyName, bRegisterOnPreview))
								{
									InDetailCategoryBuilder
										.AddCustomRow(LOCTEXT("T4bRegisterOnPreview", "bRegisterOnPreview"))
										.NameContent()
										[
											SNew(STextBlock)
											.Font(IDetailLayoutBuilder::GetDetailFont())
											.Text(FText::Format(LOCTEXT("bRegisterOnPreview", "{0}"), FText::FromString(ChildPropertyName)))
										]
										.ValueContent()
										[
											PropertyChildHandlePtr->CreatePropertyValueWidget()
										];

									bResult = true;
								}
								else if (DETAIL_COMPARE_PROPERTY(ChildPropertyName, bIsShowOnStart))
								{
									InDetailCategoryBuilder
										.AddCustomRow(LOCTEXT("T4bIsShowOnStart", "bIsShowOnStart"))
										.NameContent()
										[
											SNew(STextBlock)
											.Font(IDetailLayoutBuilder::GetDetailFont())
											.Text(FText::Format(LOCTEXT("bIsShowOnStart", "{0}"), FText::FromString(ChildPropertyName)))
										]
										.ValueContent()
										[
											PropertyChildHandlePtr->CreatePropertyValueWidget()
										];

									bResult = true;
								}
								else if (DETAIL_COMPARE_PROPERTY(ChildPropertyName, bIsEditorEditable)) // #t4f-879
								{
									InDetailCategoryBuilder
										.AddCustomRow(LOCTEXT("T4bIsEditorEditable", "bIsEditorEditable"))
										.NameContent()
										[
											SNew(STextBlock)
											.Font(IDetailLayoutBuilder::GetDetailFont())
											.Text(FText::Format(LOCTEXT("bIsEditorEditable", "{0}"), FText::FromString(ChildPropertyName)))
										]
										.ValueContent()
										[
											PropertyChildHandlePtr->CreatePropertyValueWidget()
										];

									bResult = true;
								}
								else if (DETAIL_COMPARE_PROPERTY(ChildPropertyName, bIsCustomEditable)) // #t4f-882
								{
									InDetailCategoryBuilder
										.AddCustomRow(LOCTEXT("T4bIsCustomEditable", "bIsCustomEditable"))
										.NameContent()
										[
											SNew(STextBlock)
											.Font(IDetailLayoutBuilder::GetDetailFont())
											.Text(FText::Format(LOCTEXT("bIsCustomEditable", "{0}"), FText::FromString(ChildPropertyName)))
										]
										.ValueContent()
										[
											PropertyChildHandlePtr->CreatePropertyValueWidget()
										];

									bResult = true;
								}
								//else if (DETAIL_COMPARE_PROPERTY(ChildPropertyName, bIsHideSkip)) // #t4f-1062
								//{
								//	InDetailCategoryBuilder
								//		.AddCustomRow(LOCTEXT("T4bIsHideSkip", "bIsHideSkip"))
								//		.NameContent()
								//		[
								//			SNew(STextBlock)
								//			.Font(IDetailLayoutBuilder::GetDetailFont())
								//			.Text(FText::Format(LOCTEXT("bIsHideSkip", "{0}"), FText::FromString(ChildPropertyName)))
								//		]
								//		.ValueContent()
								//		[
								//			PropertyChildHandlePtr->CreatePropertyValueWidget()
								//		];

								//	bResult = true;
								//}
								else
								{
									bResult = false;
								}

							}
						}
					}
				}
			}
		}
	}


	return bResult;
}

void FT4UIProperty::OnRefresh()
{
	if (nullptr == UserInterfaceEditor)
		return;

	if (nullptr != DetailLayoutRef)
	{
		DetailLayoutRef->ForceRefreshDetails();
	}
}

void FT4UIProperty::OnSelectedWidget(FName InWidgetSoftpath) // #453
{
	return; // #1156 사용하지 않을 예정
	//T4_LOG(Warning, TEXT("FT4UserInterfaceDetails::OnSelectedWidget - %s"), *InWidgetSoftpath.ToString());
	/*if (nullptr == UserInterfaceEditor)
		return;

	UT4UserInterfaceAsset* UserInterfaceAsset = UserInterfaceAsset = UserInterfaceEditor->GetUserInterfaceAsset();
	if (nullptr != UserInterfaceAsset)
	{
		UserInterfaceEditor->SelectedUserWidget(InWidgetSoftpath); // Test
	}*/
}

FReply FT4UIProperty::HandleOnSave()
{
	if (nullptr != UserInterfaceEditor)
	{
		UserInterfaceEditor->HandleOnExecuteSaveAsset();
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE