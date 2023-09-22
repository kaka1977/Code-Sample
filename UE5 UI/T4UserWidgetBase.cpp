// Copyright VirtualFlow, Inc. All Rights Reserved.

#include "UserWidget/T4UserWidgetBase.h"

#include "T4Framework.h"
#if WITH_EDITOR
#include "Editor/T4FrameworkEditor.h"
#endif

#if USES_SUPPORT_UE5_1 // #t4f-511
#include "Blueprint/GameViewportSubsystem.h"
#endif

#include "Animation/WidgetAnimation.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Slate/SGameLayerManager.h"
#include "Components/Button.h"//#276

#include "T4FrameworkInternal.h"

#define LOCTEXT_NAMESPACE "T4UserWidgetBase"

/**
  * #164
 */
UT4UserWidgetBase::UT4UserWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LayerType(ET4LayerType::Max)
	, bAddToViewport(false)
	, SortZOrder(0)
	, WidgetName(NAME_None)//#289
	, UserData(NAME_None) // #542
{
}

void UT4UserWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
}

void UT4UserWidgetBase::NativeDestruct()
{
	Super::NativeDestruct();
}

void UT4UserWidgetBase::OnOpenInit()
{
}

void UT4UserWidgetBase::OnCloseDeInit()
{
}

void UT4UserWidgetBase::OnChildOpenInit()
{
	FString strWidgetName = TEXT("None");
	UClass* pClass = GetClass();
	if (pClass != nullptr)
	{
		strWidgetName = pClass->GetFName().ToString();
	}

	int32 iEnd = arrChildWidget.Num();
	for (int32 i = 0; i < iEnd; ++i)
	{
		if (arrChildWidget[i].IsValid())
		{
			arrChildWidget[i]->OnOpenInit();
			arrChildWidget[i]->OnChildOpenInit();
		}
		else
		{
			arrChildWidget.RemoveAt(i);
			--i;
			--iEnd;
		}
	}
}

void UT4UserWidgetBase::OnChildCloseDeInit()
{
	int32 iEnd = arrChildWidget.Num();
	for (int32 i = 0; i < iEnd; ++i)
	{
		if (arrChildWidget[i].IsValid())
		{
			arrChildWidget[i]->OnCloseDeInit();
			arrChildWidget[i]->OnChildCloseDeInit();
		}
		else
		{
			arrChildWidget.RemoveAt(i);
			--i;
			--iEnd;
		}
	}
}

void UT4UserWidgetBase::ToggleWidget(bool InVisible) // #t4f-335
{
	// #t4f-896 : Show/Hide 처리를 Widget에 맞긴다.
	if (!InVisible && IsShown())
	{
		Show(false);
		SetVisibility(ESlateVisibility::Hidden);
	}
	else if (InVisible && !IsShown())
	{
		Show(true);
		SetVisibility(ESlateVisibility::Visible);
	}
}

void UT4UserWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)//#276
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	//#276
	for (int32 Index = 0; Index < DelayVisibilityDataArray.Num(); )
	{
		FT4DelayVisibilityData Data = DelayVisibilityDataArray[Index];
		if (Data.Delay > 0.0f)
		{
			Data.Delay -= InDeltaTime;
			if (Data.Delay <= 0.0f)
			{
				Data.Delay = 0.0f;

				Data.Widget->SetVisibility(Data.Visibility);
				Data.Widget = nullptr;

				DelayVisibilityDataArray.RemoveAt(Index);
			}
			else
			{
				DelayVisibilityDataArray[Index] = Data;
				++Index;
			}
		}
	}
}

bool UT4UserWidgetBase::Initialize()
{
	bool bResult = Super::Initialize();
	if (bResult)
	{
		// #164
		// UMG Editor 에서 불릴 수 있는데 World 가 nullptr 인 경우가 있을 수 있다.
		UWorld* UnrealWorld = GetWorld();
		if (nullptr != UnrealWorld)
		{
			check(ET4LayerType::Max == LayerType);
			LayerType = FT4EngineLayer::Get().Convert(GetWorld()); // #12 : Support Multiple LayerType
#if !WITH_EDITOR
			check(ET4LayerType::Max != LayerType); // UMG Editor 에서 컴파일 후 저장시 World Type 이 None 인 경우가 있어 에디터는 제외!
#endif
		}
	}
	return bResult;
}

void UT4UserWidgetBase::BeginDestroy()
{
	Super::BeginDestroy();
}
// #536
void UT4UserWidgetBase::InitWidget()
{
	int32 iEnd = arrChildWidget.Num();
	for (int32 i = 0; i < iEnd; ++i)
	{
		if (arrChildWidget[i].IsValid())
		{
			arrChildWidget[i]->InitWidget();
		}
		else
		{
			arrChildWidget.RemoveAt(i);
			--i;
			--iEnd;
		}
	}
}

void UT4UserWidgetBase::ReleaseWidget()//#268// #536
{
	int32 iEnd = arrChildWidget.Num();
	for (int32 i = 0; i < iEnd; ++i)
	{
		if (arrChildWidget[i].IsValid())
		{
			arrChildWidget[i]->ReleaseWidget();
		}
		else
		{
			arrChildWidget.RemoveAt(i);
			--i;
			--iEnd;
		}
	}

	ClearButtonAdapters();//#276

	DelayVisibilityDataArray.Reset();//#276
}

void UT4UserWidgetBase::OnStart() //#t4f-201
{
	int32 iEnd = arrChildWidget.Num();
	for (int32 i = 0; i < iEnd; ++i)
	{
		if (arrChildWidget[i].IsValid())
		{
			arrChildWidget[i]->OnStart();
		}
	}
}

// #t4f-173 : 위젯의 위치 초기화
void UT4UserWidgetBase::InitWidgetPosition()
{

}

// #t4f-218 : 명시적 초기화
void UT4UserWidgetBase::UserBindVariable(UT4UserWidgetBase* InChildWidget, bool InAddChild/* = false*/)
{
	if (nullptr == InChildWidget)
	{
		return;
	}

	InChildWidget->InitWidget();
	if (InAddChild)
	{
		arrChildWidget.Emplace(InChildWidget);
	}
}

void UT4UserWidgetBase::Show(bool bInEnable)
{
	//T4_LOG(Warning, TEXT("%s, %s"), *GetWidgetName().ToString(), bInEnable ? TEXT("true") : TEXT("false"));
	if (bInEnable && !bAddToViewport)
	{
#if USES_SUPPORT_UE5_1 // #t4f-511
		TryAddToViewport(GetZOrder()); // Editor 뷰포트 호출
#else
		AddToViewport(GetZOrder());
#endif

		// #1235
		OnChildOpenInit();
		OnOpenInit();

		bAddToViewport = true;
	}
	else if (!bInEnable && bAddToViewport)
	{
		// #1235
		OnChildCloseDeInit();
		OnCloseDeInit();

#if USES_SUPPORT_UE5_1 // #t4f-511
		RemoveFromParent();
#else
		RemoveFromViewport();
#endif
		bAddToViewport = false;
	}

	ToggleWidget(bInEnable); // #t4f-335
}

#if USES_SUPPORT_UE5_1 // #t4f-511
FMargin UT4UserWidgetBase::GetFullScreenOffsetOverride() const
{
	if (bIsManagedByGameViewportSubsystem)
	{
		if (UGameViewportSubsystem* Subsystem = UGameViewportSubsystem::Get(GetWorld()))
		{
			return Subsystem->GetWidgetSlot(this).Offsets;
		}
	}
	return FGameViewportWidgetSlot().Offsets;
}

void UT4UserWidgetBase::TryAddToViewport(int32 ZOrder) // 에디터 뷰포트 호출
{
#if WITH_EDITOR // #164
	UWorld* UnrealWorld = GetWorld();
	if (nullptr == UnrealWorld)
	{
		return;
	}
	//check(ET4LayerType::Max != LayerType); // #164 : UMG Editor 에서 불릴 수가 있는데, 이때는 LayerType 이 없을 수 있다.
	IT4Framework* Framework = T4Framework::GetFramework(LayerType);
	if (nullptr != Framework && Framework->IsPreview())
	{
		if (!OverrideFullScreenWidgetPtr.IsValid())
		{
			if (UPanelWidget* ParentPanel = GetParent())
			{
				FMessageLog("PIE").Error(FText::Format(LOCTEXT("WidgetAlreadyHasParent", "The widget '{0}' already has a parent widget.  It can't also be added to the viewport!"),
					FText::FromString(GetClass()->GetName())));
				return;
			}

			// First create and initialize the variable so that users calling this function twice don't
			// attempt to add the widget to the viewport again.
			TSharedRef<SConstraintCanvas> FullScreenCanvas = SNew(SConstraintCanvas);
			OverrideFullScreenWidgetPtr = FullScreenCanvas;

			TSharedRef<SWidget> UserSlateWidget = TakeWidget();
			FullScreenCanvas->AddSlot()
				.Offset(BIND_UOBJECT_ATTRIBUTE(FMargin, GetFullScreenOffsetOverride))
				.Anchors(BIND_UOBJECT_ATTRIBUTE(FAnchors, GetAnchorsInViewport))
				.Alignment(BIND_UOBJECT_ATTRIBUTE(FVector2D, GetAlignmentInViewport))
				[
					UserSlateWidget
				];

			ULocalPlayer* LocalPlayer = GetOwningLocalPlayer(); // #t4f-511

			IT4EditorViewportClient* EditorViewportClinet = Framework->GetEditorViewportClient();
			check(nullptr != EditorViewportClinet)

			// #218 : 에디터에서는 ZOrder 를 설정하지 않는다.
			// #t4f-353 : 다시 해제함 (적용하더라도 큰 문제는 없어 보임)
			// Media의 특성상 화면의 가장 앞으로 와야하지만, 다른UI(Dialogue)가 ZOrder를 조절하여 사용하는 경우
			// Editor에서는 Restore ZOrder가 적용되지 않는 경우가 생긴다.
			EditorViewportClinet->AddWidgetToScreen(LocalPlayer, FullScreenCanvas, ZOrder);
		}
		else
		{
			FMessageLog("PIE").Warning(FText::Format(LOCTEXT("WidgetAlreadyOnScreen", "The widget '{0}' was already added to the screen."),
				FText::FromString(GetClass()->GetName())));
		}
		return;
	}
#endif
	AddToViewport(ZOrder);
}
#else
void UT4UserWidgetBase::AddToScreen(ULocalPlayer* Player, int32 ZOrder)
{
	UWorld* UnrealWorld = GetWorld();
	if (nullptr == UnrealWorld)
	{
		return;
	}
#if WITH_EDITOR // #164
	//check(ET4LayerType::Max != LayerType); // #164 : UMG Editor 에서 불릴 수가 있는데, 이때는 LayerType 이 없을 수 있다.
	IT4Framework* Framework = T4Framework::GetFramework(LayerType);
	if (nullptr != Framework && Framework->IsPreview())
	{
		if (!OverrideFullScreenWidgetPtr.IsValid())
		{
			if (UPanelWidget* ParentPanel = GetParent())
			{
				FMessageLog("PIE").Error(FText::Format(LOCTEXT("WidgetAlreadyHasParent", "The widget '{0}' already has a parent widget.  It can't also be added to the viewport!"),
					FText::FromString(GetClass()->GetName())));
				return;
			}

			// First create and initialize the variable so that users calling this function twice don't
			// attempt to add the widget to the viewport again.
			TSharedRef<SConstraintCanvas> FullScreenCanvas = SNew(SConstraintCanvas);
			OverrideFullScreenWidgetPtr = FullScreenCanvas;

			TSharedRef<SWidget> UserSlateWidget = TakeWidget();
			FullScreenCanvas->AddSlot()
				.Offset(BIND_UOBJECT_ATTRIBUTE(FMargin, GetFullScreenOffset))
				.Anchors(BIND_UOBJECT_ATTRIBUTE(FAnchors, GetAnchorsInViewport))
				.Alignment(BIND_UOBJECT_ATTRIBUTE(FVector2D, GetAlignmentInViewport))
				[
					UserSlateWidget
				];

			IT4EditorViewportClient* EditorViewportClinet = Framework->GetEditorViewportClient();
			check(nullptr != EditorViewportClinet)
			// #218 : 에디터에서는 ZOrder 를 설정하지 않는다.
			// #t4f-353 : 다시 해제함 (적용하더라도 큰 문제는 없어 보임)
			// Media의 특성상 화면의 가장 앞으로 와야하지만, 다른UI(Dialogue)가 ZOrder를 조절하여 사용하는 경우
			// Editor에서는 Restore ZOrder가 적용되지 않는 경우가 생긴다.
			EditorViewportClinet->AddWidgetToScreen(Player, FullScreenCanvas, ZOrder);
		}
		else
		{
			FMessageLog("PIE").Warning(FText::Format(LOCTEXT("WidgetAlreadyOnScreen", "The widget '{0}' was already added to the screen."),
				FText::FromString(GetClass()->GetName())));
		}
		return;
	}
#endif
	Super::AddToScreen(Player, ZOrder);
}
#endif

void UT4UserWidgetBase::RemoveFromParent()
{
#if WITH_EDITOR // #164
	//check(ET4LayerType::Max != LayerType); // #164 : UMG Editor 에서 불릴 수가 있는데, 이때는 LayerType 이 없을 수 있다.
	IT4Framework* Framework = T4Framework::GetFramework(LayerType);
	if (nullptr != Framework && Framework->IsPreview())
	{
		if (!HasAnyFlags(RF_BeginDestroyed))
		{
			if (OverrideFullScreenWidgetPtr.IsValid() && nullptr != GetWorld())
			{
				TSharedPtr<SWidget> WidgetHost = OverrideFullScreenWidgetPtr.Pin();
				ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
				IT4EditorViewportClient* EditorViewportClinet = Framework->GetEditorViewportClient();
				check(nullptr != EditorViewportClinet)
				EditorViewportClinet->RemoveWidgetFromScreen(LocalPlayer, WidgetHost.ToSharedRef());
			}
			else
			{
				Super::RemoveFromParent();
			}
		}
	}
#endif
	Super::RemoveFromParent();
}

void UT4UserWidgetBase::ClearButtonAdapters() //#276
{
	for (int32 Index = 0; Index < ButtonAdapterArray.Num(); ++Index)
	{
		ButtonAdapterArray[Index]->Release();
		ButtonAdapterArray[Index]->ConditionalBeginDestroy();
		ButtonAdapterArray[Index] = nullptr;
	}
	ButtonAdapterArray.Reset();
}

void UT4UserWidgetBase::AddButtonAdapter(class UButton* InButton, UObject* InObject, const FName& InFunctionName, int32 InIndex) //#276
{
	UT4ButtonAdapter* ButtonAdapter = NewObject<UT4ButtonAdapter>();
	check(ButtonAdapter);

	//set
	ButtonAdapter->Set(InButton, InObject, InFunctionName, InIndex);

	//add
	ButtonAdapterArray.Add(ButtonAdapter);
}

void UT4UserWidgetBase::AddDelayHide(UWidget* InWidget, float InDelay)//#276
{
	FT4DelayVisibilityData Data;
	Data.Widget = InWidget;
	Data.Delay = InDelay;
	Data.Visibility = ESlateVisibility::Collapsed;

	DelayVisibilityDataArray.Add(Data);
}

UWidgetAnimation* UT4UserWidgetBase::GetAnimation(const FName& AniName) // #570
{
	UWidgetAnimation* pRet = nullptr;
	FObjectPropertyBase* pProp = FindFProperty<FObjectPropertyBase>(GetClass(), AniName);
	if (pProp)
	{
		UObject* pObj = pProp->GetObjectPropertyValue_InContainer(this);
		if (pObj)
			pRet = Cast<UWidgetAnimation>(pObj);
	}

	return pRet;
}

UT4ButtonAdapter::UT4ButtonAdapter(const FObjectInitializer& ObjectInitializer) //#276
	: Button(nullptr)
	, Index(0)
{

}

void UT4ButtonAdapter::Set(class UButton* InButton, UObject* InObject, const FName& InFunctionName, int32 InIndex) //#276
{
	Button = InButton;

	if (nullptr != Button)
	{
		//base
		Button->OnClicked.Clear();
		Button->OnClicked.AddDynamic(this, &UT4ButtonAdapter::OnClickedButton);

		//adapter
		OnClicked.Clear();
		OnClicked.BindUFunction(InObject, InFunctionName);

		//param
		Index = InIndex;
	}
	else
	{
		T4_LOG(Warning, TEXT("Button is nullptr!"));
	}
}

void UT4ButtonAdapter::Release() //#276
{
	if (nullptr != Button)
	{
		Button->OnClicked.Clear();
		Button = nullptr;
	}
	OnClicked.Clear();
	Index = 0;
}

void UT4ButtonAdapter::OnClickedButton() //#276
{
	OnClicked.ExecuteIfBound(Index);
}

#undef LOCTEXT_NAMESPACE // "T4UserWidgetBase"
