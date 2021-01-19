#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <cmath>
#include <iostream>
#include <string>
#include <time.h>
#include <opencv2/video/tracking.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio/videoio.hpp>

#define PI 			3.141592
#define WIDTH		1024
#define HEIGHT		768
#define RESIZE		1

using namespace cv;
using namespace std;


int main(int argc, char** argv) {
	double AspectRatio_g, AspectRatio_r, AreaRatio_g, AreaRatio_r;
	double error_aspectratio = 0.65;
	double error_arearatio = 0.31;
	double error_radian = 4.5;
	double error_distance = 0.1;
	double mark[4][2] = { 0 };
	double mark_temp_r[100][2] = { 0 };
	double mark_temp_g[100][2] = { 0 };
	int g_candidate[100] = { 0 };
	int r_candidate[100] = { 0 };
	bool mark_flag[4] = { false, false, false,false };
	int detected = 0;
	Mat image, image_resized, image_hsv, image_lab;
	int g_cnt, r_cnt;
	double mark_center[2];
	int vertical_cnt = 0;
	int diagonal_cnt1 = 0;
	int diagonal_cnt2 = 0;
	int r_vertical = 0;
	int r_diagonal_1 = 0;
	int r_diagonal_2[2] = { 0 };
	double gr1_size, gr2_size;

	//for(int index=10; index<880; index++){
	//	string str = to_string(index);
	//	string str_dir = "./color_mark_1/image" + str + ".bmp";
		string str_dir = "./color_mark_1/image_rect.bmp";
		cout << str_dir << endl;
		string imageName = str_dir;

		if (argc > 1) {
			imageName = argv[1];
		}
		image = imread(imageName.c_str(), IMREAD_COLOR); // Read the file
		if (image.empty()) {
			std::cout << "Could not open or find the image" << std::endl;
			return -1;
		}


		resize(image, image_resized, Size(WIDTH / RESIZE, HEIGHT / RESIZE), INTER_NEAREST);

		medianBlur(image_resized, image_resized, 3);

		cvtColor(image_resized, image_hsv, COLOR_BGR2HSV);
		cvtColor(image_resized, image_lab, COLOR_BGR2Lab);

		uchar l[HEIGHT*WIDTH/RESIZE];
		uchar a[HEIGHT*WIDTH/RESIZE];
		uchar b[HEIGHT*WIDTH/RESIZE];

		for (int row = 0; row < image_lab.rows; row++){
			uchar* pointer_row = image_lab.ptr<uchar>(row);
			for (int col = 0; col < image_lab.cols; col++){
				l[row*WIDTH+col] = pointer_row[col * 3 + 0];
				a[row*WIDTH+col] = pointer_row[col * 3 + 1];
				b[row*WIDTH+col] = pointer_row[col * 3 + 2];
			}
		}

		Mat image_l = Mat(HEIGHT/RESIZE,WIDTH/RESIZE,CV_8UC1,l);
		Mat image_a = Mat(HEIGHT/RESIZE,WIDTH/RESIZE,CV_8UC1,a);
		Mat image_b = Mat(HEIGHT/RESIZE,WIDTH/RESIZE,CV_8UC1,b);

		imwrite("image_l.bmp", image_l);
		imwrite("image_a.bmp", image_a);
		imwrite("image_b.bmp", image_b);

		Mat image_binary_g, image_binary_r;
		inRange(image_lab, Scalar(0, 100, 0), Scalar(90, 110, 255), image_binary_g);
		inRange(image_lab, Scalar(0, 140, 140), Scalar(90, 255, 255), image_binary_r);

		//To remove dots and To fill up dots from binary images
		medianBlur(image_binary_r, image_binary_r, 3);
		medianBlur(image_binary_r, image_binary_r, 3);
		medianBlur(image_binary_r, image_binary_r, 3);
		medianBlur(image_binary_g, image_binary_g, 3);
		medianBlur(image_binary_g, image_binary_g, 3);
		medianBlur(image_binary_g, image_binary_g, 3);

		// finde contours at bianry image
		vector<vector<Point>> contours_r, contours_g;
		vector<Vec4i> hierarchy_r, hierarchy_g;
		findContours(image_binary_g, contours_g, hierarchy_g, 0, 4, Point());
		findContours(image_binary_r, contours_r, hierarchy_r, 0, 4, Point());
		vector<Rect> boundRect_g(contours_g.size()), boundRect_r(contours_r.size());
		vector<Moments> mu_g(contours_g.size()), mu_r(contours_r.size());

		Scalar color = Scalar(0, 255, 0);
		Mat drawing_g = Mat::zeros(image_binary_g.size(), CV_8UC3);
		Mat drawing_r = Mat::zeros(image_binary_r.size(), CV_8UC3);
		for (int i = 0; i < contours_g.size(); i++)
		{
			drawContours(drawing_g, contours_g, i, color, 1, 8, hierarchy_g, 0, Point());
		}
		for (int i = 0; i < contours_r.size(); i++)
		{
			drawContours(drawing_r, contours_r, i, color, 1, 8, hierarchy_r, 0, Point());
		}
		//cv::namedWindow("drawing_r", WINDOW_NORMAL); // Create a window for display.
		//cv::imshow("drawing_r", drawing_r);                // Show our image inside it.
		//cv::namedWindow("drawing_g", WINDOW_NORMAL); // Create a window for display.
		//cv::imshow("drawing_g", drawing_g);

		
		// check Aspect Ratio, Area Ratio from bianry image 
		g_cnt = 0;

		//std::cout << "   #         (x,y)            h        w        Area       AspectRatio     AreaRatio" << endl;

		for (int i = 0; i < contours_g.size(); i++) {
			boundRect_g[i] = boundingRect(Mat(contours_g[i]));
			AspectRatio_g = (double)boundRect_g[i].height / boundRect_g[i].width;
			AreaRatio_g = (double)contourArea(contours_g[i]) / (boundRect_g[i].height*boundRect_g[i].width);
			//std::cout << "   g" << i << "     " << "      ,             " << boundRect_g[i].height << "        " << boundRect_g[i].width << "        " << contourArea(contours_g[i]) << "        " << AspectRatio_g << "             " << AreaRatio_g << endl;
			if (AspectRatio_g < 1 + error_aspectratio && AspectRatio_g> 1 - error_aspectratio) {
				if (AreaRatio_g > 0.3 && AreaRatio_g < 0.8) {
					mu_g[i] = moments(contours_g[i], false);
					double x = mu_g[i].m10 / mu_g[i].m00;
					double y = mu_g[i].m01 / mu_g[i].m00;
					mark_temp_g[g_cnt][0] = x * RESIZE;
					mark_temp_g[g_cnt][1] = y * RESIZE;
					//cout << "  G" << g_cnt << "     " << x << " ," << y << "     " << boundRect_g[i].height << "        " << boundRect_g[i].width << "        " << contourArea(contours_g[i]) << "        " << AspectRatio_g<< "             " << AreaRatio_g << endl;
					//putText(image_resized, to_string(g_cnt), Point(x, y), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 0.5);
					//circle(image_resized, Point(x, y), 3, Scalar(255, 0, 0), 0.5);
					g_candidate[g_cnt] = i;
					g_cnt = g_cnt + 1;
				}
			}
		}

		// check Aspect Ratio, Area Ratio from bianry image
		r_cnt = 0;
		for (int i = 0; i < contours_r.size(); i++) {
			boundRect_r[i] = boundingRect(Mat(contours_r[i]));
			AspectRatio_r = (double)boundRect_r[i].height / boundRect_r[i].width;
			AreaRatio_r = (double)contourArea(contours_r[i]) / (boundRect_r[i].height*boundRect_r[i].width);
			//std::cout << "   r" << i << "     " << "      ,             " << boundRect_r[i].height << "        " << boundRect_r[i].width << "        " << contourArea(contours_r[i]) << "        " << AspectRatio_r << "             " << AreaRatio_r << endl;
			
			if (AspectRatio_r < 1 + error_aspectratio && AspectRatio_r > 1 - error_aspectratio) {
				if (AreaRatio_r > 0.3 && AreaRatio_r < 0.8) {
					mu_r[i] = moments(contours_r[i], false);
					double x = mu_r[i].m10 / mu_r[i].m00;
					double y = mu_r[i].m01 / mu_r[i].m00;
					mark_temp_r[r_cnt][0] = x * RESIZE;
					mark_temp_r[r_cnt][1] = y * RESIZE;
					//cout << "  R" << r_cnt << "     " << x << " ," << y << "     " << boundRect_r[i].height << "        " << boundRect_r[i].width << "        " << contourArea(contours_r[i]) << "        " << AspectRatio_r << "             " << AreaRatio_r << endl;
					//putText(image_resized, to_string(r_cnt), Point(x, y), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255), 0.5);				
					//circle(image_resized, Point(x, y), 3, Scalar(255, 0, 0), 0.5);
					r_candidate[r_cnt] = i;
					r_cnt = r_cnt + 1;
				}
			}
		}

		//FInd square from LED candidates.
		int g1 = 0;
		int r1 = 0;
		int r2 = 0;
		int r3 = 0;

		for (int i = 0; i < g_cnt; i++) {
			for (int j = 0; j < r_cnt; j++) {
				//check LED Area.  green led should between 0.5~ 2 times of RED led.
				if ((0.3* contourArea(contours_g[g_candidate[i]]) > contourArea(contours_r[r_candidate[j]]) || (2.5 * contourArea(contours_g[g_candidate[i]])) < contourArea(contours_r[r_candidate[j]]))) {
					//cout << "SIZE ERROR AT: G"<<i <<"R"<<j << endl;
					continue;
				}
				double gr1[2] = { mark_temp_r[j][0] - mark_temp_g[i][0], mark_temp_r[j][1] - mark_temp_g[i][1] };
				gr1_size = sqrt(gr1[0] * gr1[0] + gr1[1] * gr1[1]);
				//cout << "gr1_size" << i << j << ": " << gr1_size << endl;;
				for (int k = j + 1; k < r_cnt; k++) {
					//check LED Area.  green led should between 0.5~ 2 times of RED led.
					if ((0.3* contourArea(contours_g[g_candidate[i]]) >= contourArea(contours_r[r_candidate[k]]) || (2.5 * contourArea(contours_g[g_candidate[i]])) <= contourArea(contours_r[r_candidate[j]]))) {
						//cout << "SIZE ERROR AT: G" << i << "R" << k << endl;
						continue;
					}
					double gr2[2] = { mark_temp_r[k][0] - mark_temp_g[i][0], mark_temp_r[k][1] - mark_temp_g[i][1] };
					gr2_size = sqrt(gr2[0] * gr2[0] + gr2[1] * gr2[1]);
					//cout << "gr2_size" << i << k << ": " << gr2_size << endl;;
					double radian = acos((gr1[0] * gr2[0] + gr1[1] * gr2[1]) / (gr2_size*gr1_size));
					radian = radian * (180 / PI);
					//cout << "radian" << i << j <<k<< ": " << radian << endl;;
					// check angle of vector gr1 and vector gr2. 
					// check length of vetor gr1 and vector gr2. 
					// determine they are LED of Squre or NOT.
					if (90 - error_radian < radian && radian < 90 + error_radian) {
						if ((1 - error_distance)*gr1_size < gr2_size && gr2_size < (1 + error_distance)*gr1_size) {
							r_vertical = k;
							vertical_cnt = vertical_cnt + 1;
							cout << "vertical" << endl;
						}
					}
					else if (45 - error_radian < radian && radian < 45 + error_radian) {
						if (sqrt(2)*(1 - error_distance)*gr1_size < gr2_size && gr2_size < sqrt(2)*(1 + error_distance)*gr1_size) {
							r_diagonal_1 = k;
							diagonal_cnt1 = diagonal_cnt1 + 1;
							cout << "diagonal1" << endl;
						}

					else if ((1 - error_distance)*gr1_size / sqrt(2) < gr2_size && gr2_size < (1 + error_distance)*gr1_size / sqrt(2)) {
							r_diagonal_2[diagonal_cnt2] = k;
							diagonal_cnt2 = diagonal_cnt2 + 1;
							cout << "diagonal2" << endl;
						}
					}
				}

				if (vertical_cnt == 1 && diagonal_cnt1 == 1) {
					mark[0][0] = mark_temp_g[i][0];
					mark[0][1] = mark_temp_g[i][1];
					mark[1][0] = mark_temp_r[r_vertical][0];
					mark[1][1] = mark_temp_r[r_vertical][1];
					mark[2][0] = mark_temp_r[r_diagonal_1][0];
					mark[2][1] = mark_temp_r[r_diagonal_1][1];
					mark[3][0] = mark_temp_r[j][0];
					mark[3][1] = mark_temp_r[j][1];
					detected = 1;
				}

				else if (diagonal_cnt2 == 2) {
					mark[0][0] = mark_temp_g[i][0];
					mark[0][1] = mark_temp_g[i][1];
					mark[1][0] = mark_temp_r[r_diagonal_2[0]][0];
					mark[1][1] = mark_temp_r[r_diagonal_2[0]][1];
					mark[2][0] = mark_temp_r[j][0];
					mark[2][1] = mark_temp_r[j][1];
					mark[3][0] = mark_temp_r[r_diagonal_2[1]][0];
					mark[3][1] = mark_temp_r[r_diagonal_2[1]][1];
					detected = 1;

					// to prevent mark1 and mark3 locate same direction about mark2.
					// if product of cross products is positive, they are same direction.  
					mark_center[0] = (mark[0][0] + mark[1][0] + mark[2][0] + mark[3][0]) / 4;
					mark_center[1] = (mark[0][1] + mark[1][1] + mark[2][1] + mark[3][1]) / 4;
					if ((((mark_center[0]-mark[0][0])*(mark[0][1]-mark[3][1]) - (mark[0][1]-mark_center[1])*(mark[3][0]-mark[0][0]))
						*((mark_center[0]-mark[0][0])*(mark[0][1]-mark[1][1]) - (mark[0][1]-mark_center[1])*(mark[1][0]-mark[0][0]))) > 0){
							detected = 0;
							cout << "MARK1 AND MARK3 ERROR: " << "They are same direction now." << endl;
						}
						
				}


				//check length of side of squre and average LED diameter. (using non-change distance 3m(between led) and 0.2m(led diameter))
				if (detected == 1) {
					double area = contourArea(contours_g[g1]) + contourArea(contours_r[r1]) + contourArea(contours_r[r2]) + contourArea(contours_r[r3]);
					double distance_ratio_cal = sqrt(2)*132 * pow(area, -0.822);
					double distance_ratio;
					if (gr1_size < gr2_size)
						distance_ratio = gr2_size / area;
					else
						distance_ratio = gr1_size / area;
					cout << "distance_ratio_cal: "<<distance_ratio_cal << endl;
					cout << "DISTANCE RATIO ERROR: " << distance_ratio << endl;
					cout << "AREA: " << area << endl;
					if (distance_ratio > distance_ratio_cal*2 || distance_ratio < distance_ratio_cal*0.5) {
						//detected = 0;
					}
				}

				// check numbering is counterclockwise. if not, change 1 and 3.
				if (((mark_center[0] - mark[0][0])*(mark[0][1] - mark[3][1]) - (mark[0][1] - mark_center[1])*(mark[3][0] - mark[0][0])) < 0) {
					double temp[2] = { mark[1][0],mark[1][1] };
					mark[1][0] = mark[3][0];
					mark[1][1] = mark[3][1];
					mark[3][0] = temp[0];
					mark[3][1] = temp[1];
				}
				if (detected == 1) {
					for (int i = 0; i < 4; i++) {
						std::cout << "LED" << i << "x,y :" << mark[i][0] << ", " << mark[i][1] << endl;
						mark_flag[i] = true;
						putText(image_resized, to_string(i), Point(mark[i][0] / RESIZE, mark[i][1] / RESIZE), FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 0, 0), 2);
					}
					break;
				}
			}
			if (detected == 1)
				break;
		}

		if (detected != 1)
			std::cout << "detect fail" << endl;

		cv::namedWindow("IMAGE", WINDOW_NORMAL); // Create a window for display.
		cv::imshow("IMAGE", image_resized);
		imwrite("image_detection.bmp", image_resized);
		cv::namedWindow("IMAGE_L", WINDOW_NORMAL); // Create a window for display.
		cv::imshow("IMAGE_L", image_l);
		cv::namedWindow("IMAGE_A", WINDOW_NORMAL); // Create a window for display.
		cv::imshow("IMAGE_A", image_a);
		cv::namedWindow("IMAGE_B", WINDOW_NORMAL); // Create a window for display.
		cv::imshow("IMAGE_B", image_b);
		//cv::namedWindow("IMAGE_G", WINDOW_NORMAL); // Create a window for display.
		//cv::imshow("IMAGE_G", image_binary_g);
		//cv::namedWindow("IMAGE_R", WINDOW_NORMAL); // Create a window for display.
		//cv::imshow("IMAGE_R", image_binary_r);

	//	cv::waitKey(10); // Wait for a keystroke in the window
	//}
	
	cv::waitKey(0); // Wait for a keystroke in the window


	return 0;
}