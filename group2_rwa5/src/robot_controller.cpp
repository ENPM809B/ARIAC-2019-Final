//
// Created by zeid on 2/27/20.
//
#include "robot_controller.h"

#include <moveit/robot_model/robot_model.h>
#include <moveit/robot_state/robot_state.h>

#include <moveit/robot_model_loader/robot_model_loader.h>


/**
 * Constructor for the robot
 * Class attributes are initialized in the constructor init list
 * You can instantiate another robot by passing the correct parameter to the constructor
 */
RobotController::RobotController(std::string arm_id_1) :
robot_controller_nh_("/ariac/"+arm_id_1),
robot_controller_options("manipulator",
        "/ariac/"+arm_id_1+"/robot_description",
        robot_controller_nh_),
robot_move_group_(robot_controller_options),

robot_controller_nh_2("/ariac/"+arm_id_1),
robot_controller_options_2("manipulator",
        "/ariac/"+arm_id_1+"/robot_description",
        robot_controller_nh_2),
robot_move_group_2(robot_controller_options_2) {

    ROS_WARN(">>>>> RobotController");

    robot_move_group_.setPlanningTime(10);
    robot_move_group_.setNumPlanningAttempts(10);
    robot_move_group_.setPlannerId("RRTConnectkConfigDefault");
    robot_move_group_.setMaxVelocityScalingFactor(1.0);
    robot_move_group_.setMaxAccelerationScalingFactor(1.0);
    robot_move_group_.allowReplanning(true);

    robot_move_group_2.setPlanningTime(10);
    robot_move_group_2.setNumPlanningAttempts(10);
    robot_move_group_2.setPlannerId("RRTConnectkConfigDefault");
    robot_move_group_2.setMaxVelocityScalingFactor(1.0);
    robot_move_group_2.setMaxAccelerationScalingFactor(1.0);
    robot_move_group_2.allowReplanning(true);

    /* These are joint positions used for the home position
     * [0] = linear_arm_actuator
     * [1] = shoulder_pan_joint
     * [2] = shoulder_lift_joint
     * [3] = elbow_joint
     * [4] = wrist_1_joint
     * [5] = wrist_2_joint
     * [6] = wrist_3_joint
     */

    home_joint_pose_ = {0.0, 3.11, -1.60, 2.0, 4.30, -1.53, 0};
    home_joint_pose_1 = {1.18, 3.11, -1.60, 2.0, 4.30, -1.53, 0};
    bin_drop_pose_ = {2.5, 3.11, -1.60, 2.0, 3.47, -1.53, 0};
    kit_drop_pose_ = {2.65, 1.57, -1.60, 2.0, 3.47, -1.53, 0};
    belt_drop_pose_ = {2.5, 0, -1.60, 2.0, 3.47, -1.53, 0};
    conveyor = {1.13, 0, -0.70, 1.65, 3.74, -1.56, 0};
    drop_part={2.65, 1.57, -1.60, 2.0, 3.47, -1.53, 0};
    home_joint_pose_2 = {-1.18, 3.11, -1.60, 2.0, 4.30, -1.53, 0};
    // conveyor_lcam = {}
    offset_ = 0.025;

    //--topic used to get the status of the gripper
    gripper_subscriber_ = gripper_nh_.subscribe(
            "/ariac/arm1/gripper/state", 10, &RobotController::GripperCallback, this);
    gripper_subscriber_2 = gripper_nh_2.subscribe(
            "/ariac/arm2/gripper/state", 10, &RobotController::GripperCallback2, this);

    // SendRobotHome();
    // SendRobotHome2();

    robot_tf_listener_.waitForTransform("arm1_linear_arm_actuator", "arm1_ee_link",
                                            ros::Time(0), ros::Duration(10));
    robot_tf_listener_.lookupTransform("/arm1_linear_arm_actuator", "/arm1_ee_link",
                                           ros::Time(0), robot_tf_transform_);


    fixed_orientation_.x = robot_tf_transform_.getRotation().x();
    fixed_orientation_.y = robot_tf_transform_.getRotation().y();
    fixed_orientation_.z = robot_tf_transform_.getRotation().z();
    fixed_orientation_.w = robot_tf_transform_.getRotation().w();
    ROS_INFO_STREAM(fixed_orientation_.x);
    ROS_INFO_STREAM(fixed_orientation_.y);
    ROS_INFO_STREAM(fixed_orientation_.z);
    ROS_INFO_STREAM(fixed_orientation_.w);

    tf::quaternionMsgToTF(fixed_orientation_,q);
    tf::Matrix3x3(q).getRPY(roll_def_,pitch_def_,yaw_def_);
        ROS_INFO_STREAM(roll_def_);
    ROS_INFO_STREAM(pitch_def_ );
    ROS_INFO_STREAM(yaw_def_);


    end_position_ = home_joint_pose_;

    robot_tf_listener_.waitForTransform("world", "arm1_ee_link", ros::Time(0),
                                            ros::Duration(10));
    robot_tf_listener_.lookupTransform("/world", "/arm1_ee_link", ros::Time(0),
                                           robot_tf_transform_);

    home_cart_pose_.position.x = robot_tf_transform_.getOrigin().x();
    home_cart_pose_.position.y = robot_tf_transform_.getOrigin().y();
    home_cart_pose_.position.z = robot_tf_transform_.getOrigin().z();
    home_cart_pose_.orientation.x = robot_tf_transform_.getRotation().x();
    home_cart_pose_.orientation.y = robot_tf_transform_.getRotation().y();
    home_cart_pose_.orientation.z = robot_tf_transform_.getRotation().z();
    home_cart_pose_.orientation.w = robot_tf_transform_.getRotation().w();

    agv_tf_listener_.waitForTransform("world", "kit_tray_1",
                                      ros::Time(0), ros::Duration(10));
    agv_tf_listener_.lookupTransform("/world", "/kit_tray_1",
                                     ros::Time(0), agv_tf_transform_);
    agv_position_.position.x = agv_tf_transform_.getOrigin().x();
    agv_position_.position.y = agv_tf_transform_.getOrigin().y();
    agv_position_.position.z = agv_tf_transform_.getOrigin().z() + 4 * offset_;

    gripper_client_ = robot_controller_nh_.serviceClient<osrf_gear::VacuumGripperControl>(
            "/ariac/arm1/gripper/control");
    gripper_client_2 = robot_controller_nh_2.serviceClient<osrf_gear::VacuumGripperControl>(
            "/ariac/arm2/gripper/control");
    // break_beam_subscriber_ = robot_controller_nh_.subscribe("/ariac/break_beam_1_change", 10, &RobotController::break_beam_callback_,this);

    counter_ = 0;
    drop_flag_ = false;

    quality_control_camera_subscriber_ = robot_controller_nh_.subscribe("/ariac/quality_control_sensor_1", 10,
    		&RobotController::qualityControlSensor1Callback, this);
    quality_control_camera_2_subscriber_ = robot_controller_nh_.subscribe("/ariac/quality_control_sensor_2", 10,
    		&RobotController::qualityControlSensor2Callback, this);

    is_faulty_ = false;
}

void RobotController::qualityControlSensor1Callback(const osrf_gear::LogicalCameraImage::ConstPtr &image_msg) {
	is_faulty_ = !image_msg->models.empty(); // if not empty then part is faulty
	if(is_faulty_){
		ROS_WARN_STREAM("Product is faulty and models.size is " << image_msg->models.size());
	}
	// else ROS_INFO_STREAM_THROTTLE(7, "prodcut is NOTTT faulty....and models.size is " << image_msg->models.size());
}

void RobotController::qualityControlSensor2Callback(const osrf_gear::LogicalCameraImage::ConstPtr &image_msg) {
	is_faulty_ = !image_msg->models.empty(); // if not empty then part is faulty
	if(is_faulty_){
		ROS_WARN_STREAM("Product is faulty and models.size is " << image_msg->models.size());
	}
	// else ROS_INFO_STREAM_THROTTLE(7, "prodcut is NOTTT faulty....and models.size is " << image_msg->models.size());
}

RobotController::~RobotController() {}

/**
 *
 * @return
 */
bool RobotController::Planner() {
    ROS_INFO_STREAM("Planning started...");
    if (robot_move_group_.plan(robot_planner_) ==
        moveit::planning_interface::MoveItErrorCode::SUCCESS) {
        plan_success_ = true;
        ROS_INFO_STREAM("Planner succeeded!");
    } else {
        plan_success_ = false;
        ROS_WARN_STREAM("Planner failed!");
    }

    return plan_success_;
}


void RobotController::Execute() {
    ros::AsyncSpinner spinner(4);
    spinner.start();
    if (this->Planner()) {
        robot_move_group_.move();
        ros::Duration(0.5).sleep();
    }
}

void RobotController::GoToTarget(const geometry_msgs::Pose& pose, int f) {

    if (f==0){
    target_pose_.orientation=fixed_orientation_;
    }
    else if(f==1){
    target_pose_.orientation= conveyor_fixed_orientation_;
    }

    target_pose_.position = pose.position ;
    ros::AsyncSpinner spinner(4);
    robot_move_group_.setPoseTarget(target_pose_);
    spinner.start();
    if (this->Planner()) {
        robot_move_group_.move();
        ros::Duration(0.5).sleep();
    }
    ROS_INFO_STREAM("Point reached...");
}

void RobotController::GoToTarget(
        std::initializer_list<geometry_msgs::Pose> list, int f) {
    ros::AsyncSpinner spinner(4);
    spinner.start();

    std::vector<geometry_msgs::Pose> waypoints;
    if(f==0)
    {
    for (auto i : list) {
        i.orientation.x = fixed_orientation_.x;
        i.orientation.y = fixed_orientation_.y;
        i.orientation.z = fixed_orientation_.z;
        i.orientation.w = fixed_orientation_.w;
        waypoints.emplace_back(i);
    }}
    else if(f==1)
    {
          for (auto i : list) {
        i.orientation.x = conveyor_fixed_orientation_.x;
        i.orientation.y = conveyor_fixed_orientation_.y;
        i.orientation.z = conveyor_fixed_orientation_.z;
        i.orientation.w = conveyor_fixed_orientation_.w;
        waypoints.emplace_back(i);
    }
    }

    moveit_msgs::RobotTrajectory traj;
    auto fraction =
            robot_move_group_.computeCartesianPath(waypoints, 0.01, 0.0, traj, true);

    ROS_WARN_STREAM("Fraction: " << fraction * 100);
    ros::Duration(0.5).sleep();

    robot_planner_.trajectory_ = traj;

    //if (fraction >= 0.3) {
        robot_move_group_.execute(robot_planner_);
        ros::Duration(0.5).sleep();
//    } else {
//        ROS_ERROR_STREAM("Safe Trajectory not found!");
//    }
}

void RobotController::SendRobotHome() {
    // ros::Duration(2.0).sleep();
    robot_move_group_.setJointValueTarget(home_joint_pose_);
    // this->execute();
    ros::AsyncSpinner spinner(4);
    spinner.start();
    if (this->Planner()) {
        robot_move_group_.move();
        ros::Duration(0.5).sleep();
    }
     ros::Duration(0.5).sleep();
}

void RobotController::SendRobotHome1() {
    // ros::Duration(2.0).sleep();
    robot_move_group_.setJointValueTarget(home_joint_pose_1);
    // this->execute();
    ros::AsyncSpinner spinner(4);
    spinner.start();
    if (this->Planner()) {
        robot_move_group_.move();
        ros::Duration(0.5).sleep();
    }
     ros::Duration(0.5).sleep();
}

void RobotController::SendRobotHome2() {
    // ros::Duration(2.0).sleep();
    robot_move_group_2.setJointValueTarget(home_joint_pose_2);
    // this->execute();
    ros::AsyncSpinner spinner(4);
    spinner.start();
    if (this->Planner()) {
        robot_move_group_2.move();
        ros::Duration(0.5).sleep();
    }
     ros::Duration(0.5).sleep();
}


void RobotController::SendRobotPosition(std::vector<double> pose) {
    // ros::Duration(2.0).sleep();
    robot_move_group_.setJointValueTarget(pose);
    // this->execute();
    ros::AsyncSpinner spinner(4);
    spinner.start();
    if (this->Planner()) {
        robot_move_group_.move();
        ros::Duration(1.5).sleep();
    }
     ros::Duration(0.5).sleep();
}

void RobotController::GripperToggle(const bool& state) {
    gripper_service_.request.enable = state;
    gripper_client_.call(gripper_service_);
    ros::Duration(1.0).sleep();
    // if (gripper_client_.call(gripper_service_)) {
    if (gripper_service_.response.success) {
        ROS_INFO_STREAM("Gripper activated!");
    } else {
        ROS_WARN_STREAM("Gripper activation failed!");
    }
}

void RobotController::GripperToggle2(const bool& state) {
    gripper_service_2.request.enable = state;
    gripper_client_2.call(gripper_service_2);
    ros::Duration(1.0).sleep();
    // if (gripper_client_.call(gripper_service_)) {
    if (gripper_service_2.response.success) {
        ROS_INFO_STREAM("Gripper activated!");
    } else {
        ROS_WARN_STREAM("Gripper activation failed!");
    }
}

bool RobotController::DropPart(geometry_msgs::Pose part_pose, int agv_id) {
  counter_++;

  pick = false;
  drop = true;

  ROS_WARN_STREAM("Dropping the part number: " << counter_);



  ROS_INFO_STREAM("Moving to end of conveyor...");
  if (agv_id == 2)
    SendRobotHome2();
  else
    SendRobotPosition(kit_drop_pose_);

  // robot_move_group_.setJointValueTarget(kit_drop_pose_);
  // this->Execute();
  ros::Duration(1.0).sleep();


  part_pose.position.z += 0.1;
  auto temp_pose = part_pose;
  // auto temp_pose = agv_position_;
  temp_pose.position.z += 0.35;

  // Going to kit here
  this->GoToTarget({temp_pose, part_pose},0);
  // ros::Duration(0.5).sleep();

  ROS_INFO_STREAM("Checking if part if faulty");

  ros::Duration(1.0).sleep();
  ros::spinOnce();
  ros::Duration(0.5).sleep();
  if( not is_faulty_ ) {
    if (agv_id == 1) {
      this->GripperToggle(false);
  	  ROS_INFO_STREAM("Dropping");
      SendRobotPosition(bin_drop_pose_);
  	  // robot_move_group_.setJointValueTarget(kit_drop_pose_);
  	  // this->Execute();
  	  ros::Duration(1.0).sleep();
  	  drop =true;
    } else {
      this->GripperToggle2(false);
  	  ROS_INFO_STREAM("Dropping");
      SendRobotPosition(bin_drop_pose_);
  	  // robot_move_group_.setJointValueTarget(kit_drop_pose_);
  	  // this->Execute();
  	  ros::Duration(1.0).sleep();
  	  drop =true;
    }
  }
  else{
	  ROS_INFO_STREAM("Moving to end of conveyor");
//	  robot_move_group_.setJointValueTarget(bin_drop_pose_);
	  // SendRobotPosition(conveyor);
//	  this->Execute();
	  ros::Duration(1.0).sleep();
    if (agv_id == 1) this->GripperToggle(false);

    else this->GripperToggle2(false);

    drop = false;
  }
  ROS_INFO_STREAM("Moving to end of conveyor...");

  // SendRobotPosition(bin_drop_pose_);
  // robot_move_group_.setJointValueTarget(kit_drop_pose_);
  // this->Execute();
  // if(faulty-==true)
  // {

  // }


  // this->sendRobotHome();
  // temp_pose = home_cart_pose_;
  // temp_pose.position.z -= 0.05;


  // this->GoToTarget({temp_pose, home_cart_pose_},1);
  return drop;
}

void RobotController::GripperCallback(
        const osrf_gear::VacuumGripperState::ConstPtr& grip) {
    gripper_state_ = grip->attached;
}

void RobotController::GripperCallback2(
        const osrf_gear::VacuumGripperState::ConstPtr& grip) {
    gripper_state_2 = grip->attached;
}


bool RobotController::PickPart(geometry_msgs::Pose& part_pose, int agv_id) {
    // gripper_state = false;
    // pick = true;
    //ROS_INFO_STREAM("fixed_orientation_" << part_pose.orientation = fixed_orientation_);
    //ROS_WARN_STREAM("Picking the part...");
    moveit::planning_interface::PlanningSceneInterface planning_scene_interface;
  moveit::planning_interface::MoveGroupInterface group(robot_controller_options);

  moveit_msgs::CollisionObject collision_object;
  collision_object.header.frame_id = robot_move_group_.getPlanningFrame();

  // The id of the object is used to identify it.
  collision_object.id = "box1";

  // Define a box to add to the world.
  shape_msgs::SolidPrimitive primitive;
  primitive.type = primitive.BOX;
  primitive.dimensions.resize(3);
  primitive.dimensions[0] = 0.6;
  primitive.dimensions[1] = 0.6;
  primitive.dimensions[2] = 0.55;

  // Define a pose for the box (specified relative to frame_id)
  geometry_msgs::Pose box_pose;
  box_pose.orientation.w = 1.0;
  box_pose.position.x = -0.3;
  box_pose.position.y = -1.916;
  box_pose.position.z = 0.45;

  collision_object.primitives.push_back(primitive);
  collision_object.primitive_poses.push_back(box_pose);
  collision_object.operation = collision_object.ADD;

  std::vector<moveit_msgs::CollisionObject> collision_objects;
  collision_objects.push_back(collision_object);
  ROS_INFO_NAMED("tutorial", "Add an object into the world");
   planning_scene_interface.applyCollisionObjects(collision_objects);

    ROS_INFO_STREAM("Moving to part...");
    if (agv_id == 1) {
      part_pose.position.z = part_pose.position.z + 0.043;
      // part_pose.position.x = part_pose.position.x - 0.02;
      // part_pose.position.y = part_pose.position.y - 0.02;
      auto temp_pose_1 = part_pose;
      temp_pose_1.position.z += 0.2;
      ROS_INFO_STREAM("Actuating the gripper..." << part_pose.position.z);
      this->GripperToggle(true);
      this->GoToTarget({temp_pose_1, part_pose},0);
      // GoToTarget(part_pose);

      // ROS_INFO_STREAM("Actuating the gripper..." << part_pose.position.z);
      // this->GripperToggle(true);
      ros::spinOnce();
      while (!gripper_state_) {
          part_pose.position.z -= 0.01;
          this->GoToTarget({temp_pose_1, part_pose},0);
          // GoToTarget(part_pose);
          ROS_INFO_STREAM("Actuating the gripper...");
          this->GripperToggle(true);
          ros::spinOnce();
      }

      ROS_INFO_STREAM("Going to waypoint...");
      // part_pose.position.z += 0.2;
      GoToTarget(temp_pose_1,0);
      // GoToTarget(part_pose);
      return gripper_state_;

    } else {
      part_pose.position.z = part_pose.position.z + 0.033;
      // part_pose.position.x = part_pose.position.x - 0.02;
      // part_pose.position.y = part_pose.position.y - 0.02;
      auto temp_pose_1 = part_pose;
      temp_pose_1.position.z += 0.2;
      ROS_INFO_STREAM("Actuating the gripper..." << part_pose.position.z);
      this->GripperToggle2(true);
      this->GoToTarget({temp_pose_1, part_pose},0);
      // GoToTarget(part_pose);

      // ROS_INFO_STREAM("Actuating the gripper..." << part_pose.position.z);
      // this->GripperToggle(true);
      ros::spinOnce();
      while (!gripper_state_2) {
          part_pose.position.z -= 0.01;
          this->GoToTarget({temp_pose_1, part_pose},0);
          // GoToTarget(part_pose);
          ROS_INFO_STREAM("Actuating the gripper...");
          this->GripperToggle2(true);
          ros::spinOnce();
      }

      ROS_INFO_STREAM("Going to waypoint...");
      // part_pose.position.z += 0.2;
      GoToTarget(temp_pose_1,0);
      // GoToTarget(part_pose);
      return gripper_state_2;
    }

}

void RobotController::sendRobotToConveyor(){
	SendRobotPosition(conveyor);
}

/*ool RobotController::PickPartconveyor(std::string product){

    ROS_INFO_STREAM("Moving to Conveyor");
    GripperToggle(true);
    SendRobotPosition(conveyor);
    robot_tf_listener_.waitForTransform("arm1_linear_arm_actuator", "arm1_ee_link",
                                            ros::Time(0), ros::Duration(10));
    robot_tf_listener_.lookupTransform("/arm1_linear_arm_actuator", "/arm1_ee_link",
                                           ros::Time(0), robot_tf_transform_);


    conveyor_fixed_orientation_.x = 0.00386643;
    conveyor_fixed_orientation_.y = 0.700713;
    conveyor_fixed_orientation_.z = 0.0037416;
    conveyor_fixed_orientation_.w = 0.713423;
   conveyor_part.position.x=1.218116;
   conveyor_part.position.y=2.214948;
   conveyor_part.position.z=0.955569-0.03;
   // GoToTarget(conveyor_part,1);

int v=0;
std::string temp;
//  while(!beam.getBeam() && beam.getpart()!=product){
//     if(beam.getpart()!=product){
//       ROS_INFO_STREAM("PRODUCT"<<product);
//       while(v==0)
//       {
//         ros::spinOnce();
//         if(beam.getBeam())
//         {
//           v=1;
//         }
//       }


//  }
// ros::spinOnce();
// }

while(!beam.getBeam())
{
  ros::spinOnce();

}
    GoToTarget(conveyor_part,1);
    ros::spinOnce();
    conveyor_part.position.z=0.955569+0.1;
    if(gripper_state_){
    GoToTarget(conveyor_part,1);
    }
    return gripper_state_;
}*/

//will require a vector of binparts, including "n"
bool RobotController::PickPartConveyor(std::string part_name, std::vector<std::string> parts_already_in_bin, 
                                      std::vector<std::string> bins_arm1, std::vector<std::string> bins_arm2, 
                                      std::vector<std::string> bin_part){
  bool present_flag = false;
  bool empty_flag = false;
  ROS_INFO_STREAM("Inside pick conveyor");

  temp_pose_["shoulder_pan_joint"] = 0;
  temp_pose_["shoulder_lift_joint"] = -0.5;
  temp_pose_["elbow_joint"] = 0.5;
  temp_pose_["wrist_1_joint"] = 0;
  temp_pose_["wrist_2_joint"] = 0;
  temp_pose_["wrist_3_joint"] = 0;
  temp_pose_["linear_arm_actuator_joint"] = 0;

  final_.orientation.w = 0.707;
  final_.orientation.y = 0.707;
  final_.position.x = 1.22;
  final_.position.y = 1.9; //0.7 - 1.8
  final_.position.z = 0.95;

  // robot_move_group_.setJointValueTarget(temp_pose_);
  // robot_move_group_.move();
  // robot_move_group_2.setJointValueTarget(temp_pose_);
  // robot_move_group_2.move();
  // robot_move_group_.setJointValueTarget(final_);
  // robot_move_group_.move();
  if(std::find(parts_already_in_bin.begin(), parts_already_in_bin.end(), part_name) != parts_already_in_bin.end()){
    ROS_INFO_STREAM("Part is present in one bin");
    present_flag = true;
  }
  else{
    ROS_INFO_STREAM("Part is not present in any bin");
    empty_flag = true;
  }

  if(present_flag == true){
    for(int i = 0; i < 6; i++){
      if(bin_part[i] == part_name){
        if((i+1) == 1 or (i+1) == 2 or (i+1) == 3){
          ROS_INFO_STREAM("Arm2 for placing from bin: " << i+1);
        }
        else if((i+1) == 4 or (i+1) == 5 or (i+1) == 6){
          ROS_INFO_STREAM("Arm1 for placing from bin: "<< i+1);
        }
      }
    }

  }
  if(empty_flag == true){
    for(int i = 0; i < 6; i++){
      if(bin_part[i] == "n"){
        if((i+1) == 1 or (i+1) == 2 or (i+1) == 3){
          ROS_INFO_STREAM("Arm2 for placing in bin: " << i+1);
        }
        else if((i+1) == 4 or (i+1) == 5 or (i+1) == 6){
          ROS_INFO_STREAM("Arm1 for placing in bin: " << i+1);
        }

      }
    }
  }
  return false;
}

bool RobotController::PickPartconveyor(std::string product){
  ROS_INFO_STREAM("Inside pick_conv function");
  gripper_state_ = false;
  ros::AsyncSpinner spinner(1);
  spinner.start();
  robot_move_group_.setPlanningTime(20);
  robot_move_group_.setNumPlanningAttempts(10);
  robot_move_group_.setPlannerId("RRTConnectkConfigDefault");
  robot_move_group_.setMaxVelocityScalingFactor(0.9);
  robot_move_group_.setMaxAccelerationScalingFactor(0.9);
  robot_move_group_.allowReplanning(true);

  temp_pose_["shoulder_pan_joint"] = 0;
  temp_pose_["shoulder_lift_joint"] = -0.5;
  temp_pose_["elbow_joint"] = 0.5;
  temp_pose_["wrist_1_joint"] = 0;
  temp_pose_["wrist_2_joint"] = 0;
  temp_pose_["wrist_3_joint"] = 0;
  temp_pose_["linear_arm_actuator_joint"] = 0;

  robot_move_group_.setJointValueTarget(temp_pose_);
  robot_move_group_.move();
  ROS_INFO_STREAM("Move to temp position");
  ros::Duration(0.5).sleep();
  final_.orientation.w = 0.707;
  final_.orientation.y = 0.707;
  final_.position.x = 1.22;
  final_.position.y = 1.9; //0.7 - 1.8
  final_.position.z = 0.95;

  robot_move_group_.setPoseTarget(final_ );
  robot_move_group_.move();
  ROS_INFO_STREAM("Move to final_ position");


  grab_pose_ = final_;
  // place_pose_ = final_;
  place_pose_.orientation.w = 0.707;
  place_pose_.orientation.y = 0.707;
  place_pose_.position.x = -0.288;
  place_pose_.position.y = 1.144; //0.7 - 1.8
  place_pose_.position.z = 0.8;

  grab_pose_.position.z = 0.928;
  // int trial = beam.object_derived;
  // ros::spin();
  while(ros::ok())
  {
    // ros::Subscriber logical_camera_subscriber_7 = node.subscribe("/ariac/logical_camera_7", 10, 
    //           &AriacSensorManager::LogicalCamera7Callback, &AriacSensorManager);
    // ROS_INFO_STREAM("Inside...");
    // ROS_INFO_STREAM_THROTTLE(2, "Grab now: "<< beam.grab_now_1);
    // ROS_INFO_STREAM_THROTTLE(2, "obj derived: "<< beam.object_derived<<" break_beam_counter: "<<beam.break_beam_counter);
    // ROS_INFO_STREAM_THROTTLE(2, "arm1 engage: "<< beam.arm1_engage_derived);

  if(beam.grab_now_1 == true && beam.object_derived == beam.break_beam_counter && beam.arm1_engage_derived == true){
    ROS_INFO_STREAM("Grabbing now...");
    // ros::Duration(1.0).sleep();
    grab_pose_.position.x = beam.x_grab;
    grab_pose_.position.y = beam.y_grab-0.15;
    grab_pose_.position.z = 0.930; //0.928
    robot_move_group_.setPoseTarget(grab_pose_);
    this->GripperToggle(true);
    robot_move_group_.move();
    ROS_INFO_STREAM("Grab pose reached");

    beam.grab_now_1 = false;
    beam.arm1_engage_derived = false;
    ROS_INFO_STREAM("Gripper state:" << gripper_state_);
    if(gripper_state_){
      place_pose_.position.x = 0.25;
      place_pose_.position.y = 1;
      place_pose_.position.z = 1.2;
      robot_move_group_.setPoseTarget(place_pose_);
      robot_move_group_.move(); 
      ROS_INFO_STREAM("place pose reached"); 
      ros::Duration(0.5).sleep();
      this->GripperToggle(false);
    }
    robot_move_group_.setPoseTarget(final_);
    robot_move_group_.move();
    ROS_INFO_STREAM("final pose reached");

  }
}
  ros::spin();
  return gripper_state_;
}

/*bool RobotController::PickPartconveyor(std::string product){
  ROS_INFO_STREAM("Inside pick_conv function");
  std::map<std::string, double> pose_after_grab;
  gripper_state_ = true;
  ros::AsyncSpinner spinner(1);
  spinner.start();
  robot_move_group_.setPlanningTime(20);
  robot_move_group_.setNumPlanningAttempts(10);
  robot_move_group_.setPlannerId("RRTConnectkConfigDefault");
  robot_move_group_.setMaxVelocityScalingFactor(0.9);
  robot_move_group_.setMaxAccelerationScalingFactor(0.9);
  robot_move_group_.allowReplanning(true);

  temp_pose_["shoulder_pan_joint"] = 0;
  temp_pose_["shoulder_lift_joint"] = -0.5;
  temp_pose_["elbow_joint"] = 0.5;
  temp_pose_["wrist_1_joint"] = 0;
  temp_pose_["wrist_2_joint"] = 0;
  temp_pose_["wrist_3_joint"] = 0;
  temp_pose_["linear_arm_actuator_joint"] = 0;

  pose_after_grab["shoulder_pan_joint"] = 1.63;
  pose_after_grab["shoulder_lift_joint"] = -1.26;
  pose_after_grab["elbow_joint"] = 2.01;
  pose_after_grab["wrist_1_joint"] = 4.02;
  pose_after_grab["wrist_2_joint"] = -1.51;
  pose_after_grab["wrist_3_joint"] = 0;
  pose_after_grab["linear_arm_actuator_joint"] = -0.05;

  robot_move_group_.setJointValueTarget(temp_pose_);
  robot_move_group_.move();
  ROS_INFO_STREAM("Move to temp position");
  ros::Duration(0.5).sleep();
  final_.orientation.w = 0.707;
  final_.orientation.y = 0.707;
  final_.position.x = 1.22;
  final_.position.y = 1.8; //0.7 - 1.8
  final_.position.z = 0.95;

  robot_move_group_.setPoseTarget(final_ );
  robot_move_group_.move();
  ROS_INFO_STREAM("Move to final_ position");


  grab_pose_ = final_;
  place_pose_ = final_;

  grab_pose_.position.z = 0.925;
  // int trial = beam.object_derived;
  // ros::spin();
  while(ros::ok())
  {
    // ros::Subscriber logical_camera_subscriber_7 = node.subscribe("/ariac/logical_camera_7", 10, 
    //           &AriacSensorManager::LogicalCamera7Callback, &AriacSensorManager);
    // ROS_INFO_STREAM("Inside...");
    // ROS_INFO_STREAM_THROTTLE(2, "Grab now: "<< beam.grab_now_1);
    // ROS_INFO_STREAM_THROTTLE(2, "obj derived: "<< beam.object_derived<<" break_beam_counter: "<<beam.break_beam_counter);
    // ROS_INFO_STREAM_THROTTLE(2, "arm1 engage: "<< beam.arm1_engage_derived);
  //   gripper_state_ = true;
  // while(gripper_state_ == true){  /*&& beam.object_derived == beam.break_beam_counter && beam.arm1_engage_derived == true
    // ROS_INFO_STREAM("Grab now: " << beam.grab_now_1);
    // gripper_state_ = true;
    if(beam.grab_now_1 == true){
    ROS_INFO_STREAM("Grabbing now...");
    // ros::Duration(0.5).sleep();
    grab_pose_.position.x = beam.x_grab;
    grab_pose_.position.y = beam.y_grab-0.10;
    grab_pose_.position.z = 0.925; //0.928
    robot_move_group_.setPoseTarget(grab_pose_);
    GripperToggle(true);
    robot_move_group_.move();
    ROS_INFO_STREAM("Grab pose reached");

    beam.grab_now_1 = false;
    beam.arm1_engage_derived = false;
    ROS_INFO_STREAM("Gripper state:" << gripper_state_);
    if(gripper_state_){
      place_pose_.position.x = 0.25;
      place_pose_.position.y = 1;
      place_pose_.position.z = 1.2;
      robot_move_group_.setPoseTarget(place_pose_);
      robot_move_group_.move(); 
      ROS_INFO_STREAM("place pose reached"); 
      ros::Duration(0.5).sleep();
      GripperToggle(false);
    }
    ROS_INFO_STREAM("Inside if...");
    if(!gripper_state_){
      ROS_INFO_STREAM("Break..");
      break;
    }
    robot_move_group_.setPoseTarget(final_);
    robot_move_group_.move();
    // ROS_INFO_STREAM("final pose reached");
  }
  // }
}
  // ros::spin();
  ROS_INFO_STREAM("Return...");
  return gripper_state_;
}*/

bool RobotController::PickPart(geometry_msgs::Pose& part_pose) {
    // gripper_state = false;
    // pick = true;
    //ROS_INFO_STREAM("fixed_orientation_" << part_pose.orientation = fixed_orientation_);
    //ROS_WARN_STREAM("Picking the part...");
    moveit::planning_interface::PlanningSceneInterface planning_scene_interface;
  moveit::planning_interface::MoveGroupInterface group(robot_controller_options);

  moveit_msgs::CollisionObject collision_object;
  collision_object.header.frame_id = robot_move_group_.getPlanningFrame();

  // The id of the object is used to identify it.
  collision_object.id = "box1";

  // Define a box to add to the world.
  shape_msgs::SolidPrimitive primitive;
  primitive.type = primitive.BOX;
  primitive.dimensions.resize(3);
  primitive.dimensions[0] = 0.6;
  primitive.dimensions[1] = 0.6;
  primitive.dimensions[2] = 0.55;

  // Define a pose for the box (specified relative to frame_id)
  geometry_msgs::Pose box_pose;
  box_pose.orientation.w = 1.0;
  box_pose.position.x = -0.3;
  box_pose.position.y = -1.916;
  box_pose.position.z = 0.45;

  collision_object.primitives.push_back(primitive);
  collision_object.primitive_poses.push_back(box_pose);
  collision_object.operation = collision_object.ADD;

  std::vector<moveit_msgs::CollisionObject> collision_objects;
  collision_objects.push_back(collision_object);
  ROS_INFO_NAMED("tutorial", "Add an object into the world");
  planning_scene_interface.applyCollisionObjects(collision_objects);

  ROS_INFO_STREAM("Moving to part...");
  part_pose.position.z = part_pose.position.z + 0.033;
      // part_pose.position.x = part_pose.position.x - 0.02;
      // part_pose.position.y = part_pose.position.y - 0.02;
  auto temp_pose_1 = part_pose;
  temp_pose_1.position.z += 0.2;
  ROS_INFO_STREAM("Actuating the gripper..." << part_pose.position.z);
  this->GripperToggle2(true);
  this->GoToTarget({temp_pose_1, part_pose},0);
      // GoToTarget(part_pose);

      // ROS_INFO_STREAM("Actuating the gripper..." << part_pose.position.z);
      // this->GripperToggle(true);
  ros::spinOnce();
  while (!gripper_state_2) {
    part_pose.position.z -= 0.01;
    this->GoToTarget({temp_pose_1, part_pose},0);
    // GoToTarget(part_pose);
    ROS_INFO_STREAM("Actuating the gripper...");
    this->GripperToggle2(true);
    ros::spinOnce();
  }

  ROS_INFO_STREAM("Going to waypoint...");
  // part_pose.position.z += 0.2;
  GoToTarget(temp_pose_1,0);
  // GoToTarget(part_pose);
  return gripper_state_2;
}

bool RobotController::DropPart(geometry_msgs::Pose part_pose) {
  counter_++;

  pick = false;
  drop = true;

  ROS_WARN_STREAM("Dropping the part number: " << counter_);



  ROS_INFO_STREAM("Moving to end of conveyor...");

  ros::Duration(1.0).sleep();

  // part_pose.position.y += 0.1;
  part_pose.position.z += 0.1;
  auto temp_pose = part_pose;

  temp_pose.position.z += 0.35;

  // Going to kit here
  this->GoToTarget({temp_pose, part_pose},0);
  // ros::Duration(0.5).sleep();

  ros::Duration(1.0).sleep();
  ros::spinOnce();
  ros::Duration(0.5).sleep();
	ROS_INFO_STREAM("Moving to end of conveyor");

	ros::Duration(1.0).sleep();

  this->GripperToggle2(false);

  drop = false;
  SendRobotHome2();

  return drop;
}
