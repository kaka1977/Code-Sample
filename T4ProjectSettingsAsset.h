// Copyright VirtualFlow, Inc. All Rights Reserved.

#pragma once

#include "T4GameDataMinimal.h"
#include "T4GameDataTypes.h"
#include "UserWidget/T4UserWidgetBase.h"//#289
#include "T4Engine/Public/Settings/T4EngineVRSettings.h" // #310
#include "T4Engine/Public/Settings/T4EngineVRSettings.h" // #310
#include "T4Engine/Public/Settings/T4EnginePlayerSettings.h"
#include "T4Framework/Public/T4FrameworkTypes.h"
#include "T4Framework/Public/T4FrameworkConstants.h"
#include "T4Framework/Public/T4FrameworkSettings.h" // #t4f-887
#include "T4UserInterfaceAsset.h"
#include "T4InputSettingAsset.h"//#487
#include "LiveLinkTypes.h" // #t4f-106
#include "DB/T4GameDBKeys.h" // #t4f-368
#include "T4ProjectSettingsAsset.generated.h"

/**
  * #177, #t4f-238 : struct, property 내 Game 단어 삭제
 */
class UDataTable;
class UT4ActionPackAsset;

struct FT4ProjectSettingsVersion
{
	enum Type
	{
		InitializeVer = 0,

		AddInitializeQuestDBKeyName, // #245

		AddGameUISettings, // #952

		RenameProjectCategory, // #t4f-242 : SocialGaming => Storytelling

		RemoveGameWord, // #t4f-238 : Game 단어 삭제

		RemoveUITemplate, // #t4f-413

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1,
	};

	T4GAMEDATA_API const static FGuid GUID;

private:
	FT4ProjectSettingsVersion() {}
};

class UTexture2D;
USTRUCT()
struct FT4ProjectControlSettings
{
	GENERATED_USTRUCT_BODY()

public:
	FT4ProjectControlSettings()
		: DefaultControlMode(ET4ProjectControlMode::Default)
		, bAllowObserverMode(false) // #1180
		, bPCMoveOnOnlyNav(false)
		, bCanDoubleJump(false) // #1236
		, bUpdateJumpByKeyRelease(true) // #T4F-14
		, bUpdateJumpByKeyPress(true) // #t4f-683
	{
	}

public:
	UPROPERTY(EditAnywhere, Category = Common, meta = (ToolTip = ""))
	ET4ProjectControlMode DefaultControlMode;

	UPROPERTY(EditAnywhere, Category = Common, meta = (ToolTip = ""))
	FT4PlayerCameraSettings OverrideCameraSettings; // #185

	UPROPERTY(EditAnywhere, Category = Common, meta = (ToolTip = ""))
	uint8 bAllowObserverMode : 1; // #1180

	UPROPERTY(EditAnywhere, Category = Common, meta = (ToolTip = ""))
	uint8 bPCMoveOnOnlyNav : 1; // #1064

	UPROPERTY(EditAnywhere, Category = Common, meta = (DisplayName = "Enable Double Jump", ToolTip = ""))
	uint8 bCanDoubleJump : 1; // #1236

	UPROPERTY(EditAnywhere, Category = Common, meta = (DisplayName = "Enable Update Jump By Arrow Key Release", ToolTip = ""))
	uint8 bUpdateJumpByKeyRelease : 1; // #t4f-14 : 점프 직전 누른 이동키 떼면 점프 중 하강

	UPROPERTY(EditAnywhere, Category = Common, meta = (DisplayName = "Enable Update Jump By Arrow Key Press", ToolTip = ""))
	uint8 bUpdateJumpByKeyPress : 1; // #t4f-683 : 점프 중 이동키 입력 시 처리
};

class UT4ActionPackAsset;
USTRUCT()
struct FT4ProjectModeSettings
{
	GENERATED_USTRUCT_BODY()

public:
	FT4ProjectModeSettings()
		: ProjectCategory(ET4ProjectCategory::Storytelling) // #952, #t4f-242
		, GameProjectCategory_DEPRECATED(ET4GameProjectCategory::RolePlaying) // #952, #t4f-242 : 삭제예정
		, bEnableGuestMode(false) // #t4f-641 : 기본 Guest 모드로 동작
		, bEnableAdventureMode(false) // #t4f-925
		, bEnableAIDocentMode(false) // #t4f-874 : AIDocent, NPC 전용 LookAt 카메라만 사용, 플레이어는 제외됨
		, LoginWorldDBKeyName(NAME_None)//#498
		, AccountWorldDBKeyName(NAME_None)//#t4f-732
		, AccountPlayerLocation(FVector::ZeroVector)//#t4f-757
		, LobbyWorldDBKeyName(NAME_None)
		, bEnablePlayerInvisibleMode(false) // #t4f-876 : AIDocent, Player를 감춘다.
		, DefaultPlayerDBKeyName(NAME_None) // #t4f-96
		, InitialWorldDBKeyName(NAME_None) // #245, #t4f-238
		, InitialQuestDBKeyName(NAME_None) // #245, #t4f-238
		, LobbyServiceMode(ET4LobbyServiceMode::ClientOnly) // #t4f-772
		, InGameWorldDBKeyName_DEPRECATED(NAME_None) // #245
		, InGameInitQuestDBKeyName_DEPRECATED(NAME_None) // #245
		, ItemPickupActionMode_DEPRECATED(ET4PickupActionMode::MouseOverAndClick) // #695
		, bHideHPBar_DEPRECATED(false) // #1056 : 4.27 패키징 오류
		, bHideFloatingDamage_DEPRECATED(false) // #1056 : 4.27 패키징 오류
		, bDisableTargetOutline_DEPRECATED(false) // #1056 : 4.27 패키징 오류
		, OverrideInputSetting_DEPRECATED(nullptr) // #1056 : 4.27 패키징 오류
	{
	}

public:
	UPROPERTY(EditAnywhere, Category = ProjectModeSettings, meta = (DisplayName = "Project Category", ToolTip = ""))
	ET4ProjectCategory ProjectCategory; // #t4f-242

	UPROPERTY()
	ET4GameProjectCategory GameProjectCategory_DEPRECATED; // #952

	UPROPERTY(EditAnywhere, Category = ProjectModeSettings, meta = (DisplayName = "Guest Mode", ToolTip = "")) // #1055 : iM+D Presentation
	bool bEnableGuestMode;

	UPROPERTY(EditAnywhere, Category = ProjectModeSettings, meta = (DisplayName = "Adventure Mode", ToolTip = "")) // #t4f-925, #t4f-1029 : 스토리텔링 Only 체크 옵션을 풀어줌
	bool bEnableAdventureMode;

	UPROPERTY(EditAnywhere, Category = ProjectModeSettings, meta = (DisplayName = "AI Docent Mode", EditCondition = "ProjectCategory == ET4ProjectCategory::Storytelling", ToolTip = "")) // #t4f-874 : AIDocent, NPC 전용 LookAt 카메라만 사용, 플레이어는 제외됨
	bool bEnableAIDocentMode;

	UPROPERTY(EditAnywhere, Category = ProjectModeSettings, meta = (ToolTip = "")) //#498
	FName LoginWorldDBKeyName;

	UPROPERTY(EditAnywhere, Category = ProjectModeSettings, meta = (ToolTip = "")) //#t4f-732
	FName AccountWorldDBKeyName;

	UPROPERTY(EditAnywhere, Category = ProjectModeSettings, meta = (ToolTip = "")) //#t4f-757
	FVector AccountPlayerLocation;

	UPROPERTY(EditAnywhere, Category = ProjectModeSettings, meta = (ToolTip = ""))
	FName LobbyWorldDBKeyName;

	UPROPERTY(EditAnywhere, Category = ProjectModeSettings, meta = (DisplayName = "Player Invisible", EditCondition = "ProjectCategory == ET4ProjectCategory::Storytelling", ToolTip = "")) // #t4f-876 : AIDocent, Player를 감춘다.
	bool bEnablePlayerInvisibleMode;

	UPROPERTY(EditAnywhere, Category = ProjectModeSettings, meta = (ToolTip = ""))
	FName DefaultPlayerDBKeyName; // #t4f-96

	UPROPERTY(EditAnywhere, Category = ProjectModeSettings, meta = (ToolTip = ""))
	FName InitialWorldDBKeyName; // #245, #t4f-238

	UPROPERTY(EditAnywhere, Category = ProjectModeSettings, meta = (ToolTip = ""))
	FName InitialQuestDBKeyName; // #245, #t4f-238

	UPROPERTY(EditAnywhere, Category = ProjectModeSettings, meta = (ToolTip = ""))
	FSoftObjectPath InitialDramaObjectPath; // #t4f-641 : Free 사용자 Only (툴에 노출하지 않음)

	UPROPERTY(EditAnywhere, Category = ProjectModeSettings, meta = (ToolTip = ""))
	ET4LobbyServiceMode LobbyServiceMode; // #t4f-772

	UPROPERTY()
	FName StartQuestDBKeyName_DEPRECATED;

	UPROPERTY()
	FName InGameWorldDBKeyName_DEPRECATED; // #245

	UPROPERTY()
	FName InGameInitQuestDBKeyName_DEPRECATED; // #245

	UPROPERTY()
	ET4PickupActionMode ItemPickupActionMode_DEPRECATED; // #695

	UPROPERTY()
	TSoftObjectPtr<UT4ActionPackAsset> DieScreenActionPackAsset_DEPRECATED; // #176

	UPROPERTY()
	bool bHideHPBar_DEPRECATED; // #208 : 옵션 제공

	UPROPERTY()
	bool bHideFloatingDamage_DEPRECATED; // #187 : TPS 류는 FloatingDamage 를 사용하지 않을 수 있도록 옵션 추가

	UPROPERTY()
	bool bDisableTargetOutline_DEPRECATED; // #208 : 타겟 아웃라인 사용 여부

	// #453 : UT4UserInterfaceAsset을 연결
	UPROPERTY()
	TSoftObjectPtr<UT4UserInterfaceAsset> OverrideUserWidgetSettings_DEPRECATED;
	//FT4GameUserWidgetSettings OverrideUserWidgetSettings;

	UPROPERTY()
	class UT4InputSettingAsset* OverrideInputSetting_DEPRECATED; // #487
};

class UT4InputSettingAsset;
USTRUCT()
struct FT4ProjectUISettings // #952
{
	GENERATED_USTRUCT_BODY()

public:
	FT4ProjectUISettings()
		: GameUITemplate_DEPRECATED(ET4GameUITemplate::Default)
		, TabFolderDBKey_DEPRECATED(NAME_None)
		, ItemPickupActionMode(ET4PickupActionMode::MouseOverAndClick) // #695
		, bHideHUD(false) // #952 : 옵션 제공
		, bHideHPBar(false) // #208 : 옵션 제공
		, bHideFloatingDamage(false) // #187
		, bDisableTargetOutline(false) // #208 : 타겟 아웃라인 사용 여부
		, bHideGameMain(false) // #t4f-892
		, bHideInventory(false) // #t4f-1015
		, bShowQuestDialogOptions(false) // #t4f-977
		, bShowQuestInfoInventory(false) // #t4f-1054
		, InventorySlotMax(10)
		//, UseSoftwareCursor(false) // #t4f-286 : SoftwareCursor (Backlog)
	{
	}

public:
	// #t4f-303
	UPROPERTY()
	ET4GameUITemplate GameUITemplate_DEPRECATED;

	UPROPERTY()
	FName TabFolderDBKey_DEPRECATED; // #t4f-386

	UPROPERTY()
	TSoftObjectPtr<UTexture2D> DefaultFolderIcon_DEPRECATED; // #t4f-386

	UPROPERTY(EditAnywhere, meta = (ToolTip = ""))
	TSoftObjectPtr<UDataTable> OverrideUserDefaultUITable; // #t4f-882 : 사용자 Default UITable 설정

	// #453 : UT4UserInterfaceAsset을 연결
	UPROPERTY(EditAnywhere, meta = (ToolTip = ""))
	TSoftObjectPtr<UT4UserInterfaceAsset> OverrideUserWidgetAsset;

	UPROPERTY(EditAnywhere, meta = (ToolTip = ""))
	TSoftObjectPtr<UT4InputSettingAsset> OverrideInputSettingAsset; // #487

	UPROPERTY(EditAnywhere, meta = (ToolTip = ""))
	ET4PickupActionMode ItemPickupActionMode; // #695

	UPROPERTY(EditAnywhere, meta = (ToolTip = ""))
	bool bHideHUD; // #952 : 옵션 제공

	UPROPERTY(EditAnywhere, meta = (DisplayName = "Hide HP Bar", ToolTip = "")) // #t4f-206 : DisplayName 수정
	bool bHideHPBar; // #208 : 옵션 제공

	UPROPERTY(EditAnywhere, meta = (ToolTip = ""))
	bool bHideFloatingDamage; // #187 : TPS 류는 FloatingDamage 를 사용하지 않을 수 있도록 옵션 추가

	UPROPERTY(EditAnywhere, meta = (ToolTip = ""))
	bool bDisableTargetOutline; // #208 : 타겟 아웃라인 사용 여부

	UPROPERTY(EditAnywhere, meta = (ToolTip = ""))
	bool bHideGameMain; // #t4f-892

	UPROPERTY(EditAnywhere, meta = (ToolTip = ""))
	bool bHideInventory; // #t4f-1015

	UPROPERTY(EditAnywhere, meta = (ToolTip = "Show Quest dialog options that have already been selected"))
	bool bShowQuestDialogOptions; // #t4f-977

	UPROPERTY(EditAnywhere, meta = (ToolTip = ""))
	bool bShowQuestInfoInventory; // #t4f-1048

	UPROPERTY(EditAnywhere, meta = (ToolTip = ""))
	TSoftObjectPtr<UT4ActionPackAsset> DieScreenActionPackAsset; // #176

	UPROPERTY(EditAnywhere, meta = (UIMin = 0, ClampMin = 0, UIMax = 200, ClampMax = 200))
	int32 InventorySlotMax = 10 ; // #t4f-291 : 인벤토리 슬롯의 최대값 설정(Server)

	// #t4f-286 : SoftwareCursor (Backlog)
	//UPROPERTY(EditAnywhere, meta = (ToolTip = ""))
	//bool UseSoftwareCursor; // #t4f-286

public:
	ET4GameUITemplate GetProjectGameUITemplate() const // #t4f-413
	{
		if (!OverrideUserWidgetAsset.IsNull())
		{
			UT4UserInterfaceAsset* UserInterfaceAsset = OverrideUserWidgetAsset.LoadSynchronous();
			if (nullptr != UserInterfaceAsset)
			{
				return UserInterfaceAsset->ProjectUITemplate;
			}
		}

		return ET4GameUITemplate::Default;
	}

	FName GetProjectTabFolderDBKey() const // #t4f-413
	{
		if (!OverrideUserWidgetAsset.IsNull())
		{
			UT4UserInterfaceAsset* UserInterfaceAsset = OverrideUserWidgetAsset.LoadSynchronous();
			if (nullptr != UserInterfaceAsset)
			{
				return UserInterfaceAsset->TabFolderDBKey;
			}
		}

		return NAME_None;
	}
	
	FSoftObjectPath GetProjectDefaultFolderIcon() const // #t4f-413
	{
		FSoftObjectPath ResultSoftObjectPath;

		if (!OverrideUserWidgetAsset.IsNull())
		{
			UT4UserInterfaceAsset* UserInterfaceAsset = OverrideUserWidgetAsset.LoadSynchronous();
			if (nullptr != UserInterfaceAsset)
			{
				ResultSoftObjectPath = UserInterfaceAsset->DefaultFolderIcon.ToSoftObjectPath();
			}
		}

		return ResultSoftObjectPath;
	}
};

USTRUCT()
struct FT4ProjectVRSettings // #310
{
	GENERATED_USTRUCT_BODY()

public:
	FT4ProjectVRSettings()
	{
	}

public:
	UPROPERTY(EditAnywhere, Category = ProjectVRSettings)
	TArray<FT4EngineVRDataSet> VRDataSets;
};

USTRUCT()
struct FT4EngineOverrideSettings // #222
{
	GENERATED_USTRUCT_BODY()

public:
	FT4EngineOverrideSettings()
	{
	}

public:
	UPROPERTY(EditAnywhere, Category = EngineOverrideSettings, meta = (ToolTip = ""))
	FString RendererSettings;

	UPROPERTY(EditAnywhere, Category = EngineOverrideSettings, meta = (ToolTip = ""))
	FString PhysicsSettings;

	UPROPERTY(EditAnywhere, Category = EngineOverrideSettings, meta = (ToolTip = ""))
	FString ConsoleVariables;
};

USTRUCT()
struct FT4ProjectPluginSettings // #t4f-106
{
	GENERATED_USTRUCT_BODY()

public:
	FT4ProjectPluginSettings()
	{
	}

public:
	UPROPERTY(EditAnywhere, Category = ProjectPluginSettings, meta = (ToolTip = ""))
	TArray<FName> UsingPluginNames;
};

USTRUCT()
struct FT4DevelopSettings // #191
{
	GENERATED_USTRUCT_BODY()

public:
	FT4DevelopSettings()
	{
	}

public:
	UPROPERTY(EditAnywhere, Category = DevelopSettings, meta = (ToolTip = ""))
	TArray<FName> HotKey_PlayerDBKeyNames; // Player QuickSpawn (ALT + 1 ~ 9)

	UPROPERTY(EditAnywhere, Category = DevelopSettings, meta = (DisplayName = "Hot Key NPC DBKey Names", ToolTip = ""))
	TArray<FName> HotKey_NPCDBKeyNames; // #50 : NPC QuickSpawn (CTRL + 1 ~ 9)
};

USTRUCT()
struct FT4PackageSettings // #405
{
	GENERATED_USTRUCT_BODY()

public:
	FT4PackageSettings()
	{
	}

public:
	UPROPERTY(EditAnywhere, Category = PackageSettings, meta = (ContentDir, LongPackageName, ToolTip = ""))
	TArray<FDirectoryPath> DirectoriesToAlwaysCook; // #405

	UPROPERTY(EditAnywhere, Category = PackageSettings, meta = (ContentDir, LongPackageName, ToolTip = ""))
	TArray<FDirectoryPath> DirectoriesToNeverCook; // #405
};

USTRUCT()
struct FT4ProjectAudioSettings // #t4f-1032
{
	GENERATED_USTRUCT_BODY()

public:
	FT4ProjectAudioSettings()
	: ControlGameAudioPercentage(0.3f)
	{
	}

public:
	UPROPERTY(EditAnywhere, config, Category = ProjectAudio)
	TSoftObjectPtr<UT4ActionPackAsset> DialogueLookAtBranchActionPackAsset; // #174

	UPROPERTY(EditAnywhere, Category = ProjectAudio, meta = (UIMin = 0.0f, ClampMin = 0.0f, UIMax = 1.0f, ClampMax = 1.0f))
	float ControlGameAudioPercentage;
};

class UTexture2D;
UCLASS(ClassGroup = T4Framework, Category = "T4Framework")
class T4GAMEDATA_API UT4ProjectSettingsAsset : public UObject // #t4f-206 : Game => Project로 용어 변경
{
	GENERATED_UCLASS_BODY()

public:
	UT4ProjectSettingsAsset()
		: bIsUseAutoDetectHMD(false)//#339
	{}

	//~ Begin UObject interface
	void Serialize(FArchive& Ar) override;
	void PostLoad() override;

#if WITH_EDITOR
	DECLARE_MULTICAST_DELEGATE(FT4OnPropertiesChanged);
	FT4OnPropertiesChanged& OnPropertiesChanged() { return OnPropertiesChangedDelegate; }
#endif // WITH_EDITOR

public:
	// #652 : 추가 시 InitializeOrResetProject(bool bInReset) 처리 필요 (#195)
	// #t4f-238 : Game => Project로 용어 변경
	UPROPERTY()
	FT4ProjectModeSettings GameModeSettings_DEPRECATED; // #t4f-238

	UPROPERTY(EditAnywhere, Category = ProjectModeSettings, meta = (ToolTip = ""))
	FT4ProjectModeSettings ProjectModeSettings; // #t4f-238

	UPROPERTY(EditAnywhere, Category = ProjectUISettings, meta = (ToolTip = ""))
	FT4ProjectUISettings ProjectUISettings; // #t4f-238

	UPROPERTY(EditAnywhere, Category = ProjectControlSettings, meta = (ToolTip = ""))
	FT4ProjectControlSettings ProjectControlSettings; // #t4f-238

	UPROPERTY(EditAnywhere, Category = Hide, meta = (ToolTip = ""))
	uint8 bIsUseAutoDetectHMD : 1;//#339

	UPROPERTY(EditAnywhere, Category = ProjectTTSSettings, meta = (ToolTip = ""))
	FT4TextToSpeechServiceSettings TTSSetings; // #t4f-887

	UPROPERTY(EditAnywhere, Category = ProjectSTTSettings, meta = (ToolTip = ""))
	FT4SpeechToTextServiceSettings STTSetings; // #t4f-895

	UPROPERTY(EditAnywhere, Category = Hide, meta = (ToolTip = ""))
	FT4ProjectVRSettings ProjectVRSetings; // #t4f-238

	UPROPERTY(EditAnywhere, Category = EngineOverrideSettings, meta = (ToolTip = ""))
	FT4EngineOverrideSettings EngineOverrideSettings; // #222

	UPROPERTY(EditAnywhere, Category = ProjectPluginSettings, meta = (ToolTip = ""))
	FT4ProjectPluginSettings ProjectPluginSettings; // #t4f-106

	UPROPERTY(EditAnywhere, Category = DevelopSettings, meta = (ToolTip = ""))
	FT4DevelopSettings DevelopSettings; // #191

	UPROPERTY(EditAnywhere, Category = PackageSettings, meta = (ToolTip = ""))
	FT4PackageSettings PackageSettings; // #405

	UPROPERTY(EditAnywhere, Category = AudioSettings, meta = (ToolTip = ""))
	FT4ProjectAudioSettings AudioSettings; // #t4f-1032

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UTexture2D* ThumbnailImage; // ThumbnailRenderer 추가 필요!
#endif

private:
#if WITH_EDITOR
	FT4OnPropertiesChanged OnPropertiesChangedDelegate;
#endif
};
