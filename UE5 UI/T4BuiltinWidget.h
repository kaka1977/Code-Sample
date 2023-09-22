// Copyright VirtualFlow, Inc. All Rights Reserved.

#pragma once

#include "T4GameplayMinimal.h"
#include "T4Framework/Classes/UserWidget/T4UserWidgetBase.h"
#include "T4GameData/Public/T4GameDataTypes.h"
#include "T4Framework/Public/T4Framework.h" // #668

#include "T4Gameplay/Public/UIAction/T4GameplayUIAction.h" // #1235
#include "T4Gameplay/Public/UIAction/T4GameplayUIActionDefine.h"
#include "Blueprint/DragDropOperation.h"

#include "T4BuiltinWidget.generated.h"


/**
  * #276
 */

UCLASS()
class T4GAMEPLAY_API UDragOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D DragOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) // #t4f-173 : 이전 위치 좌표
	FVector2D AbsoluteCoordinate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) // #t4f-201 : 마우스 클릭 스크린 좌표
	FVector2D DragOffset_ScreenSpace;
};

class IT4GameplayWidgetManager; // #570
class IT4GameplayUIActionManager;
class UT4GameplayUIAction;

UCLASS()
class T4GAMEPLAY_API UT4BuiltinWidget : public UT4UserWidgetBase
{
	GENERATED_UCLASS_BODY()

public:
	void InitWidget() override;
	void ReleaseWidget() override;

	// #t4f-235
	virtual void OnOpenInit() override;				// 위젯을 열때 초기화
	virtual void OnCloseDeInit() override;			// 위젯을 닫을때 초기화

	void SetIsShowOnStart(bool InIsShowOnStart) {//#289
		bIsShowOnStart = InIsShowOnStart;
	}
	bool IsShowOnStart() {//#289
		return bIsShowOnStart;
	}

	virtual void OnStart();//#289
	virtual void SetVisibility(ESlateVisibility InVisibility) override;//#289

	UT4BuiltinWidget* GetUIWidgetByName(FName InWidgetName);
	UT4BuiltinWidget* GetUIWidgetByNameByMng(ET4GameUIType InUIType); // #668

	void ShowMessageBox(const FString& InMessage); // #776
	void ShowBlockScreen(bool InShow, const FString& InMessage); // #t4f-732

protected:
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry, 
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override; // #t4f-201 : 이동 가능위젯과 비이동위젯이 겹칠때 처리

	bool IsShowWidget();
	virtual void ShowWidget();
	virtual void HideWidget();

	//{#306
	
	// BuiltinWidget 을 상속한 모든 보이는 위젯을 Hide 시킨다
	void HideWidgetsOnScreen();

	// Hide 시켰던 위젯을 다시 Show 한다
	void UnhideWidgetsOnScreen();
	
	// VirtualJoystick
	void HideJoystick();
	void UnhideJoystick();
	//}
	
	// #t4f-1028 
	void HideItemDescription();

	IT4GameplayWidgetManager*			GetWidgetManager() const; // #570
	IT4GameplayUIActionManager*			GetUIActionManager() const; // #1235
	UT4GameplayUIAction*				GetUIAction(const ET4UIActionType& InUIActionType); // #1235

	class UT4UserWidgetBase*          GetBuiltinWidget(ET4GameUIType InUIType);
	class UT4UserWidgetBase*          GetBuiltinWidgetByMng(ET4GameUIType InUIType); // #668
	class UT4BuiltinGameInstance*     GetBuiltinGameInstance()     const;
	class AT4BuiltinPlayerController* GetBuiltinPlayerController() const;
	class UT4PlayerClientObject*      GetPlayerClientObject()      const;

protected:
	uint8 bIsShowOnStart : 1;//#289

	UPROPERTY()
	TArray<UT4UserWidgetBase*> HideWidgetsArray;//#306

	// #t4f-235 : Widget의 연출 기능 Open/Close
	UPROPERTY(Transient)
	UWidgetAnimation* PlayOpen;

	UPROPERTY(Transient)
	UWidgetAnimation* PlayClose;

	bool Clicked_RightMouseButton; // #t4f-173
};
