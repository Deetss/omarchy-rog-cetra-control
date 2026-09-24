#include <QGuiApplication>
#include <QQuickView>
#include <QQmlEngine>
#include <QQuickItem>
#include <QTimer>
#include <QImage>
#include <QVariant>
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <QDir>
#include <QFileInfo>
#include <QSGRendererInterface>
int main(int argc,char**argv){
 if(qEnvironmentVariableIsSet("CETRA_BAR_FORCE_OPENGL")) { QQuickWindow::setSceneGraphBackend("rhi"); QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL); }
 QGuiApplication app(argc,argv);
 if(argc!=3)return 2;
 QQuickView view;
 view.engine()->addImportPath(QString::fromLocal8Bit(argv[1])+"/imports");
 view.setSource(QUrl::fromLocalFile(QString::fromLocal8Bit(argv[1])+"/Preview.qml"));
 if(view.status()!=QQuickView::Ready)return 3;
 view.show();
 QTimer::singleShot(600,[&]{
  QVariant ok;
  if(!QMetaObject::invokeMethod(view.rootObject(),"check",Q_RETURN_ARG(QVariant,ok))||!ok.toBool()){app.exit(4);return;}
  const QImage img=view.grabWindow();
  if(img.isNull()||!img.save(QString::fromLocal8Bit(argv[2]))){app.exit(5);return;}
  const auto api=view.rendererInterface()->graphicsApi();
  std::printf("Qt graphics API: %d\n",int(api));
  if(qEnvironmentVariableIsSet("CETRA_BAR_FORCE_OPENGL") && api!=QSGRendererInterface::OpenGL){
    std::fprintf(stderr,"FAIL: requested OpenGL but Qt selected another backend\n"); app.exit(8); return;
  }
  const auto actual=img.copy(40,490,27,26);
  const QString outputDir=QFileInfo(QString::fromLocal8Bit(argv[2])).absolutePath();
  actual.save(outputDir+"/actual-size.png");
  actual.scaled(324,312,Qt::IgnoreAspectRatio,Qt::FastTransformation).save(outputDir+"/pixel-zoom.png");
  int edgePixels=0;
  for(int y=5;y<21;++y)for(int x=5;x<21;++x){
    const int c=actual.pixelColor(x,y).red();
    if(c>8 && c<247)++edgePixels;
  }
  std::printf("Actual-size outline partial-coverage pixels: %d\n",edgePixels);
  auto crop=[&](int n,int x,int y,int w,int h){return img.copy((n%4)*190+40+x,(n/4)*150+30+y,w,h);};
  auto energy=[](const QImage &image){long sum=0; for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x){auto c=image.pixelColor(x,y);sum+=c.red()+c.green()+c.blue();}return sum;};
  auto ear=[&](int n){return crop(n,24,20,60,64);};
  auto mic=[&](int n){return crop(n,84,24,24,56);};
  // CurveRenderer can round a handful of edge pixels differently after translation.
  // Allow one color step in at most four pixels, never displacement or new geometry.
  auto samePixels=[](const QImage &a,const QImage &b){
    if(a.size()!=b.size())return false;
    int changed=0;
    for(int y=0;y<a.height();++y)for(int x=0;x<a.width();++x){
      const auto ca=a.pixelColor(x,y),cb=b.pixelColor(x,y);
      const int d=std::max({std::abs(ca.red()-cb.red()),std::abs(ca.green()-cb.green()),std::abs(ca.blue()-cb.blue())});
      if(d>1)return false;
      if(d && ++changed>4)return false;
    }
    return true;
  };
  bool pass=true;
  auto require=[&](bool b,const char*why){if(!b){std::fprintf(stderr,"FAIL: %s\n",why);pass=false;}};
  require(edgePixels>=20,"actual-size earbud outline must have antialiased edges");
  require(energy(ear(1))>energy(ear(0))+20000,"full charge must fill the actual earbud silhouette");
  require(energy(ear(3))>energy(ear(0)),"unknown charge must differ from measured zero");
  require(samePixels(ear(3),ear(4)),"invalid charge must match unavailable charge");
  require(energy(ear(2).copy(30,0,30,64))>energy(ear(2).copy(0,0,30,64))+10000,"left/right charge must be independent and not reversed");
  for(int n:{5,6,7,8})require(samePixels(ear(n),ear(9)),"speech, unavailable signal or meter visibility must not move/change earbuds");
  require(energy(mic(6))>energy(mic(5)),"microphone must visibly respond to amplitude");
  require(mic(7)!=mic(5),"dim no-data outline must differ from bright measured-silence outline");
  require(energy(mic(8))==0,"hidden microphone must paint nothing");
  require(ear(10)!=ear(9),"theme color must propagate to the earbud artwork");
  qInfo()<<(pass?"PASS":"FAIL")<<"actual Qt bar pixels: charge, unknowns, speech, independent sides, stationary icon, hidden mic, theme";
  app.exit(pass?0:7);
 });
 QTimer::singleShot(10000,[&]{app.exit(6);});
 return app.exec();
}
