#include "SpartaGameState.h"
#include "CoinItem.h"
#include "SpartaGameInstance.h"
#include "SpartaPlayerController.h"
#include "SpawnVolume.h"
#include "../../../../../Games/UE_5.6/Engine/Intermediate/Build/Win64/x64/UnrealEditor/Development/ChaosVehiclesEngine/Definitions.ChaosVehiclesEngine.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

ASpartaGameState::ASpartaGameState()
{
	Score = 0;
	SpawnedCoinCount = 0;
	CollectedCoinCount = 0;
	WaveDuration = 30.0f;
	CurrentLevelIndex = 0;
	MaxLevels = 3;
	MaxWave = 3;
	LevelMapNames = { "Level1", "Level2", "Level3"};
}

void ASpartaGameState::BeginPlay()
{
	Super::BeginPlay();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
			FString::Printf(TEXT("LevelMapNames.Num(): %d | CurrentLevelIndex: %d"),
				LevelMapNames.Num(), CurrentLevelIndex));
	}
	
	StartLevel();

	if (!GetWorldTimerManager().IsTimerActive(HUDUpdateTimerHandle))
	{
		GetWorldTimerManager().SetTimer(
			HUDUpdateTimerHandle,
			this,
			&ASpartaGameState::UpdateHUD,
			0.1f,
			true
		);		
	}
}

int32 ASpartaGameState::GetScore() const
{
	return Score;
}

void ASpartaGameState::AddScore(int32 Amount)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			SpartaGameInstance->AddToScore(Amount);
		}
	}
}

void ASpartaGameState::StartLevel()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASpartaPlayerController* SpartaPlayerController = Cast<ASpartaPlayerController>(PlayerController))
		{
			SpartaPlayerController->ShowGameHUD();
		}
	}
	
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			CurrentLevelIndex = SpartaGameInstance->CurrentLevelIndex;
		}
	}

	CurrentWaveIndex = 0;
	StartWave();
}

void ASpartaGameState::StartWave()
{
	CurrentWaveIndex++;
	SpawnedCoinCount = 0;
	CollectedCoinCount = 0;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("Wave %d Start!"), CurrentWaveIndex));	
	}
	
	TArray<AActor*> FoundVolumes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnVolume::StaticClass(), FoundVolumes);

	int32 ItemToSpawn = 20 + (CurrentWaveIndex * 10);

	if (FoundVolumes.Num() > 0)
	{
		ASpawnVolume* SpawnVolume = Cast<ASpawnVolume>(FoundVolumes[0]);
		if (SpawnVolume)
		{
			for (int32 i = 0; i < ItemToSpawn; i++)
			{
				AActor* SpawnedActor = SpawnVolume->SpawnRandomItem();
				if (SpawnedActor && SpawnedActor->IsA(ACoinItem::StaticClass()))
				{
					SpawnedCoinCount++;
				}
			}
		}
	}

	GetWorldTimerManager().SetTimer(
		WaveTimerHandle,
		this,
		&ASpartaGameState::OnWaveTimeUp,
		WaveDuration,
		false
	);
}

void ASpartaGameState::OnLevelTimeUp()
{
	EndLevel();
}

void ASpartaGameState::OnWaveTimeUp()
{
	if (CurrentWaveIndex < MaxWave)
	{
		StartWave();
	}
	else
	{
		EndLevel();
	}
}

void ASpartaGameState::OnCoinCollected()
{
	CollectedCoinCount++;

	if (SpawnedCoinCount > 0 && CollectedCoinCount >= SpawnedCoinCount)
	{
		GetWorldTimerManager().ClearTimer(WaveTimerHandle);

		if (CurrentWaveIndex < MaxWave)
		{
			StartWave();
		}
		else
		{
			EndLevel();	
		}
	}
}

void ASpartaGameState::EndLevel()
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			AddScore(Score);
			CurrentLevelIndex++;
			SpartaGameInstance->CurrentLevelIndex = CurrentLevelIndex;
		}

		if (CurrentLevelIndex >= MaxLevels)
		{
			OnGameOver();
			return;
		}

		if (LevelMapNames.IsValidIndex(CurrentLevelIndex))
		{
			UGameplayStatics::OpenLevel(GetWorld(), LevelMapNames[CurrentLevelIndex]);
		}
		else
		{
			OnGameOver();
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("EndLevel() ½ÇÇàµÊ. ÇöÀç ÀÎµ¦½º: %d"), CurrentLevelIndex);
}

void ASpartaGameState::OnGameOver()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASpartaPlayerController* SpartaPlayerController = Cast<ASpartaPlayerController>(PlayerController))
		{
			SpartaPlayerController->SetPause(true);
			SpartaPlayerController->ShowMainMenu(true);
		}
	}
}

void ASpartaGameState::UpdateHUD()
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->IsPendingKillEnabled() || World->bIsTearingDown)
	{
		GetWorldTimerManager().ClearTimer(HUDUpdateTimerHandle);
		return;
	}
	
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASpartaPlayerController* SpartaPlayerController = Cast<ASpartaPlayerController>(PlayerController))
		{
			UUserWidget* HUDWidget = SpartaPlayerController->GetHUDWiget();
			if (!IsValid(HUDWidget) || HUDWidget->IsPendingKillEnabled() || !HUDWidget->IsInViewport())
			{
				GetWorldTimerManager().ClearTimer(HUDUpdateTimerHandle);
				return;
			}
			UTextBlock* TimeText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Time")));
			if (IsValid(TimeText))
			{
				float RemainingTime = GetWorldTimerManager().GetTimerRemaining(WaveTimerHandle);
				TimeText->SetText(FText::FromString(FString::Printf(TEXT("Time: %.1f"), RemainingTime)));
			}

			if (UTextBlock* ScoreText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Score"))))
			{
				if (UGameInstance* GameInstance = GetGameInstance())
				{
					USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
					if (SpartaGameInstance)
					{
						ScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score: %d"), SpartaGameInstance->TotalScore)));
					}
				}
			}

			if (UTextBlock* LevelIndexText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Level"))))
			{
				LevelIndexText->SetText(FText::FromString(FString::Printf(TEXT("Level: %d"), CurrentLevelIndex + 1)));
			}
		}
	}
}
