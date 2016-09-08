/* -------------------------------------------------------------------------- *
 *                      OpenSim:  testNested<odel.cpp                         *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2016 Stanford University and the Authors                *
 * Author(s): Ajay Seth,                                                      *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

/*=============================================================================

Tests Include:
    1. Pendulum Model with Device that includes WeldJoints

//=============================================================================*/
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>
#include <OpenSim/Auxiliary/auxiliaryTestFunctions.h>

using namespace OpenSim;
using namespace std;

void testPendulumModelWithJointInDevice();
void testPendulumModelWithDeviceModel();

// Create Device as Concrete Container Component (like Model) of Components
class Device : public ModelComponent {
    OpenSim_DECLARE_CONCRETE_OBJECT(Device, ModelComponent);
};

int main()
{
    SimTK::Array_<std::string> failures;

    try { testPendulumModelWithJointInDevice(); }
    catch (const std::exception& e){
        cout << e.what() << endl; 
        failures.push_back("testPendulumModelWithJointInDevice");
    }
    try { testPendulumModelWithDeviceModel(); }
    catch (const std::exception& e) {
        cout << e.what() << endl;
        failures.push_back("testPendulumModelWithDeviceModel");
    }


    if (!failures.empty()) {
        cout << "Done, with failure(s): " << failures << endl;
        return 1;
    }

    cout << "Done. All cases passed." << endl;

    return 0;
}

//==============================================================================
// Test Cases
//==============================================================================

void testPendulumModelWithJointInDevice()
{
    using namespace SimTK;
    Vec3 tolerance(SimTK::Eps);

    cout << "Running testPendulumModelWithJointInDevice" << endl;

    // Load the pendulum model
    Model* pendulum = new Model("double_pendulum.osim");
    
    // Create a new empty device;
    Device* device = new Device();
    device->setName("device");

    // Build the device
    // Create bodies 
    auto* cuffA = new OpenSim::Body("cuffA", 1.0, Vec3(0), Inertia(0.5));
    auto* cuffB = new OpenSim::Body("cuffB", 1.0, Vec3(0), Inertia(0.5));

    // add Bodies to the device
    device->addComponent(cuffA);
    device->addComponent(cuffB);

    // Create WeldJoints to anchor cuff Bodies to the pendulum.
    auto* anchorA = new WeldJoint();
    anchorA->setName("anchorA");
    anchorA->updConnector("child_frame").connect(*cuffA);

    auto* anchorB = new WeldJoint();
    anchorB->setName("anchorB");
    anchorB->updConnector("child_frame").connect(*cuffB);

    device->addComponent(anchorA);
    device->addComponent(anchorB);

    // Connect the device to the pendulum
    auto bodies = pendulum->getComponentList<PhysicalFrame>();
    auto& body = bodies.begin();
    anchorA->updConnector("parent_frame").connect(*body);
    body++;
    anchorB->updConnector("parent_frame").connect(*body);

    pendulum->addModelComponent(device);

    State& s = pendulum->initSystem();
}


void testPendulumModelWithDeviceModel()
{
    using namespace SimTK;
    Vec3 tolerance(SimTK::Eps);

    cout << "Running testPendulumModelWithDeviceModel" << endl;

    // Load the pendulum model
    Model* pendulum = new Model("double_pendulum.osim");

    // Create a new empty device;
    Model* device = new Model();
    device->setName("device");

    // Build the device
    // Create bodies 
    auto* cuffA = new OpenSim::Body("cuffA", 1.0, Vec3(0), Inertia(0.5));
    auto* cuffB = new OpenSim::Body("cuffB", 1.0, Vec3(0), Inertia(0.5));

    // add Bodies to the device
    device->addComponent(cuffA);
    device->addComponent(cuffB);

    // Create WeldJoints to anchor cuff Bodies to the pendulum.
    auto* anchorA = new WeldJoint();
    anchorA->setName("anchorA");
    anchorA->updConnector("child_frame").connect(*cuffA);

    auto* anchorB = new WeldJoint();
    anchorB->setName("anchorB");
    anchorB->updConnector("child_frame").connect(*cuffB);

    device->addComponent(anchorA);
    device->addComponent(anchorB);

    // Connect the device to the pendulum
    auto bodies = pendulum->getComponentList<PhysicalFrame>();
    auto& body = bodies.begin();
    anchorA->updConnector("parent_frame").connect(*body);
    body++;
    anchorB->updConnector("parent_frame").connect(*body);

    pendulum->addModelComponent(device);

    State& s = pendulum->initSystem();
}
