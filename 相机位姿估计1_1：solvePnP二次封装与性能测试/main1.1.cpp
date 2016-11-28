#include "stdafx.h"

#include "..\include\PNPSolver.h"

//本例用于测试对opencv的solvePNP函数的二次封装类PNPSolver
//@Author：VShawn
//@URL：http://www.cnblogs.com/singlex/

/*************相机物理参数***********/
//内参数
double fx = 6800.7;
double fy = 6798.1;
double u0 = 3065.8;
double v0 = 1667.6;
//镜头畸变参数
double k1 = -0.189314;
double k2 = 0.444657;
double p1 = -0.00116176;
double p2 = 0.00164877;
double k3 = -2.57547;

void test1();
void test2();
void test3();
void test4();

int main()
{
	//测试共面4点的求解
	test1();

	cout << endl << endl << endl << endl << endl << endl << endl << endl;

	//非共面四点求解位姿
	test2();

	cout << endl << endl << endl << endl << endl << endl << endl << endl;

	//用5个点估计位姿
	test3();

	cout << endl << endl << endl << endl << endl << endl << endl << endl;

	//三种方法速度测试
	test4();
	return 0;
}

void test1()
{
	cout << "test1:四点共面" << endl;
	//初始化PNPSolver类
	PNPSolver p4psolver;
	//初始化相机参数
	p4psolver.SetCameraMatrix(fx, fy, u0, v0);
	//设置畸变参数
	p4psolver.SetDistortionCoefficients(k1, k2, p1, p2, k3);

	p4psolver.Points3D.push_back(cv::Point3f(0, 0, 0));		//P1三维坐标的单位是毫米
	p4psolver.Points3D.push_back(cv::Point3f(0, 200, 0));	//P2
	p4psolver.Points3D.push_back(cv::Point3f(150, 0, 0));	//P3
	p4psolver.Points3D.push_back(cv::Point3f(150, 200, 0));	//P4
	//p4psolver.Points3D.push_back(cv::Point3f(0, 100, 105));	//P5

	cout << "test1:特征点世界坐标 = " << endl << p4psolver.Points3D << endl;

	p4psolver.Points2D.push_back(cv::Point2f(2985, 1688));	//P1
	p4psolver.Points2D.push_back(cv::Point2f(5081, 1690));	//P2
	p4psolver.Points2D.push_back(cv::Point2f(2997, 2797));	//P3
	p4psolver.Points2D.push_back(cv::Point2f(5544, 2757));	//P4
	//p4psolver.Points2D.push_back(cv::Point2f(4148, 673));	//P5

	cout << "test1:图中特征点坐标 = " << endl << p4psolver.Points2D << endl;

	if (p4psolver.Solve(PNPSolver::METHOD::CV_P3P) == 0)
		cout << "test1:CV_P3P方法:	相机位姿→" << "Oc坐标=" << p4psolver.Position_OcInW << "	相机旋转=" << p4psolver.Theta_W2C << endl;
	if (p4psolver.Solve(PNPSolver::METHOD::CV_ITERATIVE) == 0)
		cout << "test1:CV_ITERATIVE方法:	相机位姿→" << "Oc坐标=" << p4psolver.Position_OcInW << "	相机旋转=" << p4psolver.Theta_W2C << endl;
	if (p4psolver.Solve(PNPSolver::METHOD::CV_EPNP) == 0)
		cout << "test1:CV_EPNP方法:	相机位姿→" << "Oc坐标=" << p4psolver.Position_OcInW << "	相机旋转=" << p4psolver.Theta_W2C << endl;

	return;
}


void test2()
{
	cout << "test2:四点不共面" << endl;
	//初始化PNPSolver类
	PNPSolver p4psolver;
	//初始化相机参数
	p4psolver.SetCameraMatrix(fx, fy, u0, v0);
	//设置畸变参数
	p4psolver.SetDistortionCoefficients(k1, k2, p1, p2, k3);

	p4psolver.Points3D.push_back(cv::Point3f(0, 0, 0));		//P1三维坐标的单位是毫米
	p4psolver.Points3D.push_back(cv::Point3f(0, 200, 0));	//P2
	p4psolver.Points3D.push_back(cv::Point3f(150, 0, 0));	//P3
	//p4psolver.Points3D.push_back(cv::Point3f(150, 200, 0));	//P4
	p4psolver.Points3D.push_back(cv::Point3f(0, 100, 105));	//P5

	cout << "test2:特征点世界坐标 = " << endl << p4psolver.Points3D << endl;

	p4psolver.Points2D.push_back(cv::Point2f(2985, 1688));	//P1
	p4psolver.Points2D.push_back(cv::Point2f(5081, 1690));	//P2
	p4psolver.Points2D.push_back(cv::Point2f(2997, 2797));	//P3
	//p4psolver.Points2D.push_back(cv::Point2f(5544, 2757));	//P4
	p4psolver.Points2D.push_back(cv::Point2f(4148, 673));	//P5

	cout << "test2:图中特征点坐标 = " << endl << p4psolver.Points2D << endl;

	if (p4psolver.Solve(PNPSolver::METHOD::CV_P3P) == 0)
		cout << "test2:CV_P3P方法:	相机位姿→" << "Oc坐标=" << p4psolver.Position_OcInW << "	相机旋转=" << p4psolver.Theta_W2C << endl;
	if (p4psolver.Solve(PNPSolver::METHOD::CV_ITERATIVE) == 0)
		cout << "test2:CV_ITERATIVE方法:	相机位姿→" << "Oc坐标=" << p4psolver.Position_OcInW << "	相机旋转=" << p4psolver.Theta_W2C << endl;
	if (p4psolver.Solve(PNPSolver::METHOD::CV_EPNP) == 0)
		cout << "test2:CV_EPNP方法:	相机位姿→" << "Oc坐标=" << p4psolver.Position_OcInW << "	相机旋转=" << p4psolver.Theta_W2C << endl;

	return;
}

void test3()
{
	cout << "test3:五点求解" << endl;
	//初始化PNPSolver类
	PNPSolver p4psolver;
	//初始化相机参数
	p4psolver.SetCameraMatrix(fx, fy, u0, v0);
	//设置畸变参数
	p4psolver.SetDistortionCoefficients(k1, k2, p1, p2, k3);

	p4psolver.Points3D.push_back(cv::Point3f(0, 0, 0));		//P1三维坐标的单位是毫米
	p4psolver.Points3D.push_back(cv::Point3f(0, 200, 0));	//P2
	p4psolver.Points3D.push_back(cv::Point3f(150, 0, 0));	//P3
	p4psolver.Points3D.push_back(cv::Point3f(150, 200, 0));	//P4
	p4psolver.Points3D.push_back(cv::Point3f(0, 100, 105));	//P5

	cout << "test3:特征点世界坐标 = " << endl << p4psolver.Points3D << endl;

	p4psolver.Points2D.push_back(cv::Point2f(2985, 1688));	//P1
	p4psolver.Points2D.push_back(cv::Point2f(5081, 1690));	//P2
	p4psolver.Points2D.push_back(cv::Point2f(2997, 2797));	//P3
	p4psolver.Points2D.push_back(cv::Point2f(5544, 2757));	//P4
	p4psolver.Points2D.push_back(cv::Point2f(4148, 673));	//P5

	cout << "test3:图中特征点坐标 = " << endl << p4psolver.Points2D << endl;

	if (p4psolver.Solve(PNPSolver::METHOD::CV_P3P) == 0)
		cout << "test3:CV_P3P方法:	相机位姿→" << "Oc坐标=" << p4psolver.Position_OcInW << "	相机旋转=" << p4psolver.Theta_W2C << endl;
	if (p4psolver.Solve(PNPSolver::METHOD::CV_ITERATIVE) == 0)
		cout << "test3:CV_ITERATIVE方法:	相机位姿→" << "Oc坐标=" << p4psolver.Position_OcInW << "	相机旋转=" << p4psolver.Theta_W2C << endl;
	if (p4psolver.Solve(PNPSolver::METHOD::CV_EPNP) == 0)
		cout << "test3:CV_EPNP方法:	相机位姿→" << "Oc坐标=" << p4psolver.Position_OcInW << "	相机旋转=" << p4psolver.Theta_W2C << endl;

	return;
}

void test4()
{
	cout << "test4:算法速度测试" << endl;
	//初始化PNPSolver类
	PNPSolver p4psolver;
	//初始化相机参数
	p4psolver.SetCameraMatrix(fx, fy, u0, v0);
	//设置畸变参数
	p4psolver.SetDistortionCoefficients(k1, k2, p1, p2, k3);

	p4psolver.Points3D.push_back(cv::Point3f(0, 0, 0));		//P1三维坐标的单位是毫米
	p4psolver.Points3D.push_back(cv::Point3f(0, 200, 0));	//P2
	p4psolver.Points3D.push_back(cv::Point3f(150, 0, 0));	//P3
	p4psolver.Points3D.push_back(cv::Point3f(150, 200, 0));	//P4
	//p4psolver.Points3D.push_back(cv::Point3f(0, 100, 105));	//P5

	//cout << "test1:特征点世界坐标 = " << endl << p4psolver.Points3D << endl;

	p4psolver.Points2D.push_back(cv::Point2f(2985, 1688));	//P1
	p4psolver.Points2D.push_back(cv::Point2f(5081, 1690));	//P2
	p4psolver.Points2D.push_back(cv::Point2f(2997, 2797));	//P3
	p4psolver.Points2D.push_back(cv::Point2f(5544, 2757));	//P4
	//p4psolver.Points2D.push_back(cv::Point2f(4148, 673));	//P5

	//cout << "test1:图中特征点坐标 = " << endl << p4psolver.Points2D << endl;

	double t = cv::getTickCount();
	for (int i = 0; i < 1000; i++)
	{
		p4psolver.Solve(PNPSolver::METHOD::CV_P3P);
	}
	cout << "test4:CV_P3P方法执行1000次用时:				" << (cv::getTickCount() - t) / cv::getTickFrequency() * 1000 << "ms" << endl;

	t = cv::getTickCount();
	for (int i = 0; i < 1000; i++)
	{
		p4psolver.Solve(PNPSolver::METHOD::CV_ITERATIVE);
	}
	cout << "test4:CV_ITERATIVE方法执行1000次用时:			" << (cv::getTickCount() - t) / cv::getTickFrequency() * 1000 << "ms" << endl;

	t = cv::getTickCount();
	for (int i = 0; i < 1000; i++)
	{
		p4psolver.Solve(PNPSolver::METHOD::CV_EPNP);
	}
	cout << "test4:CV_EPNP方法执行1000次用时:			" << (cv::getTickCount() - t) / cv::getTickFrequency() * 1000 << "ms" << endl;

	return;
}