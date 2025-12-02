#include "vehicle.h"
#include "audio.h"
#include "input.h"
#include "physics.h"
#include "render.h"
#include "convert.h"
#include "model.h"

#include "../vendor/imgui/imgui.h"
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <vector>

static std::vector<Vehicle*> existingVehicles;
VehicleSettings *curLoadingVehicleSettings = nullptr;


static void VehiclePostCollideCallback(JPH::VehicleConstraint &inVehicle, const JPH::PhysicsStepListenerContext &inContext)
{
    //Vehicle *car = GetVehicleFromVehicleConstraint(&inVehicle);
    //SDL_assert(car != nullptr); // Have all vehicles been created with
                                // CreateVehicle()?
    // Audio stuff
    //JPH::WheeledVehicleController *controller = static_cast<JPH::WheeledVehicleController*>
    //    (inVehicle.GetController());
    //JPH::VehicleEngine &engine = controller->GetEngine();
    /*
    float rpm = engine.GetCurrentRPM();
    SDL_SetAudioStreamFrequencyRatio(car->engineSnd->stream, rpm / 2000.0f + 0.5);
    //SDL_SetAudioStreamGain(car->engineSnd->stream, rpm / 3000.0f + 0.3);
    SDL_SetAudioStreamGain(car->engineSnd->stream, 0);

    float averageSlip = 0;
    float slipLong;
    //Wheel *wheel1 = vehicleConstraint->GetWheels()[0];
    //SDL_Log("ang vel: %f", wheel1->GetAngularVelocity());
    for (JPH::Wheel *wheel : inVehicle.GetWheels()) {
        JPH::WheelWV *wheel2 = static_cast<JPH::WheelWV*> (wheel);
        if (!wheel->HasContact()) {
            continue;
        }
        JPH::Vec3 wPos = JPH::Vec3(car->mBody->GetWorldTransform() * JPH::Vec4(wheel->GetSettings()->mPosition, 1.0f));
        JPH::Vec3 wVel = car->mBody->GetPointVelocity(wPos);
        //float wheelLongVel = wheel->GetAngularVelocity() * wheel->GetSettings()->mRadius;
        //slipLong = SDL_fabsf(wheelLongVel) - SDL_fabsf(wVel.Dot(wheel->GetContactLongitudinal()));
        //slipLong = SDL_fabsf(slipLong);
        //SDL_Log("%f", slipLong);

        slipLong = wheel2->mLongitudinalSlip;

        //float slipLat = wVel.Dot(wheel->GetContactLateral());

        float slipLat = SDL_sin(wheel2->mLateralSlip) * wVel.Length();

        float slip = SDL_sqrt(slipLong*slipLong + slipLat*slipLat);
        averageSlip += slip / inVehicle.GetWheels().size();
        if (car == &Phys::GetCar()) {
            SDL_Log("slip long: %f, lat: %f", slipLong, slipLat);
        }
    }
    //SDL_Log("slip long: %f", SDL_fabsf(slipLong));
    //SDL_Log("wheel velocity: %f", wVel.Length());
    //SDL_Log("Wheels: %d", vehicleConstraint->GetWheels().size());
    //SDL_Log("ang vel: %f", wheel->GetAngularVelocity());
    //SDL_Log("%f, %f, %f", wPos.GetX(), wPos.GetY(), wPos.GetZ());
    //SDL_Log("Slip average: %f", averageSlip);


    float driftGain = SDL_min(averageSlip / 130.0f, 1.0f);
    float driftPitch = car->mBody->GetLinearVelocity().Length() / 30.0f + 0.6;
    SDL_SetAudioStreamFrequencyRatio(car->driftSnd->stream, driftPitch);
    SDL_SetAudioStreamGain(car->driftSnd->stream, driftGain);
    */
}


// Must call before loading a car model with the CarNodeCallback. This will load
// collision and other info from the car model into this VehicleSettings obj
// when LoadModel is called.
static void PrepareLoadCar(VehicleSettings *vsToLoad)
{
    curLoadingVehicleSettings = vsToLoad;
}


static bool CarNodeCallback(const aiNode *node, aiMatrix4x4 transform)
{
    SDL_assert(curLoadingVehicleSettings); // Call PrepareLoadCar before loading
                                           // the car
    aiQuaternion aRotation;
    aiVector3D aPosition;
    aiVector3D aScale;
    transform.Decompose(aScale, aRotation, aPosition);

    JPH::RMat44 joltTransform = ToJoltMat4(transform);
    JPH::Vec3 position = joltTransform.GetTranslation();
    SDL_Log("%f, %f, %f", aScale.x, aScale.y, aScale.z);
    if (SDL_strcmp(node->mName.C_Str(), "WheelPosFR") == 0) {
        SDL_assert(SDL_fabs(aScale.x - aScale.y) < 0.01); // Wheel is oval-shaped, not circular
        curLoadingVehicleSettings->AddWheel(position, true, aScale.x, aScale.z);
    }
    else if (SDL_strcmp(node->mName.C_Str(), "WheelPosFL") == 0) {
        SDL_assert(SDL_fabs(aScale.x - aScale.y) < 0.01); // Wheel is oval-shaped, not circular
        curLoadingVehicleSettings->AddWheel(position, true, aScale.x, aScale.z);
    }
    else if (SDL_strcmp(node->mName.C_Str(), "WheelPosRR") == 0) {
        SDL_assert(SDL_fabs(aScale.x - aScale.y) < 0.01); // Wheel is oval-shaped, not circular
        curLoadingVehicleSettings->AddWheel(position, false, aScale.x, aScale.z);
    }
    else if (SDL_strcmp(node->mName.C_Str(), "WheelPosRL") == 0) {
        SDL_assert(SDL_fabs(aScale.x - aScale.y) < 0.01); // Wheel is oval-shaped, not circular
        curLoadingVehicleSettings->AddWheel(position, false, aScale.x, aScale.z);
    }
    else if (SDL_strncmp(node->mName.C_Str(), "CollisionBox", 12) == 0) {
        curLoadingVehicleSettings->AddCollisionBox(ToJoltVec3(aPosition), ToJoltVec3(aScale));
    }
    else if (SDL_strcmp(node->mName.C_Str(), "HeadLightLeft") == 0) {
        curLoadingVehicleSettings->headLightLeftTransform = joltTransform;
    }
    else if (SDL_strcmp(node->mName.C_Str(), "HeadLightRight") == 0) {
        curLoadingVehicleSettings->headLightRightTransform = joltTransform;
    }
    return true;
}


Vehicle* GetVehicleFromVehicleConstraint(const JPH::VehicleConstraint *constraint)
{
    for (size_t i = 0; i < existingVehicles.size(); i++) {
        if (constraint == existingVehicles[i]->mVehicleConstraint) {
            return existingVehicles[i];
        }
    }
    return nullptr;
}


VehicleSettings GetVehicleSettingsFromFile(const char* filename)
{
    SDL_Log("Loading vehicle settings from file %s", filename);
    char* fileData = (char*) SDL_LoadFile(filename, NULL);
    json j = json::parse(fileData);

    VehicleSettings vs;
    vs.modelFile      = j["model_file"];
    vs.wheelModelFile = j["wheel_model_file"];
    vs.mass           = j["mass"];
    vs.frontCamber    = glm::radians((float) j["front_camber"]);
    vs.frontToe       = glm::radians((float) j["front_toe"]);
    vs.frontCaster    = glm::radians((float) j["front_caster"]);
    vs.frontKingPin   = glm::radians((float) j["front_king_pin"]);
    vs.rearCamber     = glm::radians((float) j["rear_camber"]);
    vs.rearToe        = glm::radians((float) j["rear_toe"]);
    vs.longGrip       = j["long_grip"];
    vs.latGrip        = j["lat_grip"];
    vs.maxTorque      = j["max_torque"];
    vs.suspensionMinLength = j["suspension_min_length"];
    vs.suspensionMaxLength = j["suspension_max_length"];
    vs.suspensionFrequency = j["suspension_frequency"];
    vs.suspensionDamping   = j["suspension_damping"];
    return vs;
}


void VehicleSettings::Init()
{
    // TODO: Destroy these models
    PrepareLoadCar(this);
    vehicleModel = LoadModel(modelFile.c_str(), CarNodeCallback);
    wheelModel = LoadModel(wheelModelFile.c_str());
}


void VehicleSettings::AddWheel(JPH::Vec3 position, bool isSteering, float wheelRadius, float wheelWidth)
{
    SDL_Log("Add Wheel with position %f, %f, %f", 
            position.GetX(), position.GetY(), position.GetZ());
	//const float wheel_radius = 0.31f;
	//const float wheel_width = 0.27f;
    JPH::WheelSettingsWV *wheel = new JPH::WheelSettingsWV;
    //JPH::Vec3 up = JPH::Vec3(0, 1, 0);
	wheel->mPosition = position;
    wheel->mMaxSteerAngle = isSteering ? SDL_PI_F / 8.0 : 0;
    wheel->mRadius = wheelRadius;
    wheel->mWidth = wheelWidth;
    wheel->mSuspensionMinLength = suspensionMinLength;
    wheel->mSuspensionMaxLength = suspensionMaxLength;
    wheel->mSuspensionSpring.mFrequency = suspensionFrequency;
    wheel->mSuspensionSpring.mDamping = suspensionDamping;

    mWheels.push_back(wheel);
}


void VehicleSettings::AddCollisionBox(JPH::Vec3 position, JPH::Vec3 scale)
{
    SDL_Log("Adding collision box with size (%f, %f, %f) and pos (%f, %f, %f",
            scale.GetX(), scale.GetY(), scale.GetZ(),
            position.GetX(), position.GetY(), position.GetZ());
    if (!mCompoundShape) {
        mCompoundShape = new JPH::StaticCompoundShapeSettings;
    }

    JPH::Ref<JPH::Shape> shape = JPH::OffsetCenterOfMassShapeSettings(-position, new JPH::BoxShape(scale)).Create().Get();
    mCompoundShape->AddShape(position, JPH::Quat::sIdentity(), shape);
}


bool Vehicle::IsWheelFlipped(int wheelIndex)
{
    const JPH::WheelSettings *settings = mVehicleConstraint->GetWheels()[wheelIndex]
                                                     ->GetSettings();
    return settings->mPosition.GetX() > 0.0f;
}


JPH::WheelSettings* Vehicle::GetWheelFR()
{
    for (JPH::WheelSettings* wheel : mWheels) {
        if (wheel->mPosition.GetX() < 0.0f && wheel->mPosition.GetZ() > 0.0) {
            return wheel;
        }
    }
    return nullptr;
}
JPH::WheelSettings* Vehicle::GetWheelFL()
{
    for (JPH::WheelSettings* wheel : mWheels) {
        if (wheel->mPosition.GetX() > 0.0f && wheel->mPosition.GetZ() > 0.0) {
            return wheel;
        }
    }
    return nullptr;
}
JPH::WheelSettings* Vehicle::GetWheelRR()
{
    for (JPH::WheelSettings* wheel : mWheels) {
        if (wheel->mPosition.GetX() < 0.0f && wheel->mPosition.GetZ() < 0.0) {
            return wheel;
        }
    }
    return nullptr;
}
JPH::WheelSettings* Vehicle::GetWheelRL()
{
    for (JPH::WheelSettings* wheel : mWheels) {
        if (wheel->mPosition.GetX() > 0.0f && wheel->mPosition.GetZ() < 0.0) {
            return wheel;
        }
    }
    return nullptr;
}


void Vehicle::Init(VehicleSettings &settings)
{
    mSettings = &settings;
    JPH::PhysicsSystem &physicsSystem = Phys::GetPhysicsSystem();
    JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

    //static VehicleSettings settings = GetVehicleSettingsFromFile("data/car.json");
    mWheels = settings.mWheels;
    mCompoundShape = settings.mCompoundShape;
    mVehicleModel = settings.vehicleModel;
    mWheelModel = settings.wheelModel;

    // Set up wheels
    JPH::Vec3 frontWheelUp = JPH::Vec3(SDL_sin(settings.frontCamber), SDL_cos(settings.frontCamber), 0.0);
    JPH::Vec3 rearWheelUp = JPH::Vec3(SDL_sin(settings.rearCamber), SDL_cos(settings.rearCamber), 0.0);
    JPH::Vec3 frontSteeringAxis = JPH::Vec3(-SDL_tan(settings.frontKingPin), 1, -SDL_tan(settings.frontCaster)).Normalized();
    JPH::Vec3 frontWheelForward = JPH::Vec3(-SDL_sin(settings.frontToe), 0, SDL_cos(settings.frontToe));
    JPH::Vec3 rearWheelForward = JPH::Vec3(-SDL_sin(settings.rearToe), 0, SDL_cos(settings.rearToe));
            
    JPH::Vec3 flipX = JPH::Vec3(-1, 1, 1);

    JPH::Vec3 frontSuspensionDir = frontSteeringAxis * JPH::Vec3(-1, -1, -1);

    JPH::WheelSettings *fr = GetWheelFR();
    JPH::WheelSettings *fl = GetWheelFL();
    JPH::WheelSettings *rr = GetWheelRR();
    JPH::WheelSettings *rl = GetWheelRL();
    SDL_assert(fr != nullptr && fl != nullptr && rr != nullptr && rl != nullptr);
    fr->mWheelUp = frontWheelUp * flipX;
    fl->mWheelUp = frontWheelUp;
    rr->mWheelUp = rearWheelUp * flipX;
    rl->mWheelUp = rearWheelUp;

    fr->mWheelForward = frontWheelForward * flipX;
    fl->mWheelForward = frontWheelForward;
    fr->mSuspensionDirection = frontSuspensionDir * flipX;
    fl->mSuspensionDirection = frontSuspensionDir;
    fr->mSteeringAxis = frontSteeringAxis * flipX;
    fl->mSteeringAxis = frontSteeringAxis;

    rr->mWheelForward = rearWheelForward * flipX;
    rl->mWheelForward = rearWheelForward;

    // Move wheels up a bit so that they rest where placed in the model.
    
    fr->mPosition -= fr->mSuspensionDirection * settings.suspensionMinLength;
    fl->mPosition -= fl->mSuspensionDirection * settings.suspensionMinLength;
    rr->mPosition -= rr->mSuspensionDirection * settings.suspensionMinLength;
    rl->mPosition -= rl->mSuspensionDirection * settings.suspensionMinLength;
    


    // Create collision tester
	//colTester = new VehicleCollisionTesterCastSphere(Layers::MOVING, 0.5f * wheel_width);
	//colTester = new VehicleCollisionTesterCastCylinder(Layers::MOVING);
	mColTester = new JPH::VehicleCollisionTesterRay(Phys::Layers::MOVING);
    
	// Create vehicle body
    JPH::RVec3 position(6, 3, 12);
    SDL_assert(mCompoundShape != nullptr);
    JPH::BodyCreationSettings carBodySettings(mCompoundShape, position, 
                                              JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.0f),
                                              JPH::EMotionType::Dynamic, Phys::Layers::MOVING);

	carBodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
	carBodySettings.mMassPropertiesOverride.mMass = settings.mass;
	mBody = bodyInterface.CreateBody(carBodySettings);
	bodyInterface.AddBody(mBody->GetID(), JPH::EActivation::Activate);

	// Create vehicle constraint
    JPH::VehicleConstraintSettings constraintSettings;

    constraintSettings.mWheels = mWheels;

    JPH::WheeledVehicleControllerSettings *controller = new JPH::WheeledVehicleControllerSettings;
	constraintSettings.mController = controller;
    controller->mEngine.mMaxTorque = settings.maxTorque;

    // Set which wheels are driven
	controller->mDifferentials.resize(1);
    for (unsigned int i = 0; i < mWheels.size(); i++) {
        if (mWheels[i].GetPtr() == fl) {
            controller->mDifferentials[0].mLeftWheel = i;
        }
        else if (mWheels[i].GetPtr() == fr) {
            controller->mDifferentials[0].mRightWheel = i;
        }
    }
    

    controller->mTransmission.mMode = JPH::ETransmissionMode::Auto;
    SDL_Log("%f", controller->mTransmission.mShiftUpRPM);

	mVehicleConstraint = new JPH::VehicleConstraint(*mBody, constraintSettings);
	physicsSystem.AddConstraint(mVehicleConstraint);
	physicsSystem.AddStepListener(mVehicleConstraint);

	mVehicleConstraint->SetVehicleCollisionTester(mColTester);
    mVehicleConstraint->SetPostCollideCallback(VehiclePostCollideCallback);

    float &longGripRef = mSettings->longGrip;
    float &latGripRef = mSettings->latGrip;
	static_cast<JPH::WheeledVehicleController *>(mVehicleConstraint->GetController())->SetTireMaxImpulseCallback(
		[&longGripRef, &latGripRef](JPH::uint, float &outLongitudinalImpulse, float &outLateralImpulse, float inSuspensionImpulse, float inLongitudinalFriction, float inLateralFriction, float, float, float)
		{
            JPH::uint velSteps = Phys::GetPhysicsSystem().GetPhysicsSettings()
                                                         .mNumVelocitySteps;
			outLongitudinalImpulse = velSteps * inLongitudinalFriction 
                                     * inSuspensionImpulse
                                     * longGripRef;
			outLateralImpulse = inLateralFriction * inSuspensionImpulse
                                * latGripRef;
		});

    //RVec3 com = Phys::GetCarPos();
    //SDL_Log("car com (%f, %f, %f)", com.GetX(), com.GetY(), com.GetZ());
    engineSnd = Audio::CreateSoundFromFile("data/sound/car_engine.wav");
    driftSnd = Audio::CreateSoundFromFile("data/sound/drift.wav");

    // Head Lights
    const float cutoffInner = SDL_cos(SDL_PI_F / 5.0);
    const float cutoffOuter = SDL_cos(SDL_PI_F / 4.0);
    headLightLeft = Render::CreateSpotLight();
    headLightLeft->mColour = glm::vec3(150.0, 100.0, 50.0);
    headLightLeft->mCutoffInner = cutoffInner;
    headLightLeft->mCutoffOuter = cutoffOuter;
    headLightLeft->mEnableShadows = false;

    headLightRight = Render::CreateSpotLight();
    headLightRight->mColour = glm::vec3(150.0, 100.0, 50.0);
    headLightRight->mCutoffInner = cutoffInner;
    headLightRight->mCutoffOuter = cutoffOuter;
    headLightRight->mEnableShadows = false;
    
    headLightLeftTransform = settings.headLightLeftTransform;
    headLightRightTransform = settings.headLightRightTransform;

    // Sounds
    engineSnd->doRepeat = true;
    driftSnd->doRepeat = true;

}


void Vehicle::DebugGUI(unsigned int id)
{
    JPH::Vec3 flipX = JPH::Vec3(-1, 1, 1);
    JPH::WheelSettings *fr = GetWheelFR();
    JPH::WheelSettings *fl = GetWheelFL();
    JPH::WheelSettings *rr = GetWheelRR();
    JPH::WheelSettings *rl = GetWheelRL();

    char title[20];
    SDL_snprintf(title, 20, "Vehicle Debug %d", id);
    ImGui::Begin(title);


    // Front Camber
    float frontCamberDeg = glm::degrees(mSettings->frontCamber);
    const float frontCamberBefore = frontCamberDeg;
    ImGui::SliderFloat("Front Camber", &frontCamberDeg, -20.0f, 20.0f);
    if (frontCamberDeg != frontCamberBefore) {
        mSettings->frontCamber = glm::radians(frontCamberDeg);
        JPH::Vec3 frontWheelUp = JPH::Vec3(SDL_sin(mSettings->frontCamber), SDL_cos(mSettings->frontCamber), 0.0);
        fr->mWheelUp = frontWheelUp * flipX;
        fl->mWheelUp = frontWheelUp;
    }

    // Rear Camber
    float rearCamberDeg = glm::degrees(mSettings->rearCamber);
    const float rearCamberBefore = rearCamberDeg;
    ImGui::SliderFloat("Rear Camber", &rearCamberDeg, -20.0f, 20.0f);
    if (rearCamberDeg != rearCamberBefore) {
        mSettings->rearCamber = glm::radians(rearCamberDeg);
        JPH::Vec3 rearWheelUp = JPH::Vec3(SDL_sin(mSettings->rearCamber), SDL_cos(mSettings->rearCamber), 0.0);
        rr->mWheelUp = rearWheelUp * flipX;
        rl->mWheelUp = rearWheelUp;
    }

    // Front Caster
    float frontCasterDeg = glm::degrees(mSettings->frontCaster);
    const float frontCasterBefore = frontCasterDeg;
    ImGui::SliderFloat("Front Caster", &frontCasterDeg, -45.0f, 45.0f);
    if (frontCasterDeg != frontCasterBefore) {
        mSettings->frontCaster = glm::radians(frontCasterDeg);
        JPH::Vec3 frontSteeringAxis = JPH::Vec3(-SDL_tan(mSettings->frontKingPin), 1, -SDL_tan(mSettings->frontCaster)).Normalized();
        JPH::Vec3 frontSuspensionDir = frontSteeringAxis * JPH::Vec3(-1, -1, -1);
        fr->mSteeringAxis = frontSteeringAxis * flipX;
        fl->mSteeringAxis = frontSteeringAxis;
        fr->mSuspensionDirection = frontSuspensionDir * flipX;
        fl->mSuspensionDirection = frontSuspensionDir;
    }
    
    // Front Toe
    float frontToeDeg = glm::degrees(mSettings->frontToe);
    const float frontToeBefore = frontToeDeg;
    ImGui::SliderFloat("Front Toe", &frontToeDeg, -45.0f, 45.0f);
    if (frontToeDeg != frontToeBefore) {
        mSettings->frontToe = glm::radians(frontToeDeg);
        JPH::Vec3 frontWheelForward = JPH::Vec3(-SDL_sin(mSettings->frontToe), 0, SDL_cos(mSettings->frontToe));
        fr->mWheelForward = frontWheelForward * flipX;
        fl->mWheelForward = frontWheelForward;
    }
    
    // Rear Toe
    float rearToeDeg = glm::degrees(mSettings->rearToe);
    const float rearToeBefore = rearToeDeg;
    ImGui::SliderFloat("Rear Toe", &rearToeDeg, -45.0f, 45.0f);
    if (rearToeDeg != rearToeBefore) {
        mSettings->rearToe = glm::radians(rearToeDeg);
        JPH::Vec3 rearWheelForward = JPH::Vec3(-SDL_sin(mSettings->rearToe), 0, SDL_cos(mSettings->rearToe));
        rr->mWheelForward = rearWheelForward * flipX;
        rl->mWheelForward = rearWheelForward;
    }

    // Max Torque
    ImGui::SliderFloat("Max Torque", &(mSettings->maxTorque), 0.0, 1200.0);
    static_cast<JPH::WheeledVehicleController *>(mVehicleConstraint->GetController())->GetEngine().mMaxTorque = mSettings->maxTorque;

    // Suspension
    //const float minLengthBefore = mSettings->suspensionMinLength;
    ImGui::SliderFloat("Suspension Min Length", &(mSettings->suspensionMinLength), 0.0, 1.0);
    ImGui::SliderFloat("Suspension Max Length", &(mSettings->suspensionMaxLength), 0.0, 1.0);
    ImGui::SliderFloat("Suspension Frequency", &(mSettings->suspensionFrequency), 0.0, 8.0);
    ImGui::SliderFloat("Suspension Damping", &(mSettings->suspensionDamping), 0.0, 8.0);
    for (JPH::Ref<JPH::WheelSettings> wheel : mWheels) {
        wheel->mSuspensionMinLength = mSettings->suspensionMinLength;
        wheel->mSuspensionMaxLength = mSettings->suspensionMaxLength;
        wheel->mSuspensionSpring.mFrequency = mSettings->suspensionFrequency;
        wheel->mSuspensionSpring.mDamping = mSettings->suspensionDamping;
    }

    // Mass
    const float massBefore = mSettings->mass;
    ImGui::SliderFloat("Mass", &(mSettings->mass), 1.0, 4000.0);
    if (massBefore != mSettings->mass) {
        JPH::MotionProperties* motionProperties = mBody->GetMotionProperties();
        JPH::MassProperties massProperties = mBody->GetShape()->GetMassProperties();
        massProperties.ScaleToMass(mSettings->mass);
        motionProperties->SetMassProperties(JPH::EAllowedDOFs::All, massProperties);
    }

    // Tire Grip
    ImGui::SliderFloat("Longitudinal Grip", &(mSettings->longGrip), 0.5f, 5.0f);
    ImGui::SliderFloat("Latiudinal Grip",   &(mSettings->latGrip),  0.5f, 5.0f);
    
    ImGui::Text("Current Steering: %f", mSteer);
    JPH::Vec3 position = mBody->GetCenterOfMassPosition();
    JPH::Vec3 velocity = mBody->GetLinearVelocity();
    float speedKPH = velocity.Length() * 3.6;
    ImGui::Text("Position: %f, %f, %f", position.GetX(), position.GetY(), position.GetZ());
    ImGui::Text("Speed: %.1f kph", speedKPH);

    ImGui::End();
}

void Vehicle::HoldInPlace()
{
    mIsHeldInPlace = true;
}
    
void Vehicle::ReleaseFromHold()
{
    mIsHeldInPlace = false;
}


float Vehicle::GetEngineRPM()
{
    JPH::WheeledVehicleController *controller = static_cast<JPH::WheeledVehicleController*>
        (mVehicleConstraint->GetController());
    JPH::VehicleEngine &engine = controller->GetEngine();
    float rpm = engine.GetCurrentRPM();
    return rpm;
}


float Vehicle::GetLongVelocity()
{
    return (mBody->GetRotation().Conjugated() * mBody->GetLinearVelocity()).GetZ();
}


void Vehicle::PrePhysicsUpdate(float delta)
{
    JPH::BodyInterface &bodyInterface = Phys::GetPhysicsSystem().GetBodyInterface();

    // Longitudinal velocity local to the car
    float longVelocity = GetLongVelocity();

    // Limit steering angle based on velocity
    const float limitVelStart = 20.0;
    const float limitVelEnd = 40.0;
    const float minSteerLimitFactor = 0.7;
    //mSteer *= SDL_clamp(longVelocity, limitVelStart, limitVelEnd)
    float steerLimitFactor = -(1.0 - minSteerLimitFactor) 
                             * (longVelocity - limitVelStart) 
                             / (limitVelEnd - limitVelStart) + 1.0;
    steerLimitFactor = SDL_clamp(steerLimitFactor, minSteerLimitFactor, 1.0);
    mSteer *= steerLimitFactor;

    if (mIsHeldInPlace) {
        mForward = 0.0;
        mHandbrake = 1.0;
    }

    if (mSteer || mForward || mBrake)
        bodyInterface.ActivateBody(mBody->GetID());

    JPH::WheeledVehicleController *controller = static_cast<JPH::WheeledVehicleController*>
        (mVehicleConstraint->GetController());
    // Don't know what this does
    controller->SetDifferentialLimitedSlipRatio(1.4f);
    for (JPH::VehicleDifferentialSettings &d : controller->GetDifferentials())
        d.mLimitedSlipRatio = 1.4f;

    
    JPH::VehicleEngine &engine = controller->GetEngine();
    float rpm = engine.GetCurrentRPM();
    controller->GetTransmission().Update(delta, rpm, mForward, true);


    if (longVelocity < 0.1f && mDrivingDir > 0.0f && mBrake > 0.0f) {
        // Braked to a stop when going forward, switch to going backward
        mDrivingDir = -1.0f;
    }
    else if (longVelocity > -0.1f && mDrivingDir < 0.0f && mForward > 0.0f) {
        // Braked to a stop when going backward, switch to going forward
        mDrivingDir = 1.0f;
    }

    if (mDrivingDir < 0.0f) {
        if (mBrake > 0.0f) {
            mForward = -mBrake;
            mBrake = 0.0f;
        }
        if (mForward > 0.0f) {
            mBrake = mForward;
            mForward = 0.0f;
        }
    }


    controller->SetDriverInput(mForward, mSteer, mBrake, mHandbrake);

    SDL_SetAudioStreamFrequencyRatio(engineSnd->stream, rpm / 2000.0f + 0.5);
    SDL_SetAudioStreamGain(engineSnd->stream, rpm / 3000.0f + 0.3);

    float averageSlip = 0;
    float slipLong;
    //Wheel *wheel1 = vehicleConstraint->GetWheels()[0];
    //SDL_Log("ang vel: %f", wheel1->GetAngularVelocity());
    for (JPH::Wheel *wheel : mVehicleConstraint->GetWheels()) {
        JPH::WheelWV *wheel2 = static_cast<JPH::WheelWV*> (wheel);
        if (!wheel2->HasContact()) {
            continue;
        }
        JPH::Vec3 wPos = JPH::Vec3(mBody->GetWorldTransform() * JPH::Vec4(wheel->GetSettings()->mPosition, 1.0f));
        JPH::Vec3 wVel = mBody->GetPointVelocity(wPos);
        float wheelLongVel = wheel->GetAngularVelocity() * wheel->GetSettings()->mRadius;
        slipLong = SDL_fabsf(wheelLongVel) - SDL_fabsf(wVel.Dot(wheel->GetContactLongitudinal()));
        slipLong = SDL_fabsf(slipLong);
        //SDL_Log("%f", slipLong);

        //slipLong = SDL_min(wheel2->mLongitudinalSlip, 1.0) * 10.0;
        //slipLong *= wVel.Dot(wheel->GetContactLongitudinal());

        //float slipLat = wVel.Dot(wheel->GetContactLateral());

        float slipLat = SDL_sin(wheel2->mLateralSlip) * wVel.Length();

        constexpr float gain = 4.0;
        float slip = SDL_sqrt(slipLong*slipLong + slipLat*slipLat) * gain;
        averageSlip += slip / mVehicleConstraint->GetWheels().size();
        /*if (this == &Phys::GetCar()) {
            SDL_Log("slip long: %f, lat: %f", slipLong, slipLat);
        }
        */
    }
    //SDL_Log("slip long: %f", SDL_fabsf(slipLong));
    //SDL_Log("wheel velocity: %f", wVel.Length());
    //SDL_Log("Wheels: %d", vehicleConstraint->GetWheels().size());
    //SDL_Log("ang vel: %f", wheel->GetAngularVelocity());
    //SDL_Log("%f, %f, %f", wPos.GetX(), wPos.GetY(), wPos.GetZ());
    //SDL_Log("Slip average: %f", averageSlip);


    float driftGain = SDL_min(averageSlip / 130.0f, 1.0f);
    float driftPitch = mBody->GetLinearVelocity().Length() / 30.0f + 0.6;
    SDL_SetAudioStreamFrequencyRatio(driftSnd->stream, driftPitch);
    SDL_SetAudioStreamGain(driftSnd->stream, driftGain);


}


void Vehicle::Update()
{
    JPH::RMat44 bodyTransform = mBody->GetWorldTransform();

    headLightLeft->mPosition = ToGlmVec3(JPH::Vec3(
                bodyTransform * headLightLeftTransform * JPH::Vec4(0, 0, 0, 1)));
    headLightLeft->mDirection = ToGlmVec3(JPH::Vec3(
                bodyTransform * headLightLeftTransform * JPH::Vec4(0, 1, 0, 0)));

    headLightRight->mPosition = ToGlmVec3(JPH::Vec3(
                bodyTransform * headLightRightTransform * JPH::Vec4(0, 0, 0, 1)));
    headLightRight->mDirection = ToGlmVec3(JPH::Vec3(
                bodyTransform * headLightRightTransform * JPH::Vec4(0, 1, 0, 0)));
}



JPH::RMat44 Vehicle::GetWheelTransform(int wheelNum)
{
    JPH::RMat44 wheelTransform = mVehicleConstraint->GetWheelWorldTransform
                            (wheelNum, JPH::Vec3::sAxisY(), JPH::Vec3::sAxisX());
    return wheelTransform;
}


JPH::RVec3 Vehicle::GetPos()
{
    JPH::BodyInterface &bodyInterface = Phys::GetBodyInterface();
    return bodyInterface.GetCenterOfMassPosition(mBody->GetID());
}


JPH::Quat Vehicle::GetRotation()
{
    JPH::BodyInterface &bodyInterface = Phys::GetPhysicsSystem().GetBodyInterface();
    return bodyInterface.GetRotation(mBody->GetID());
}


void Vehicle::Destroy()
{
    JPH::BodyInterface &bodyInterface = Phys::GetPhysicsSystem().GetBodyInterface();

    bodyInterface.RemoveBody(mBody->GetID());
    bodyInterface.DestroyBody(mBody->GetID());

    Render::DestroySpotLight(headLightLeft);
    Render::DestroySpotLight(headLightRight);
}


const std::vector<Vehicle*>& GetExistingVehicles()
{
    return existingVehicles;
}


Vehicle* CreateVehicle()
{
    Vehicle *newVehicle = new Vehicle;
    existingVehicles.push_back(newVehicle);
    return newVehicle;
}

void DestroyVehicle(Vehicle *toDestroy)
{
    for (size_t i = 0; i < existingVehicles.size(); i++) {
        if (existingVehicles[i] == toDestroy) {
            existingVehicles[i]->Destroy();
            existingVehicles[i] = nullptr;
        }
    }
    delete toDestroy;
}

    

std::vector<Vehicle*>& Vehicle::GetExistingVehicles()
{
    return existingVehicles;
}

void Vehicle::PrePhysicsUpdateAllVehicles(float delta)
{
    for (Vehicle *v : existingVehicles) {
        v->PrePhysicsUpdate(delta);
    }
}

void Vehicle::UpdateAllVehicles()
{
    for (Vehicle *v : existingVehicles) {
        v->Update();
    }
}



int Vehicle::NumExistingVehicles()
{
    return existingVehicles.size();
}


void Vehicle::DestroyAllVehicles()
{
    for (Vehicle *v : existingVehicles) {
        DestroyVehicle(v);
    }
    existingVehicles.clear();
}
