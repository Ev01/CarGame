#include "vehicle.h"
#include "audio.h"
#include "input.h"
#include "physics.h"


#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>

#include <SDL3/SDL.h>


static void VehiclePostCollideCallback(JPH::VehicleConstraint &inVehicle, const JPH::PhysicsStepListenerContext &inContext)
{
    Vehicle &car = Phys::GetCar();
    // Audio stuff
    JPH::WheeledVehicleController *controller = static_cast<JPH::WheeledVehicleController*>
        (inVehicle.GetController());
    JPH::VehicleEngine &engine = controller->GetEngine();
    float rpm = engine.GetCurrentRPM();
    SDL_SetAudioStreamFrequencyRatio(car.engineSnd->stream, rpm / 4000.0f);

    float averageSlip = 0;
    float slipLong;
    //Wheel *wheel1 = vehicleConstraint->GetWheels()[0];
    //SDL_Log("ang vel: %f", wheel1->GetAngularVelocity());
    for (JPH::Wheel *wheel : inVehicle.GetWheels()) {
        if (!wheel->HasContact()) {
            continue;
        }
        JPH::Vec3 wPos = JPH::Vec3(car.mBody->GetWorldTransform() * JPH::Vec4(wheel->GetSettings()->mPosition, 1.0f));
        JPH::Vec3 wVel = car.mBody->GetPointVelocity(wPos);
        float wheelLongVel = wheel->GetAngularVelocity() * wheel->GetSettings()->mRadius;
        slipLong = SDL_fabsf(wheelLongVel) - SDL_fabsf(wVel.Dot(wheel->GetContactLongitudinal()));
        slipLong = SDL_fabsf(slipLong);
        //SDL_Log("%f", slipLong);

        float slipLat = wVel.Dot(wheel->GetContactLateral());
        float slip = slipLong*slipLong + slipLat*slipLat;
        averageSlip += slip / inVehicle.GetWheels().size();
        //SDL_Log("slip long: %f, lat: %f", slipLong, slipLat);
    }
    //SDL_Log("slip long: %f", SDL_fabsf(slipLong));
    //SDL_Log("wheel velocity: %f", wVel.Length());
    //SDL_Log("Wheels: %d", vehicleConstraint->GetWheels().size());
    //SDL_Log("ang vel: %f", wheel->GetAngularVelocity());
    //SDL_Log("%f, %f, %f", wPos.GetX(), wPos.GetY(), wPos.GetZ());
    //SDL_Log("Slip average: %f", averageSlip);


    float driftGain = SDL_min(averageSlip / 130.0f, 1.0f);
    float driftPitch = car.mBody->GetLinearVelocity().Length() / 30.0f + 0.6;
    SDL_SetAudioStreamFrequencyRatio(car.driftSnd->stream, driftPitch);
    SDL_SetAudioStreamGain(car.driftSnd->stream, driftGain);
}


void Vehicle::AddWheel(JPH::Vec3 position, bool isSteering)
{
    SDL_Log("Add Wheel with position %f, %f, %f", 
            position.GetX(), position.GetY(), position.GetZ());
	const float wheel_radius = 0.45f;
	const float wheel_width = 0.2f;
    JPH::WheelSettingsWV *wheel = new JPH::WheelSettingsWV;
	wheel->mPosition = position;
    wheel->mMaxSteerAngle = isSteering ? SDL_PI_F / 5.0 : 0;
    wheel->mRadius = wheel_radius;
    wheel->mWidth = wheel_width;
    wheel->mSuspensionMinLength = 0.1f;
    wheel->mSuspensionMaxLength = 0.3f;

    //wheel->mSuspensionSpring.mFrequency = 0.5f;
    //wheel->mSuspensionSpring.mDamping = 1.0f;


    mWheels.push_back(wheel);
}


void Vehicle::AddCollisionBox(JPH::Vec3 position, JPH::Vec3 scale)
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


void Vehicle::Init()
{
    JPH::PhysicsSystem &physicsSystem = Phys::GetPhysicsSystem();
    JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

    // Create collision tester
	//colTester = new VehicleCollisionTesterCastSphere(Layers::MOVING, 0.5f * wheel_width);
	//colTester = new VehicleCollisionTesterCastCylinder(Layers::MOVING);
	mColTester = new JPH::VehicleCollisionTesterRay(Phys::Layers::MOVING);
    
	// Create vehicle body
    JPH::RVec3 position(6, 3, 12);
    JPH::BodyCreationSettings carBodySettings(mCompoundShape, position, JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.0f), JPH::EMotionType::Dynamic, Phys::Layers::MOVING);
	carBodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
	carBodySettings.mMassPropertiesOverride.mMass = 1500.0f;
	mBody = bodyInterface.CreateBody(carBodySettings);
	bodyInterface.AddBody(mBody->GetID(), JPH::EActivation::Activate);

	// Create vehicle constraint
    JPH::VehicleConstraintSettings constraintSettings;

    constraintSettings.mWheels = mWheels;

    JPH::WheeledVehicleControllerSettings *controller = new JPH::WheeledVehicleControllerSettings;
	constraintSettings.mController = controller;

	controller->mDifferentials.resize(1);
    for (unsigned int i = 0; i < mWheels.size(); i++) {
        JPH::Vec3 pos = mWheels[i]->mPosition;
        if (pos.GetX() < 0.0f && pos.GetZ() > 1.0f) {
            controller->mDifferentials[0].mLeftWheel = i;
        }
        else if (pos.GetX() > 1.0f && pos.GetZ() > 1.0f) {
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

    // TODO: remove and re-tweak car settings
	static_cast<JPH::WheeledVehicleController *>(mVehicleConstraint->GetController())->SetTireMaxImpulseCallback(
		[](JPH::uint, float &outLongitudinalImpulse, float &outLateralImpulse, float inSuspensionImpulse, float inLongitudinalFriction, float inLateralFriction, float, float, float)
		{
			outLongitudinalImpulse = 10.0f * inLongitudinalFriction * inSuspensionImpulse;
			outLateralImpulse = inLateralFriction * inSuspensionImpulse;
		});

    //RVec3 com = Phys::GetCarPos();
    //SDL_Log("car com (%f, %f, %f)", com.GetX(), com.GetY(), com.GetZ());
    engineSnd = Audio::CreateSoundFromFile("sound/car_engine.wav");
    driftSnd = Audio::CreateSoundFromFile("sound/drift.wav");
    engineSnd->doRepeat = true;
    driftSnd->doRepeat = true;
}


void Vehicle::Update(float delta)
{
    JPH::BodyInterface &bodyInterface = Phys::GetPhysicsSystem().GetBodyInterface();
    //mSteer = mSteer + (mSteerTarget - mSteer) * 0.1f;
    if (!Input::GetGamepad()) {
        mSteer += glm::sign(mSteerTarget - mSteer) * 0.1f;
        mSteer = SDL_clamp(mSteer, -1.0f, 1.0f);
    } else {
        mSteer = mSteerTarget;
    }

    //RVec3 com = Phys::GetCarPos();
    //SDL_Log("car com (%f, %f, %f)", com.GetX(), com.GetY(), com.GetZ());

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

    // Longitudinal velocity local to the car
    float longVelocity = (mBody->GetRotation().Conjugated() * mBody->GetLinearVelocity()).GetZ();

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

    controller->SetDriverInput(mForward, mSteer, mBrake, 0.0f);
}


void Vehicle::ProcessInput()
{
    if (Input::GetGamepad()) {
        mForward = Input::GetGamepadAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
        mBrake = Input::GetGamepadAxis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
        mSteerTarget = Input::GetGamepadAxis(SDL_GAMEPAD_AXIS_LEFTX);
    }
    else {
        mSteerTarget = Input::GetScanAxis(SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT);
        mBrake = (float) Input::IsScanDown(SDL_SCANCODE_DOWN);
        mForward = (float) Input::IsScanDown(SDL_SCANCODE_UP);
    }
}


JPH::RMat44 Vehicle::GetWheelTransform(int wheelNum)
{
    JPH::RMat44 wheelTransform = mVehicleConstraint->GetWheelWorldTransform
                            (wheelNum, JPH::Vec3::sAxisY(), JPH::Vec3::sAxisX());
    return wheelTransform;
}


JPH::RVec3 Vehicle::GetPos()
{
    JPH::BodyInterface &bodyInterface = Phys::GetPhysicsSystem().GetBodyInterface();
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
}
