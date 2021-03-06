#include "ros/ros.h"
#include <geometry_msgs/Twist.h>
#include "geometry_msgs/Point.h"
#include "geometry_msgs/Quaternion.h"
#include "nav_msgs/Odometry.h"
#include "std_msgs/String.h"
#include "std_msgs/Float32.h"
#include <cmath>
#include <tf/transform_datatypes.h>
#include "geometry_msgs/Point.h"

#define rotation_threshold 2




class rotation_action {
private:

    ros::NodeHandle n;

    // communication with cmd_vel to send command to the mobile robot
    ros::Publisher pub_cmd_vel;

    // communication with odometry
    ros::Publisher pub_change_odometry;
    ros::Subscriber sub_odometry;
    
    // communication with decision
    ros::Publisher pub_rotation_done;
    ros::Subscriber sub_rotation_to_do;

    float rotation_to_do, rotation_done, rotation_offset;
    int cond_rotation;// boolean to check if we still have to rotate or not
    float kp ; 
    float ki ; 
    float kd ; 
    float ep ;
    float ei ; 
    float ed ; 
    

public:
    int flag;
rotation_action() {
    flag = 0;
    
    // communication with cmd_vel to command the mobile robot
    pub_cmd_vel = n.advertise<geometry_msgs::Twist>("cmd_vel", 1);

    // communication with odometry
    pub_change_odometry = n.advertise<geometry_msgs::Point>("change_odometry", 1);
    sub_odometry = n.subscribe("odom", 1, &rotation_action::odomCallback, this);
    cond_rotation = 0;

    // communication with decision
    pub_rotation_done = n.advertise<std_msgs::Float32>("rotation_done", 1);
    sub_rotation_to_do = n.subscribe("rotation_to_do", 1, &rotation_action::rotation_to_doCallback, this);//this is the rotation that has to be performed
    kp = 0.1;
    ki = 0.001;
    kd = 0.0;
    ep = 0.0;
    ed = 0.0;
    ei = 0.0;

}

void odomCallback(const nav_msgs::Odometry::ConstPtr& o) {

    rotation_done = tf::getYaw(o->pose.pose.orientation);

    
    if(!flag)
    {
        //rotation_offset = rotation_done;
        rotation_offset = 0.0;
        flag++;
    }
    rotation_done = rotation_done - rotation_offset; 
    
    //ROS_INFO("(rotation_node) rotation_done: %f, rotation_to_do: %f ", rotation_done*180/M_PI, rotation_to_do*180/M_PI);
    if ( cond_rotation ) {
        geometry_msgs::Twist twist;
        twist.linear.x = 0;
        twist.linear.y = 0;
        twist.linear.z = 0;

        twist.angular.x = 0;
        twist.angular.y = 0;
        twist.angular.z = 0;

       // ROS_INFO("x (rotation_node) rotation_done: %f, rotation_to_do: %f ", (rotation_to_do -  rotation_done)*180/M_PI, rotation_to_do*180/M_PI);

        if (fabs(rotation_to_do - rotation_done)> 0.1 )  {
         
         
         
         // implementation of a PID controller
          
         	ed = ed - (rotation_to_do - rotation_done);
            ep = (rotation_to_do - rotation_done);//the current error is the difference between the rotation_to_do (ie, the angle to reach) and the rotation_done (ie, the current angle)
			ei +=ep;
			twist.angular.z = kp * ep + ki * ei + kd * ed;
            //twist.angular.z *= kp * ep + ki * ei + kd * ed;
            ROS_INFO("Twist angle in rotation %f ", twist.angular.z*180/M_PI);

            pub_cmd_vel.publish(twist);
        }
        else {
          
            cond_rotation = 0;
            //ROS_INFO("y (rotation_node) rotation_to_do: %f", rotation_to_do*180/M_PI);
            //ROS_INFO("z (rotation_node) rotation_done: %f", rotation_done*180/M_PI);
            ep = 0;
            ei = 0;
            ed = 0;
            std_msgs::Float32 msg_rotation_done;
            msg_rotation_done.data = rotation_done;
            ROS_INFO("w (rotation_node) rotation_done : %f", msg_rotation_done.data*180/M_PI);

            pub_rotation_done.publish(msg_rotation_done);//we sent the rotation_done to decision_node;
            ros::Duration(0.5).sleep();
            
        }
            
    }
 }

void rotation_to_doCallback(const std_msgs::Float32::ConstPtr & a) {
// process the rotation to do received from the decision node

    ROS_INFO("\n(rotation_node) processing the rotation_to_do received from the decision node");
    //getchar();
    rotation_to_do = a->data;

    ROS_INFO("(rotation_node) rotation_to_do : %f", rotation_to_do*180/M_PI);
    //getchar();
    cond_rotation = 1;//we will perform a rotation

    //rotation_done = 0.0;
    
    /*
    std_msgs::Float32 msg_rotation_done;
    msg_rotation_done.data = rotation_done;
    ROS_INFO("DT (rotation_node) rotation_done : %f", msg_rotation_done.data*180/M_PI);

    pub_rotation_done.publish(msg_rotation_done);//we sent the rotation_done to decision_node;
    ros::Duration(1.0).sleep();*/


    //reset odometry
    geometry_msgs::Point p;
    p.x = 0.0;
    p.y = 0.0;
    p.z = 0.0;
   pub_change_odometry.publish(p);
   ros::Duration(0.5).sleep();

}

};

int main(int argc, char **argv){

    ros::init(argc, argv, "rotation_action");

    rotation_action bsObject;

    ros::spin();

    return 0;
}
