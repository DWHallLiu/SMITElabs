// Fill out your copyright notice in the Description page of Project Settings.


#include "SLAgni.h"

ASLAgni::ASLAgni()
{
	UnitName = "Agni";
	BaseMovementSpeed = 355;
	BaseBasicAttackDamage = 34;
	BasicAttackDamagePerLevel = 1.5;
	BasicAttackPowerScaling = 0.2;
	BaseBasicAttackSpeed = 1;
	BasicAttackSpeedPerLevel = .012;
	BasicAttackRefireProgression = { 1 };
	BasicAttackPrefireProgression = { .15 };
	BasicAttackRangeProgression = { 55 };
	BasicAttackDamageProgression = { 1 };
	BasicAttackDisjointProgression = { 0, 0 };
	RangedBasicAttackProjectileSizeProgression = { 3 };
	RangedBasicAttackProjectileSpeedProgression = { 100 };
	bCleaveProgression = { false };
	CleaveDamageProgression = { 0 };
	RangedCleaveRangeProgression = { 0 };
	bIsBasicAttackRangedProgression = { true };
	bHasScalingPrefireProgression = { false };
	bIsPhysicalDamage = false;
	BaseHealth = 360;
	BasePhysicalProtections = 11;
	PhysicalProtectionsPerLevel = 2.6;
	BaseMagicalProtections = 30;
	MagicalProtectionsPerLevel = .9;
	BaseHealthPerFive = 7;
	HealthPerFivePerLevel = .47;
	Ability1Cooldowns = { 12, 12, 12, 12, 12 };
	Ability2Cooldowns = { 15, 14, 13, 12, 11 };
	Ability3Cooldowns = { 15, 15, 15, 15, 15 };
	Ability4Cooldowns = { 18, 18, 18, 18, 18 };
	Ability1ManaCost = { 60, 65, 70, 75, 80 };
	Ability2ManaCost = { 60, 70, 80, 90, 100 };
	Ability3ManaCost = { 70, 75, 80, 85, 90 };
	Ability4ManaCost = { 0, 0, 0, 0, 0 };

	Ability1TargeterComponents.Add(CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NoxiousFumesTargeter")));
	Ability1TargeterComponents[0]->SetupAttachment(AbilityAimComponent);
	Ability1TargeterComponents[0]->SetVisibility(false);

	Ability2TargeterComponents.Add(CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlameWaveTargeter")));
	Ability2TargeterComponents[0]->SetupAttachment(RootComponent);
	Ability2TargeterComponents[0]->SetVisibility(false);

	Ability3TargeterComponents.Add(CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PathOfFlamesTargeter")));
	Ability3TargeterComponents[0]->SetupAttachment(RootComponent);
	Ability3TargeterComponents[0]->SetVisibility(false);

	Ability4TargeterComponents.Add(CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RainFireTargeter")));
	Ability4TargeterComponents[0]->SetupAttachment(AbilityAimComponent);
	Ability4TargeterComponents[0]->SetVisibility(false);

	RainFireTimerDelegate.BindUFunction(this, FName("AddUltCharge"), false);
}

void ASLAgni::OnBasicAttackHit(TArray<ISLVulnerable*> Targets)
{
	Super::OnBasicAttackHit(Targets);

	for (ISLVulnerable* CurrentTarget : Targets)
	{
		if (CombustionCount < 4)
		{ 
			++CombustionCount;
			if (CombustionCount >= 4)GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Combustion is ready!"));
			else GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Combustion Passive Count: %i"), CombustionCount));
		}
	}
}

void ASLAgni::UseAbility1()
{
	Ability1TargeterComponents[0]->SetVisibility(false);
	if (!IsAbilityAvailable(Ability1CooldownTimerHandle, Ability1Level, Ability1ManaCost, "Noxious Fumes [1]", bAbility1IsPrimed)) return;
	ActivateCooldownTimer(Ability1CooldownTimerHandle, Ability1Cooldowns[Ability1Level - 1], "Noxious Fumes [1]", Ability1ManaCost[Ability1Level - 1], true);
}

void ASLAgni::UseAbility2()
{
	Ability2TargeterComponents[0]->SetVisibility(false);
	if (!IsAbilityAvailable(Ability2CooldownTimerHandle, Ability2Level, Ability2ManaCost, "Flame Wave [2]", bAbility2IsPrimed)) return;
	if (CombustionCount >= 4)
	{
		ConsumeCombustionStacks();
	}
	ActivateCooldownTimer(Ability2CooldownTimerHandle, Ability2Cooldowns[Ability2Level - 1], "Flame Wave [2]", Ability2ManaCost[Ability2Level - 1], true);
	
}

void ASLAgni::UseAbility3()
{
	Ability3TargeterComponents[0]->SetVisibility(false);
	if (!IsAbilityAvailable(Ability3CooldownTimerHandle, Ability3Level, Ability3ManaCost, "Path of Flames [3]", bAbility3IsPrimed)) return;
	ActivateCooldownTimer(Ability3CooldownTimerHandle, Ability3Cooldowns[Ability3Level - 1], "Path of Flames [3]", Ability3ManaCost[Ability3Level - 1], true);
}

void ASLAgni::UseAbility4()
{
	//TODO Prevent using ability at Rank 0
	if (!bAbility4IsPrimed) return;
	Ability4TargeterComponents[0]->SetVisibility(false);
	if (Ability4Level <= 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("No points in Rain Fire [Ult].")));
		return;
	}
	if (UltCharges <= 0)
	{ 
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Rain Fire has no charges available for %fs."), GetWorld()->GetTimerManager().GetTimerRemaining(Ability4CooldownTimerHandle)));
		return;
	}

	if (CombustionCount >= 4)
	{
		ConsumeCombustionStacks();
	}
	--UltCharges;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Rain fire used! %i/3 charges left."), UltCharges));
	CheckRainFireTimer();
}

void ASLAgni::LevelAbility4()
{
	Super::LevelAbility4();

	if (Ability4Level == 1) AddUltCharge();
}

void ASLAgni::AimAbility4()
{
	if (UltCharges > 0)
	{
		for (UStaticMeshComponent* SMC : Ability4TargeterComponents)
		{
			SMC->SetVisibility(true);
			bAbility4IsPrimed = true;
		}
	}
}

void ASLAgni::ConsumeCombustionStacks()
{
	CombustionCount = 0;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Combustion Stacks Consumed."));
}

void ASLAgni::CheckRainFireTimer()
{
	if (!GetWorld()->GetTimerManager().IsTimerActive(Ability4CooldownTimerHandle) && UltCharges < 3) GetWorld()->GetTimerManager().SetTimer(Ability4CooldownTimerHandle, RainFireTimerDelegate, Ability4Cooldowns[Ability4Level - 1] * (1 - CooldownReductionPercentage), false);
}

void ASLAgni::AddUltCharge()
{
	++UltCharges;
	GetWorld()->GetTimerManager().ClearTimer(Ability4CooldownTimerHandle);
	CheckRainFireTimer();
}