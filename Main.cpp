#include "Main.h"

#include "Config.h"
#include "ImAcq.h"
#include "Gui.h"
#include "TLDUtil.h"
#include <fstream>
#include <iostream>
using namespace std;
using namespace tld;
using namespace cv;

void Main::doWork(IplImage* img,bool UpdateorRecognise,const char* faceName)
{
    Mat grey(img->height, img->width, CV_8UC1);
    cvtColor(cv::Mat(img), grey, CV_BGR2GRAY);

    tld->detectorCascade->imgWidth = grey.cols;
    tld->detectorCascade->imgHeight = grey.rows;
    tld->detectorCascade->imgWidthStep = grey.step;

    if(UpdateorRecognise==true)
    {
        //increase facecount value in face entry file
        int numfacesinDatabase;
        fstream  file;
        file.open("facedatabaseFile",ios::in|ios::out);
        file>> numfacesinDatabase;
        file.seekp(0);
        if (numfacesinDatabase < 10)
        {
          file<<" ";
        }
        if (numfacesinDatabase >=10 && numfacesinDatabase<100)
        {
          file << "  ";
        }
        file.seekp(0);
        file<< ++numfacesinDatabase <<"\n";
        file.close();
        //enter name in databasefile
        file.open("facedatabaseFile",ios::in|ios::out|ios::app);
        file<<faceName<<"\n";
        file.close();
        initialBB = new int[4];
        initialBB[0] = 5;
        initialBB[1] = 5;
        initialBB[2] = img->width-5;
        initialBB[3] = img->height-5;
        Rect bb = tldArrayToRect(initialBB);
        tld->selectObject(grey, &bb);
        cvtColor(cv::Mat(img), grey, CV_BGR2GRAY);
        tld->processImage(img);
        tld->writeToFile(faceName);
    }

    if(UpdateorRecognise==false)//loadModel && modelPath != NULL)
    {
        int numFacesinDatabase;
        string filename;
        fstream faceentrydatabaseFile("facedatabaseFile",fstream::in | fstream::out | fstream::app);
        faceentrydatabaseFile >> numFacesinDatabase;
        for (int i=0;i<numFacesinDatabase;i++)
        {
            faceentrydatabaseFile >>filename;
            const char *constfilename=filename.c_str();
            tld->readFromFile(constfilename);//modelPath);
            cvtColor(cv::Mat(img), grey, CV_BGR2GRAY);
            tld->processImage(img);
            std::cout<<constfilename<<std::endl;
            std::cout<< tld->currConf<<std::endl;
        }
        faceentrydatabaseFile.close();
     }
    return ;
}
