// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/HUD.h"
#include "KarzyKartsHud.generated.h"


UCLASS(config = Game)
class AKarzyKartsHud : public AHUD
{
	GENERATED_BODY()

public:
	AKarzyKartsHud();

	/** Font used to render the vehicle info */
	UPROPERTY()
	UFont* HUDFont;

	// Begin AHUD interface
	virtual void DrawHUD() override;
	// End AHUD interface
};
