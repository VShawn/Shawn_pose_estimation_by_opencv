#include "stdafx.h"

#include <opencv2\opencv.hpp>
#include "camera\MindVisionCAM\MindVisionCAM.h"
#include "..\include\PNPSolver.h"
using namespace std;


//本程序将展示一个实时相机位姿估计的例程，其中的原理在前文中已经说过了，利用《相机位姿估计1_1：OpenCV、solvePnP二次封装与性能测试》中构建的类
//使得程序处理更加简单。本例程利用HSV空间，跟踪红色的特征点，将跟踪到的特征点用于解PNP问题，得到相机位姿（相机的世界坐标与相机的三个旋转角）。
//最后使用labview中的三维图片控件，对整个系统进行3D重建。

//@author:VShawn(singlex@foxmail.com)
//说明文章地址：http://www.cnblogs.com/singlex/p/pose_estimation_2.html




vector<cv::Point2f> lastCenters;//记录上一轮特征点的像素坐标

//窗口的鼠标点击事件
//点击一次添加一个跟踪特征点
void on_mouse(int event, int x, int y, int flag, void *param)
{
	if (event == CV_EVENT_LBUTTONDOWN)
	{
		//只记录四个跟踪点。
		if (lastCenters.size() < 4)
		{
			lastCenters.push_back(cv::Point2f(x, y));
			cout << "add 1 point :" << lastCenters.size() << "/4" << endl;
		}
	}
}

//跟踪特征点
//在输入点附近查找红色区域，求出重心，作为特征点新的位置
//输入为，1当前图像，2被跟踪特征点上一轮的位置
//返回本次跟踪结果
cv::Point2f tracking(cv::Mat image, const cv::Point2f lastcenter)
{
	//cv::GaussianBlur(image, image, cv::Size(11, 11), 0);
	/***********初始化ROI**********/
	const int r = 100;//检测半径
	const int r2 = r * 2;

	int startx = lastcenter.x - r;
	int starty = lastcenter.y - r;
	if (startx < 0)
		startx = 0;
	if (starty < 0)
		starty = 0;

	int width = r2;
	int height = r2;
	if (startx + width >= image.size().width)
		startx = image.size().width - 1 - width;
	if (starty + height >= image.size().height)
		starty = image.size().height - 1 - height;

	cv::Mat roi = image(cv::Rect(startx, starty, width, height));
	cv::Mat roiHSV;
	cv::cvtColor(roi, roiHSV, CV_BGR2HSV);//将BGR图像转为HSV图像

	vector<cv::Mat> hsv;
	cv::split(roiHSV, hsv);//将hsv三个通道分离
	cv::Mat h = hsv[0];
	cv::Mat s = hsv[1];
	cv::Mat v = hsv[2];


	cv::Mat roiBinary = cv::Mat::zeros(roi.size(), CV_8U);//二值图像，255的地方表示判断为红色

	/*************判断颜色****************/
	const double ts = 0.5 * 255;//s阈值，小于该值不判断
	const double tv = 0.1 * 255;//v阈值，小于该值不判断
	const double th = 0 * 180 / 360;//h中心
	const double thadd = 30 * 180 / 360;//h范围在th±thadd内的才被算作是红色

	//通过特定阈值，对HSV图像进行二值化
	for (int i = 0; i < roi.size().height; i++)
	{
		uchar *ptrh = h.ptr<uchar>(i);
		uchar *ptrs = s.ptr<uchar>(i);
		uchar *ptrv = v.ptr<uchar>(i);
		uchar *ptrbin = roiBinary.ptr<uchar>(i);

		for (int j = 0; j < roi.size().width; j++)
		{
			if (ptrs[j] < ts || ptrv[j] < tv)
				continue;
			if (th + thadd > 180)
				if (ptrh[j] < th - thadd && ptrh[j] > th + thadd - 180)
					continue;
			if (th - thadd < 0)
				if (ptrh[j] < th - thadd + 180 && ptrh[j] > th + thadd)
					continue;

			ptrbin[j] = 255;//找出红色的像素点，在对应位置标记255
		}
	}

	/*****************对二值化图像求出连通域 重心****************/
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(roiBinary.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

	//可能会有多个连通域，分别计算出他们的重心
	std::vector<cv::Point2f> gravityCenters;//重心点集
	for (int i = 0; i < contours.size(); i++)
	{
		if (contours[i].size() < 10)//连通域太小
			continue;

		int xsum = 0;
		int ysum = 0;
		for (int j = 0; j < contours[i].size(); j++)
		{
			xsum += contours[i][j].x;
			ysum += contours[i][j].y;
		}
		double gpx = xsum / contours[i].size();
		double gpy = ysum / contours[i].size();
		gravityCenters.push_back(cv::Point2f(gpx + startx, gpy + starty));
	}

	/*********************返回最优点******************/
	//找到重心跟上一轮位置最接近的那个
	cv::Point ret = lastcenter;
	double dist = 1000000000;
	double distX = 1000000000;
	double distY = 1000000000;
	for (int i = 0; i < gravityCenters.size(); i++)
	{
		if (distX > abs(lastcenter.x - gravityCenters[i].x) && distY > abs(lastcenter.y - gravityCenters[i].y))
		{
			double newdist = sqrt((lastcenter.x - gravityCenters[i].x)*(lastcenter.x - gravityCenters[i].x) + (lastcenter.y - gravityCenters[i].y)*(lastcenter.y - gravityCenters[i].y));
			if (dist > newdist)
			{
				distX = abs(lastcenter.x - gravityCenters[i].x);
				distY = abs(lastcenter.y - gravityCenters[i].y);
				dist = newdist;
				ret = gravityCenters[i];
			}
		}
	}
	return ret;
}
int main()
{
	//初始化相机，此处替换为你的相机驱动程序
	MindVisionCAM mvcam;
	mvcam.ExposureTimeMS = 60;//设置曝光时间
	mvcam.AnalogGain = 4;//设置模拟增益
	if (mvcam.Init())
	{
		//成功初始化相机后，启动采图
		mvcam.StartCapture();

		//创建一个窗口用于显示
		cv::namedWindow("CamPos", 0);
		cvSetMouseCallback("CamPos", on_mouse, NULL);//绑定鼠标点击事件，通过鼠标点击添加追踪点。

		//相机内参数
		double fx = 1196.98;
		double fy = 1194.61;
		double u0 = 634.075;
		double v0 = 504.842;
		//镜头畸变参数
		double k1 = -0.475732;
		double k2 = 0.405008;
		double p1 = 0.00196334;
		double p2 = -0.00201087;
		double k3 = -0.337634;

		//初始化位姿估计类
		PNPSolver p4psolver;

		//初始化相机参数
		p4psolver.SetCameraMatrix(fx, fy, u0, v0);
		//设置畸变参数
		p4psolver.SetDistortionCoefficients(k1, k2, p1, p2, k3);
		
		//将特征点的世界坐标添加进去
		p4psolver.Points3D.push_back(cv::Point3f(0, 0, 0));		//P1三维坐标的单位是毫米
		p4psolver.Points3D.push_back(cv::Point3f(0, 200, 0));	//P2
		p4psolver.Points3D.push_back(cv::Point3f(150, 0, 0));	//P3
		p4psolver.Points3D.push_back(cv::Point3f(150, 200, 0));	//P4
		//p4psolver.Points3D.push_back(cv::Point3f(0, 100, 105));	//P5

		std::cout << "请在屏幕上点击确定特征点，特征点的点选顺序应与p4psolver.Points3D的存储顺序一致。" << endl;
		while (cv::waitKey(1) != 27)//按下ecs键退出程序
		{
			if (cv::waitKey(1) == 'r')//按下r键重新设置特征点
			{
				lastCenters.clear();
				std::cout << "清除所有特征点，请重新在屏幕上点击确定特征点。" << endl;
			}

			//提取相机当前帧图像，修改为你的相机驱动
			cv::Mat img = mvcam.Grub();
			cv::Mat paintBoard = cv::Mat::zeros(img.size(), CV_8UC3);//新建一个Mat，用于存储绘制的东西


			//追踪并更新特征点位置
			if (lastCenters.size() > 0)
			{
				//通过HSV颜色追踪特征点
				for (int i = 0; i < lastCenters.size(); i++)
				{
					lastCenters[i] = tracking(img, lastCenters[i]);//更新追踪的特征点坐标
					cv::circle(paintBoard, lastCenters[i], 8, cv::Scalar(0, 255, 0), 3);//将跟踪到的点绘制在paintBoard上
				}
			}


			//特征点数量足够时，才求解位姿
			if (lastCenters.size() >= 4)
			{
				//首先将位姿估计类内的特征点的像素坐标记录清0。
				p4psolver.Points2D.clear();
				//然后将新跟踪到的特征点像素坐标插入
				for (int i = 0; i < lastCenters.size(); i++)
				{
					p4psolver.Points2D.push_back(lastCenters[i]);
				}
				//解位姿
				p4psolver.Solve(PNPSolver::METHOD::CV_P3P);


				//将点重投影回图像，检验投影点是否正确
				vector<cv::Point3f> r;
				r.push_back(cv::Point3f(0, 100, 105));//重投影点事件坐标
				vector<cv::Point2f>	ps = p4psolver.WordFrame2ImageFrame(r);
				//将重投影点绘在paintBoard上
				for (int i = 0; i < ps.size(); i++)
				{
					cv::circle(paintBoard, ps[i], 5, cv::Scalar(0, 255, 255), -1);
				}

				//输出位姿信息到txt
				ofstream fout1("D:\\pnp_theta.txt");
				fout1 << p4psolver.Theta_W2C.x << endl << p4psolver.Theta_W2C.y << endl << p4psolver.Theta_W2C.z << endl;
				fout1.close();
				ofstream fout2("D:\\pnp_t.txt");
				fout2 << p4psolver.Position_OcInW.x << endl << p4psolver.Position_OcInW.y << endl << p4psolver.Position_OcInW.z << endl;
				fout2.close();
			}

			//将当前帧图像+paintBoard图像显示出来
			cv::imshow("CamPos", (img - paintBoard) + paintBoard + paintBoard);
		}
		//退出程序，释放相机
		mvcam.Release();
	}
	else
		std::cout << "相机初始化失败！" << endl;
	return 0;
}