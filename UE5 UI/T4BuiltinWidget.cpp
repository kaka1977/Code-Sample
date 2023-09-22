// Copyright VirtualFlow, Inc. All Rights Reserved.

#include "UserWidget/T4BuiltinWidget.h"
#include "T4BuiltinWidgetUtils.h"

#include "T4GameplayWidgetManager.h"
#include "T4GameplayUIActionManager.h"
#include "T4GameplayUIActionManager.h"

#include "T4BuiltinGameInstance.h"
#include "Player/T4BuiltinPlayerController.h"
#include "Game/Client/ClientObject/Player/T4PlayerClientObject.h"

#include "UserWidget/Common/T4Builtin_Common_Message.h"

#include "Animation/WidgetAnimation.h"

#include "UserWidget/T4BuiltinInventoryWidget.h"
#include "UserWidget/T4BuiltinInventoryLayoutWidget.h" // #t4f-201

#include "T4GameplayInternal.h"

/**
  * #276
 */
UT4BuiltinWidget::UT4BuiltinWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UT4BuiltinWidget::InitWidget()
{
	Super::InitWidget();

	PlayOpen = GetAnimation("Widget_Open");
	PlayClose = GetAnimation("Widget_Close");
}

void UT4BuiltinWidget::ReleaseWidget()
{
	Super::ReleaseWidget();
}

void UT4BuiltinWidget::OnOpenInit()
{
	Super::OnOpenInit();

	//#t4f-235
	if (nullptr != PlayOpen)
	{
		PlayAnimation(PlayOpen, PlayOpen->GetStartTime(), 1, EUMGSequencePlayMode::Forward);
	}
}

void UT4BuiltinWidget::OnCloseDeInit()
{
	Super::OnCloseDeInit();

	//#t4f-235
	if (nullptr != PlayClose)
	{
		PlayAnimation(PlayClose, PlayClose->GetStartTime(), 1, EUMGSequencePlayMode::Forward);
	}
}

void UT4BuiltinWidget::OnStart()//#289
{
	Super::OnStart();

	if (bIsShowOnStart)
	{
		ShowWidget();
	}

	Clicked_RightMouseButton = false; // #t4f-235
}

void UT4BuiltinWidget::SetVisibility(ESlateVisibility InVisibility)//#289
{
	ESlateVisibility NewVisibility = InVisibility;

	// ZOrder 정리, DefaultT4FrameworkCore.ini 참조
	// ZOrder 로 정리가 안된다고 하면 Modal 옵션을 추가하거나, Widget 에서 컨트롤을 추가해서 Modal 처럼 사용할 수 있도록 한다.
	
	// #306
	// 기본적으로는 위젯을 보이기 위해서 Visible 을 쓰고 있다. 필요한 부분에서만 Visible 이 들어갈 수 있도록 Override 해준다.
	// UT4UserBaseWidget::SetVisibility(InVisibility);

	if (NewVisibility == ESlateVisibility::Visible)
	{
		NewVisibility = ESlateVisibility::SelfHitTestInvisible;
	}

	Super::SetVisibility(NewVisibility);
}

// #t4f-201 : 이동 가능위젯과 비이동위젯이 겹칠때 처리
bool UT4BuiltinWidget::NativeOnDrop(
	const FGeometry& InGeometry, 
	const FDragDropEvent& InDragDropEvent, 
	UDragDropOperation* InOperation
)
{
	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

	UDragOperation* DragOperationResult = Cast<UDragOperation>(InOperation);

	if (!IsValid(DragOperationResult))
	{
		T4_LOG(Verbose, TEXT("DragOperation Item nullptr"))
			return false;
	}

	T4BuiltinWidgetUtils::OnDrop_WidgetRevert(LayerType, InOperation->Payload);

	return true;
}

bool UT4BuiltinWidget::IsShowWidget()
{
	if (AT4BuiltinPlayerController* PlayerController = GetBuiltinPlayerController())
	{
		return PlayerController->IsShownUserWidget(GetWidgetName());
	}
	return false;
}

void UT4BuiltinWidget::ShowWidget()
{
	if (AT4BuiltinPlayerController* PlayerController = GetBuiltinPlayerController())
	{
		PlayerController->ShowUserWidget(GetWidgetName(), true, false);
	}
}

void UT4BuiltinWidget::HideWidget()
{
	if (AT4BuiltinPlayerController* PlayerController = GetBuiltinPlayerController())
	{
		PlayerController->ShowUserWidget(GetWidgetName(), false, false);
	}
}

void UT4BuiltinWidget::HideWidgetsOnScreen()//#306
{
	if (AT4BuiltinPlayerController* PlayerController = GetBuiltinPlayerController())
	{
		PlayerController->HideWidgetsOnScreen(this);
	}
}

void UT4BuiltinWidget::UnhideWidgetsOnScreen()//#306
{
	if (AT4BuiltinPlayerController* PlayerController = GetBuiltinPlayerController())
	{
		PlayerController->UnhideWidgetsOnScreen();
	}
}

void UT4BuiltinWidget::HideJoystick()//#306
{
	if (AT4BuiltinPlayerController* PlayerController = GetBuiltinPlayerController())
	{
		PlayerController->SetVirtualJoystickVisibility(false);
	}
}

void UT4BuiltinWidget::UnhideJoystick()//#306
{
	if (AT4BuiltinPlayerController* PlayerController = GetBuiltinPlayerController())
	{
		PlayerController->SetVirtualJoystickVisibility(true);
	}
}

class UT4UserWidgetBase* UT4BuiltinWidget::GetBuiltinWidget(ET4GameUIType InUIType)
{
	if (AT4BuiltinPlayerController* PlayerController = GetBuiltinPlayerController())
	{
		return PlayerController->GetBuiltinWidgetByMng(InUIType);
	}

	return nullptr;
}

IT4GameplayWidgetManager* UT4BuiltinWidget::GetWidgetManager() const // #570
{
	check(ET4LayerType::Max > LayerType);
	return GetGameplayWidgetManagerByLayer(LayerType);
}

IT4GameplayUIActionManager* UT4BuiltinWidget::GetUIActionManager() const // #1235
{
	check(ET4LayerType::Max > LayerType);
	return GetGameplayUIActionManagerLayer(LayerType);
}

UT4GameplayUIAction* UT4BuiltinWidget::GetUIAction(const ET4UIActionType& InUIActionType) // #1235
{
	IT4GameplayUIActionManager* IUIActionManager = GetUIActionManager();
	if (nullptr != IUIActionManager)
	{
		return IUIActionManager->GetUIAction(InUIActionType, LayerType);
	}

	return nullptr;
}

// #668
class UT4UserWidgetBase* UT4BuiltinWidget::GetBuiltinWidgetByMng(ET4GameUIType InUIType)
{
	IT4GameplayWidgetManager* WidgetManager = GetGameplayWidgetManagerByLayer(LayerType);
	if (nullptr != WidgetManager)
	{
		UT4UserWidgetBase* GetUserWidget = WidgetManager->GetWidgetBase(InUIType, LayerType);
		if (GetUserWidget != nullptr)
		{
			//T4_LOG(Verbose, TEXT("Test UT4BuiltinWidget::GetBuiltinWidgetByMng..."));
			return GetUserWidget;
		}
	}

	return nullptr;
}

class UT4BuiltinGameInstance* UT4BuiltinWidget::GetBuiltinGameInstance() const
{
	if (AT4BuiltinPlayerController* PlayerController = GetBuiltinPlayerController())
	{
		return Cast<UT4BuiltinGameInstance>(PlayerController->GetGameInstance()); // WARN: Widget::GetGameInstance사용 금지!! (에디터 시뮬 오류)
	}
	return nullptr;
}

class AT4BuiltinPlayerController* UT4BuiltinWidget::GetBuiltinPlayerController() const
{
	IT4Framework* Framework = T4Framework::GetFramework(LayerType); // WARN: Widget::GetGameInstance사용 금지!! (에디터 시뮬 오류)
	if (nullptr != Framework)
	{
		return Cast<AT4BuiltinPlayerController>(Framework->GetPlayerController());
	}

	return nullptr;
}

class UT4PlayerClientObject* UT4BuiltinWidget::GetPlayerClientObject() const
{
	if (AT4BuiltinPlayerController* Controller = GetBuiltinPlayerController())
	{
		return Cast<UT4PlayerClientObject>(Controller->GetOwnedObjectBase());
	}
	return nullptr;
}

// #594 다른 위젯에 접근하기 위한 함수
UT4BuiltinWidget* UT4BuiltinWidget::GetUIWidgetByName(FName InWidgetName)
{
	AT4BuiltinPlayerController* PlayerController = GetBuiltinPlayerController();
	if (nullptr != PlayerController && PlayerController->HasPlayerActor())
	{
		return PlayerController->GetBuiltinWidgetByMng(InWidgetName);
	}

	return nullptr;
}

// #668
UT4BuiltinWidget* UT4BuiltinWidget::GetUIWidgetByNameByMng(ET4GameUIType InUIType)
{
	UT4UserWidgetBase* UserWidgetBase = GetBuiltinWidgetByMng(InUIType);
	if (nullptr != UserWidgetBase)
	{
		return Cast<UT4BuiltinWidget>(UserWidgetBase);
	}

	return nullptr;
}


void UT4BuiltinWidget::ShowMessageBox(const FString& InMessage) // #776
{
	T4BuiltinWidgetUtils::ShowMessageBox(LayerType, InMessage); // #1055
}

void UT4BuiltinWidget::ShowBlockScreen(bool InShow, const FString& InMessage) // #t4f-732
{
	T4BuiltinWidgetUtils::ShowBlockScreen(LayerType, InShow, InMessage);
}

void UT4BuiltinWidget::HideItemDescription() // #t4f-1028 
{
	UT4UserWidgetBase* pWidget = GetBuiltinWidgetByMng(ET4GameUIType::InventoryLayout);
	if (nullptr != pWidget)
	{
		UT4BuiltinInventoryLayoutWidget* InventoryWidget = static_cast<UT4BuiltinInventoryLayoutWidget*>(pWidget);
		if (nullptr != InventoryWidget)
		{
			InventoryWidget->CloseWidget_PopUp();
		}
	}
}