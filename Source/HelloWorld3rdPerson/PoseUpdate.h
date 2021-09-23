// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BvHParser.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PoseUpdate.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HELLOWORLD3RDPERSON_API UPoseUpdate : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPoseUpdate();

	UPROPERTY(BlueprintReadWrite)
	int inputNumFrames; 

	UPROPERTY(BlueprintReadWrite)
	int inputFPS; 

	UPROPERTY(BlueprintReadWrite)
	int inputFrameCounter; 
	
	UPROPERTY(BlueprintReadWrite)
	float frameTime; 

	// Access to the bones and their transforms
	UPROPERTY(BlueprintReadWrite)
	class UPoseableMeshComponent* posableMeshComponent; 

	// Names extracted from the attatched model
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> targetBoneNames;

	// Mapping from input (bvh) bones to output (ue4 model) bones, the retargetting
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, FName> skeletonMapping; 

	// Parser for reading in the bvh input
	bvh::BvHParser parser; 

	// Structure storing the bvh hierarchy and frame data
	bvh::BvHSkeleton bvhskeleton; 


	//##############################################################################
	// Scene accessors/debugging
	//##############################################################################
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, FTransform> frameTransforms;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, FRotator> frameRotations;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, FTransform> initialTransformsUE4;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, FTransform> initialTransformsBVH;

	//UFUNCTION(BlueprintCallable)
	//void UpdateBones() { }


protected:

	float targetframeTime; 

	// Called when the game starts
	virtual void BeginPlay() override;

	void BuildSkeletonMapping();
	void UpdateToNextFrame();
	bool NewAnimationFrame(float DeltaTime);
	FTransform GetBoneTransformOrigin(FName boneName); 
	FTransform GetBoneTransformUpdate(FName boneName); 
	// FRotator GetBoneRotationUpdate(FName boneName); 
	// FVector GetBoneLocationUpdate(FName boneName); 

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
