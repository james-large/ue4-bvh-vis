// Fill out your copyright notice in the Description page of Project Settings.

#include "PoseUpdate.h"
#include "Joint.h"
#include "Components/PoseableMeshComponent.h"
#include "Misc/App.h"


// Sets default values for this component's properties
UPoseUpdate::UPoseUpdate()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


int countChar(FString str, char c) {
    int count = 0;

    for(int i=0; i < str.Len(); i++)  
		if(str[i] == c)
            count++;

    return count;
}


// Called when the game starts
void UPoseUpdate::BeginPlay()
{
	Super::BeginPlay();

	// FString inputFileName = FString("C:/Users/James/Documents/Unreal Projects/HelloWorld3rdPerson/Source/HelloWorld3rdPerson/right_take14_noFingers_shallow12_scale_local.bvh");
	FString inputFileName = FString("C:/Users/James/Documents/Unreal Projects/HelloWorld3rdPerson/Source/HelloWorld3rdPerson/take1_hasFingers_deep2_scale_local.bvh");
	// FString inputFileName = FString("C:/Users/James/Documents/Unreal Projects/HelloWorld3rdPerson/Source/HelloWorld3rdPerson/custom.bvh");
	
	parser.parse(inputFileName, &bvhskeleton);


	UE_LOG(LogTemp, Display, TEXT("Num joints : %d"), bvhskeleton.joints_.size());
	UE_LOG(LogTemp, Display, TEXT("Num joints names  : %d"), bvhskeleton.joint_name_lookup_.size());


	// sourceBoneNames = TArray<FName>(); 
	inputNumFrames = bvhskeleton.num_frames(); 
	// targetframeTime = bvhskeleton.frame_time();
	targetframeTime = 0.5f;
	inputFPS = 1.0 / targetframeTime;



	posableMeshComponent = Cast<UPoseableMeshComponent>(GetOwner()->GetComponentByClass(UPoseableMeshComponent::StaticClass())); 
	
	targetBoneNames = TArray<FName>(); 
	posableMeshComponent->GetBoneNames(targetBoneNames);

	BuildSkeletonMapping();

	inputFrameCounter = 0;
	frameTime = 0.0f;

	//initial frame
	bvhskeleton.recalculate_joint_transforms(inputFrameCounter);
	UpdateToNextFrame();
}


// Called every frame
void UPoseUpdate::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (NewAnimationFrame(DeltaTime)) { 
		UpdateToNextFrame();
	}

	

	//if new frame given
		//update frame counter
		//read line
		//update bone angle of source skeleton 
		//apply retargetting 
		//update bone angles of ue4 skeleton 
}




void UPoseUpdate::UpdateToNextFrame() {

	bvhskeleton.recalculate_joint_transforms(inputFrameCounter);

	frameTransforms.Empty();
	frameRotations.Empty();

	for (auto& mapping : skeletonMapping) {
		FName sourceBone = mapping.Key;
		FName targetBone = mapping.Value;

		FTransform currentTransform = posableMeshComponent->GetBoneTransformByName(targetBone, EBoneSpaces::ComponentSpace);
		
		
		// FTransform newTransform = GetBoneTransformUpdate(sourceBone);
		FRotator fix;
		fix.Roll = 0.0;
		fix.Pitch = 90.0;
		fix.Yaw = 0.0;

		FRotator originalRotation = initialTransformsUE4[targetBone].Rotator();
		FRotator newRotation = GetBoneTransformUpdate(sourceBone).Rotator(); 
		// FTransform newTransform(currentTransform.Rotator() +fix, currentTransform.GetTranslation(), currentTransform.GetScale3D()); 
		// FTransform newTransform(originalRotation, currentTransform.GetTranslation(), currentTransform.GetScale3D()); 
		// FTransform newTransform(newRotation+originalRotation, currentTransform.GetTranslation(), currentTransform.GetScale3D()); 
		FTransform newTransform(newRotation+fix, currentTransform.GetTranslation(), currentTransform.GetScale3D()); 
		// FTransform newTransform(newRotation+originalRotation+fix, currentTransform.GetTranslation(), currentTransform.GetScale3D()); 
		// FTransform newTransform = GetBoneTransformUpdate(sourceBone);

		

		// posableMeshComponent->SetBoneTransformByName(targetBone, currentTransform * newTransform, EBoneSpaces::ComponentSpace); 
		// posableMeshComponent->SetBoneTransformByName(targetBone, newTransform, EBoneSpaces::ComponentSpace); 

		
		posableMeshComponent->SetBoneRotationByName(targetBone, newTransform.Rotator(), EBoneSpaces::ComponentSpace); 

		frameTransforms.Add(targetBone, newTransform); 
		frameRotations.Add(targetBone, newRotation); 

		// WorldSpace
		// ComponentSpace

		// FRotator currentRotation = posableMeshComponent->GetBoneRotationByName(targetBone, EBoneSpaces::ComponentSpace);
		// FVector currentLocation = posableMeshComponent->GetBoneLocationByName(targetBone, EBoneSpaces::ComponentSpace);

		// FRotator newRotation = GetBoneRotationUpdate(sourceBone);
		// FVector newLocation = GetBoneLocationUpdate(sourceBone);

		// posableMeshComponent->SetBoneRotationByName(targetBone, currentRotation + newRotation, EBoneSpaces::ComponentSpace); 
		// posableMeshComponent->SetBoneLocationByName(targetBone, currentLocation + newLocation, EBoneSpaces::ComponentSpace);
		// posableMeshComponent->SetBoneRotationByName(targetBone, newRotation, EBoneSpaces::ComponentSpace); 
		// posableMeshComponent->SetBoneLocationByName(targetBone, newLocation, EBoneSpaces::ComponentSpace);
	}
}




bool UPoseUpdate::NewAnimationFrame(float DeltaTime) { 
	frameTime += DeltaTime;

	if (frameTime > targetframeTime) {
		frameTime -= targetframeTime;

		inputFrameCounter++;
		if (inputFrameCounter >= inputNumFrames)
			inputFrameCounter = 0; //todo looping for now, update eventually

		return true; 
	}

	return false;
}

FTransform UPoseUpdate::GetBoneTransformOrigin(FName boneName) {
	std::shared_ptr<bvh::Joint> joint = nullptr;

	if (bvhskeleton.have_joint(TCHAR_TO_UTF8(*boneName.ToString()))) {
		 joint = bvhskeleton.get_joint(TCHAR_TO_UTF8(*boneName.ToString())); 
	}
	else { 
		UE_LOG(LogTemp, Error, TEXT("Joint name not found in bvhskeleton : %s"), *boneName.ToString());
		return FTransform();
		//todo error handling, or this is just e.g. a tongue that we dont have, and ignore. where should that decision be made
	}

	FRotator offset(joint->offset().z, -joint->offset().x, joint->offset().y);

	return FTransform(offset);
}

FTransform UPoseUpdate::GetBoneTransformUpdate(FName boneName) {
	std::shared_ptr<bvh::Joint> joint = nullptr;

	if (bvhskeleton.have_joint(TCHAR_TO_UTF8(*boneName.ToString()))) {
		 joint = bvhskeleton.get_joint(TCHAR_TO_UTF8(*boneName.ToString())); 
	}
	else { 
		UE_LOG(LogTemp, Error, TEXT("Joint name not found in bvhskeleton : %s"), *boneName.ToString());
		return FTransform();
		//todo error handling, or this is just e.g. a tongue that we dont have, and ignore. where should that decision be made
	}

	return joint->ltm(inputFrameCounter);
}





void UPoseUpdate::BuildSkeletonMapping() {
	skeletonMapping.Add(FName(TEXT("body_world")), 	FName(TEXT("root")));
	skeletonMapping.Add(FName(TEXT("b_root")), 	FName(TEXT("pelvis")));
	skeletonMapping.Add(FName(TEXT("b_spine0")), 	FName(TEXT("spine_01")));
	// skeletonMapping.Add(FName(TEXT("b_spine1")), 	FName(TEXT("spine_02")));
	// skeletonMapping.Add(FName(TEXT("b_spine2")), 	FName(TEXT("spine_03")));
	
	// //// skeletonMapping.Add(FName(TEXT("b_spine3")), 	FName(TEXT("spine_03"))); //mismatch, 3v4 spine

	// skeletonMapping.Add(FName(TEXT("b_neck0")), 	FName(TEXT("neck_01")));
	// skeletonMapping.Add(FName(TEXT("b_head")), 		FName(TEXT("head")));

	// skeletonMapping.Add(FName(TEXT("b_l_arm")), 	FName(TEXT("upperarm_l")));
	// skeletonMapping.Add(FName(TEXT("b_l_forearm")), FName(TEXT("lowerarm_l")));
	// skeletonMapping.Add(FName(TEXT("b_l_wrist")), 	FName(TEXT("hand_l")));
	
	// skeletonMapping.Add(FName(TEXT("b_r_shoulder")), 	FName(TEXT("clavicle_r")));
	skeletonMapping.Add(FName(TEXT("b_r_arm")), 	FName(TEXT("upperarm_r")));
	skeletonMapping.Add(FName(TEXT("b_r_arm_twist")), 	FName(TEXT("upperarm_twist_01_r")));
	skeletonMapping.Add(FName(TEXT("b_r_forearm")), FName(TEXT("lowerarm_r")));
	skeletonMapping.Add(FName(TEXT("b_r_wrist_twist")), FName(TEXT("lowerarm_twist_01_r")));
	skeletonMapping.Add(FName(TEXT("b_r_wrist")), 	FName(TEXT("hand_r")));

	
	// skeletonMapping.Add(FName(TEXT("b_l_upleg")), 	FName(TEXT("thigh_l")));
	// skeletonMapping.Add(FName(TEXT("b_l_leg")), FName(TEXT("calf_l")));
	// skeletonMapping.Add(FName(TEXT("b_l_foot_twist")), 	FName(TEXT("calf_twist_01_l")));
	// skeletonMapping.Add(FName(TEXT("b_l_foot")), 	FName(TEXT("foot_l")));
	
	// skeletonMapping.Add(FName(TEXT("b_r_upleg")), 	FName(TEXT("thigh_r")));
	// skeletonMapping.Add(FName(TEXT("b_r_leg")), FName(TEXT("calf_r")));
	// skeletonMapping.Add(FName(TEXT("b_r_foot_twist")), 	FName(TEXT("calf_twist_01_r")));
	// skeletonMapping.Add(FName(TEXT("b_r_foot")), 	FName(TEXT("foot_r")));

	for (auto& mapping : skeletonMapping) {
		FName sourceBone = mapping.Key;
		FName targetBone = mapping.Value;


		FTransform currentTransformBVH = posableMeshComponent->GetBoneTransformByName(sourceBone, EBoneSpaces::ComponentSpace);
		initialTransformsBVH.Add(targetBone, currentTransformBVH);

		FTransform currentTransformUE4 = posableMeshComponent->GetBoneTransformByName(targetBone, EBoneSpaces::ComponentSpace);
		initialTransformsUE4.Add(targetBone, currentTransformUE4);

		
	}



// void UPoseUpdate::BuildSkeletonMapping() {
// 	skeletonMapping.Add(FName(TEXT("body_world")), 	FName(TEXT("root")));
// 	skeletonMapping.Add(FName(TEXT("b_root")), 	FName(TEXT("pelvis")));
// 	skeletonMapping.Add(FName(TEXT("b_spine0")), 	FName(TEXT("spine_01")));
// 	skeletonMapping.Add(FName(TEXT("b_spine1")), 	FName(TEXT("spine_02")));
// 	skeletonMapping.Add(FName(TEXT("b_spine2")), 	FName(TEXT("spine_03")));
	

// 	for (auto& mapping : skeletonMapping) {
// 		FName sourceBone = mapping.Key;
// 		FName targetBone = mapping.Value;


// 		// FTransform currentTransformBVH = posableMeshComponent->GetBoneTransformByName(sourceBone, EBoneSpaces::ComponentSpace);
// 		// initialTransformsBVH.Add(targetBone, currentTransformBVH);

// 		FTransform currentTransformUE4 = posableMeshComponent->GetBoneTransformByName(targetBone, EBoneSpaces::ComponentSpace);
// 		initialTransformsUE4.Add(targetBone, currentTransformUE4);

		
// 	}






}

















// FRotator UPoseUpdate::GetBoneRotationUpdate(FName boneName) {
// 	std::shared_ptr<bvh::Joint> joint = nullptr;

// 	if (bvhskeleton.have_joint(TCHAR_TO_UTF8(*boneName.ToString()))) {
// 		 joint = bvhskeleton.get_joint(TCHAR_TO_UTF8(*boneName.ToString())); 
// 	}
// 	else { 
// 		UE_LOG(LogTemp, Error, TEXT("Joint name not found in bvhskeleton : %s"), *boneName.ToString());
// 		return FRotator(0.0f, 0.0f, 0.0f);
// 		//todo error handling, or this is just e.g. a tongue that we dont have, and ignore. where should that decision be made
// 	}

// 	// GetWorld()->bDebugPauseExecution = true;

// 	float xRot = joint->channel_data(inputFrameCounter, bvh::Joint::Channel::XROTATION);
// 	float yRot = joint->channel_data(inputFrameCounter, bvh::Joint::Channel::YROTATION);
// 	float zRot = joint->channel_data(inputFrameCounter, bvh::Joint::Channel::ZROTATION);

// 	UE_LOG(LogTemp, Display, TEXT("%s rotation update: %.4f %.4f %.4f"), *boneName.ToString(), xRot, yRot, zRot)

// 	return FRotator(xRot, yRot, zRot);
// }

// FVector UPoseUpdate::GetBoneLocationUpdate(FName boneName) { 
// 	std::shared_ptr<bvh::Joint> joint = nullptr;

// 	if (bvhskeleton.have_joint(TCHAR_TO_UTF8(*boneName.ToString()))) {
// 		 joint = bvhskeleton.get_joint(TCHAR_TO_UTF8(*boneName.ToString())); 
// 	}
// 	else { 
// 		UE_LOG(LogTemp, Error, TEXT("Joint name not found in bvhskeleton : %s"), *boneName.ToString());
// 		return FVector(0.0f, 0.0f, 0.0f);
// 		//todo error handling, or this is just e.g. a tongue that we dont have, and ignore. where should that decision be made
// 	}

// 	// GetWorld()->bDebugPauseExecution = true;

// 	float xOffset = joint->offset().x;
// 	float yOffset = joint->offset().y;
// 	float zOffset = joint->offset().z;

// 	float xLoc = joint->channel_data(inputFrameCounter, bvh::Joint::Channel::XPOSITION);
// 	float yLoc = joint->channel_data(inputFrameCounter, bvh::Joint::Channel::YPOSITION);
// 	float zLoc = joint->channel_data(inputFrameCounter, bvh::Joint::Channel::ZPOSITION);

// 	UE_LOG(LogTemp, Display, TEXT("%s position update: %.4f %.4f %.4f"), *boneName.ToString(), xLoc, yLoc, zLoc)

// 	return FVector(xOffset+xLoc, yOffset+yLoc, zOffset+zLoc);
// }

