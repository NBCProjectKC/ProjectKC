#include "ProjectKC/UI/Common/Core/KCUITags.h"

namespace KCUITags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Game, "UI.Layer.Game", "Persistent in-game HUD layer.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Menu, "UI.Layer.Menu", "Main menu, lobby, settings, and result screen layer.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_GameMenu, "UI.Layer.GameMenu", "In-game pause and game menu layer.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Modal, "UI.Layer.Modal", "User-response dialog layer.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_System, "UI.Layer.System", "High-priority system and network dialog layer.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Toast, "UI.Layer.Toast", "Short-lived notification layer.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Indicator, "UI.Layer.Indicator", "World-space and interaction indicator layer.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Transition, "UI.Layer.Transition", "Fade, black screen, and transition layer.");
}
