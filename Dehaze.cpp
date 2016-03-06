#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <string>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

Mat getDarkChannel(Mat ini,vector<float> &air){
    // Split the initial image into three channels.
    vector<Mat> cols;
    split(ini,cols);

    Mat dark(ini.size(),CV_8UC1,Scalar::all(0));
    int side = 5;// the side length of the patches.
    
    MatIterator_<uchar> it = dark.begin<uchar>();
    Mat ROI;
    int height = ini.rows,width = ini.cols,channels = ini.channels();
    for(int y = 0;y < height;++ y)
        for(int x = 0;x < width;++ x){
            //Calc the edges of the patch
            int left,right,up,down;  
            if(x - side + 1 >= 0) left = x - side + 1;
            else left = 0;
            if(x + side <= width) right = x + side;
            else right = width;
            if(y - side + 1 >= 0) up = y - side + 1;
            else up = 0;
            if(y + side <= height) down = y + side;
            else down = height; 
            //Get the dark channel of this patch and arrange it to the central pixel.
            double tmp,mn = 300.0,mx;
            for(int i = 0;i < channels;++ i){
                ROI = cols[i](Range(up,down),Range(left,right));
                minMaxLoc(ROI,&tmp,&mx);
                if(mn > tmp) mn = tmp;
            }
            /*
            if(mn - 0.0 > 1e-9)
                printf("%d,%d : %.8f\n",y,x,mn);
            */
            *it = saturate_cast<uchar>(mn); 
            ++ it;
        }
    /*Debug
    namedWindow("DarkChannel",WINDOW_AUTOSIZE);
    imshow("darkChannel",dark);

    waitKey(0);
    exit(0);
    */

    //Find the pixel with highest intensity.
    Point mnLoc,mxLoc;
    double mn,mx,tmp;
    minMaxLoc(dark,&mn,&mx,&mnLoc,&mxLoc);
    //Get the edges of the patch.
    int l,r,u,d,x = mxLoc.x,y = mxLoc.y;
    if(x - side + 1 >= 0) l = x - side + 1;
    else l = 0;
    if(x + side <= width) r = x + side;
    else r = width;
    if(y - side + 1 >= 0) u = y - side + 1;
    else u = 0;
    if(y + side <= height) d = y + side;
    else d = height;
    for(int i = 0;i < 3;++ i){
        ROI = cols[i](Range(u,d),Range(l,r));
        minMaxLoc(ROI,&mn,&tmp);
        air.push_back(tmp);
    }
    
    return dark;
}

Mat getTransmissionMap(Mat dark,vector<float> airLight){
    //Get the minimum channel of airlight.
    double w = 0.95;
    double mn = airLight[0];
    for(int i = 1;i < 3;++ i)
    if(airLight[i] < mn)
        mn = airLight[i];

    Mat ret(dark.size(),CV_32F); 
    //Calc the transmission rate of each pixel.
    MatIterator_<float> it = ret.begin<float>();
    MatIterator_<uchar> dk = dark.begin<uchar>();
    int height = dark.rows,width = dark.cols;
    for(int y = 0;y < height;++ y)
        for(int x = 0;x < width;++ x){
            *it = (1.0 - w * (*dk) / mn);
            ++ it;
            ++ dk;
        }
    
    /*Debug
    namedWindow("transMap",WINDOW_AUTOSIZE);
    imshow("transMap",ret);
    waitKey(0);
    //exit(0);
    */
    return ret;
}

Mat restoreImage(Mat ini,Mat trans,vector<float> air){
    Mat ret(ini.size(),CV_8UC3,Scalar::all(0)); 

    double t0 = 0.1;//t0 is an introduced parameter to make sure that there remains some fog.
    
    MatIterator_<Vec3b> it = ret.begin<Vec3b>(),itI = ini.begin<Vec3b>();
    MatIterator_<float> tr = trans.begin<float>();

    int height = ini.rows,width = ini.cols;

    //Restore every pixel.
    for(int y = 0;y < height;++ y)
        for(int x = 0;x < width;++ x){
            double t = *tr;
            if(t < t0) t = t0;
            for(int i = 0;i < 3;++ i)
                (*it)[i] = ((*itI)[i] - air[i]) / t + air[i]; 
            ++ it; ++ itI; ++ tr;
        }

    return ret;
}

Mat dehaze(Mat ini){
    vector<float> airLight;
    Mat darkChannel = getDarkChannel(ini,airLight);// Get the dark channel image of the initial one.
    Mat transmission = getTransmissionMap(darkChannel,airLight);// Get the transmission map of the image based on the dark channel image.
    Mat ret = restoreImage(ini,transmission,airLight);// Restore the initial image.
    return ret;
}

int main(int argc,char** argv){
    Mat ini = imread(argv[1],1);
    Mat ret = dehaze(ini);
    
    imwrite("./Ret.jpg",ret);

    namedWindow("Initial",WINDOW_AUTOSIZE);
    namedWindow("Dehaze",WINDOW_AUTOSIZE);

    imshow("Initial",ini);
    imshow("Dehaze",ret);

    waitKey(0);
    return 0;
}
