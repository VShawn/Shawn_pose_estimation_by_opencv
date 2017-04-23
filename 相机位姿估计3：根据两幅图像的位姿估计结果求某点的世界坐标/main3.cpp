#include "stdafx.h"
#include <opencv2\opencv.hpp>

#include <include/PNPSolver.h>
#include "..\include\PNPSolver.h"
#include "..\include\GetDistanceOf2linesIn3D.h"
using namespace std;

const int F = 350;//相机镜头焦距


//该函数用于计算某点的世界坐标
//输入参数
//pointInImage1：待求点在图1中的二维坐标
//p4psolver1：图1解出的PNPSolver类
//pointInImage2：待求点在图2中的二维坐标
//p4psolver2：图2解出的PNPSolver类
//返回
//待求点在世界坐标系下坐标
cv::Point3f GetPointInWorld(cv::Point2f pointInImage1, PNPSolver& p4psolver1, cv::Point2f pointInImage2, PNPSolver& p4psolver2)
{

	//将P投射到相机坐标系，再经过反旋转求出向量OcP，最终获得图1中，直线OcP上的两个点坐标，确定了直线的方程
	cv::Point3f point2find1_CF = p4psolver1.ImageFrame2CameraFrame(pointInImage1, F);//待求点P在图一状态下的相机坐标系坐标
	double Oc1P_x1 = point2find1_CF.x;
	double Oc1P_y1 = point2find1_CF.y;
	double Oc1P_z1 = point2find1_CF.z;
	//进行三次反向旋转，得到世界坐标系中向量OcP的值
	PNPSolver::CodeRotateByZ(Oc1P_x1, Oc1P_y1, p4psolver1.Theta_W2C.z, Oc1P_x1, Oc1P_y1);
	PNPSolver::CodeRotateByY(Oc1P_x1, Oc1P_z1, p4psolver1.Theta_W2C.y, Oc1P_x1, Oc1P_z1);
	PNPSolver::CodeRotateByX(Oc1P_y1, Oc1P_z1, p4psolver1.Theta_W2C.x, Oc1P_y1, Oc1P_z1);
	//两点确定一条直线
	cv::Point3f a1(p4psolver1.Position_OcInW.x, p4psolver1.Position_OcInW.y, p4psolver1.Position_OcInW.z);
	cv::Point3f a2(p4psolver1.Position_OcInW.x + Oc1P_x1, p4psolver1.Position_OcInW.y + Oc1P_y1, p4psolver1.Position_OcInW.z + Oc1P_z1);











	//将P投射到相机坐标系，再经过反旋转求出向量Oc1P，最终获得图2中，直线Oc2P上的两个点坐标，确定了直线的方程
	cv::Point3f point2find2_CF = p4psolver2.ImageFrame2CameraFrame(pointInImage2, F);//待求点P在图二状态下的相机坐标系坐标
	double Oc2P_x2 = point2find2_CF.x;
	double Oc2P_y2 = point2find2_CF.y;
	double Oc2P_z2 = point2find2_CF.z;
	//进行三次反向旋转，得到世界坐标系中向量OcP的值
	PNPSolver::CodeRotateByZ(Oc2P_x2, Oc2P_y2, p4psolver2.Theta_W2C.z, Oc2P_x2, Oc2P_y2);
	PNPSolver::CodeRotateByY(Oc2P_x2, Oc2P_z2, p4psolver2.Theta_W2C.y, Oc2P_x2, Oc2P_z2);
	PNPSolver::CodeRotateByX(Oc2P_y2, Oc2P_z2, p4psolver2.Theta_W2C.x, Oc2P_y2, Oc2P_z2);
	//两点确定一条直线
	cv::Point3f b1(p4psolver2.Position_OcInW.x, p4psolver2.Position_OcInW.y, p4psolver2.Position_OcInW.z);
	cv::Point3f b2(p4psolver2.Position_OcInW.x + Oc2P_x2, p4psolver2.Position_OcInW.y + Oc2P_y2, p4psolver2.Position_OcInW.z + Oc2P_z2);








	/*************************求出P的坐标**************************/
	//现在我们获得了关于点P的两条直线a1a2与b1b2
	//于是两直线的交点便是点P的位置
	//但由于存在测量误差，两条直线不可能是重合的，于是退而求其次
	//求出两条直线最近的点，就是P所在的位置了。

	GetDistanceOf2linesIn3D g;//初始化
	g.SetLineA(a1.x, a1.y, a1.z, a2.x, a2.y, a2.z);//输入直线A上的两个点坐标
	g.SetLineB(b1.x, b1.y, b1.z, b2.x, b2.y, b2.z);//输入直线B上的两个点坐标
	g.GetDistance();//计算距离
	double d = g.distance;//获得距离
	//点PonA与PonB分别为直线A、B上最接近的点，他们的中点就是P的坐标
	double x = (g.PonA_x + g.PonB_x) / 2;
	double y = (g.PonA_y + g.PonB_y) / 2;
	double z = (g.PonA_z + g.PonB_z) / 2;




	return cv::Point3f(x, y, z);
}


//本程序通过两副图的位姿，求出未知点P的空间坐标（世界坐标）
//@Author：VShawn
//@URL：http://www.cnblogs.com/singlex/
//详细原理与说明：
int main()
{
	/*************初始化物理参数***********/

	//相机内参数
	double camD[9] = {
		6800.7, 0, 3065.8,
		0, 6798.1, 1667.6,
		0, 0, 1 };

	double fx = camD[0];
	double fy = camD[4];
	double u0 = camD[2];
	double v0 = camD[5];

	//镜头畸变参数
	double k1 = -0.189314;
	double k2 = 0.444657;
	double p1 = -0.00116176;
	double p2 = 0.00164877;
	double k3 = -2.57547;



	/********第一幅图********/
	PNPSolver p4psolver1;
	//初始化相机参数
	p4psolver1.SetCameraMatrix(fx, fy, u0, v0);
	//设置畸变参数
	p4psolver1.SetDistortionCoefficients(k1, k2, p1, p2, k3);

	p4psolver1.Points3D.push_back(cv::Point3f(0, 0, 0));		//P1三维坐标的单位是毫米
	p4psolver1.Points3D.push_back(cv::Point3f(0, 200, 0));		//P2
	p4psolver1.Points3D.push_back(cv::Point3f(150, 0, 0));		//P3
	p4psolver1.Points3D.push_back(cv::Point3f(150, 200, 0));	//P4
	//p4psolver1.Points3D.push_back(cv::Point3f(0, 100, 105));	//P5

	cout << "特征点世界坐标 = " << endl << p4psolver1.Points3D << endl << endl << endl;

	//求出图一中几个特征点与待求点P的坐标
	//cv::Mat img1 = cv::imread("1.jpg");
	p4psolver1.Points2D.push_back(cv::Point2f(2985, 1688));	//P1
	p4psolver1.Points2D.push_back(cv::Point2f(5081, 1690));	//P2
	p4psolver1.Points2D.push_back(cv::Point2f(2997, 2797));	//P3
	p4psolver1.Points2D.push_back(cv::Point2f(5544, 2757));	//P4
	//p4psolver1.Points2D.push_back(cv::Point2f(4148, 673));	//P5

	cout << "图一中特征点坐标 = " << endl << p4psolver1.Points2D << endl;


	if (p4psolver1.Solve(PNPSolver::METHOD::CV_P3P) != 0)
		return -1;

	cout << "图一中相机位姿" << endl << "Oc坐标=" << p4psolver1.Position_OcInW << "      相机旋转=" << p4psolver1.Theta_W2C << endl;
	cout << endl << endl;
	/**********************/











	/********第2幅图********/
	PNPSolver p4psolver2;
	//初始化相机参数
	p4psolver2.SetCameraMatrix(fx, fy, u0, v0);
	//畸变参数
	p4psolver2.SetDistortionCoefficients(k1, k2, p1, p2, k3);

	p4psolver2.Points3D.push_back(cv::Point3f(0, 0, 0));		//三维坐标的单位是毫米
	p4psolver2.Points3D.push_back(cv::Point3f(0, 200, 0));		//P2
	p4psolver2.Points3D.push_back(cv::Point3f(150, 0, 0));		//P3
	p4psolver2.Points3D.push_back(cv::Point3f(150, 200, 0));	//P4
	//p4psolver2.Points3D.push_back(cv::Point3f(0, 100, 105));	//P5

	//求出图二中几个特征点与待求点P的坐标
	//cv::Mat img2 = cv::imread("2.jpg");
	p4psolver2.Points2D.push_back(cv::Point2f(3062, 3073));	//P1
	p4psolver2.Points2D.push_back(cv::Point2f(3809, 3089));	//P2
	p4psolver2.Points2D.push_back(cv::Point2f(3035, 3208));	//P3
	p4psolver2.Points2D.push_back(cv::Point2f(3838, 3217));	//P4
	//p4psolver2.Points2D.push_back(cv::Point2f(3439, 2691));	//P5
	cout << "图二中特征点坐标 = " << endl << p4psolver2.Points2D << endl;
	if (p4psolver2.Solve(PNPSolver::METHOD::CV_P3P) != 0)
		return -1;
	cout << "图二中相机位姿" << endl << "Oc坐标=" << p4psolver2.Position_OcInW << "      相机旋转=" << p4psolver2.Theta_W2C << endl;

	/**********************/









	cv::Point2f point2find1_IF = cv::Point2f(4149, 671);//待求点P在图1中坐标
	cv::Point2f point2find2_IF = cv::Point2f(3439, 2691);//待求点P在图2中坐标


	cv::Point3f p = GetPointInWorld(point2find1_IF, p4psolver1, point2find2_IF, p4psolver2);
	cout << endl << "-------------------------------------------------------------" << endl;
	cout << "解得P世界坐标 = (" << p.x << "," << p.y << "," << p.z << ")" << endl;



	//注：为了更精确的计算出空间坐标，可以计算出多组P的位置，并取它们的重心
	//End @VShawn(http://www.cnblogs.com/singlex/)
	return 0;
}