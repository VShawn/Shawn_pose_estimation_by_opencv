#pragma once
#include <opencv2\opencv.hpp>
#include <math.h>
#include <iostream>
#include <fstream>
using namespace std;


// 本类用于快速解决PNP问题，顺带解决空间绕轴旋转以及图像系、相机系、世界系三系坐标投影问题
// 默认使用Gao的P3P+重投影法，要求输入4个特征点
// 调用顺序：
// 1.初始化本类
// 2.调用SetCameraMatrix(),SetDistortionCoefficients()设置好相机内参数与镜头畸变参数
// 3.向Points3D，Points2D中添加一一对应的特征点对
// 4.调用Solve()方法运行计算
// 5.从RoteM, TransM, W2CTheta等属性中提出结果
//
// 原理参见：http://www.cnblogs.com/singlex/category/911880.html
// Author：VShawn
// Ver:2016.11.25.0
class PNPSolver
{
public:
	PNPSolver();
	//带参数初始化
	PNPSolver(double fx, double fy, double u0, double v0, double k_1, double  k_2, double  p_1, double  p_2, double k_3);
	~PNPSolver();

	enum METHOD
	{
		CV_ITERATIVE = CV_ITERATIVE,
		CV_P3P = CV_P3P,
		CV_EPNP = CV_EPNP
	};

	/***********************位姿估计所用特征点**************************/
	vector<cv::Point3f> Points3D;//存储四个点的世界坐标
	vector<cv::Point2f> Points2D;//存储四个点的图像坐标

	/***********************位姿估计结果**************************/
	//最后求出的旋转矩阵与平移矩阵
	cv::Mat RoteM, TransM;
	//世界系到相机系的三轴旋转欧拉角，世界系照此旋转后可以与相机坐标系完全平行。
	//旋转顺序为x、y、z
	cv::Point3f Theta_W2C;
	//相机系到世界系的三轴旋转欧拉角，相机坐标系照此旋转后可以与世界坐标系完全平行。
	//旋转顺序为z、y、x
	cv::Point3f Theta_C2W;
	//相机坐标系中，世界坐标系原点Ow的坐标
	cv::Point3f Position_OwInC;
	//世界坐标系中，相机坐标系原点Oc的坐标
	cv::Point3f Position_OcInW;


	/*********************公有方法*****************************/

	//解PNP问题，获得位姿信息
	//调用后在RoteM, TransM, W2CTheta等属性中提取计算结果，属性说明参见注释
	//输出参数：CV_ITERATIVE，CV_P3P（默认），CV_EPNP，具体参见Opencv documentation.
	//实测
	//CV_ITERATIVE迭代法似乎只能用4个共面特征点求解，5个点或非共面4点解不出正确的解
	//CV_P3P的Gao的方法可以使用任意四个特征点，特征点数量不能少于4也不能多于4
	//CV_EPNP方法可以实现特征点数>=4的问题求解，不需要4点共面
	//返回值：
	//0正确
	//-1相机内参数或畸变参数未设置
	//-2未提供足够的特征点，或特征点数目不匹配
	//-3输入的点数据有误，详见printf信息
	int Solve(METHOD method = METHOD::CV_P3P);
	
	//根据计算出的结果将世界坐标重投影到图像，返回像素坐标点集
	//使用前需要先用Solve()解出相机位姿
	//输入为世界坐标系的点坐标集合
	//输出为点投影到图像上的图像坐标集合
	vector<cv::Point2f> WordFrame2ImageFrame(vector<cv::Point3f> WorldPoints);



	//根据输入的参数将图像坐标转换到相机坐标中
	//使用前需要先用Solve()解出相机位姿
	//输入为图像上的点坐标
	//double F为镜头焦距
	//输出为点在焦距=F时的相机坐标系坐标
	cv::Point3f ImageFrame2CameraFrame(cv::Point2f p, double F);




	//设置相机内参数矩阵
	void SetCameraMatrix(double fx, double fy, double u0, double v0)
	{
		camera_matrix = cv::Mat(3, 3, CV_64FC1, cv::Scalar::all(0));
		camera_matrix.ptr<double>(0)[0] = fx;
		camera_matrix.ptr<double>(0)[2] = u0;
		camera_matrix.ptr<double>(1)[1] = fy;
		camera_matrix.ptr<double>(1)[2] = v0;
		camera_matrix.ptr<double>(2)[2] = 1.0f;
	}
	//设置畸变系数矩阵
	void SetDistortionCoefficients(double k_1, double  k_2, double  p_1, double  p_2, double k_3)
	{
		distortion_coefficients = cv::Mat(5, 1, CV_64FC1, cv::Scalar::all(0));
		distortion_coefficients.ptr<double>(0)[0] = k_1;
		distortion_coefficients.ptr<double>(1)[0] = k_2;
		distortion_coefficients.ptr<double>(2)[0] = p_1;
		distortion_coefficients.ptr<double>(3)[0] = p_2;
		distortion_coefficients.ptr<double>(4)[0] = k_3;
	}












	/********************公有静态方法*********************/
	//点绕任意向量旋转，右手系
	static cv::Point3f RotateByVector(double old_x, double old_y, double old_z, double vx, double vy, double vz, double theta)
	{
		double r = theta * CV_PI / 180;
		double c = cos(r);
		double s = sin(r);
		double new_x = (vx*vx*(1 - c) + c) * old_x + (vx*vy*(1 - c) - vz*s) * old_y + (vx*vz*(1 - c) + vy*s) * old_z;
		double new_y = (vy*vx*(1 - c) + vz*s) * old_x + (vy*vy*(1 - c) + c) * old_y + (vy*vz*(1 - c) - vx*s) * old_z;
		double new_z = (vx*vz*(1 - c) - vy*s) * old_x + (vy*vz*(1 - c) + vx*s) * old_y + (vz*vz*(1 - c) + c) * old_z;
		return cv::Point3f(new_x, new_y, new_z);
	}

	//将空间点绕Z轴旋转
	//输入参数 x y为空间点原始x y坐标
	//thetaz为空间点绕Z轴旋转多少度，角度制范围在-180到180
	//outx outy为旋转后的结果坐标
	static void CodeRotateByZ(double x, double y, double thetaz, double& outx, double& outy)
	{
		double x1 = x;//将变量拷贝一次，保证&x == &outx这种情况下也能计算正确
		double y1 = y;
		double rz = thetaz * CV_PI / 180;
		outx = cos(rz) * x1 - sin(rz) * y1;
		outy = sin(rz) * x1 + cos(rz) * y1;
	}

	//将空间点绕Y轴旋转
	//输入参数 x z为空间点原始x z坐标
	//thetay为空间点绕Y轴旋转多少度，角度制范围在-180到180
	//outx outz为旋转后的结果坐标
	static void CodeRotateByY(double x, double z, double thetay, double& outx, double& outz)
	{
		double x1 = x;
		double z1 = z;
		double ry = thetay * CV_PI / 180;
		outx = cos(ry) * x1 + sin(ry) * z1;
		outz = cos(ry) * z1 - sin(ry) * x1;
	}

	//将空间点绕X轴旋转
	//输入参数 y z为空间点原始y z坐标
	//thetax为空间点绕X轴旋转多少度，角度制，范围在-180到180
	//outy outz为旋转后的结果坐标
	static void CodeRotateByX(double y, double z, double thetax, double& outy, double& outz)
	{
		double y1 = y;//将变量拷贝一次，保证&y == &y这种情况下也能计算正确
		double z1 = z;
		double rx = thetax * CV_PI / 180;
		outy = cos(rx) * y1 - sin(rx) * z1;
		outz = cos(rx) * z1 + sin(rx) * y1;
	}
private:

	cv::Mat camera_matrix;//内参数矩阵
	cv::Mat distortion_coefficients;//畸变系数

	cv::Mat rvec;//解出来的旋转向量
	cv::Mat tvec;//解出来的平移向量
};

