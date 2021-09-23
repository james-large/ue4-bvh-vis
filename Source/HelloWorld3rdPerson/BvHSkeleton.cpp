#include "BVHSkeleton.h"

// // #include "utils.h"

#include "CoreMinimal.h"
#include "Math/UnrealMathVectorConstants.h"
#include "Math.h"
#include <map>

namespace bvh {

enum class Axis {
  X,
  Y,
  Z
};

float radians(float angle) {
    return (angle * 3.141592) / 180;
}

FMatrix rotation_matrix(Axis axis, float angle) {
    const float eps = std::numeric_limits<float>::epsilon(); 

    float rangle = radians(angle);

    float sin_a, cos_a; 
    FMath::SinCos(&sin_a, &cos_a, rangle); 
    float msin_a = fabs(sin_a) < eps ?
        0.0f : (-1.0f) * sin_a;

    if (fabs(sin_a) < eps) sin_a = 0.0f;
    if (fabs(cos_a) < eps) cos_a = 0.0f;

    FMatrix matrix; 
    matrix.SetIdentity();

    if (axis == Axis::X) {
        matrix.M[1][1] = cos_a;
        matrix.M[1][2] = msin_a;//sin_a
        matrix.M[2][1] = sin_a;//msin_a
        matrix.M[2][2] = cos_a;
    } else if (axis == Axis::Y) {
        matrix.M[0][0] = cos_a;
        matrix.M[0][2] = sin_a;//msin_a
        matrix.M[2][0] = msin_a;//sin_a
        matrix.M[2][2] = cos_a;
    } else {
        matrix.M[0][0] = cos_a;
        matrix.M[0][1] = msin_a;//sin_a
        matrix.M[1][0] = sin_a;//msin_a
        matrix.M[1][1] = cos_a;
    }

    return matrix;
}

FMatrix calc_rotation(const std::map<Axis, float> &ordered_rotations) {  
    FMatrix comp_matrix; 
    comp_matrix.SetIdentity();

    for (auto iter = ordered_rotations.rbegin(); iter != ordered_rotations.rend(); ++iter) {
        Axis axis = iter->first; 
        float angle = iter->second; 

        FMatrix axis_matrix = rotation_matrix(axis, angle);

        comp_matrix *= axis_matrix;

        
        //UE_LOG(LogTemp, Display, TEXT("Bad structure of .bvh file. %s should be on the top of the file"), *FString(kHierarchy.c_str())); 
    }

    return comp_matrix;
}




void BvHSkeleton::recalculate_joint_transforms(unsigned frame, std::shared_ptr<Joint> current_joint) {

    if (current_joint == NULL) {
        if (root_joint_ == NULL)
            return;
        else
            current_joint = root_joint_;
    }

    std::vector<float> frameData = current_joint->channel_data()[frame];
    std::vector<float> prevFrameData = frame > 0 ? current_joint->channel_data()[frame-1] : std::vector<float>(6); 


    FVector offset(0.0, 0.0, 0.0);
    // FVector offset = FVector(
    //     current_joint->offset().z, 
    //     -current_joint->offset().x,
    //     current_joint->offset().y);
    FVector translation;
    FRotator rotation;

    std::map<Axis, float> ordered_rotations; 

    FMatrix rotationMatrix; 
    rotationMatrix.SetIdentity();
    
    FTransform offsetTransform(offset);

    /*************************************
     * right hand, y up, to left hand, z up
     * 
     */
    for (int j = 0;  j < current_joint->channels_order().size(); j++) {
        if (current_joint->channels_order()[j] == Joint::Channel::XPOSITION) {
            // translation.Y = frameData[j] - prevFrameData[j];
            translation.Y = -frameData[j];
        }
        else if (current_joint->channels_order()[j] == Joint::Channel::YPOSITION) {
            // translation.Z = frameData[j] - prevFrameData[j];
            translation.Z = frameData[j];
        }
        else if (current_joint->channels_order()[j] == Joint::Channel::ZPOSITION) {
            // translation.X = -frameData[j] - prevFrameData[j];
            translation.X = frameData[j];
        }
        else if (current_joint->channels_order()[j] == Joint::Channel::XROTATION) {
            // rotation.Pitch = frameData[j] - prevFrameData[j];
            // rotation.Pitch = -frameData[j];
            
            // rotationMatrix *= rotation_matrix(Axis::Y, -frameData[j]);  
            ordered_rotations[Axis::Y] = -frameData[j]; 
        }
        else if (current_joint->channels_order()[j] == Joint::Channel::YROTATION) {
            // rotation.Yaw = frameData[j] - prevFrameData[j];
            // rotation.Yaw = frameData[j];
            
            // rotationMatrix *= rotation_matrix(Axis::Z, frameData[j]); 
            ordered_rotations[Axis::Z] = frameData[j]; 
        }
        else if (current_joint->channels_order()[j] == Joint::Channel::ZROTATION) {
            // rotation.Roll = -frameData[j] - prevFrameData[j];
            // rotation.Roll = frameData[j];
            
            // rotationMatrix *= rotation_matrix(Axis::X, frameData[j]); 
            ordered_rotations[Axis::X] = frameData[j]; 
        }
    }

    rotationMatrix = calc_rotation(ordered_rotations); 

    // FTransform translationTransform(FVector(.0f,.0f,.0f));
    FTransform translationTransform(translation);

    // rotation.Normalize(); 
    // FTransform rotationTransform(rotation);


    // rotationMatrix = rotation_matrix(Axis::X, 75);
    FTransform rotationTransform(rotationMatrix);
    // FRotator testRotationX = rotationTransform.Rotator(); 

    
    // rotationMatrix = rotation_matrix(Axis::Y, 35);
    // rotationTransform = FTransform(rotationMatrix);
    // FRotator testRotationY = rotationTransform.Rotator(); 

    
    // rotationMatrix = rotation_matrix(Axis::Z, 19);
    // rotationTransform = FTransform(rotationMatrix);
    // FRotator testRotationZ = rotationTransform.Rotator(); 

    rotationTransform.NormalizeRotation();

    FTransform joint_transform;

    // if this joint has a parent, inherit its transform and offset from it
    if (current_joint->parent() != NULL) {
        FTransform parentTransform = current_joint->parent()->ltm(frame); 

        // if (!parentTransform.IsRotationNormalized()) {
        //     parentTransform.NormalizeRotation();
        // }
        // if (!joint_transform.IsRotationNormalized()) {
        //     joint_transform.NormalizeRotation();
        // }
        // if (!offsetTransform.IsRotationNormalized()) {
        //     offsetTransform.NormalizeRotation();
        // }
        joint_transform = parentTransform * offsetTransform;  
    }
    else // otherwise we're at the root, take the raw world translation
        joint_transform = translationTransform * offsetTransform;

    joint_transform *= rotationTransform;
    joint_transform.NormalizeRotation();

    current_joint->set_ltm(joint_transform, frame);

    for (auto& child : current_joint->children()) {
        recalculate_joint_transforms(frame, child);
    }
}
}





    // for (int j = 0;  j < current_joint->channels_order().size(); j++) {
    //     if (current_joint->channels_order()[j] == Joint::Channel::XPOSITION)
    //         translation.X = frameData[j];
    //     else if (current_joint->channels_order()[j] == Joint::Channel::YPOSITION)
    //         translation.Y = frameData[j];
    //     else if (current_joint->channels_order()[j] == Joint::Channel::ZPOSITION)
    //         translation.Z = frameData[j];
    //     else if (current_joint->channels_order()[j] == Joint::Channel::XROTATION)
    //         rotation.Pitch = frameData[j];
    //     else if (current_joint->channels_order()[j] == Joint::Channel::YROTATION)
    //         rotation.Roll = frameData[j];
    //     else if (current_joint->channels_order()[j] == Joint::Channel::ZROTATION)
    //         rotation.Yaw = frameData[j];
    // }