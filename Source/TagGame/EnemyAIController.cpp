// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TagGameGameMode.h"

AEnemyAIController::AEnemyAIController()
{
	BlackboardAsset = NewObject<UBlackboardData>();
	BlackboardAsset->UpdatePersistentKey<UBlackboardKeyType_Object>(FName("Target"));
	BlackboardAsset->UpdatePersistentKey<UBlackboardKeyType_Object>(FName("TargetBall"));
	BlackboardAsset->UpdatePersistentKey<UBlackboardKeyType_Object>(FName("TargetCover"));

	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	BlackboardComponent->SetValueAsObject("Target", this->GetWorld()->GetFirstPlayerController()->GetPawn());

	GoToPlayer = MakeShared<FAivState>(
		[this]() {

			AActor* Player = Cast<AActor>(BlackboardComponent->GetValueAsObject("Target"));
			MoveToActor(Player, 50.0f);
		},
		nullptr,
		[this](const float DeltaTime) -> TSharedPtr<FAivState> {
			EPathFollowingStatus::Type State = GetMoveStatus();

			if (State == EPathFollowingStatus::Moving)
			{
				return nullptr;
			}

			if (BlackboardComponent->GetValueAsObject("TargetBall"))
			{
				AActor* BestBall = Cast<AActor>(BlackboardComponent->GetValueAsObject("TargetBall"));
				BestBall->AttachToActor(GetWorld()->GetFirstPlayerController()->GetPawn(), FAttachmentTransformRules::KeepRelativeTransform);
				BestBall->SetActorRelativeLocation(FVector(0, 0, 0));
				BlackboardComponent->SetValueAsObject("TargetBall", nullptr);
			}

			return SearchForCoverPoint;
		}
	);

	SearchForBall = MakeShared<FAivState>(
		[this]() 
		{
			AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
			const ATagGameGameMode* AIGameMode = Cast<ATagGameGameMode>(GameMode);

			const TArray<ABall*>& BallsList = AIGameMode->GetBalls();

			float MaxDistance = INT_MAX;
			ABall* BestBall = nullptr;

			for (ABall* Ball : BallsList)
			{
				if (!Ball->GetAttachParentActor())
				{
					const float Distance = FVector::Distance(GetPawn()->GetActorLocation(), Ball->GetActorLocation());

					if (Distance < MaxDistance)
					{
						BestBall = Ball;
						MaxDistance = Distance;
					}
				}
			}
			
			BlackboardComponent->SetValueAsObject("TargetBall", BestBall);
		},
		[this]()
		{
			CurrentState->CallEnter();
		},
		[this](const float DeltaTime) -> TSharedPtr<FAivState> {
			if (BlackboardComponent->GetValueAsObject("TargetBall"))
			{
				return GoToBall;
			}
			CurrentState->CallExit();
			return SearchForBall;
			
		}
	);

	GoToBall = MakeShared<FAivState>(
		[this]() {
			MoveToActor(Cast<AActor>(BlackboardComponent->GetValueAsObject("TargetBall")), 50.0f);
		},
		nullptr,
		[this](const float DeltaTime) -> TSharedPtr<FAivState> {
			EPathFollowingStatus::Type State = GetMoveStatus();

			if (State == EPathFollowingStatus::Moving)
			{
				return nullptr;
			}
			if (Cast<AActor>(BlackboardComponent->GetValueAsObject("TargetBall"))->GetAttachParentActor())
			{
				return SearchForBall;
			}
			return GrabBall;
		}
	);

	GrabBall = MakeShared<FAivState>(
		[this]()
		{
			if (Cast<AActor>(BlackboardComponent->GetValueAsObject("TargetBall"))->GetAttachParentActor())
			{
				//BestBall = nullptr;
				BlackboardComponent->SetValueAsObject("TargetBall", nullptr);
			}
		},
		nullptr,
		[this](const float DeltaTime) -> TSharedPtr<FAivState> {

			if (!BlackboardComponent->GetValueAsObject("TargetBall"))
			{
				return SearchForBall;
			}

			AActor* BestBall = Cast<AActor>(BlackboardComponent->GetValueAsObject("TargetBall"));
			BestBall->AttachToActor(GetPawn(), FAttachmentTransformRules::KeepRelativeTransform);
			BestBall->SetActorRelativeLocation(FVector(0, 0, 0));

			if (BestBall->GetAttachParentActor() != GetPawn())
			{
				return SearchForBall;
			}

			return GoToPlayer;
		}
	);

	SearchForCoverPoint = MakeShared<FAivState>(
		[this]()
		{
			AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
			const ATagGameGameMode* AIGameMode = Cast<ATagGameGameMode>(GameMode);

			const TArray<ATargetPoint*>& RandomCoverPoints = AIGameMode->GetCoverPoints();

			float MaxDistance = INT_MAX;
			ATargetPoint* BestCover = nullptr;
			for (ATargetPoint* Cover : RandomCoverPoints)
			{
				if (!Cover->GetAttachParentActor())
				{
					const float Distance = FVector::Distance(GetPawn()->GetActorLocation(), Cover->GetActorLocation());

					if (Distance < MaxDistance)
					{
						BestCover = Cover;
						MaxDistance = Distance;
					}
				}
			}

			BlackboardComponent->SetValueAsObject("TargetCover", BestCover);
		},
		[this]()
		{
			CurrentState->CallEnter();
		},
		[this](const float DeltaTime) -> TSharedPtr<FAivState> {

			if (BlackboardComponent->GetValueAsObject("TargetCover") && !Cast<ATargetPoint>(BlackboardComponent->GetValueAsObject("TargetCover"))->GetAttachParentActor())
			{
				return GoToCover;
			}
			CurrentState->CallExit();
			return SearchForCoverPoint;
		});

	GoToCover = MakeShared<FAivState>(
		[this]() {
			MoveToActor(Cast<ATargetPoint>(BlackboardComponent->GetValueAsObject("TargetCover")), 50.0f);
		},
		nullptr,
		[this](const float DeltaTime) -> TSharedPtr<FAivState> {
			EPathFollowingStatus::Type State = GetMoveStatus();

			if (State == EPathFollowingStatus::Moving)
			{
				return nullptr;
			}

			if (Cast<ATargetPoint>(BlackboardComponent->GetValueAsObject("TargetCover"))->GetAttachParentActor())
			{
				BlackboardComponent->SetValueAsObject("TargetCover", nullptr);
				return SearchForCoverPoint;
			}
			Cast<ATargetPoint>(BlackboardComponent->GetValueAsObject("TargetCover"))->AttachToActor(GetPawn(), FAttachmentTransformRules::KeepWorldTransform);

			return GoToShootDistance;
		}
	);

	GoToShootDistance = MakeShared<FAivState>(
		[this]() {

			AActor* Player = Cast<AActor>(BlackboardComponent->GetValueAsObject("Target"));
			MoveToActor(Player, ShootDistance);
		},
		[this]()
		{
			CurrentState->CallEnter();
		},
		[this](const float DeltaTime) -> TSharedPtr<FAivState> {
			EPathFollowingStatus::Type State = GetMoveStatus();

			if (State == EPathFollowingStatus::Moving)
			{
				return nullptr;
			}

			float ActualDistance = FVector::Distance(GetPawn()->GetActorLocation(), GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation());

			if (ActualDistance > ShootDistance)
			{
				CurrentState->CallExit();
				return GoToShootDistance;
			}

			return ShootToPlayer;

		});

	CurrentState = SearchForBall;
	CurrentState->CallEnter();
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState)
	{
		CurrentState = CurrentState->CallTick(DeltaTime);
	}
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	BlackboardComponent->InitializeBlackboard(*BlackboardAsset);
}
