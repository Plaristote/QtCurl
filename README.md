# QtCurl

A simple wrapper around libcurl's easy interface. Allows you to send requests via curl using [QNetworkRequest](https://doc.qt.io/qt-6/qnetworkrequest.html), and receive the response as a [QNetworkReply](https://doc.qt.io/qt-6/qnetworkreply.html).

```c++
#include <qcurl.h>
#include <QDebug>

int main(int argc, const char** argv)
{
  QCurl curl;
  QNetworkRequest request;
  QNetworkReply* reply;

  request.setUrl(QUrl("https://duckduckgo.com"));
  reply = curl.send(request);
  if (reply)
  {
    qDebug() << "Received response:"
      << "status=" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt()
      << "body=" << reply->readAll();
    return 0;
  }
  else
    qDebug() << "QCurl could not send the request";
  return -1;
}
```

