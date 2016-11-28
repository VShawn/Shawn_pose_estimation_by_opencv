// SlovePNPByOpenCV.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <opencv2\opencv.hpp>
#include <math.h>
#include <iostream>
#include <fstream>
#include "SolvePNPBy4Points.h"

#include "GetDistanceOf2linesIn3D.h"
using namespace std;

//将空间点绕Z轴旋转
//输入参数 x y为空间点原始x y坐标
//thetaz为空间点绕Z轴旋转多少度，角度制范围在-180到180
//outx outy为旋转后的结果坐标
void codeRotateByZ(double x, double y, double thetaz, double& outx, double& outy)
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
void codeRotateByY(double x, double z, double thetay, double& outx, double& outz)
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
void codeRotateByX(double y, double z, double thetax, double& outy, double& outz)
{
	double y1 = y;//将变量拷贝一次，保证&y == &y这种情况下也能计算正确
	double z1 = z;
	double rx = thetax * CV_PI / 180;
	outy = cos(rx) * y1 - sin(rx) * z1;
	outz = cos(rx) * z1 + sin(rx) * y1;
}


//点绕任意向量旋转，右手系
//输入参数old_x，old_y，old_z为旋转前空间点的坐标
//vx，vy，vz为旋转轴向量
//theta为旋转角度角度制，范围在-180到180
//返回值为旋转后坐标点
cv::Point3f RotateByVector(double old_x, double old_y, double old_z, double vx, double vy, double vz, double theta)
{
	double r = theta * CV_PI / 180;
	double c = cos(r);
	double s = sin(r);
	double new_x = (vx*vx*(1 - c) + c) * old_x + (vx*vy*(1 - c) - vz*s) * old_y + (vx*vz*(1 - c) + vy*s) * old_z;
	double new_y = (vy*vx*(1 - c) + vz*s) * old_x + (vy*vy*(1 - c) + c) * old_y + (vy*vz*(1 - c) - vx*s) * old_z;
	double new_z = (vx*vz*(1 - c) - vy*s) * old_x + (vy*vz*(1 - c) + vx*s) * old_y + (vz*vz*(1 - c) + c) * old_z;
	return cv::Point3f(new_x, new_y, new_z);
}
void test();

vector<cv::Point2f> Points2D;//用于定位的四个特征点
vector<cv::Point2f> Points2DNeedToFind;
cv::Mat imgDisp;

//将图像坐标的点(pix)转换到相机坐标系(mm)中
//输入相机内参数
//double F为镜头焦距
cv::Point3f imageFrame2CameraFrame(cv::Point2f p, double fx, double fy, double u0, double v0, double F)
{
	double zc = F;
	double xc = (p.x - u0)*F / fx;
	double yc = (p.y - v0)*F / fy;
	return cv::Point3f(xc, yc, zc);
}

void on_mouse(int event, int x, int y, int flag, void *param)
{
	if (event == CV_EVENT_LBUTTONDOWN)
	{
		//只记录四个跟踪点。
		if (Points2D.size() < 4)
		{
			Points2D.push_back(cv::Point2f(x, y));
			cout << "add 1 point :" << Points2D.size() << "/4" << endl;
			cv::circle(imgDisp,  cv::Point2f(x, y), 25, cv::Scalar(255, 0, 0), 5);
			cv::imshow("CamPos", imgDisp);
		}
		else if (Points2DNeedToFind.size() == 0)
		{
			Points2DNeedToFind.push_back(cv::Point2f(x, y));
			cv::circle(imgDisp, cv::Point2f(x, y), 5, cv::Scalar(0, 255, 255), -1);
			cv::imshow("CamPos", imgDisp);
		}
	}
}

double Dot(cv::Point3f a, cv::Point3f b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
cv::Point3f Cross(cv::Point3f a, cv::Point3f b){ return cv::Point3f(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); }
double Length(cv::Point3f a) { return sqrt(Dot(a, a)); }

int main(int argc, _TCHAR* argv[])
{
	double camD[9] = {
		6800.7, 0, 3065.8,
		0, 6798.1, 1667.6,
		0, 0, 1 };

	double fx = camD[0];
	double fy = camD[4];
	double u0 = camD[2];
	double v0 = camD[5];


	SolvePNPBy4Points sp4p;
	//初始化相机参数
	sp4p.SetCameraMatrix(fx, fy, u0, v0);
	//畸变参数
	sp4p.SetDistortionCoefficients(-0.189314, 0.444657, -0.00116176, 0.00164877, -2.57547);




	sp4p.Points3D.push_back(cv::Point3f(0, 0, 0));    //三维坐标的单位是毫米
	sp4p.Points3D.push_back(cv::Point3f(0, 200, 0));
	sp4p.Points3D.push_back(cv::Point3f(150, 0, 0));
	sp4p.Points3D.push_back(cv::Point3f(150, 200, 0));

	cv::namedWindow("CamPos", 0);
	cvSetMouseCallback("CamPos", on_mouse, NULL);//绑定鼠标点击事件，通过鼠标点击添加特征点。

	cv::Mat img1 = cv::imread("1.jpg");
	cv::Mat img2 = cv::imread("2.jpg");
	cv::Point3d vec1;//世界坐标系中相机坐标系的位置
	cv::Point3d vec2;
	cv::Point2f point2find1_IF;//待求点的图像坐标系坐标
	cv::Point2f point2find2_IF ;//待求点的图像坐标系坐标
	cv::Point3f point2find1_CF;
	cv::Point3f point2find2_CF;


	/********第一幅图********/
	sp4p.Points2D.push_back(cv::Point2f(2985, 1688));
	sp4p.Points2D.push_back(cv::Point2f(5077, 1684));
	sp4p.Points2D.push_back(cv::Point2f(2998, 2793));
	sp4p.Points2D.push_back(cv::Point2f(5546, 2754));
	Points2DNeedToFind.push_back(cv::Point2f(4149, 671));

	if (sp4p.Solve() != 0)
		return -1;
	vec1.x = sp4p.Position_OcInW.x;
	vec1.y = sp4p.Position_OcInW.y;
	vec1.z = sp4p.Position_OcInW.z;
	point2find1_IF = cv::Point2f(Points2DNeedToFind[0]);
	Points2D.clear();
	Points2DNeedToFind.clear();
	sp4p.Points2D.clear();


	//处理第一幅图的直线方程
	point2find1_CF = imageFrame2CameraFrame(point2find1_IF, fx, fy, u0, v0, 350);//待求点
	double x1 = point2find1_CF.x;
	double y1 = point2find1_CF.y;
	double z1 = point2find1_CF.z;
	//进行三次反向旋转
	codeRotateByZ(x1, y1, sp4p.Theta_W2C.z, x1, y1);
	codeRotateByY(x1, z1, sp4p.Theta_W2C.y, x1, z1);
	codeRotateByX(y1, z1, sp4p.Theta_W2C.x, y1, z1);

	//两点确定一条直线
	cv::Point3f a1(sp4p.Position_OcInW.x, sp4p.Position_OcInW.y, sp4p.Position_OcInW.z);
	cv::Point3f a2(sp4p.Position_OcInW.x + x1, sp4p.Position_OcInW.y + y1, sp4p.Position_OcInW.z + z1);
	cv::Point3f l1vector(x1, y1, z1);//直线1的方向向量



	/********第2幅图********/
	//imgDisp = img2;
	//cv::imshow("CamPos", imgDisp);
	//while (Points2D.size() < 4 || Points2DNeedToFind.size() < 1)
	//{
	//	if (cv::waitKey(5) == 'r')
	//		Points2D.clear();
	//}
	//ofstream fout2("2.txt");
	//fout2 << Points2D << endl;
	//fout2 << Points2DNeedToFind << endl;
	//fout2.close();

	//for (int i = 0; i < Points2D.size(); i++)
	//{
	//	if (Points2D.size() >= 4)//当特征点超过四个时
	//	{
	//		sp4p.Points2D.push_back(Points2D[i]);
	//	}
	//}


	sp4p.Points2D.push_back(cv::Point2f(3062, 3073));
	sp4p.Points2D.push_back(cv::Point2f(3809, 3089));
	sp4p.Points2D.push_back(cv::Point2f(3035, 3208));
	sp4p.Points2D.push_back(cv::Point2f(3838, 3217));
	Points2DNeedToFind.push_back(cv::Point2f(3439, 2691));


	if (sp4p.Solve() != 0)
		return -1;
	vec2.x = sp4p.Position_OcInW.x;
	vec2.y = sp4p.Position_OcInW.y;
	vec2.z = sp4p.Position_OcInW.z;
	point2find2_IF = cv::Point2f(Points2DNeedToFind[0]);
	Points2D.clear();
	Points2DNeedToFind.clear();

	/**********************/

	//处理第二幅图的直线方程
	point2find2_CF = imageFrame2CameraFrame(point2find2_IF, fx, fy, u0, v0, 350);//待求点
	double x2 = point2find2_CF.x;
	double y2 = point2find2_CF.y;
	double z2 = point2find2_CF.z;
	//进行三次反向旋转
	codeRotateByZ(x2, y2, sp4p.Theta_W2C.z, x2, y2);
	codeRotateByY(x2, z2, sp4p.Theta_W2C.y, x2, z2);
	codeRotateByX(y2, z2, sp4p.Theta_W2C.x, y2, z2);

	//两点确定一条直线
	cv::Point3f b1(sp4p.Position_OcInW.x, sp4p.Position_OcInW.y, sp4p.Position_OcInW.z);
	cv::Point3f b2(sp4p.Position_OcInW.x + x2, sp4p.Position_OcInW.y + y2, sp4p.Position_OcInW.z + z2);
	cv::Point3f l2vector(x2, y2, z2);//直线2的方向向量


	if (1 == 1)
	{
		/*************************求两直线最近点**************************/
		//http://blog.sina.com.cn/s/blog_a401a1ea0101ij9z.html
		//向量叉乘获得公共法向量
		//a=(X1,Y1,Z1),b=(X2,Y2,Z2),
		//a×b=（Y1Z2-Y2Z1,Z1X2-Z2X1,X1Y2-X2Y1）
		double X1 = l1vector.x, Y1 = l1vector.y, Z1 = l1vector.z;
		double X2 = l2vector.x, Y2 = l2vector.y, Z2 = l2vector.z;
		cv::Point3f N(Y1*Z2 - Y2*Z1, Z1*X2 - Z2*X1, X1*Y2 - X2*Y1);//法向量N

		//求最短距离
		/*在两直线上分别选取点A, B(任意)，得到向量AB，
			求向量AB在向量N方向的投影即为两异面直线间的距离了（就是最短距离啦）。*/

		cv::Point3f AB(b1.x - a1.x, b1.y - a1.y, b1.z - a1.z);

		/*a在b上的投影是 | a | cos<a, b >= a*b / | b |
			如：
			a = (1, 2, 3)
			b = (2, 1, 4)
			a在b上的投影为：
			a*b = 2 + 2 + 12 = 16
			| b |= √(2 ^ 2 + 1 ^ 2 + 4 ^ 2) = √21
			a在b上的投影为：
			16 / √21         */

		double axb = Dot(AB, N);
		double N_abs = sqrt(Dot(N, N));
		double dist = axb / N_abs;//求出了两直线的距离

		//设交点为C, D，带入公垂线N的对称式中，又因为C, D两点分别满足一开始的直线方程，所以得到关于C（或D）的两个连等方程，分别解出来就好了！
		//C.x =  


		//方法来自：http://blog.csdn.net/pi9nc/article/details/11820545
		cv::Point3f d1 = a2 - a1, d2 = b2 - b1;
		cv::Point3f e = b1 - a1;
		cv::Point3f cross_e_d2 = Cross(e, d2);
		cv::Point3f cross_e_d1 = Cross(e, d1);
		cv::Point3f cross_d1_d2 = Cross(d1, d2);

		double t1, t2;
		t1 = Dot(cross_e_d2, cross_d1_d2);
		t2 = Dot(cross_e_d1, cross_d1_d2);
		double dd = Length((Cross(d1, d2)));
		t1 /= dd*dd;
		t2 /= dd*dd;
		//得到最近的位置
		cv::Point3f ans1, ans2;
		ans1 = (a1 + (a2 - a1)*t1);
		ans2 = (b1 + (b2 - b1)*t2);
		printf("%.6f %.6f %.6f\n", ans1.x, ans1.y, ans1.z);
		printf("%.6f %.6f %.6f\n", ans2.x, ans2.y, ans2.z);
		printf("middle: %.6f %.6f %.6f\n", (ans1.x + ans2.x) / 2, (ans1.y + ans2.y) / 2, (ans1.z + ans2.z) / 2);




		GetDistanceOf2linesIn3D g;//初始化
		g.SetLineA(a1.x, a1.y, a1.z, a2.x, a2.y, a2.z);//输入直线A上的两个点坐标
		g.SetLineB(b1.x, b1.y, b1.z, b2.x, b2.y, b2.z);//输入直线B上的两个点坐标
		g.GetDistance();//计算距离
		double d = g.distance;//获得距离
		double x = g.PonA_x;//获得AB间最近的两点中A线上的点的x坐标
		double y = g.PonA_y;//获得AB间最近的两点中A线上的点的y坐标
		double z = g.PonA_z;//获得AB间最近的两点中A线上的点的z坐标
	}
	test();
	return 0;
}

void test()
{

}