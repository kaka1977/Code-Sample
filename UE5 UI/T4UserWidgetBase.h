// Copyright VirtualFlow, Inc. All Rights Reserved.

#pragma once

#include "T4FrameworkMinimal.h"
#include "T4Engine/Public/T4EngineLayer.h"
#include "Blueprint/UserWidget.h"
#include "T4UserWidgetBase.generated.h"

/*
* #276
*/
USTRUCT()
struct FT4DelayVisibilityData
{
	GENERATED_BODY()

public:
	FT4DelayVisibilityData()
		: Delay(0.0f)
		, Visibility(ESlateVisibility::Collapsed)
		, Widget(nullptr)
	{}

public:
	float Delay;

	ESlateVisibility Visibility;

	UPROPERTY()
	class UWidget* Widget;
};

/**
  * #164
 */
class SWidget;
class UWidgetAnimation;
UCLASS()
class T4FRAMEWORK_API UT4UserWidgetBase : public UUserWidget
{
	GENERATED_UCLASS_BODY()

public: // #525
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual void OnOpenInit();				// 위젯을 열때 초기화
	virtual void OnCloseDeInit();			// 위젯을 닫을때 초기화
	virtual void OnChildOpenInit();			// After OnOpenInit, Recursive
	virtual void OnChildCloseDeInit();		// After OnCloseDeInit, Recursive
	virtual void ToggleWidget(bool InVisible); // #t4f-335, #t4f-896 : Toggle될 경우 처리가 필요할 경우 사용

public:
	virtual void InitWidget(); //#268 #545
	virtual void ReleaseWidget(); //#268,#276	

	int32 GetZOrder() const { return SortZOrder; }
	void SetZOrder(int32 InZOrder) { SortZOrder = InZOrder; }

	bool IsShown() const { return bAddToViewport; }
	void Show(bool bInEnable);

	virtual void OnStart(); //#t4f-201

	template<typename T> T* GetChildWidget(const FName& InName) { // #259
		return Cast<T>(GetWidgetFromName(InName));
	}

	//{#289
	void SetWidgetName(const FName& InName) { WidgetName = InName; }
	const FName& GetWidgetName() const { return WidgetName;}
	//}

	void SetUserData(FName InUserData) { UserData = InUserData; } // #542
	FName GetUserData() const { return UserData; } // #542

	// #668
	void SetWidgetUIType(int32 InGameUIType) { WidgetUIType = InGameUIType; }
	int32 GetWidgetUIType() const { return WidgetUIType; }

	// #570
	UWidgetAnimation* GetAnimation(const FName& AniName);

	// #525
public:
	template<typename T>
	bool BindWidgetVariable_Impl(T*& pWidget, FName InWidgetName, bool InShow)
	{
		UWidget* pFindWidget = GetWidgetFromName(InWidgetName);
		if (pFindWidget != nullptr)
		{
			pWidget = Cast<T>(pFindWidget);
			if (pWidget != nullptr)
			{
				if (InShow == false)
				{
					pWidget->SetVisibility(ESlateVisibility::Collapsed);
				}

				if (UT4UserWidgetBase* pChildWidget = Cast<UT4UserWidgetBase>(pWidget))
				{
					pChildWidget->InitWidget(); // #t4f-195 : 명시적 초기화는 여기에서~~
					arrChildWidget.Emplace(pChildWidget);
				}

				return true;
			}
		}

		return false;
	}

	template<typename T>
	bool BindVariable_Impl(T*& pProperty, FName PrppertyName)
	{
		FProperty* pProp = GetClass()->FindPropertyByName(PrppertyName);
		if (pProp != nullptr && pProp->GetClass() == FObjectProperty::StaticClass())
		{
			FObjectProperty* pObjProp = Cast<FObjectProperty>(pProp);
			if (pObjProp->PropertyClass == T::StaticClass())
			{
				UObject* pObj = pObjProp->GetObjectPropertyValue_InContainer(this);
				pProperty = Cast<T>(pObj);
				return true;
			}
		}

		return false;
	}

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override; //#276

	virtual bool Initialize() override;
	virtual void BeginDestroy() override;
	// #t4f-218 : CreateWidget을 사용하여 UClass 접근하는 위젯은 명시적으로 호출을 해야함
	// 관리되지않는 Slot(n개)일경우 Child에 추가하면 GC 전까지 남아있어 권장하지 않음.
	// 하나의 위젯일 경우 True로 권장
	void UserBindVariable(UT4UserWidgetBase* InChildWidget, bool InAddChild = false); // BindWidgetVariable_Impl과 같은 열활

#if USES_SUPPORT_UE5_1 // #t4f-511
	FMargin GetFullScreenOffsetOverride() const;
	virtual void TryAddToViewport(int32 ZOrder); // 에디터 뷰포트 호출
#else
	virtual void AddToScreen(ULocalPlayer* LocalPlayer, int32 ZOrder) override;
#endif 

	virtual void InitWidgetPosition(); // #t4f-173

public:
	virtual void RemoveFromParent() override; // #668
	
	//{#276
public:
	void ClearButtonAdapters();
	void AddButtonAdapter(class UButton* InButton, UObject* InObject, const FName& InFunctionName, int32 InIndex);

	void AddDelayHide(UWidget* InWidget, float InDelay);
private:
	UPROPERTY()
	TArray<class UT4ButtonAdapter*> ButtonAdapterArray;

	UPROPERTY()
	TArray<FT4DelayVisibilityData> DelayVisibilityDataArray;
	//}#276

	UPROPERTY(Transient) // #525
	TArray<TWeakObjectPtr<UT4UserWidgetBase>> arrChildWidget;

protected:
	ET4LayerType LayerType;

	bool bAddToViewport;
	int32 SortZOrder;

#if WITH_EDITOR
	TWeakPtr<SWidget> OverrideFullScreenWidgetPtr; // #164 : EditorViewportClient 지원
#endif

private:
	int32 WidgetUIType; // #757
	FName WidgetName;//#289

	FName UserData; // #542
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FT4OnButtonClicked, int32, InIndex); //#276

/**
 * #276
 */
UCLASS()
class T4FRAMEWORK_API UT4ButtonAdapter : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	void Set(class UButton* InButton, UObject* InObject, const FName& InFunctionName, int32 InIndex);
	void Release();

private:
	UFUNCTION()
	void OnClickedButton();

private:

	FT4OnButtonClicked OnClicked;

	UPROPERTY()
	UButton* Button;
	UPROPERTY()
	int32 Index;
};

#define BindWidgetVariable(pWidget, WidgetName)				\
if (!BindWidgetVariable_Impl(pWidget, WidgetName, true)) {  \
	T4_LOG(Verbose, TEXT("Failed BindWidgetVariable : %s, %s"), *((FName)WidgetName).ToString(), *GetName()); }


#define BindWidgetVariableSetVisible(pWidget, WidgetName, Show)		\
if (!BindWidgetVariable_Impl(pWidget, WidgetName, Show)) {			\
	T4_LOG(Verbose, TEXT("Failed BindWidgetVariable : %s, %s"), *((FName)WidgetName).ToString(), *GetName()); }

#define BindVariable(pProperty, PropertyName)		\
if (!BindVariable_Impl(pProperty, PropertyName)) {  \
	T4_LOG(Verbose, TEXT("Failed BindWidgetVariable : %s, %s"), *((FName)PropertyName).ToString(), *GetName()); }