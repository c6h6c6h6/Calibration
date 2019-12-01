﻿/*
 * 2019.11.25 encode by hd,zy,ljy
 * calibration for encoder2odom
 * paper: antonelli2007
*/
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cstdio>
#include <string>
#include <fstream>
#include <chrono>
#include <cmath>
#include<ros/ros.h>

#include <Eigen/Core>
#include <Eigen/SVD>
#include <Eigen/Dense>

using namespace std;
using namespace Eigen;
#define PI 3.14159267

class OdomData //里程计数据类型
{
public:
    struct DataInPath //一条轨迹内的里程计数据
    {
        int states_num;            //脉冲状态数量
        vector<int> per_state_puls_r;//每次脉冲所含的脉冲个数
        vector<int> per_state_puls_l;//每次脉冲所含的脉冲个数
        vector<cv::Point2d> per_state_puls_lr;
        vector<double> timestamp;  //时戳
        vector<double> theta_cumu;
        vector<cv::Point2d> location_obsev;
        vector<double> theta_observ;
        vector<double> pulsl_state_sum;
        vector<double> pulsr_state_sum;
        double pulsl_sum;
        double pulsr_sum;
        cv::Point3d location_init;

        vector<double> vel_r;
        vector<double> vel_l;
        vector<double> omiga_r;
        vector<double> omiga_l;
        vector<double> vel_body;
        vector<double> omiga_body;
        vector<cv::Point3d> location_body;
    };
    struct CalReult
    {
        Eigen::VectorXf alpha;
        double wheel_left_radius;
        double wheel_right_radius;
        double wheels_space;
    };
    struct OdomConfig
    {
        double alphal;
        double alphar;
        double wheel_left_radius;
        double wheel_right_radius;
        double wheels_space;
        double output_hz;
        double pulse_per_circle;
    };
};

class OdomCal
{
public:
    OdomCal(){}
    OdomCal(bool sim_used, std::string test_file_path, std::string file_name_prefix, std::string file_observ_name,
            int file_num, double puls_per_circle, std::string result_file_path, std::string result_file_name):
       sim_used_(sim_used), path_num_(file_num), puls_per_circle_(puls_per_circle), file_observ_name_(file_observ_name)
    {
        arc_per_puls_ = 2*PI/puls_per_circle_;
        result_.wheels_space = -1000;
        result_.wheel_left_radius = -1000;
        result_.wheel_right_radius = -1000;

        std::string file_name;
        for(int i=0; i<path_num_; i++)
        {
            file_name = test_file_path+"/"+file_name_prefix+std::to_string(i+1)+".txt";
            file_names_.push_back(file_name);

            data_.push_back(getData(file_name));
        }
        file_name = test_file_path+"/"+file_observ_name+".txt";
        getObserv(file_name);
        solveAlpha();
        calThetaCumu();
        solveParaments();
        file_name = result_file_path+"/"+result_file_name;
        recordResult(file_name);
    }

    std::vector<std::double_t > proString(const std::string &s);
    OdomData::DataInPath getData(const std::string file_name);
    void getObserv(const std::string file_name);
    void solveAlpha();
    void solveParaments();
    void calThetaCumu();
    void recordResult(const std::string file_name);

private:
    bool sim_used_;
    int path_num_;
    double puls_per_circle_;
    double arc_per_puls_;
    vector<std::string> file_names_;
    std::string file_observ_name_;
    vector<OdomData::DataInPath> data_;
    OdomData::CalReult result_;
    MatrixXf b_;
};

//Cut information from message string accoording to ",".
std::vector<std::double_t > OdomCal::proString(const string &s)
{
    //分割有效数据，存入vector中
    std::vector<std::double_t > v;
    double temp;
    std::string::size_type w_head, w_tail;
    w_tail = s.find(",");//设定查找标志“，”,返回找到的第一个位置
    w_head = 0;
    while(std::string::npos !=w_tail)//如果找到
    {
        std::stringstream ss(s.substr(w_head, w_tail-w_head));
        ss >> temp;
        v.push_back(temp); //将w_head开头的一个字符放置到容器中
        w_head = w_tail +1;                           //给新头位置赋值
        w_tail = s.find("," , w_head);                //从头位置开始查找“，”，将位置返回给tail
    }
    if(w_head != s.length()){
        std::stringstream ss(s.substr(w_head));
        ss >> temp;
        v.push_back(temp);
    }//提取最后一个符合条件的信息
    return v;
}
OdomData::DataInPath OdomCal::getData(const std::string file_name)
{
    OdomData::DataInPath data;
    ifstream my_reader;
    std::cout<<"Get data from "<<file_name<<endl;
    my_reader.open(file_name);
    if(my_reader.fail())
        std::cout<<"Can't read "<<file_name<<std::endl;

    int num=0;
    data.pulsl_sum = 0.0;
    data.pulsr_sum = 0.0;

    while(!my_reader.eof()){
        string buffer;
        getline(my_reader,buffer);
        vector<std::double_t > v1 = proString(buffer);
        if(my_reader.eof())
            break;

        int puls_left, puls_right;
        puls_left = v1.at(1);
        puls_right = v1.at(2);
        double time = v1.at(0);
        data.timestamp.push_back(time);
        data.per_state_puls_l.push_back(puls_left);
        data.per_state_puls_r.push_back(puls_right);
        data.per_state_puls_lr.push_back(cv::Point2d(puls_left,puls_right));

        data.pulsl_sum += puls_left;
        data.pulsr_sum += puls_right;
        data.pulsl_state_sum.push_back(data.pulsl_sum);
        data.pulsr_state_sum.push_back(data.pulsr_sum);
        if(sim_used_)
        {
            data.theta_observ.push_back(v1.at(3));
            data.location_obsev.push_back(cv::Point2d(v1.at(4),v1.at(5)));
        }
        num+=1;
    }
    data.states_num = num;
    my_reader.close();
    return data;
}
void OdomCal::getObserv(const std::string file_name)
{
    ifstream my_reader;
    VectorXf b_temp(VectorXf::Zero(path_num_*3));
    //cout<<"b_temp="<<b_temp<<endl;
    std::cout<<"Get data from "<<file_name<<endl;
    my_reader.open(file_name);
    if(my_reader.fail())
        std::cout<<"Can't read "<<file_name<<std::endl;

    int num=0;
    while(!my_reader.eof()&&num<path_num_){
        string buffer;
        getline(my_reader,buffer);
        vector<std::double_t > v1 = proString(buffer);
        if(my_reader.eof())
            break;
        cv::Point3d location_init, location_end;
        location_init.x = v1.at(0);
        location_init.y = v1.at(1);
        location_init.z = v1.at(2)*PI/180.0;
        location_end.x = v1.at(3);
        location_end.y = v1.at(4);
        location_end.z = v1.at(5)*PI/180.0;

        b_temp(num*3,0)   = location_end.x-location_init.x;
        b_temp(num*3+1,0) = location_end.y-location_init.y;
        b_temp(num*3+2,0) = location_end.z-location_init.z;
        data_[num].location_init = location_init;

        num+=1;
    }
    b_ = b_temp;
    my_reader.close();
}

void OdomCal::solveAlpha()
{
    MatrixXf A(MatrixXf::Zero(path_num_,2));
    MatrixXf b(MatrixXf::Zero(path_num_,1));

    for(size_t i=0; i<path_num_; i++)
    {
        A(i, 0) = data_[i].pulsl_sum * arc_per_puls_;
        A(i, 1) = data_[i].pulsr_sum * arc_per_puls_;
        b(i, 0) = b_(i*3+2);
    }
//    std::cout<<"A_arc = \n"<<A<<std::endl;
//    std::cout<<"A_angle = \n"<<A*180/PI<<std::endl;
//    std::cout<<"b = \n"<<b<<std::endl;

    Eigen::JacobiSVD<Eigen::MatrixXf> svdOfA(A, Eigen::ComputeThinU | Eigen::ComputeThinV);
    result_.alpha = svdOfA.solve(b);
//    std::cout<<"alpha = "<<result_.alpha<<std::endl;
}
void OdomCal::solveParaments()
{
    MatrixXf A(MatrixXf::Zero(2*path_num_,1));
    MatrixXf b(MatrixXf::Zero(2*path_num_,1));

    double alpharl = -result_.alpha(1)/(result_.alpha(0) + 1e-5);
    for(size_t i=0; i<path_num_; i++)
    {
        double a1=0;
        double a2=0;
        for(size_t j=0; j<data_[i].states_num; j++)
        {
            double pulseNL, pulseNR,costheta, sintheta, theta_odom, delta_theta;

            if(j==0)
            {
                theta_odom = data_[i].location_init.z;
                delta_theta = 0.0;
                pulseNL = data_[i].per_state_puls_lr.at(0).x;
                pulseNR = data_[i].per_state_puls_lr.at(0).y;
            }
            else
            {
                theta_odom = data_[i].location_init.z+data_[i].theta_cumu.at(j-1);
                delta_theta = result_.alpha(0) * pulseNL * arc_per_puls_ +result_.alpha(1) * pulseNR * arc_per_puls_;
                pulseNL = data_[i].per_state_puls_lr.at(j-1).x;
                pulseNR = data_[i].per_state_puls_lr.at(j-1).y;
            }
            costheta = cos(theta_odom + 0.5 * delta_theta);
            sintheta = sin(theta_odom + 0.5 * delta_theta);
            a1 += 0.5 * (alpharl *  arc_per_puls_ * pulseNR +  arc_per_puls_ * pulseNL) * costheta;
            a2 += 0.5 * (alpharl *  arc_per_puls_ * pulseNR +  arc_per_puls_ * pulseNL) * sintheta;
        }
        A(i*2,0)= a1;
        A(i*2+1,0) = a2;
        b(i*2,0) =   b_(i*3);
        b(i*2+1,0) = b_(i*3+1);
    }
    Eigen::JacobiSVD<Eigen::MatrixXf> svdOfA(A, Eigen::ComputeThinU | Eigen::ComputeThinV);
    Eigen::MatrixXf X = svdOfA.solve(b);

    result_.wheel_left_radius = X(0);
    result_.wheel_right_radius = result_.wheel_left_radius*alpharl;
    result_.wheels_space = -result_.wheel_left_radius/result_.alpha(0);
    std::cout<<"=================================="<<std::endl;
    std::cout<<"results : \nwheel_left_radius  = "<<result_.wheel_left_radius<<"m, \nwheel_right_radius = "<<result_.wheel_right_radius
            <<"m, \nwheels_space = "<<result_.wheels_space<<"m"<<std::endl;
}
void OdomCal::calThetaCumu()
{
    for(size_t i=0; i<path_num_; i++)
    {
        double theta_cumu=0;
        for(size_t j=0; j<data_[i].states_num; j++)
        {
            double alpha_left  = result_.alpha(0);
            double alpha_right = result_.alpha(1);
            double pulseNL = data_[i].per_state_puls_lr.at(j).x;
            double pulseNR = data_[i].per_state_puls_lr.at(j).y;

            theta_cumu  += alpha_left*pulseNL*arc_per_puls_
                    +alpha_right*pulseNR * arc_per_puls_;
            data_[i].theta_cumu.push_back(theta_cumu);
        }
    }
}
void OdomCal::recordResult(std::string file_name)
{
    std::ofstream my_writer;
    my_writer.open(file_name.c_str(), std::ios::trunc);
    assert(my_writer.is_open());

    std::cout<<"Record to "<<file_name<<std::endl;
    my_writer<<"results : \nwheel_left_radius  = "<<result_.wheel_left_radius<<"m, \nwheel_right_radius = "<<result_.wheel_right_radius
            <<"m, \nwheels_space = "<<result_.wheels_space<<"m"<<std::endl;

    my_writer.close();
}

int main(int argc, char ** argv)
{
    ros::init(argc, argv, "odomcal");
    ros::NodeHandle nh("~");


    std::string test_file_path,file_name_prefix,file_observ_name,result_file_path,result_file_name;
    int file_num;
    double puls_per_circle;
    bool sim_used;


    nh.param<bool>("sim_used" , sim_used, false);
    nh.param<std::string>("test_file_path" , test_file_path, std::string("../../../src/odomcal/test"));
    nh.param<std::string>("file_name_prefix", file_name_prefix, std::string("test_"));
    nh.param<int>("file_num", file_num,3);
    nh.param<double>("puls_per_circle", puls_per_circle, 100000);
    nh.param<std::string>("file_observ_name", file_observ_name, std::string("observations"));
    nh.param<std::string>("result_file_path" , result_file_path, std::string("../../../src/odomcal/result"));
    nh.param<std::string>("result_file_name" , result_file_name, std::string("result.txt"));

    ROS_INFO_STREAM("Odometer Calibration!");
    std::cout<<puls_per_circle<< " puls in one circle."<<std::endl;
    OdomCal odom_cal(sim_used, test_file_path, file_name_prefix,file_observ_name, file_num, puls_per_circle, result_file_path, result_file_name);

    return 0;
}

