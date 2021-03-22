// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartReplicationComponent.h"
#include "Net\UnrealNetwork.h"

// Sets default values for this component's properties
UGoKartReplicationComponent::UGoKartReplicationComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}

// Called when the game starts
void UGoKartReplicationComponent::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
}

// Called every frame
void UGoKartReplicationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (MovementComponent == nullptr) return;

	FGoKartMove LastMove = MovementComponent->GetLastMove();

	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		UnacknowledgedMoves.Add(LastMove);
		Server_SendMove(LastMove);
	}

	//We are the server and in control of the pawn
	if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		UpdateServerState(LastMove);
		
	}

	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		ClientTick(DeltaTime);
	}
}

void UGoKartReplicationComponent::UpdateServerState(const FGoKartMove& Move)
{
	if (MovementComponent == nullptr) return;

	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

void UGoKartReplicationComponent::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

	if (MovementComponent == nullptr) return;

	if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER) return;
	
	auto LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;

	FHermiteCubicSpline Spline = CreateSpline();

	InterpolateLocation(Spline, LerpRatio);
	
	InterpolateVelocity(Spline, LerpRatio);
	
	InterpolateRotation(LerpRatio);
}

FHermiteCubicSpline UGoKartReplicationComponent::CreateSpline()
{
	FHermiteCubicSpline Spline;
	Spline.TargetLocation = ServerState.Transform.GetLocation();
	Spline.StartLocation = ClientStartTransform.GetLocation();
	Spline.StartDerivative = ClientStartVelocity * VelocityToDerivative();
	Spline.TargetDerivative = ServerState.Velocity * VelocityToDerivative();
	
	return Spline;
}

void UGoKartReplicationComponent::InterpolateLocation(const FHermiteCubicSpline& Spline, float LerpRatio)
{
	auto NewLocation = Spline.InterpolateLocation(LerpRatio);
	GetOwner()->SetActorLocation(NewLocation);
}

void UGoKartReplicationComponent::InterpolateVelocity(const FHermiteCubicSpline& Spline, float LerpRatio)
{
	auto SlerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;
	auto NewDerivative = Spline.InterpolateDerivative(LerpRatio);
	auto NewVelocity = NewDerivative / VelocityToDerivative();

	MovementComponent->SetVelocity(NewVelocity);
}

void UGoKartReplicationComponent::InterpolateRotation(float LerpRatio)
{
	auto StartRotation = ClientStartTransform.GetRotation();
	auto TargetRotation = ServerState.Transform.GetRotation();
	auto NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);
	GetOwner()->SetActorRotation(NewRotation);
}


float UGoKartReplicationComponent::VelocityToDerivative()
{
	return ClientTimeBetweenLastUpdates * 100;
}



void UGoKartReplicationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGoKartReplicationComponent, ServerState);
}

void UGoKartReplicationComponent::OnRep_ServerState()
{
	switch (GetOwnerRole())
	{
	case ROLE_AutonomousProxy:
		AutonomousProxy_OnRep_ServerState();
		break;
	case ROLE_SimulatedProxy:
		SimulatedProxy_OnRep_ServerState();
		break;
	default:
		break;
	}
}

void UGoKartReplicationComponent::AutonomousProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearAcknowledgeMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}

void UGoKartReplicationComponent::SimulatedProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;

	ClientStartTransform = GetOwner()->GetActorTransform();
	ClientStartVelocity = MovementComponent->GetVelocity();
}

void UGoKartReplicationComponent::ClearAcknowledgeMoves(FGoKartMove LastMove)
{
	TArray<FGoKartMove> NewMoves;

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		if (Move.Time > LastMove.Time)
		{
			NewMoves.Add(Move);
		}
	}

	UnacknowledgedMoves = NewMoves;
}

void UGoKartReplicationComponent::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (MovementComponent == nullptr) return;

	MovementComponent->SimulateMove(Move);

	UpdateServerState(Move);
}

bool UGoKartReplicationComponent::Server_SendMove_Validate(FGoKartMove Move)
{
	return true;
}

