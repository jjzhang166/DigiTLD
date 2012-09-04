/** ===========================================================
 * @file
 *
 * This file is a part of digiKam project
 * <a href="http://www.digikam.org">http://www.digikam.org</a>
 *
 * @date   2010-06-21
 * @brief  GUI test program for libkface
 *
 * @author Copyright (C) 2010 by Alex Jironkin
 *         <a href="mailto:alexjironkin at gmail dot com">alexjironkin at gmail dot com</a>
 *         Copyright (C) 2010 by Aditya Bhatt
 *         <a href="mailto:adityabhatt1991 at gmail dot com">adityabhatt1991 at gmail dot com</a>
 *         Copyright (C) 2010-2012 by Gilles Caulier
 *         <a href="mailto:caulier dot gilles at gmail dot com">caulier dot gilles at gmail dot com</a>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "mainwindow.moc"
#include "ui_mainwindow.h"
#include <cstdlib>
// Qt includes

#include <QDebug>
#include <QLayout>
#include <QFormLayout>

// KDE include
#include <iostream>
#include <kdebug.h>
#include <kfiledialog.h>
#include <cv.h>
#include <highgui.h>
#include "Main.h"
#include "Config.h"
#include "ImAcq.h"
#include "Gui.h"
using namespace tld;

using namespace std;
using namespace KFaceIface;

MainWindow::MainWindow(QWidget* const parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->openImageBtn, SIGNAL(clicked()),
            this, SLOT(openImage()));

    connect(ui->openConfigBtn, SIGNAL(clicked()),
            this, SLOT(openConfig()));

    connect(ui->detectFacesBtn, SIGNAL(clicked()),
            this, SLOT(detectFaces()));

    connect(ui->recogniseBtn, SIGNAL(clicked()),
            this, SLOT(recognise()));

    connect(ui->updateDatabaseBtn, SIGNAL(clicked()),
            this, SLOT(updateConfig()));

    connect(ui->horizontalSlider, SIGNAL(valueChanged(int)),
	    this, SLOT(updateAccuracy()));


    myScene             = new QGraphicsScene();
    QGridLayout* layout = new QGridLayout;
    myView              = new QGraphicsView(myScene);

    myView->setCacheMode(QGraphicsView::CacheBackground);
    myScene->setItemIndexMethod(QGraphicsScene::NoIndex);

    setMouseTracking(true);
    layout->addWidget(myView);

    ui->widget->setLayout(layout);

    myView->show();

    database = new Database(Database::InitAll, ".");

    ui->configLocation->setText(QDir::currentPath());
    ui->horizontalSlider->setValue(database->detectionAccuracy());

    lastFileOpenPath = QDir::currentPath();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent* e)
{
  
  QMainWindow::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

void MainWindow::openImage()
{
    QString file = KFileDialog::getOpenFileName(
            lastFileOpenPath,
            "Image Files (*.png *.jpg *.bmp *.pgm)",
            this,
            "Open Image"
        );

    if (file.isEmpty())
        return;

    lastFileOpenPath = QFileInfo(file).absolutePath();

    clearScene();

    kDebug() << "Opened file " << file.toAscii().data();

    QPixmap* photo = new QPixmap(file);
    lastPhotoItem  = new QGraphicsPixmapItem(*photo);
    currentPhoto   = photo->toImage();

    if(1.0*ui->widget->width()/photo->width() < 1.*ui->widget->height()/photo->height())
    {
        scale = 1.0*ui->widget->width()/photo->width();
    }
    else
    {
        scale = 1.0*ui->widget->height()/photo->height();
    }

    lastPhotoItem->setScale(scale);

    myScene->addItem(lastPhotoItem);
}

void MainWindow::openConfig()
{
    QString directory = KFileDialog::getExistingDirectory(
            QDir::currentPath(),
            this,
            "Select Config Directory"
        );

    ui->configLocation->setText(directory);

    database = new Database(Database::InitAll, directory);
}

void MainWindow::detectFaces()
{
    currentFaces.clear();
    currentFaces = database->detectFaces(currentPhoto);
    Face face;
    kDebug() << "libkface detected : " << currentFaces.size() << " faces.";
    FaceItem* item=0;
    foreach(item, faceitems)
        item->setVisible(false);

    faceitems.clear();
    for(int i = 0; i < currentFaces.size(); ++i)
    {
        face = currentFaces[i];
        faceitems.append(new FaceItem(0, myScene, face.toRect(), scale));
        kDebug() << face.toRect()<<endl;
    }
}

void MainWindow::updateConfig()
{
    for(int i = 0; i < currentFaces.size(); ++i)
    {
            Main *main = new Main();
            Config config;
            ImAcq *imAcq = imAcqAlloc();
            Gui *gui = new Gui();

            main->gui = gui;
            main->imAcq = imAcq;

            config.configure(main);

            srand(main->seed);

            imAcqInit(imAcq);
            Face face;

            face = currentFaces[i];
            QImage oneface = currentPhoto.copy(face.toRect());
            IplImage *faceImage = cvCreateImageHeader( cvSize(oneface.width(), oneface.height()), IPL_DEPTH_8U, 4);
            faceImage->imageData = (char*) oneface.bits();

            uchar* newdata = (uchar*) malloc(sizeof(uchar) * oneface.byteCount());
            memcpy(newdata, oneface.bits(),oneface.byteCount());
            faceImage->imageData = (char*) newdata;
            QByteArray ba = faceitems[i]->text().toLocal8Bit();
            const char *faceName= ba.data();
            std::cout<<faceName<<std::endl;
            main->doWork(faceImage,true,faceName);//(const char*)face.name().toLocal8Bit().data());
            delete main;
     }
}

void MainWindow::updateAccuracy()
{
    int value = ui->horizontalSlider->value();
    ui->lcdNumber->display(value);
    database->setDetectionAccuracy(value);
}

void MainWindow::clearScene()
{
    QList<QGraphicsItem*> list = myScene->items();

    for(int i=0; i<list.size(); i++)
    {
        myScene->removeItem(list.at(i));
    }
}

void MainWindow::recognise()
{
    for(int i = 0; i < currentFaces.size(); ++i)
    {
            Main *main = new Main();
            Config config;
            ImAcq *imAcq = imAcqAlloc();
            Gui *gui = new Gui();

            main->gui = gui;
            main->imAcq = imAcq;

            config.configure(main);

            srand(main->seed);

            imAcqInit(imAcq);
            Face face;

            face = currentFaces[i];
            QImage oneface = currentPhoto.copy(face.toRect());
            IplImage *faceImage = cvCreateImageHeader( cvSize(oneface.width(), oneface.height()), IPL_DEPTH_8U, 4);
            faceImage->imageData = (char*) oneface.bits();

            uchar* newdata = (uchar*) malloc(sizeof(uchar) * oneface.byteCount());
            memcpy(newdata, oneface.bits(), oneface.byteCount());
            faceImage->imageData = (char*) newdata;
            const char *faceFile="facedatabaseFile";
            std::cout<<"posiibility of face "<<i<<" is\n"<<std::endl;

            main->doWork(faceImage,false,faceFile);//(const char*)face.name().toLocal8Bit().data());
            delete main;
     }
}
