// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

USTRUCT()
struct FGoKartMove 
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY()
		float Throttle;

	UPROPERTY()
		float SteeringThrow;

	UPROPERTY()
		float DeltaTime;

	UPROPERTY()
		float Time;
};

USTRUCT()
struct FGOKartState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FTransform Transform;

	UPROPERTY()
		FVector Velocity;

	UPROPERTY()
		FGoKartMove LastMove;
};

UCLASS()
class KARZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	UPROPERTY(EditAnywhere)
		float Mass = 1000;

	//The force applied to the car when the throttle is fully down(N)
	UPROPERTY(EditAnywhere)
		float MaxDrivingForce = 10000;

	UPROPERTY(EditAnywhere)
		float DragCoefficient = 16;

	UPROPERTY(EditAnywhere)
		float RollingResistanceCoefficient = 0.01f;

	UPROPERTY(EditAnywhere)
		float MinTurningRadius = 10;

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
		FGOKartState ServerState;

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_SendMove(FGoKartMove Move);

	UFUNCTION()
		void OnRep_ServerState();

	void MoveForward(float Value);
	void MoveRight(float Value);

	void SimulateMove(const FGoKartMove& Move);

	FGoKartMove CreateMove(float DeltaTime);

	void ClearAcknowledgeMoves(FGoKartMove LastMove);

	void UpdateLocationFromVelocity(float DeltaTime);

	void ApplyRotation(float DeltaTime, float SteeringThrow);

	FVector GetAirResistance();
	FVector GetRollingResistance();
	
	FVector Velocity;

	float Throttle;

	float SteeringThrow;

	TArray<FGoKartMove> UnacknowledgedMoves;
};