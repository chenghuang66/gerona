#include <ros/ros.h>
#include <tf/transform_listener.h>
#include <sensor_msgs/PointCloud2.h>

#include "../../scan2cloud/include/scan2cloud/scan_processor.h"

class Scan2CloudHelper {
public:

    Scan2CloudHelper():
        private_node_("~")
    {
        // init parameter with a default value
        private_node_.param<std::string>("fixedFrame",fixed_frame_,"base_link");


        proc_.SetParams(private_node_);

        float affMin,affMax;
        private_node_.param<float>("angleFilterFrontMin",affMin,-5.0f);
        private_node_.param<float>("angleFilterFrontMax",affMax,5.0f);
        angleFront_.setX(affMin);
        angleFront_.setY(affMax);

        float afbMin,afbMax;
        private_node_.param<float>("angleFilterBackMin",afbMin,-5.0f);
        private_node_.param<float>("angleFilterBackMax",afbMax,5.0f);
        angleBack_ .setX(afbMin);
        angleBack_ .setY(afbMax);



        GetScanMask(private_node_,"maskFront",maskFront_);
        GetScanMask(private_node_,"maskBack",maskBack_);

        proc_.fixed_frame_ = fixed_frame_;

        scan_sub_front_ = node_.subscribe<sensor_msgs::LaserScan>(
                    "scan_front", 2, boost::bind(&Scan2CloudHelper::scanCallback, this, _1, false));
        scan_sub_back_  = node_.subscribe<sensor_msgs::LaserScan>(
                    "scan_back", 2, boost::bind(&Scan2CloudHelper::scanCallback, this ,_1, true));
        point_cloud_publisher_ = node_.advertise<sensor_msgs::PointCloud2>("cloud_total", 1, false);
    }

    void GetScanMask(ros::NodeHandle &nh, std::string param, std::vector<bool> &mask)
    {
        std::vector<int> calib_values;
        std::vector<int> calib_values_default = {811, 0, 1};

        nh.param<std::vector<int> >(param, calib_values, calib_values_default);

        mask.resize(calib_values[0],true);

        for(int i = 1; i < calib_values.size(); i++)
        {
            mask[calib_values[i]] = false;
        }

    }

    void scanCallback(const sensor_msgs::LaserScan::ConstPtr &scan_in, bool is_back)
    {
        lastStamp_ = scan_in->header.stamp;

        if (is_back) {
            proc_.ProcessScan(*scan_in,maskBack_,angleBack_,back_points_);
            cbScanback_ = true;
        } else {
            proc_.ProcessScan(*scan_in,maskFront_,angleFront_,front_points_);
            cbScanfront_ = true;
        }

    }

    void spin()
    {

        cbScanfront_ = false;
        cbScanback_ = false;
        ros::Rate loopRate(500);
        while(ros::ok()){
            //ros::spinOnce();
            cbScanfront_ = false;
            cbScanback_ = false;

            ros::getGlobalCallbackQueue()->callAvailable(ros::WallDuration(0));

            if(cbScanfront_ || cbScanback_){
                proc_.CreateCloud(back_points_,front_points_,fixed_frame_,lastStamp_,cloud_total_);
                point_cloud_publisher_.publish(cloud_total_);
            }

            //this->updateParameter();

            loopRate.sleep();
        }
    }
private:
    ros::NodeHandle node_;
    ros::NodeHandle private_node_;
    tf::TransformListener tfListener_;

    ros::Publisher point_cloud_publisher_;
    ros::Subscriber scan_sub_back_;
    ros::Subscriber scan_sub_front_;

    std::vector<tf::Point> front_points_;
    std::vector<tf::Point> back_points_;


    sensor_msgs::PointCloud2 cloud_total_;

    ros::Time lastStamp_;


    bool cbScanfront_;
    bool cbScanback_;

    // ros params
    std::string fixed_frame_;

    ScanProcessor proc_;

    std::vector<bool> maskFront_;
    std::vector<bool> maskBack_;
    tf::Point angleFront_;
    tf::Point angleBack_;









};


int main(int argc, char** argv)
{
    ros::init(argc, argv, "scan2cloud", ros::init_options::NoSigintHandler);

    Scan2CloudHelper scanHelper;
    scanHelper.spin();

    return 0;
}

