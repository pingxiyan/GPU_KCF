#include <fstream>
#include <sstream>
#include <iostream>  
#include <cstring>  
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include "kcftracker.hpp"

#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/video/tracking.hpp"

using namespace std;
using namespace cv;

static string FileFolder = "../test_data/traffic";
static std::vector<std::string> files;

bool GetInput()
{
    DIR *pDir;
    struct dirent *ent;
	
    char absolutepath[512];
    pDir = opendir(FileFolder.c_str());
	
    
    while ((ent = readdir(pDir)) != NULL)
    {
	if (ent->d_type & DT_DIR)
	{

		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
		{
			continue;
		}
			
	}
	else
	{
		sprintf(absolutepath, "%s/%s", FileFolder.c_str(), ent->d_name);
                std::string filename(absolutepath);
                std::string suffixStr = filename.substr(filename.find_last_of('.') + 1);
                        
                if(strcmp(suffixStr.c_str(), "jpg") == 0)
	        files.push_back(filename);
	}
     }

     sort(files.begin(),files.end());
     
}


cv::Rect GetRoi()
{
	cv::Rect roi;

	ifstream groundtruthFile;
	string groundtruth = "/groundtruth.txt";
        //cout<<FileFolder + groundtruth<<endl;
	groundtruthFile.open(FileFolder + groundtruth);
	string firstLine;
	getline(groundtruthFile, firstLine);
	groundtruthFile.close();

	istringstream ss(firstLine);
	// Read groundtruth like a dumb
	float x1, y1, x2, y2, x3, y3, x4, y4;
	char ch;
	ss >> x1; ss >> ch; ss >> y1; ss >> ch; ss >> x2; ss >> ch; ss >> y2; ss >> ch;
	ss >> x3; ss >> ch; ss >> y3; ss >> ch; ss >> x4; ss >> ch; ss >> y4;

	// Using min and max of X and Y for groundtruth rectangle
	float xMin = min(x1, min(x2, min(x3, x4)));
	float yMin = min(y1, min(y2, min(y3, y4)));
	float width = max(x1, max(x2, max(x3, x4))) - xMin;
	float height = max(y1, max(y2, max(y3, y4))) - yMin;

	roi.x = xMin; roi.y = yMin; roi.width = width; roi.height = height;
        //cout<<roi.x <<" "<<roi.y<<" "<<roi.width<<" "<<roi.height<<endl;
	return roi;
}


int main(int argc, char* argv[])
{
        GetInput();
        
	KCFTracker* tracking = new KCFTracker();
	if (files.size()!=0)
	{
            Mat orig_image = imread(files[0]);
            if( (orig_image.rows %4 != 0) || (orig_image.cols %4 !=0 ))
            {   
                 Size aligned_size((orig_image.cols/4)*4,(orig_image.rows/4)*4);
                 resize(orig_image, orig_image , aligned_size);
            }
            UMat gpu_image= orig_image.getUMat( ACCESS_READ );
	    cv::Rect roi = GetRoi();
	    tracking->init(roi, gpu_image);
	
            rectangle(orig_image, Point(roi.x, roi.y), Point(roi.x + roi.width, roi.y + roi.height), Scalar(0, 255, 255), 1, 8);
	    //cv::namedWindow("KCF_GPU", cv::WINDOW_AUTOSIZE);
	    //cv::imshow("KCF_GPU", orig_image);
            //cv::waitKey(0);

	    for (std::vector<string>::iterator it=files.begin()+1; it!=files.end(); ++it)
	    {
                cout<<*it<<endl;
                orig_image= imread(*it);
                if( (orig_image.rows %4 != 0) || (orig_image.cols %4 !=0 ))
                {   
                   Size aligned_size((orig_image.cols/4)*4,(orig_image.rows/4)*4);
                   resize(orig_image, orig_image , aligned_size);
                }
                gpu_image = orig_image.getUMat( ACCESS_READ );
		cv::Rect res= tracking->update(gpu_image);
                //cout<<res.x <<" "<<res.y<<" "<<res.width<<" "<<res.height<<endl;
                rectangle(orig_image, Point(res.x, res.y), Point(res.x + res.width, res.y + res.height), Scalar(0, 255, 255), 1, 8);
		cv::namedWindow("KCF_GPU", cv::WINDOW_AUTOSIZE);
		cv::imshow("KCF_GPU", orig_image);
                char c = (char)cv::waitKey(0);
                if(c==27)
                   break;
	    }
        
        }
	//std::cout << "Average speed " << average_speed_ms / tracking->GetFrameNO() << "ms. (" << 1000.0 / (average_speed_ms / tracking->GetFrameNO()) << "fps)" << std::endl;
	delete tracking;

	return 0;
}
