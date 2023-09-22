// Copyright VirtualFlow, Inc. All Rights Reserved.

#pragma once

#include "T4GameplayMinimal.h"
#include "T4BuiltinWidget.h"

#include "T4BuiltinDeckMainWidget.generated.h"


/**
  * #t4f-678
 */

class UScrollBox;
class UButton;
class UT4BuiltinButtonExt;
class UT4BuiltinButtonImage;
//class UT4BuiltinDeckSlotMain;
class UDataTable;
class UT4BuiltinDeckDetail;
class UT4BuiltinDeckInfo;
struct FT4ProjectServiceTableRow;
struct FT4DeckPackData;
struct FT4DeckSlotData;

UCLASS()
class T4GAMEPLAY_API UT4BuiltinDeckMainWidget : public UT4BuiltinWidget
{
	GENERATED_UCLASS_BODY()

public:
	void InitWidget() override;
	void ReleaseWidget() override;
	void OnStart() override;

	void WidgetInititalze();
	void SetValue();

	const FT4ProjectServiceTableRow* GetUserProjectServiceTableRow(UDataTable* InDataTable, FName InProjectID) const;
	const FT4ProjectServiceTableRow* GetUserProjectServiceTableRow(FName InProjectID) const;

public:
	UFUNCTION()
	void OnClickCloseDeckDetail();

	UFUNCTION()
	void OnClickDeckDetail(FName InProjectID);

	UFUNCTION()
	void OnClickedBack();

	UFUNCTION()
	void OnClickedProfile();

private:
	bool LoadDeckInfo();
	void CreateDeck();

	void TryCreateUserProjectServiceDataTable();

	void CreateDeckInfo(const FT4DeckPackData& InDeckPackData); // const FName& InTitleName, const TArray<FT4DeckSlotData>& InDeckSlotData);
	void SetVisibleDeckDetail(bool InVisible);

private:
	//UPROPERTY(Transient)
	//UT4BuiltinDeckSlotMain* DeckSlotMain;

	UPROPERTY(Transient)
	UScrollBox* ScrollBox_DeckInfo;

	UPROPERTY(Transient)
	UT4BuiltinDeckDetail* DeckDetail;

	UPROPERTY(Transient)
	UT4BuiltinButtonExt* Btn_Back;

	UPROPERTY(Transient)
	UT4BuiltinButtonImage* BtnImg_Profile;

	bool Initialize;
	TWeakObjectPtr<UDataTable> UsetProjectServiceTablePtr; // FT4ProjectServiceTableRow
};
